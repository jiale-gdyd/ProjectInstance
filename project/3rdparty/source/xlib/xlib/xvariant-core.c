#include <string.h>
#include <xlib/xlib/config.h>
#include <xlib/xlib/xmem.h>
#include <xlib/xlib/xbytes.h>
#include <xlib/xlib/xslice.h>
#include <xlib/xlib/xatomic.h>
#include <xlib/xlib/xbitlock.h>
#include <xlib/xlib/xrefcount.h>
#include <xlib/xlib/xtestutils.h>
#include <xlib/xlib/xlib_trace.h>
#include <xlib/xlib/xvariant-core.h>
#include <xlib/xlib/xvariant-internal.h>
#include <xlib/xlib/xvariant-serialiser.h>

struct _XVariant {
    XVariantTypeInfo      *type_info;
    xsize                 size;

    union {
        struct {
            XBytes        *bytes;
            xconstpointer data;
            xsize         ordered_offsets_up_to;
            xsize         checked_offsets_up_to;
        } serialised;

        struct {
            XVariant      **children;
            xsize         n_children;
        } tree;
    } contents;

    xint                  state;
    xatomicrefcount       ref_count;
    xsize                 depth;
};

#define STATE_LOCKED            1
#define STATE_SERIALISED        2
#define STATE_TRUSTED           4
#define STATE_FLOATING          8

static void x_variant_lock(XVariant *value)
{
    x_bit_lock(&value->state, 0);
}

static void x_variant_unlock(XVariant *value)
{
    x_bit_unlock(&value->state, 0);
}

static void x_variant_release_children(XVariant *value)
{
    xsize i;

    x_assert(value->state & STATE_LOCKED);
    x_assert(~value->state & STATE_SERIALISED);

    for (i = 0; i < value->contents.tree.n_children; i++) {
        x_variant_unref(value->contents.tree.children[i]);
    }

    x_free(value->contents.tree.children);
}

static void x_variant_fill_gvs(XVariantSerialised *, xpointer);

static void x_variant_ensure_size(XVariant *value)
{
    x_assert(value->state & STATE_LOCKED);

    if (value->size == (xsize)-1) {
        xsize n_children;
        xpointer *children;

        children = (xpointer *)value->contents.tree.children;
        n_children = value->contents.tree.n_children;
        value->size = x_variant_serialiser_needed_size(value->type_info, x_variant_fill_gvs, children, n_children);
    }
}

inline static XVariantSerialised x_variant_to_serialised(XVariant *value)
{
    x_assert(value->state & STATE_SERIALISED);
    {
        XVariantSerialised serialised = {
            value->type_info,
            (xpointer)value->contents.serialised.data,
            value->size,
            value->depth,
            value->contents.serialised.ordered_offsets_up_to,
            value->contents.serialised.checked_offsets_up_to,
        };

        return serialised;
    }
}

static void x_variant_serialise(XVariant *value, xpointer data)
{
    xsize n_children;
    xpointer *children;
    XVariantSerialised serialised = { 0, };

    x_assert(~value->state & STATE_SERIALISED);
    x_assert(value->state & STATE_LOCKED);

    serialised.type_info = value->type_info;
    serialised.size = value->size;
    serialised.data = (xuchar *)data;
    serialised.depth = value->depth;
    serialised.ordered_offsets_up_to = 0;
    serialised.checked_offsets_up_to = 0;

    children = (xpointer *)value->contents.tree.children;
    n_children = value->contents.tree.n_children;

    x_variant_serialiser_serialise(serialised, x_variant_fill_gvs, children, n_children);
}

static void x_variant_fill_gvs(XVariantSerialised *serialised, xpointer data)
{
    XVariant *value = (XVariant *)data;

    x_variant_lock(value);
    x_variant_ensure_size(value);
    x_variant_unlock(value);

    if (serialised->type_info == NULL) {
        serialised->type_info = value->type_info;
    }
    x_assert(serialised->type_info == value->type_info);

    if (serialised->size == 0) {
        serialised->size = value->size;
    }

    x_assert(serialised->size == value->size);
    serialised->depth = value->depth;

    if (value->state & STATE_SERIALISED) {
        serialised->ordered_offsets_up_to = value->contents.serialised.ordered_offsets_up_to;
        serialised->checked_offsets_up_to = value->contents.serialised.checked_offsets_up_to;
    } else {
        serialised->ordered_offsets_up_to = 0;
        serialised->checked_offsets_up_to = 0;
    }

    if (serialised->data) {
        x_variant_store(value, serialised->data);
    }
}

static void x_variant_ensure_serialised(XVariant *value)
{
    x_assert(value->state & STATE_LOCKED);

    if (~value->state & STATE_SERIALISED) {
        XBytes *bytes;
        xpointer data;

        TRACE(XLIB_VARIANT_START_SERIALISE(value, value->type_info));
        x_variant_ensure_size(value);
        data = x_malloc(value->size);
        x_variant_serialise(value, data);

        x_variant_release_children(value);

        bytes = x_bytes_new_take(data, value->size);
        value->contents.serialised.data = x_bytes_get_data(bytes, NULL);
        value->contents.serialised.bytes = bytes;
        value->contents.serialised.ordered_offsets_up_to = X_MAXSIZE;
        value->contents.serialised.checked_offsets_up_to = X_MAXSIZE;
        value->state |= STATE_SERIALISED;
        TRACE(XLIB_VARIANT_END_SERIALISE(value, value->type_info));
    }
}

static XVariant *x_variant_alloc(const XVariantType *type, xboolean serialised, xboolean trusted)
{
    XVariant *value;

    value = x_slice_new(XVariant);
    value->type_info = x_variant_type_info_get(type);
    value->state = (serialised ? STATE_SERIALISED : 0) | (trusted ? STATE_TRUSTED : 0) | STATE_FLOATING;
    value->size = (xssize)-1;
    x_atomic_ref_count_init(&value->ref_count);
    value->depth = 0;

    return value;
}

XVariant *x_variant_new_from_bytes(const XVariantType *type, XBytes *bytes, xboolean trusted)
{
    xsize size;
    XVariant *value;
    xuint alignment;
    XBytes *owned_bytes = NULL;
    XVariantSerialised serialised;

    value = x_variant_alloc(type, TRUE, trusted);
    x_variant_type_info_query(value->type_info, &alignment, &size);

    serialised.type_info = value->type_info;
    serialised.data = (xuchar *)x_bytes_get_data(bytes, &serialised.size);
    serialised.depth = 0;
    serialised.ordered_offsets_up_to = trusted ? X_MAXSIZE : 0;
    serialised.checked_offsets_up_to = trusted ? X_MAXSIZE : 0;

    if (!x_variant_serialised_check(serialised)) {
#ifdef HAVE_POSIX_MEMALIGN
        xpointer aligned_data = NULL;
        xsize aligned_size = x_bytes_get_size(bytes);

        if ((aligned_size != 0) && (posix_memalign(&aligned_data, MAX(sizeof(void *), alignment + 1), aligned_size) != 0)) {
            x_error("posix_memalign failed");
        }

        if (aligned_size != 0) {
            memcpy(aligned_data, x_bytes_get_data(bytes, NULL), aligned_size);
        }

        bytes = owned_bytes = x_bytes_new_with_free_func(aligned_data, aligned_size, free, aligned_data);
        aligned_data = NULL;
#else
        bytes = owned_bytes = x_bytes_new(x_bytes_get_data(bytes, NULL), x_bytes_get_size(bytes));
#endif
    }

    value->contents.serialised.bytes = x_bytes_ref(bytes);

    if (size && x_bytes_get_size(bytes) != size) {
        value->contents.serialised.data = NULL;
        value->size = size;
    } else {
        value->contents.serialised.data = x_bytes_get_data(bytes, &value->size);
    }

    value->contents.serialised.ordered_offsets_up_to = trusted ? X_MAXSIZE : 0;
    value->contents.serialised.checked_offsets_up_to = trusted ? X_MAXSIZE : 0;
    x_clear_pointer(&owned_bytes, x_bytes_unref);

    TRACE(XLIB_VARIANT_FROM_BUFFER(value, value->type_info, value->ref_count, value->state));

    return value;
}

XVariant *x_variant_new_from_children(const XVariantType *type, XVariant **children, xsize n_children, xboolean trusted)
{
    XVariant *value;

    value = x_variant_alloc(type, FALSE, trusted);
    value->contents.tree.children = children;
    value->contents.tree.n_children = n_children;
    TRACE(XLIB_VARIANT_FROM_CHILDREN(value, value->type_info, value->ref_count, value->state));

    return value;
}

XVariantTypeInfo *x_variant_get_type_info(XVariant *value)
{
    return value->type_info;
}

xboolean x_variant_is_trusted(XVariant *value)
{
    return (value->state & STATE_TRUSTED) != 0;
}

xsize x_variant_get_depth(XVariant *value)
{
    return value->depth;
}

void x_variant_unref(XVariant *value)
{
    x_return_if_fail(value != NULL);

    TRACE(XLIB_VARIANT_UNREF(value, value->type_info, value->ref_count, value->state));

    if (x_atomic_ref_count_dec(&value->ref_count)) {
        if X_UNLIKELY(value->state & STATE_LOCKED) {
            x_critical("attempting to free a locked XVariant instance. This should never happen.");
        }

        value->state |= STATE_LOCKED;
        x_variant_type_info_unref(value->type_info);

        if (value->state & STATE_SERIALISED) {
            x_bytes_unref(value->contents.serialised.bytes);
        } else {
            x_variant_release_children(value);
        }

        memset(value, 0, sizeof (XVariant));
        x_slice_free(XVariant, value);
    }
}

XVariant *x_variant_ref(XVariant *value)
{
    x_return_val_if_fail(value != NULL, NULL);

    TRACE(XLIB_VARIANT_REF(value, value->type_info, value->ref_count, value->state));

    x_atomic_ref_count_inc(&value->ref_count);

    return value;
}

XVariant *x_variant_ref_sink(XVariant *value)
{
    x_return_val_if_fail(value != NULL, NULL);
    x_return_val_if_fail(!x_atomic_ref_count_compare(&value->ref_count, 0), NULL);

    x_variant_lock(value);

    TRACE(XLIB_VARIANT_REF_SINK(value, value->type_info, value->ref_count, value->state, value->state & STATE_FLOATING));

    if (~value->state & STATE_FLOATING) {
        x_variant_ref(value);
    } else {
        value->state &= ~STATE_FLOATING;
    }
    x_variant_unlock(value);

    return value;
}

XVariant *x_variant_take_ref(XVariant *value)
{
    x_return_val_if_fail(value != NULL, NULL);
    x_return_val_if_fail(!x_atomic_ref_count_compare(&value->ref_count, 0), NULL);

    TRACE(XLIB_VARIANT_TAKE_REF(value, value->type_info, value->ref_count, value->state, value->state & STATE_FLOATING));

    x_atomic_int_and(&value->state, ~STATE_FLOATING);
    return value;
}

xboolean x_variant_is_floating(XVariant *value)
{
    x_return_val_if_fail(value != NULL, FALSE);
    return (value->state & STATE_FLOATING) != 0;
}

xsize x_variant_get_size(XVariant *value)
{
    x_variant_lock(value);
    x_variant_ensure_size(value);
    x_variant_unlock(value);

    return value->size;
}

xconstpointer x_variant_get_data(XVariant *value)
{
    x_variant_lock(value);
    x_variant_ensure_serialised(value);
    x_variant_unlock(value);

    return value->contents.serialised.data;
}

XBytes *x_variant_get_data_as_bytes(XVariant *value)
{
    xsize size;
    xsize bytes_size;
    const xchar *data;
    const xchar *bytes_data;

    x_variant_lock(value);
    x_variant_ensure_serialised(value);
    x_variant_unlock(value);

    bytes_data = (const xchar *)x_bytes_get_data(value->contents.serialised.bytes, &bytes_size);
    data = (const xchar *)value->contents.serialised.data;
    size = value->size;

    if (data == NULL) {
        x_assert(size == 0);
        data = bytes_data;
    }

    if (data == bytes_data && size == bytes_size) {
        return x_bytes_ref(value->contents.serialised.bytes);
    } else {
        return x_bytes_new_from_bytes(value->contents.serialised.bytes, data - bytes_data, size);
    }
}

xsize x_variant_n_children(XVariant *value)
{
    xsize n_children;

    x_variant_lock(value);
    if (value->state & STATE_SERIALISED) {
        n_children = x_variant_serialised_n_children(x_variant_to_serialised(value));
    } else {
        n_children = value->contents.tree.n_children;
    }
    x_variant_unlock(value);

    return n_children;
}

XVariant *x_variant_get_child_value(XVariant *value, xsize index_)
{
    x_return_val_if_fail(value->depth < X_MAXSIZE, NULL);

    if (~x_atomic_int_get(&value->state) & STATE_SERIALISED) {
        x_return_val_if_fail(index_ < x_variant_n_children(value), NULL);

        x_variant_lock(value);
        if (~value->state & STATE_SERIALISED) {
            XVariant *child;

            child = x_variant_ref(value->contents.tree.children[index_]);
            x_variant_unlock(value);

            return child;
        }
        x_variant_unlock(value);
    }

    {
        XVariant *child;
        XVariantSerialised s_child;
        XVariantSerialised serialised = x_variant_to_serialised(value);

        s_child = x_variant_serialised_get_child(serialised, index_);
        value->contents.serialised.ordered_offsets_up_to = MAX(value->contents.serialised.ordered_offsets_up_to, serialised.ordered_offsets_up_to);
        value->contents.serialised.checked_offsets_up_to = MAX(value->contents.serialised.checked_offsets_up_to, serialised.checked_offsets_up_to);

        if (!(value->state & STATE_TRUSTED) && x_variant_type_info_query_depth(s_child.type_info) >= X_VARIANT_MAX_RECURSION_DEPTH - value->depth) {
            x_assert(x_variant_is_of_type(value, X_VARIANT_TYPE_VARIANT));
            x_variant_type_info_unref(s_child.type_info);

            return x_variant_new_tuple (NULL, 0);
        }

        child = x_slice_new(XVariant);
        child->type_info = s_child.type_info;
        child->state = (value->state & STATE_TRUSTED) | STATE_SERIALISED;
        child->size = s_child.size;
        x_atomic_ref_count_init(&child->ref_count);
        child->depth = value->depth + 1;
        child->contents.serialised.bytes =
        x_bytes_ref(value->contents.serialised.bytes);
        child->contents.serialised.data = s_child.data;
        child->contents.serialised.ordered_offsets_up_to = (value->state & STATE_TRUSTED) ? X_MAXSIZE : s_child.ordered_offsets_up_to;
        child->contents.serialised.checked_offsets_up_to = (value->state & STATE_TRUSTED) ? X_MAXSIZE : s_child.checked_offsets_up_to;

        TRACE(XLIB_VARIANT_FROM_PARENT(child, child->type_info, child->ref_count, child->state, value));

        return child;
    }
}

XVariant *x_variant_maybe_get_child_value(XVariant *value, xsize index_)
{
    x_return_val_if_fail(value->depth < X_MAXSIZE, NULL);

    if (x_atomic_int_get(&value->state) & STATE_SERIALISED) {
        x_return_val_if_fail(index_ < x_variant_n_children(value), NULL);

        x_variant_lock(value);

        if (~value->state & STATE_SERIALISED) {
            XVariant *child;

            child = x_variant_ref(value->contents.tree.children[index_]);
            x_variant_unlock(value);

            return child;
        }

        x_variant_unlock(value);
    }

    {
        XVariantSerialised s_child;
        XVariantSerialised serialised = x_variant_to_serialised(value);

        s_child = x_variant_serialised_get_child(serialised, index_);

        if (!(value->state & STATE_TRUSTED) && s_child.data == NULL) {
            x_variant_type_info_unref(s_child.type_info);
            return NULL;
        }

        x_variant_type_info_unref(s_child.type_info);
        return x_variant_get_child_value(value, index_);
    }
}

void x_variant_store(XVariant *value, xpointer data)
{
    x_variant_lock(value);
    if (value->state & STATE_SERIALISED) {
        if (value->contents.serialised.data != NULL) {
            memcpy(data, value->contents.serialised.data, value->size);
        } else {
            memset(data, 0, value->size);
        }
    } else {
        x_variant_serialise(value, data);
    }
    x_variant_unlock(value);
}

xboolean x_variant_is_normal_form(XVariant *value)
{
    if (value->state & STATE_TRUSTED) {
        return TRUE;
    }

    x_variant_lock(value);
    if (value->depth >= X_VARIANT_MAX_RECURSION_DEPTH) {
        return FALSE;
    }

    if (value->state & STATE_SERIALISED) {
        if (x_variant_serialised_is_normal(x_variant_to_serialised(value))) {
            value->state |= STATE_TRUSTED;
        }
    } else {
        xsize i;
        xboolean normal = TRUE;

        for (i = 0; i < value->contents.tree.n_children; i++) {
            normal &= x_variant_is_normal_form(value->contents.tree.children[i]);
        }

        if (normal) {
            value->state |= STATE_TRUSTED;
        }
    }
    x_variant_unlock(value);

    return (value->state & STATE_TRUSTED) != 0;
}
