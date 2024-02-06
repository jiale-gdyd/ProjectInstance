#include <string.h>
#include <stdlib.h>

#include <xlib/xlib/config.h>
#include <xlib/xlib/xmem.h>
#include <xlib/xlib/xhash.h>
#include <xlib/xlib/xarray.h>
#include <xlib/xlib/xbytes.h>
#include <xlib/xlib/xslice.h>
#include <xlib/xlib/xqsort.h>
#include <xlib/xlib/xalloca.h>
#include <xlib/xlib/xthread.h>
#include <xlib/xlib/xmessages.h>
#include <xlib/xlib/xrefcount.h>
#include <xlib/xlib/xtestutils.h>
#include <xlib/xlib/xutilsprivate.h>

#define MIN_ARRAY_SIZE              16

typedef struct _XRealArray XRealArray;

struct _XRealArray {
    xuint8          *data;
    xuint           len;
    xuint           elt_capacity;
    xuint           elt_size;
    xuint           zero_terminated : 1;
    xuint           clear : 1;
    xatomicrefcount ref_count;
    XDestroyNotify  clear_func;
};

#define x_array_elt_len(array, i)                   ((xsize)(array)->elt_size * (i))
#define x_array_elt_pos(array, i)                   ((array)->data + x_array_elt_len((array),(i)))
#define x_array_elt_zero(array, pos, len)           (memset(x_array_elt_pos((array), pos), 0, x_array_elt_len((array), len)))
#define x_array_zero_terminate(array)                   \
    X_STMT_START{                                       \
        if ((array)->zero_terminated) {                 \
            x_array_elt_zero((array), (array)->len, 1); \
        }                                               \
    } X_STMT_END

static void x_array_maybe_expand(XRealArray *array, xuint len);

XArray *x_array_new(xboolean zero_terminated, xboolean clear, xuint elt_size)
{
    x_return_val_if_fail(elt_size > 0, NULL);
#if (UINT_WIDTH / 8) >= XLIB_SIZEOF_SIZE_T
    x_return_val_if_fail(elt_size <= X_MAXSIZE / 2 - 1, NULL);
#endif

    return x_array_sized_new(zero_terminated, clear, elt_size, 0);
}

XArray *x_array_new_take(xpointer data, xsize len, xboolean clear, xsize element_size)
{
    XArray *array;
    XRealArray *rarray;

    x_return_val_if_fail(data != NULL || len == 0, NULL);
    x_return_val_if_fail(len <= X_MAXUINT, NULL);
    x_return_val_if_fail(element_size <= X_MAXUINT, NULL);

    array = x_array_sized_new(FALSE, clear, element_size, 0);
    rarray = (XRealArray *)array;
    rarray->data = (xuint8 *)x_steal_pointer(&data);
    rarray->len = len;
    rarray->elt_capacity = len;

    return array;
}

XArray *x_array_new_take_zero_terminated(xpointer data, xboolean clear, xsize element_size)
{
    XArray *array;
    xsize len = 0;

    x_return_val_if_fail(element_size <= X_MAXUINT, NULL);

    if (data != NULL) {
        xuint8 *array_data = data;

        for (xsize i = 0; ; ++i) {
            const xuint8 *element_start = array_data + (i * element_size);
            if (*element_start == 0 && memcmp(element_start, element_start + 1, element_size - 1) == 0) {
                break;
            }

            len += 1;
        }
    }

    x_return_val_if_fail(len <= X_MAXUINT, NULL);

    array = x_array_new_take(data, len, clear, element_size);
    ((XRealArray *)array)->zero_terminated = TRUE;

    return array;
}

xpointer x_array_steal(XArray *array, xsize *len)
{
    xpointer segment;
    XRealArray *rarray;

    x_return_val_if_fail(array != NULL, NULL);

    rarray = (XRealArray *)array;
    segment = (xpointer)rarray->data;

    if (len != NULL) {
        *len = rarray->len;
    }

    rarray->data = NULL;
    rarray->len = 0;
    rarray->elt_capacity = 0;

    return segment;
}

XArray *x_array_sized_new(xboolean zero_terminated, xboolean clear, xuint elt_size, xuint reserved_size)
{
    XRealArray *array;

    x_return_val_if_fail(elt_size > 0, NULL);
#if (UINT_WIDTH / 8) >= XLIB_SIZEOF_SIZE_T
    x_return_val_if_fail(elt_size <= X_MAXSIZE / 2 - 1, NULL);
#endif

    array = x_slice_new(XRealArray);

    array->data = NULL;
    array->len = 0;
    array->elt_capacity = 0;
    array->zero_terminated = (zero_terminated ? 1 : 0);
    array->clear = (clear ? 1 : 0);
    array->elt_size = elt_size;
    array->clear_func = NULL;

    x_atomic_ref_count_init(&array->ref_count);

    if (array->zero_terminated || (reserved_size != 0)) {
        x_array_maybe_expand(array, reserved_size);
        x_assert(array->data != NULL);
        x_array_zero_terminate(array);
    }

    return (XArray *)array;
}

void x_array_set_clear_func(XArray *array, XDestroyNotify clear_func)
{
    XRealArray *rarray = (XRealArray *)array;
    x_return_if_fail(array != NULL);

    rarray->clear_func = clear_func;
}

XArray *x_array_ref(XArray *array)
{
    XRealArray *rarray = (XRealArray *)array;
    x_return_val_if_fail(array, NULL);

    x_atomic_ref_count_inc(&rarray->ref_count);
    return array;
}

typedef enum {
    FREE_SEGMENT     = 1 << 0,
    PRESERVE_WRAPPER = 1 << 1
} ArrayFreeFlags;

static xchar *array_free(XRealArray *, ArrayFreeFlags);

void x_array_unref(XArray *array)
{
    XRealArray *rarray = (XRealArray *)array;
    x_return_if_fail(array);

    if (x_atomic_ref_count_dec(&rarray->ref_count)) {
        array_free(rarray, FREE_SEGMENT);
    }
}

xuint x_array_get_element_size(XArray *array)
{
    XRealArray *rarray = (XRealArray *)array;
    x_return_val_if_fail(array, 0);

    return rarray->elt_size;
}

xchar *x_array_free(XArray *farray, xboolean free_segment)
{
    ArrayFreeFlags flags;
    XRealArray *array = (XRealArray *)farray;

    x_return_val_if_fail(array, NULL);

    flags = (ArrayFreeFlags)(free_segment ? FREE_SEGMENT : 0);
    if (!x_atomic_ref_count_dec(&array->ref_count)) {
        flags = (ArrayFreeFlags)(flags | PRESERVE_WRAPPER);
    }

    return array_free(array, flags);
}

static xchar *array_free(XRealArray *array, ArrayFreeFlags flags)
{
    xchar *segment;

    if (flags & FREE_SEGMENT) {
        if (array->clear_func != NULL) {
            xuint i;

            for (i = 0; i < array->len; i++) {
                array->clear_func(x_array_elt_pos(array, i));
            }
        }

        x_free(array->data);
        segment = NULL;
    } else {
        segment = (xchar *)array->data;
    }

    if (flags & PRESERVE_WRAPPER) {
        array->data = NULL;
        array->len = 0;
        array->elt_capacity = 0;
    } else {
        x_slice_free1(sizeof(XRealArray), array);
    }

    return segment;
}

XArray *x_array_append_vals(XArray *farray, xconstpointer data, xuint len)
{
    XRealArray *array = (XRealArray *)farray;
    x_return_val_if_fail(array, NULL);

    if (len == 0) {
        return farray;
    }

    x_array_maybe_expand(array, len);

    memcpy(x_array_elt_pos(array, array->len), data, x_array_elt_len(array, len));
    array->len += len;
    x_array_zero_terminate(array);

    return farray;
}

XArray *x_array_prepend_vals(XArray *farray, xconstpointer data, xuint len)
{
    XRealArray *array = (XRealArray *)farray;
    x_return_val_if_fail(array, NULL);

    if (len == 0) {
        return farray;
    }

    x_array_maybe_expand(array, len);

    memmove(x_array_elt_pos(array, len), x_array_elt_pos(array, 0), x_array_elt_len(array, array->len));
    memcpy(x_array_elt_pos(array, 0), data, x_array_elt_len(array, len));

    array->len += len;
    x_array_zero_terminate(array);

    return farray;
}

XArray *x_array_insert_vals(XArray *farray, xuint index_, xconstpointer data, xuint len)
{
    XRealArray *array = (XRealArray *)farray;
    x_return_val_if_fail(array, NULL);

    if (len == 0) {
        return farray;
    }

    if (index_ >= array->len) {
        x_array_maybe_expand(array, index_ - array->len + len);
        return x_array_append_vals(x_array_set_size(farray, index_), data, len);
    }

    x_array_maybe_expand (array, len);
    memmove(x_array_elt_pos(array, len + index_), x_array_elt_pos(array, index_), x_array_elt_len(array, array->len - index_));
    memcpy(x_array_elt_pos(array, index_), data, x_array_elt_len(array, len));

    array->len += len;
    x_array_zero_terminate(array);

    return farray;
}

XArray *x_array_set_size (XArray *farray, xuint length)
{
    XRealArray *array = (XRealArray *)farray;
    x_return_val_if_fail(array, NULL);

    if (length > array->len) {
        x_array_maybe_expand(array, length - array->len);

        if (array->clear) {
            x_array_elt_zero(array, array->len, length - array->len);
        }
    } else if (length < array->len) {
        x_array_remove_range(farray, length, array->len - length);
    }

    array->len = length;
    x_array_zero_terminate(array);

    return farray;
}

XArray *x_array_remove_index(XArray *farray, xuint index_)
{
    XRealArray *array = (XRealArray *)farray;

    x_return_val_if_fail(array, NULL);
    x_return_val_if_fail(index_ < array->len, NULL);

    if (array->clear_func != NULL) {
        array->clear_func(x_array_elt_pos(array, index_));
    }

    if (index_ != (array->len - 1)) {
        memmove(x_array_elt_pos(array, index_), x_array_elt_pos(array, index_ + 1), x_array_elt_len(array, array->len - index_ - 1));
    }
    array->len -= 1;

    if (X_UNLIKELY(x_mem_gc_friendly)) {
        x_array_elt_zero(array, array->len, 1);
    } else {
        x_array_zero_terminate(array);
    }

    return farray;
}

XArray *x_array_remove_index_fast(XArray *farray, xuint index_)
{
    XRealArray *array = (XRealArray *)farray;

    x_return_val_if_fail(array, NULL);
    x_return_val_if_fail(index_ < array->len, NULL);

    if (array->clear_func != NULL) {
        array->clear_func(x_array_elt_pos(array, index_));
    }

    if (index_ != (array->len - 1)) {
        memcpy(x_array_elt_pos(array, index_), x_array_elt_pos(array, array->len - 1), x_array_elt_len(array, 1));
    }
    array->len -= 1;

    if (X_UNLIKELY(x_mem_gc_friendly)) {
        x_array_elt_zero(array, array->len, 1);
    } else {
        x_array_zero_terminate(array);
    }

    return farray;
}

XArray *x_array_remove_range(XArray *farray, xuint index_, xuint length)
{
    XRealArray *array = (XRealArray *)farray;

    x_return_val_if_fail(array, NULL);
    x_return_val_if_fail(index_ <= array->len, NULL);
    x_return_val_if_fail(index_ <= X_MAXUINT - length, NULL);
    x_return_val_if_fail((index_ + length) <= array->len, NULL);

    if (array->clear_func != NULL) {
        xuint i;
        for (i = 0; i < length; i++) {
            array->clear_func(x_array_elt_pos(array, index_ + i));
        }
    }

    if ((index_ + length) != array->len) {
        memmove(x_array_elt_pos(array, index_), x_array_elt_pos(array, index_ + length), (array->len - (index_ + length)) * array->elt_size);
    }
    array->len -= length;

    if (X_UNLIKELY(x_mem_gc_friendly)) {
        x_array_elt_zero(array, array->len, length);
    } else {
        x_array_zero_terminate(array);
    }

    return farray;
}

void x_array_sort(XArray *farray, XCompareFunc compare_func)
{
    XRealArray *array = (XRealArray *)farray;
    x_return_if_fail(array != NULL);

    if (array->len > 0) {
        x_qsort_with_data(array->data, array->len, array->elt_size, (XCompareDataFunc)compare_func, NULL);
    }
}

void x_array_sort_with_data(XArray *farray, XCompareDataFunc compare_func, xpointer user_data)
{
    XRealArray *array = (XRealArray *)farray;
    x_return_if_fail(array != NULL);

    if (array->len > 0) {
        x_qsort_with_data(array->data, array->len, array->elt_size, compare_func, user_data);
    }
}

xboolean x_array_binary_search(XArray *array, xconstpointer target, XCompareFunc compare_func, xuint *out_match_index)
{
    xint val;
    xboolean result = FALSE;
    xuint left, middle = 0, right;
    XRealArray *_array = (XRealArray *)array;

    x_return_val_if_fail(_array != NULL, FALSE);
    x_return_val_if_fail(compare_func != NULL, FALSE);

    if (X_LIKELY(_array->len)) {
        left = 0;
        right = _array->len - 1;

        while (left <= right) {
            middle = left + (right - left) / 2;

            val = compare_func(_array->data + (_array->elt_size * middle), target);
            if (val == 0) {
                result = TRUE;
                break;
            } else if (val < 0) {
                left = middle + 1;
            } else if (middle > 0) {
                right = middle - 1;
            } else {
                break;
            }
        }
    }

    if (result && (out_match_index != NULL)) {
        *out_match_index = middle;
    }

    return result;
}

static void x_array_maybe_expand(XRealArray *array, xuint len)
{
    xuint max_len, want_len;

    max_len = MIN(X_MAXSIZE / 2 / array->elt_size, X_MAXUINT) - array->zero_terminated;

    if X_UNLIKELY((max_len - array->len) < len) {
        x_error("adding %u to array would overflow", len);
    }

    want_len = array->len + len + array->zero_terminated;
    if (want_len > array->elt_capacity) {
        xsize want_alloc = x_nearest_pow(x_array_elt_len(array, want_len));
        want_alloc = MAX(want_alloc, MIN_ARRAY_SIZE);

        array->data = (xuint8 *)x_realloc(array->data, want_alloc);

        if (X_UNLIKELY(x_mem_gc_friendly)) {
            memset(x_array_elt_pos(array, array->elt_capacity), 0, x_array_elt_len(array, want_len - array->elt_capacity));
        }

        array->elt_capacity = MIN(want_alloc / array->elt_size, X_MAXUINT);
    }
}

typedef struct _XRealPtrArray XRealPtrArray;

struct _XRealPtrArray {
    xpointer       *pdata;
    xuint           len;
    xuint           alloc;
    xatomicrefcount ref_count;
    xuint8          null_terminated : 1;
    XDestroyNotify  element_free_func;
};

static void x_ptr_array_maybe_expand(XRealPtrArray *array, xuint len);

static void ptr_array_maybe_null_terminate(XRealPtrArray *rarray)
{
    if (X_UNLIKELY(rarray->null_terminated)) {
        rarray->pdata[rarray->len] = NULL;
    }
}

static XPtrArray *ptr_array_new(xuint reserved_size, XDestroyNotify element_free_func, xboolean null_terminated)
{
    XRealPtrArray *array;

    array = x_slice_new(XRealPtrArray);

    array->pdata = NULL;
    array->len = 0;
    array->alloc = 0;
    array->null_terminated = null_terminated ? 1 : 0;
    array->element_free_func = element_free_func;

    x_atomic_ref_count_init(&array->ref_count);

    if (reserved_size != 0) {
        if (X_LIKELY(reserved_size < X_MAXUINT) && null_terminated) {
            reserved_size++;
        }

        x_ptr_array_maybe_expand(array, reserved_size);
        x_assert(array->pdata != NULL);

        if (null_terminated) {
            array->pdata[0] = NULL;
        }
    }

    return (XPtrArray *)array;
}

XPtrArray *x_ptr_array_new(void)
{
    return ptr_array_new(0, NULL, FALSE);
}

XPtrArray *x_ptr_array_new_take(xpointer *data, xsize len, XDestroyNotify element_free_func)
{
    XPtrArray *array;
    XRealPtrArray *rarray;

    x_return_val_if_fail(data != NULL || len == 0, NULL);
    x_return_val_if_fail(len <= X_MAXUINT, NULL);

    array = ptr_array_new(0, element_free_func, FALSE);
    rarray = (XRealPtrArray *)array;

    rarray->pdata = x_steal_pointer(&data);
    rarray->len = len;
    rarray->alloc = len;

    return array;
}

XPtrArray *x_ptr_array_new_take_null_terminated(xpointer *data, XDestroyNotify element_free_func)
{
    xsize len = 0;
    XPtrArray *array;

    if (data != NULL) {
        for (xsize i = 0; data[i] != NULL; ++i) {
            len += 1;
        }
    }

    x_return_val_if_fail(len <= X_MAXUINT, NULL);

    array = x_ptr_array_new_take(x_steal_pointer(&data), len, element_free_func);
    ((XRealPtrArray *)array)->null_terminated = TRUE;

    return array;
}

static XPtrArray *ptr_array_new_from_array(xpointer *data, xsize len, XCopyFunc copy_func, xpointer copy_func_user_data, XDestroyNotify element_free_func, xboolean null_terminated)
{
    XPtrArray *array;
    XRealPtrArray *rarray;

    x_assert(data != NULL || len == 0);
    x_assert(len <= X_MAXUINT);

    array = ptr_array_new(len, element_free_func, null_terminated);
    rarray = (XRealPtrArray *)array;

    if (copy_func != NULL) {
        for (xsize i = 0; i < len; i++) {
            rarray->pdata[i] = copy_func (data[i], copy_func_user_data);
        }
    } else if (len != 0) {
        memcpy(rarray->pdata, data, len * sizeof(xpointer));
    }

    if (null_terminated && (rarray->pdata != NULL)) {
        rarray->pdata[len] = NULL;
    }

    rarray->len = len;
    return array;
}

XPtrArray *x_ptr_array_new_from_array(xpointer *data, xsize len, XCopyFunc copy_func, xpointer copy_func_user_data, XDestroyNotify element_free_func)
{
    x_return_val_if_fail(data != NULL || len == 0, NULL);
    x_return_val_if_fail(len <= X_MAXUINT, NULL);

    return ptr_array_new_from_array(data, len, copy_func, copy_func_user_data, element_free_func, FALSE);
}

XPtrArray *x_ptr_array_new_from_null_terminated_array(xpointer *data, XCopyFunc copy_func, xpointer copy_func_user_data, XDestroyNotify element_free_func)
{
    xsize len = 0;

    if (data != NULL) {
        for (xsize i = 0; data[i] != NULL; ++i) {
            len += 1;
        }
    }

    x_assert(data != NULL || len == 0);
    x_return_val_if_fail(len <= X_MAXUINT, NULL);

    return ptr_array_new_from_array(data, len, copy_func, copy_func_user_data, element_free_func, TRUE);
}

xpointer *x_ptr_array_steal(XPtrArray *array, xsize *len)
{
    xpointer *segment;
    XRealPtrArray *rarray;

    x_return_val_if_fail(array != NULL, NULL);

    rarray = (XRealPtrArray *)array;
    segment = (xpointer *)rarray->pdata;

    if (len != NULL) {
        *len = rarray->len;
    }
    rarray->pdata = NULL;
    rarray->len = 0;
    rarray->alloc = 0;

    return segment;
}

XPtrArray *x_ptr_array_copy (XPtrArray *array, XCopyFunc func, xpointer user_data)
{
    XPtrArray *new_array;
    XRealPtrArray *rarray = (XRealPtrArray *)array;

    x_return_val_if_fail(array != NULL, NULL);

    new_array = ptr_array_new(0, rarray->element_free_func, rarray->null_terminated);
    if (rarray->alloc > 0) {
        x_ptr_array_maybe_expand((XRealPtrArray *)new_array, array->len + rarray->null_terminated);

        if (array->len > 0) {
            if (func != NULL) {
                xuint i;

                for (i = 0; i < array->len; i++) {
                    new_array->pdata[i] = func(array->pdata[i], user_data);
                }
            } else {
                memcpy(new_array->pdata, array->pdata, array->len * sizeof (*array->pdata));
            }

            new_array->len = array->len;
        }

        ptr_array_maybe_null_terminate((XRealPtrArray *) new_array);
    }

    return new_array;
}

XPtrArray *x_ptr_array_sized_new(xuint reserved_size)
{
    return ptr_array_new(reserved_size, NULL, FALSE);
}

XArray *x_array_copy(XArray *array)
{
    XRealArray *new_rarray;
    XRealArray *rarray = (XRealArray *)array;

    x_return_val_if_fail(rarray != NULL, NULL);

    new_rarray = (XRealArray *)x_array_sized_new(rarray->zero_terminated, rarray->clear, rarray->elt_size, rarray->elt_capacity);
    new_rarray->len = rarray->len;
    if (rarray->len > 0) {
        memcpy(new_rarray->data, rarray->data, rarray->len * rarray->elt_size);
    }
    x_array_zero_terminate(new_rarray);

    return (XArray *)new_rarray;
}

XPtrArray *x_ptr_array_new_with_free_func(XDestroyNotify element_free_func)
{
    return ptr_array_new(0, element_free_func, FALSE);
}

XPtrArray *x_ptr_array_new_full(xuint reserved_size, XDestroyNotify element_free_func)
{
    return ptr_array_new(reserved_size, element_free_func, FALSE);
}

XPtrArray *x_ptr_array_new_null_terminated(xuint reserved_size, XDestroyNotify element_free_func, xboolean null_terminated)
{
    return ptr_array_new(reserved_size, element_free_func, null_terminated);
}

void x_ptr_array_set_free_func(XPtrArray *array, XDestroyNotify  element_free_func)
{
    XRealPtrArray *rarray = (XRealPtrArray *)array;
    x_return_if_fail(array);

    rarray->element_free_func = element_free_func;
}

xboolean x_ptr_array_is_null_terminated(XPtrArray *array)
{
    x_return_val_if_fail(array, FALSE);
    return ((XRealPtrArray *)array)->null_terminated;
}

XPtrArray *x_ptr_array_ref(XPtrArray *array)
{
    XRealPtrArray *rarray = (XRealPtrArray *)array;

    x_return_val_if_fail(array, NULL);
    x_atomic_ref_count_inc(&rarray->ref_count);

    return array;
}

static xpointer *ptr_array_free(XPtrArray *, ArrayFreeFlags);

void x_ptr_array_unref(XPtrArray *array)
{
    XRealPtrArray *rarray = (XRealPtrArray *)array;
    x_return_if_fail(array);

    if (x_atomic_ref_count_dec(&rarray->ref_count)) {
        ptr_array_free(array, FREE_SEGMENT);
    }
}

xpointer *x_ptr_array_free(XPtrArray *array, xboolean free_segment)
{
    ArrayFreeFlags flags;
    XRealPtrArray *rarray = (XRealPtrArray *)array;

    x_return_val_if_fail(rarray, NULL);

    flags = (ArrayFreeFlags)(free_segment ? FREE_SEGMENT : 0);

#ifndef __COVERITY__
    if (!x_atomic_ref_count_dec(&rarray->ref_count)) {
        flags = (ArrayFreeFlags)(flags | PRESERVE_WRAPPER);
    }
#endif

    return ptr_array_free(array, flags);
}

static xpointer *ptr_array_free(XPtrArray *array, ArrayFreeFlags flags)
{
    xpointer *segment;
    XRealPtrArray *rarray = (XRealPtrArray *)array;

    if (flags & FREE_SEGMENT) {
        xpointer *stolen_pdata = x_steal_pointer(&rarray->pdata);
        if (rarray->element_free_func != NULL) {
            xuint i;

            for (i = 0; i < rarray->len; ++i) {
                rarray->element_free_func(stolen_pdata[i]);
            }
        }

        x_free(stolen_pdata);
        segment = NULL;
    } else {
        segment = rarray->pdata;
        if (!segment && rarray->null_terminated) {
            segment = (xpointer *)x_new0(char *, 1);
        }
    }

    if (flags & PRESERVE_WRAPPER) {
        rarray->pdata = NULL;
        rarray->len = 0;
        rarray->alloc = 0;
    } else {
        x_slice_free1(sizeof(XRealPtrArray), rarray);
    }

    return segment;
}

static void x_ptr_array_maybe_expand(XRealPtrArray *array, xuint len)
{
    xuint max_len;

    max_len = MIN(X_MAXSIZE / 2 / sizeof (xpointer), X_MAXUINT);

    if X_UNLIKELY((max_len - array->len) < len) {
        x_error("adding %u to array would overflow", len);
    }

    if ((array->len + len) > array->alloc) {
        xuint old_alloc = array->alloc;
        xsize want_alloc = x_nearest_pow(sizeof(xpointer) * (array->len + len));
        want_alloc = MAX(want_alloc, MIN_ARRAY_SIZE);
        array->alloc = MIN(want_alloc / sizeof(xpointer), X_MAXUINT);
        array->pdata = (xpointer *)x_realloc(array->pdata, want_alloc);

        if (X_UNLIKELY(x_mem_gc_friendly)) {
            for ( ; old_alloc < array->alloc; old_alloc++) {
                array->pdata[old_alloc] = NULL;
            }
        }
    }
}

void x_ptr_array_set_size(XPtrArray *array, xint length)
{
    xuint length_unsigned;
    XRealPtrArray *rarray = (XRealPtrArray *)array;

    x_return_if_fail(rarray);
    x_return_if_fail((rarray->len == 0) || ((rarray->len != 0) && (rarray->pdata != NULL)));
    x_return_if_fail(length >= 0);

    length_unsigned = (xuint)length;
    if (length_unsigned > rarray->len) {
        xuint i;

        if (X_UNLIKELY(rarray->null_terminated) && (length_unsigned - rarray->len > X_MAXUINT - 1)) {
            x_error("array would overflow");
        }

        x_ptr_array_maybe_expand(rarray, (length_unsigned - rarray->len) + rarray->null_terminated);

        for (i = rarray->len; i < length_unsigned; i++) {
            rarray->pdata[i] = NULL;
        }
        rarray->len = length_unsigned;

        ptr_array_maybe_null_terminate(rarray);
    } else if (length_unsigned < rarray->len) {
        x_ptr_array_remove_range(array, length_unsigned, rarray->len - length_unsigned);
    }
}

static xpointer ptr_array_remove_index(XPtrArray *array, xuint index_, xboolean fast, xboolean free_element)
{
    xpointer result;
    XRealPtrArray *rarray = (XRealPtrArray *)array;

    x_return_val_if_fail(rarray, NULL);
    x_return_val_if_fail((rarray->len == 0) || ((rarray->len != 0) && (rarray->pdata != NULL)), NULL);
    x_return_val_if_fail(index_ < rarray->len, NULL);

    result = rarray->pdata[index_];

    if ((rarray->element_free_func != NULL) && free_element) {
        rarray->element_free_func (rarray->pdata[index_]);
    }

    if ((index_ != rarray->len - 1) && !fast) {
        memmove(rarray->pdata + index_, rarray->pdata + index_ + 1, sizeof(xpointer) * (rarray->len - index_ - 1));
    } else if (index_ != rarray->len - 1) {
        rarray->pdata[index_] = rarray->pdata[rarray->len - 1];
    }
    rarray->len -= 1;

    if (rarray->null_terminated || X_UNLIKELY(x_mem_gc_friendly)) {
        rarray->pdata[rarray->len] = NULL;
    }

    return result;
}

xpointer x_ptr_array_remove_index(XPtrArray *array, xuint index_)
{
    return ptr_array_remove_index(array, index_, FALSE, TRUE);
}

xpointer x_ptr_array_remove_index_fast(XPtrArray *array, xuint index_)
{
    return ptr_array_remove_index(array, index_, TRUE, TRUE);
}

xpointer x_ptr_array_steal_index(XPtrArray *array, xuint index_)
{
    return ptr_array_remove_index(array, index_, FALSE, FALSE);
}

xpointer x_ptr_array_steal_index_fast(XPtrArray *array, xuint index_)
{
    return ptr_array_remove_index(array, index_, TRUE, FALSE);
}

XPtrArray *x_ptr_array_remove_range(XPtrArray *array, xuint index_, xuint length)
{
    xuint i;
    XRealPtrArray *rarray = (XRealPtrArray *)array;

    x_return_val_if_fail(rarray != NULL, NULL);
    x_return_val_if_fail(rarray->len == 0 || (rarray->len != 0 && rarray->pdata != NULL), NULL);
    x_return_val_if_fail(index_ <= rarray->len, NULL);
    x_return_val_if_fail(index_ <= X_MAXUINT - length, NULL);
    x_return_val_if_fail(length == 0 || index_ + length <= rarray->len, NULL);

    if (length == 0) {
        return array;
    }

    if (rarray->element_free_func != NULL) {
        for (i = index_; i < index_ + length; i++) {
            rarray->element_free_func (rarray->pdata[i]);
        }
    }

    if (index_ + length != rarray->len) {
        memmove(&rarray->pdata[index_], &rarray->pdata[index_ + length], (rarray->len - (index_ + length)) * sizeof (xpointer));
    }

    rarray->len -= length;
    if (X_UNLIKELY(x_mem_gc_friendly)) {
        for (i = 0; i < length; i++) {
            rarray->pdata[rarray->len + i] = NULL;
        }
    } else {
        ptr_array_maybe_null_terminate(rarray);
    }

    return array;
}

xboolean x_ptr_array_remove(XPtrArray *array, xpointer data)
{
    xuint i;

    x_return_val_if_fail(array, FALSE);
    x_return_val_if_fail(array->len == 0 || (array->len != 0 && array->pdata != NULL), FALSE);

    for (i = 0; i < array->len; i += 1) {
        if (array->pdata[i] == data) {
            x_ptr_array_remove_index(array, i);
            return TRUE;
        }
    }

    return FALSE;
}

xboolean x_ptr_array_remove_fast(XPtrArray *array, xpointer data)
{
    xuint i;
    XRealPtrArray *rarray = (XRealPtrArray *)array;

    x_return_val_if_fail(rarray, FALSE);
    x_return_val_if_fail(rarray->len == 0 || (rarray->len != 0 && rarray->pdata != NULL), FALSE);

    for (i = 0; i < rarray->len; i += 1) {
        if (rarray->pdata[i] == data) {
            x_ptr_array_remove_index_fast(array, i);
            return TRUE;
        }
    }

    return FALSE;
}

void x_ptr_array_add(XPtrArray *array, xpointer data)
{
    XRealPtrArray *rarray = (XRealPtrArray *)array;

    x_return_if_fail(rarray);
    x_return_if_fail(rarray->len == 0 || (rarray->len != 0 && rarray->pdata != NULL));

    x_ptr_array_maybe_expand(rarray, 1u + rarray->null_terminated);
    rarray->pdata[rarray->len++] = data;
    ptr_array_maybe_null_terminate(rarray);
}

void x_ptr_array_extend(XPtrArray *array_to_extend, XPtrArray *array, XCopyFunc func, xpointer user_data)
{
    XRealPtrArray *rarray_to_extend = (XRealPtrArray *)array_to_extend;

    x_return_if_fail(array_to_extend != NULL);
    x_return_if_fail(array != NULL);

    if (array->len == 0u) {
        return;
    }

    if (X_UNLIKELY(array->len == X_MAXUINT) && rarray_to_extend->null_terminated) {
        x_error("adding %u to array would overflow", array->len);
    }

    x_ptr_array_maybe_expand(rarray_to_extend, array->len + rarray_to_extend->null_terminated);

    if (func != NULL) {
        xuint i;

        for (i = 0; i < array->len; i++) {
            rarray_to_extend->pdata[i + rarray_to_extend->len] = func(array->pdata[i], user_data);
        }
    } else if (array->len > 0) {
        memcpy(rarray_to_extend->pdata + rarray_to_extend->len, array->pdata, array->len * sizeof (*array->pdata));
    }

    rarray_to_extend->len += array->len;
    ptr_array_maybe_null_terminate(rarray_to_extend);
}

void x_ptr_array_extend_and_steal(XPtrArray  *array_to_extend, XPtrArray *array)
{
    xpointer *pdata;
    x_ptr_array_extend(array_to_extend, array, NULL, NULL);

    pdata = x_steal_pointer(&array->pdata);
    array->len = 0;
    ((XRealPtrArray *)array)->alloc = 0;
    x_ptr_array_unref(array);
    x_free(pdata);
}

void x_ptr_array_insert(XPtrArray *array, xint index_, xpointer data)
{
    XRealPtrArray *rarray = (XRealPtrArray *)array;

    x_return_if_fail(rarray);
    x_return_if_fail(index_ >= -1);
    x_return_if_fail(index_ <= (xint)rarray->len);

    x_ptr_array_maybe_expand(rarray, 1u + rarray->null_terminated);

    if (index_ < 0) {
        index_ = rarray->len;
    }

    if ((xuint)index_ < rarray->len) {
        memmove(&(rarray->pdata[index_ + 1]), &(rarray->pdata[index_]), (rarray->len - index_) * sizeof (xpointer));
    }

    rarray->len++;
    rarray->pdata[index_] = data;

    ptr_array_maybe_null_terminate(rarray);
}

void x_ptr_array_sort(XPtrArray *array, XCompareFunc compare_func)
{
    x_return_if_fail(array != NULL);

    if (array->len > 0) {
        x_qsort_with_data(array->pdata, array->len, sizeof(xpointer), (XCompareDataFunc)compare_func, NULL);
    }
}

void x_ptr_array_sort_with_data(XPtrArray *array, XCompareDataFunc compare_func, xpointer user_data)
{
    x_return_if_fail(array != NULL);

    if (array->len > 0) {
        x_qsort_with_data(array->pdata, array->len, sizeof(xpointer), compare_func, user_data);
    }
}

static inline xint compare_ptr_array_values(xconstpointer a, xconstpointer b, xpointer user_data)
{
    xconstpointer aa = *((xconstpointer *)a);
    xconstpointer bb = *((xconstpointer *)b);
    XCompareFunc compare_func = (XCompareFunc)user_data;

    return compare_func(aa, bb);
}

void x_ptr_array_sort_values(XPtrArray *array, XCompareFunc compare_func)
{
    x_ptr_array_sort_with_data(array, compare_ptr_array_values, compare_func);
}

typedef struct {
    XCompareDataFunc compare_func;
    xpointer         user_data;
} XPtrArraySortValuesData;

static inline xint compare_ptr_array_values_with_data(xconstpointer a, xconstpointer b, xpointer user_data)
{
    xconstpointer aa = *((xconstpointer *)a);
    xconstpointer bb = *((xconstpointer *)b);
    XPtrArraySortValuesData *data = (XPtrArraySortValuesData *)user_data;

    return data->compare_func(aa, bb, data->user_data);
}

void x_ptr_array_sort_values_with_data(XPtrArray *array, XCompareDataFunc compare_func, xpointer user_data)
{
    x_ptr_array_sort_with_data(array, compare_ptr_array_values_with_data, &(XPtrArraySortValuesData) {
        .compare_func = compare_func,
        .user_data = user_data,
    });
}

void x_ptr_array_foreach(XPtrArray *array, XFunc func, xpointer user_data)
{
    xuint i;

    x_return_if_fail(array);

    for (i = 0; i < array->len; i++) {
        (*func)(array->pdata[i], user_data);
    }
}

xboolean x_ptr_array_find(XPtrArray *haystack, xconstpointer needle, xuint *index_)
{
    return x_ptr_array_find_with_equal_func(haystack, needle, NULL, index_);
}

xboolean x_ptr_array_find_with_equal_func(XPtrArray *haystack, xconstpointer needle, XEqualFunc equal_func, xuint *index_)
{
    xuint i;

    x_return_val_if_fail(haystack != NULL, FALSE);

    if (equal_func == NULL) {
        equal_func = x_direct_equal;
    }

    for (i = 0; i < haystack->len; i++) {
        if (equal_func(x_ptr_array_index(haystack, i), needle)) {
            if (index_ != NULL) {
                *index_ = i;
            }

            return TRUE;
        }
    }

    return FALSE;
}

XByteArray *x_byte_array_new(void)
{
    return (XByteArray *)x_array_sized_new(FALSE, FALSE, 1, 0);
}

xuint8 *x_byte_array_steal(XByteArray *array, xsize *len)
{
    return (xuint8 *)x_array_steal((XArray *)array, len);
}

XByteArray *x_byte_array_new_take(xuint8 *data, xsize len)
{
    XRealArray *real;
    XByteArray *array;

    x_return_val_if_fail(len <= X_MAXUINT, NULL);

    array = x_byte_array_new();
    real = (XRealArray *)array;
    x_assert(real->data == NULL);
    x_assert(real->len == 0);

    real->data = data;
    real->len = len;
    real->elt_capacity = len;

    return array;
}

XByteArray *x_byte_array_sized_new(xuint reserved_size)
{
    return (XByteArray *)x_array_sized_new(FALSE, FALSE, 1, reserved_size);
}

xuint8 *x_byte_array_free(XByteArray *array, xboolean free_segment)
{
    return (xuint8 *)x_array_free((XArray *)array, free_segment);
}

XBytes *x_byte_array_free_to_bytes(XByteArray *array)
{
    xsize length;

    x_return_val_if_fail(array != NULL, NULL);

    length = array->len;
    return x_bytes_new_take(x_byte_array_free(array, FALSE), length);
}

XByteArray *x_byte_array_ref(XByteArray *array)
{
    return (XByteArray *)x_array_ref((XArray *)array);
}

void x_byte_array_unref(XByteArray *array)
{
    x_array_unref((XArray *)array);
}

XByteArray *x_byte_array_append(XByteArray *array, const xuint8 *data, xuint len)
{
    x_array_append_vals((XArray *)array, (xuint8 *)data, len);
    return array;
}

XByteArray *x_byte_array_prepend(XByteArray *array, const xuint8 *data, xuint len)
{
    x_array_prepend_vals((XArray *)array, (xuint8 *)data, len);
    return array;
}

XByteArray *x_byte_array_set_size (XByteArray *array, xuint length)
{
    x_array_set_size((XArray *)array, length);
    return array;
}

XByteArray *x_byte_array_remove_index(XByteArray *array, xuint index_)
{
    x_array_remove_index((XArray *)array, index_);
    return array;
}

XByteArray *x_byte_array_remove_index_fast(XByteArray *array, xuint index_)
{
    x_array_remove_index_fast((XArray *)array, index_);
    return array;
}

XByteArray *x_byte_array_remove_range(XByteArray *array, xuint index_, xuint length)
{
    x_return_val_if_fail(array, NULL);
    x_return_val_if_fail(index_ <= array->len, NULL);
    x_return_val_if_fail(index_ <= X_MAXUINT - length, NULL);
    x_return_val_if_fail(index_ + length <= array->len, NULL);

    return (XByteArray *)x_array_remove_range((XArray *)array, index_, length);
}

void x_byte_array_sort(XByteArray *array, XCompareFunc compare_func)
{
    x_array_sort((XArray *)array, compare_func);
}

void x_byte_array_sort_with_data(XByteArray *array, XCompareDataFunc compare_func, xpointer user_data)
{
    x_array_sort_with_data((XArray *)array, compare_func, user_data);
}
