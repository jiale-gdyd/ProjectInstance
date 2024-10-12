#include <string.h>
#include <xlib/xlib/config.h>

#include <xlib/xlib/xmem.h>
#include <xlib/xlib/xbytes.h>
#include <xlib/xlib/xarray.h>
#include <xlib/xlib/xslice.h>
#include <xlib/xlib/xatomic.h>
#include <xlib/xlib/xrefcount.h>
#include <xlib/xlib/xstrfuncs.h>
#include <xlib/xlib/xmessages.h>
#include <xlib/xlib/xtestutils.h>

#if XLIB_SIZEOF_VOID_P == 8
#define X_BYTES_MAX_INLINE          (128 - sizeof(XBytesInline))
#else
#define X_BYTES_MAX_INLINE          (64 - sizeof(XBytesInline))
#endif

struct _XBytes {
    xconstpointer   data;
    xsize           size;
    xatomicrefcount ref_count;
    XDestroyNotify  free_func;
    xpointer        user_data;
};

typedef struct {
    XBytes bytes;
    xsize  padding;
    xuint8 inline_data[];
} XBytesInline;
X_STATIC_ASSERT(X_STRUCT_OFFSET(XBytesInline, inline_data) == (6 * XLIB_SIZEOF_VOID_P));

XBytes *x_bytes_new(xconstpointer data, xsize size)
{
    x_return_val_if_fail(data != NULL || size == 0, NULL);

    if (size <= X_BYTES_MAX_INLINE) {
        XBytesInline *bytes;
        bytes = x_malloc(sizeof *bytes + size);
        bytes->bytes.data = bytes->inline_data;
        bytes->bytes.size = size;
        bytes->bytes.free_func = NULL;
        bytes->bytes.user_data = NULL;
        x_atomic_ref_count_init(&bytes->bytes.ref_count);
        memcpy(bytes->inline_data, data, size);
        return(XBytes *)bytes;
    }

    return x_bytes_new_take(x_memdup2(data, size), size);
}

XBytes *x_bytes_new_take(xpointer data, xsize size)
{
    return x_bytes_new_with_free_func(data, size, x_free, data);
}

XBytes *x_bytes_new_static(xconstpointer data, xsize size)
{
    return x_bytes_new_with_free_func(data, size, NULL, NULL);
}

XBytes *x_bytes_new_with_free_func(xconstpointer data, xsize size, XDestroyNotify free_func, xpointer user_data)
{
    XBytes *bytes;

    x_return_val_if_fail(data != NULL || size == 0, NULL);

    bytes = x_new(XBytes, 1);
    bytes->data = data;
    bytes->size = size;
    bytes->free_func = free_func;
    bytes->user_data = user_data;
    x_atomic_ref_count_init(&bytes->ref_count);

    return (XBytes *)bytes;
}

XBytes *x_bytes_new_from_bytes(XBytes *bytes, xsize offset, xsize length)
{
    xchar *base;

    x_return_val_if_fail(bytes != NULL, NULL);
    x_return_val_if_fail(offset <= bytes->size, NULL);
    x_return_val_if_fail(offset + length <= bytes->size, NULL);

    if (offset == 0 && length == bytes->size) {
        return x_bytes_ref(bytes);
    }

    base = (xchar *)bytes->data + offset;

    while (bytes->free_func == (xpointer)x_bytes_unref) {
        bytes = (XBytes *)bytes->user_data;
    }

    x_return_val_if_fail(bytes != NULL, NULL);
    x_return_val_if_fail(base >= (xchar *)bytes->data, NULL);
    x_return_val_if_fail(base <= (xchar *)bytes->data + bytes->size, NULL);
    x_return_val_if_fail(base + length <= (xchar *)bytes->data + bytes->size, NULL);

    return x_bytes_new_with_free_func(base, length, (XDestroyNotify)x_bytes_unref, x_bytes_ref(bytes));
}

xconstpointer x_bytes_get_data(XBytes *bytes, xsize *size)
{
    x_return_val_if_fail(bytes != NULL, NULL);
    if (size) {
        *size = bytes->size;
    }

    return bytes->data;
}

xsize x_bytes_get_size(XBytes *bytes)
{
    x_return_val_if_fail(bytes != NULL, 0);
    return bytes->size;
}

XBytes *x_bytes_ref(XBytes *bytes)
{
    x_return_val_if_fail(bytes != NULL, NULL);
    x_atomic_ref_count_inc(&bytes->ref_count);

    return bytes;
}

void x_bytes_unref(XBytes *bytes)
{
    if (bytes == NULL) {
        return;
    }

    if (x_atomic_ref_count_dec(&bytes->ref_count)) {
        if (bytes->free_func != NULL) {
            bytes->free_func(bytes->user_data);
        }

        x_free(bytes);
    }
}

xboolean x_bytes_equal(xconstpointer bytes1, xconstpointer bytes2)
{
    const XBytes *b1 = (const XBytes *)bytes1;
    const XBytes *b2 = (const XBytes *)bytes2;

    x_return_val_if_fail(bytes1 != NULL, FALSE);
    x_return_val_if_fail(bytes2 != NULL, FALSE);

    return b1->size == b2->size && (b1->size == 0 || memcmp(b1->data, b2->data, b1->size) == 0);
}

xuint x_bytes_hash(xconstpointer bytes)
{
    xuint32 h = 5381;
    const signed char *p, *e;
    const XBytes *a = (const XBytes *)bytes;

    x_return_val_if_fail(bytes != NULL, 0);

    for (p = (signed char *)a->data, e = (signed char *)a->data + a->size; p != e; p++) {
        h = (h << 5) + h + *p;
    }

    return h;
}

xint x_bytes_compare(xconstpointer bytes1, xconstpointer bytes2)
{
    xint ret;
    const XBytes *b1 = (const XBytes *)bytes1;
    const XBytes *b2 = (const XBytes *)bytes2;

    x_return_val_if_fail(bytes1 != NULL, 0);
    x_return_val_if_fail(bytes2 != NULL, 0);

    ret = memcmp(b1->data, b2->data, MIN(b1->size, b2->size));
    if (ret == 0 && b1->size != b2->size) {
        ret = b1->size < b2->size ? -1 : 1;
    }

    return ret;
}

static xpointer try_steal_and_unref(XBytes *bytes, XDestroyNotify free_func, xsize *size)
{
    xpointer result;

    if (bytes->free_func != free_func || bytes->data == NULL || bytes->user_data != bytes->data) {
        return NULL;
    }

    if (x_atomic_ref_count_compare(&bytes->ref_count, 1)) {
        *size = bytes->size;
        result = (xpointer)bytes->data;
        x_assert(result != NULL);
        x_free(bytes);
        return result;
    }

    return NULL;
}

xpointer x_bytes_unref_to_data(XBytes *bytes, xsize *size)
{
    xpointer result;

    x_return_val_if_fail(bytes != NULL, NULL);
    x_return_val_if_fail(size != NULL, NULL);

    result = try_steal_and_unref(bytes, x_free, size);
    if (result == NULL) {
        result = x_memdup2(bytes->data, bytes->size);
        *size = bytes->size;
        x_bytes_unref(bytes);
    }

    return result;
}

XByteArray *x_bytes_unref_to_array(XBytes *bytes)
{
    xsize size;
    xpointer data;

    x_return_val_if_fail(bytes != NULL, NULL);

    data = x_bytes_unref_to_data(bytes, &size);
    return x_byte_array_new_take((xuint8 *)data, size);
}

xconstpointer x_bytes_get_region(XBytes *bytes, xsize element_size, xsize offset, xsize n_elements)
{
    xsize total_size;
    xsize end_offset;

    x_return_val_if_fail(element_size > 0, NULL);

    if (!x_size_checked_mul(&total_size, element_size, n_elements)) {
        return NULL;
    }

    if (!x_size_checked_add(&end_offset, offset, total_size)) {
        return NULL;
    }

    if (end_offset > bytes->size) {
        return NULL;
    }

    return ((xuchar *)bytes->data) + offset;
}
