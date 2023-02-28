#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>

#include <xlib/xlib/config.h>
#include <xlib/xlib/xlibconfig.h>
#include <xlib/xlib/xmem.h>
#include <xlib/xlib/xerror.h>
#include <xlib/xlib/xstdio.h>
#include <xlib/xlib/xatomic.h>
#include <xlib/xlib/xconvert.h>
#include <xlib/xlib/xlibintl.h>
#include <xlib/xlib/xmessages.h>
#include <xlib/xlib/xstrfuncs.h>
#include <xlib/xlib/xfileutils.h>
#include <xlib/xlib/xmappedfile.h>

#ifndef _O_BINARY
#define _O_BINARY           0
#endif

#ifndef O_CLOEXEC
#define O_CLOEXEC           0
#endif

#ifndef MAP_FAILED
#define MAP_FAILED          ((void *)-1)
#endif

struct _XMappedFile {
    xchar    *contents;
    xsize    length;
    xpointer free_func;
    int      ref_count;
};

static void x_mapped_file_destroy(XMappedFile *file)
{
    if (file->length) {
        munmap(file->contents, file->length);
    }

    x_slice_free(XMappedFile, file);
}

static XMappedFile *mapped_file_new_from_fd(int fd, xboolean writable, const xchar *filename, XError **error)
{
    struct stat st;
    XMappedFile *file;

    file = x_slice_new0(XMappedFile);
    file->ref_count = 1;
    file->free_func = (xpointer)x_mapped_file_destroy;

    if (fstat(fd, &st) == -1) {
        int save_errno = errno;
        xchar *display_filename = filename ? x_filename_display_name(filename) : NULL;

        x_set_error(error, X_FILE_ERROR, x_file_error_from_errno(save_errno),
            _("Failed to get attributes of file “%s%s%s%s”: fstat() failed: %s"),
            display_filename ? display_filename : "fd",
            display_filename ? "' " : "",
            display_filename ? display_filename : "",
            display_filename ? "'" : "",
            x_strerror(save_errno));
        x_free(display_filename);
        goto out;
    }

    if (st.st_size == 0 && S_ISREG(st.st_mode)) {
        file->length = 0;
        file->contents = NULL;
        return file;
    }

    file->contents = (xchar *)MAP_FAILED;

    if (sizeof(st.st_size) > sizeof(xsize) && st.st_size > (off_t)X_MAXSIZE) {
        errno = EINVAL;
    } else {
        file->length = (xsize) st.st_size;
        file->contents = (xchar *)mmap(NULL, file->length, writable ? PROT_READ|PROT_WRITE : PROT_READ, MAP_PRIVATE, fd, 0);
    }

    if (file->contents == MAP_FAILED) {
        int save_errno = errno;
        xchar *display_filename = filename ? x_filename_display_name(filename) : NULL;

        x_set_error(error,
            X_FILE_ERROR,
            x_file_error_from_errno(save_errno),
            _("Failed to map %s%s%s%s: mmap() failed: %s"),
            display_filename ? display_filename : "fd",
            display_filename ? "' " : "",
            display_filename ? display_filename : "",
            display_filename ? "'" : "",
            x_strerror(save_errno));
        x_free(display_filename);
        goto out;
    }

    return file;

out:
    x_slice_free(XMappedFile, file);
    return NULL;
}

XMappedFile *x_mapped_file_new (const xchar *filename, xboolean writable, XError **error)
{
    int fd;
    XMappedFile *file;

    x_return_val_if_fail(filename != NULL, NULL);
    x_return_val_if_fail(!error || *error == NULL, NULL);

    fd = x_open(filename, (writable ? O_RDWR : O_RDONLY) | _O_BINARY | O_CLOEXEC, 0);
    if (fd == -1) {
        int save_errno = errno;
        xchar *display_filename = x_filename_display_name(filename);

        x_set_error(error, X_FILE_ERROR, x_file_error_from_errno(save_errno), _("Failed to open file “%s”: open() failed: %s"), display_filename, x_strerror(save_errno));
        x_free(display_filename);
        return NULL;
    }

    file = mapped_file_new_from_fd(fd, writable, filename, error);
    close(fd);

    return file;
}

XMappedFile *x_mapped_file_new_from_fd(xint fd, xboolean writable, XError **error)
{
    return mapped_file_new_from_fd(fd, writable, NULL, error);
}

xsize x_mapped_file_get_length(XMappedFile *file)
{
    x_return_val_if_fail(file != NULL, 0);
    return file->length;
}

xchar *x_mapped_file_get_contents(XMappedFile *file)
{
    x_return_val_if_fail(file != NULL, NULL);
    return file->contents;
}

void x_mapped_file_free(XMappedFile *file)
{
    x_mapped_file_unref(file);
}

XMappedFile *x_mapped_file_ref(XMappedFile *file)
{
    x_return_val_if_fail(file != NULL, NULL);
    x_atomic_int_inc(&file->ref_count);

    return file;
}

void x_mapped_file_unref(XMappedFile *file)
{
    x_return_if_fail(file != NULL);

    if (x_atomic_int_dec_and_test(&file->ref_count)) {
        x_mapped_file_destroy(file);
    }
}

XBytes *x_mapped_file_get_bytes (XMappedFile *file)
{
    x_return_val_if_fail(file != NULL, NULL);
    return x_bytes_new_with_free_func(file->contents, file->length, (XDestroyNotify)x_mapped_file_unref, x_mapped_file_ref(file));
}
