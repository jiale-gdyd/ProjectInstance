#include <xlib/xlib/config.h>
#include <xlib/xlib/xhash.h>
#include <xlib/xlib/xarray.h>
#include <xlib/xlib/xpathbuf.h>
#include <xlib/xlib/xmessages.h>
#include <xlib/xlib/xstrfuncs.h>
#include <xlib/xlib/xfileutils.h>

typedef struct {
    XPtrArray *path;
    char      *extension;
    xpointer  padding[6];
} RealPathBuf;

X_STATIC_ASSERT(sizeof(XPathBuf) == sizeof(RealPathBuf));

#define PATH_BUF(b)             ((RealPathBuf *)(b))

XPathBuf *x_path_buf_init(XPathBuf *buf)
{
    RealPathBuf *rbuf = PATH_BUF(buf);

    rbuf->path = NULL;
    rbuf->extension = NULL;

    return buf;
}

XPathBuf *x_path_buf_init_from_path(XPathBuf *buf, const char *path)
{
    x_return_val_if_fail(buf != NULL, NULL);
    x_return_val_if_fail((path == NULL) || (*path != '\0'), NULL);

    x_path_buf_init(buf);

    if (path == NULL) {
        return buf;
    }

    return x_path_buf_push(buf, path);
}

void x_path_buf_clear(XPathBuf *buf)
{
    RealPathBuf *rbuf = PATH_BUF (buf);

    x_return_if_fail(buf != NULL);

    x_clear_pointer(&rbuf->path, x_ptr_array_unref);
    x_clear_pointer(&rbuf->extension, x_free);
}

char *x_path_buf_clear_to_path(XPathBuf *buf)
{
    char *res;

    x_return_val_if_fail(buf != NULL, NULL);

    res = x_path_buf_to_path(buf);
    x_path_buf_clear(buf);

    return x_steal_pointer(&res);
}

XPathBuf *x_path_buf_new(void)
{
    return x_path_buf_init(x_new(XPathBuf, 1));
}

XPathBuf *x_path_buf_new_from_path(const char *path)
{
    return x_path_buf_init_from_path(x_new(XPathBuf, 1), path);
}

void x_path_buf_free(XPathBuf *buf)
{
    x_return_if_fail(buf != NULL);

    x_path_buf_clear(buf);
    x_free(buf);
}

char *x_path_buf_free_to_path(XPathBuf *buf)
{
    char *res;

    x_return_val_if_fail(buf != NULL, NULL);

    res = x_path_buf_clear_to_path(buf);
    x_path_buf_free(buf);

    return x_steal_pointer(&res);
}

XPathBuf *x_path_buf_copy(XPathBuf *buf)
{
    XPathBuf *copy;
    RealPathBuf *rcopy;
    RealPathBuf *rbuf = PATH_BUF(buf);

    x_return_val_if_fail(buf != NULL, NULL);

    copy = x_path_buf_new();
    rcopy = PATH_BUF(copy);

    if (rbuf->path != NULL) {
        rcopy->path = x_ptr_array_new_null_terminated(rbuf->path->len, x_free, TRUE);
        for (xuint i = 0; i < rbuf->path->len; i++) {
            const char *p = x_ptr_array_index(rbuf->path, i);
            if (p != NULL) {
                x_ptr_array_add(rcopy->path, x_strdup(p));
            }
        }
    }

    rcopy->extension = x_strdup(rbuf->extension);
    return copy;
}

XPathBuf *x_path_buf_push(XPathBuf *buf, const char *path)
{
    RealPathBuf *rbuf = PATH_BUF(buf);

    x_return_val_if_fail(buf != NULL, NULL);
    x_return_val_if_fail((path != NULL) && (*path != '\0'), buf);

    if (x_path_is_absolute(path)) {
        char **elements = x_strsplit(path, X_DIR_SEPARATOR_S, -1);

        x_free(elements[0]);
        elements[0] = x_strdup("/");

        x_clear_pointer(&rbuf->path, x_ptr_array_unref);
        rbuf->path = x_ptr_array_new_null_terminated(x_strv_length(elements), x_free, TRUE);

        for (xuint i = 0; elements[i] != NULL; i++) {
            if (*elements[i] != '\0') {
                x_ptr_array_add(rbuf->path, x_steal_pointer(&elements[i]));
            } else {
                x_free(elements[i]);
            }
        }

        x_free(elements);
    } else {
        char **elements = x_strsplit(path, X_DIR_SEPARATOR_S, -1);

        if (rbuf->path == NULL) {
            rbuf->path = x_ptr_array_new_null_terminated(x_strv_length(elements), x_free, TRUE);
        }

        for (xuint i = 0; elements[i] != NULL; i++) {
            if (*elements[i] != '\0') {
                x_ptr_array_add(rbuf->path, x_steal_pointer(&elements[i]));
            } else {
                x_free(elements[i]);
            }
        }

        x_free(elements);
    }

    return buf;
}

xboolean x_path_buf_pop(XPathBuf *buf)
{
    RealPathBuf *rbuf = PATH_BUF(buf);

    x_return_val_if_fail(buf != NULL, FALSE);
    x_return_val_if_fail(rbuf->path != NULL, FALSE);

    if (rbuf->path->len > 1) {
        x_ptr_array_remove_index(rbuf->path, rbuf->path->len - 1);
        return TRUE;
    }

    return FALSE;
}

xboolean x_path_buf_set_filename(XPathBuf *buf, const char *file_name)
{
    x_return_val_if_fail(buf != NULL, FALSE);
    x_return_val_if_fail(file_name != NULL, FALSE);

    if (PATH_BUF(buf)->path == NULL) {
        return FALSE;
    }

    x_path_buf_pop(buf);
    x_path_buf_push(buf, file_name);

    return TRUE;
}

xboolean x_path_buf_set_extension(XPathBuf *buf, const char *extension)
{
    RealPathBuf *rbuf = PATH_BUF(buf);

    x_return_val_if_fail(buf != NULL, FALSE);

    if (rbuf->path != NULL) {
        return x_set_str(&rbuf->extension, extension);
    }

    return FALSE;
}

char *x_path_buf_to_path(XPathBuf *buf)
{
    char *path = NULL;
    RealPathBuf *rbuf = PATH_BUF(buf);

    x_return_val_if_fail(buf != NULL, NULL);

    if (rbuf->path != NULL) {
        path = x_build_filenamev((char **)rbuf->path->pdata);
    }

    if ((path != NULL) && (rbuf->extension != NULL)) {
        char *tmp = x_strconcat(path, ".", rbuf->extension, NULL);

        x_free(path);
        path = x_steal_pointer(&tmp);
    }

    return path;
}

xboolean x_path_buf_equal(xconstpointer v1, xconstpointer v2)
{
    if (v1 == v2) {
        return TRUE;
    }

    char *p1 = x_path_buf_to_path((XPathBuf *) v1);
    char *p2 = x_path_buf_to_path((XPathBuf *) v2);

    xboolean res = p1 != NULL && p2 != NULL ? x_str_equal(p1, p2) : FALSE;

    x_free(p1);
    x_free(p2);

    return res;
}
