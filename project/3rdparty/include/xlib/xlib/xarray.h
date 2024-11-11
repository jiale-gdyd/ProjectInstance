#ifndef __X_ARRAY_H__
#define __X_ARRAY_H__

#include "xtypes.h"

X_BEGIN_DECLS

typedef struct _XBytes XBytes;
typedef struct _XArray XArray;
typedef struct _XPtrArray XPtrArray;
typedef struct _XByteArray XByteArray;

struct _XArray {
    xchar *data;
    xuint len;
};

struct _XByteArray {
    xuint8 *data;
    xuint  len;
};

struct _XPtrArray {
    xpointer *pdata;
    xuint    len;
};

#define x_array_append_val(a, v)                x_array_append_vals(a, &(v), 1)
#define x_array_prepend_val(a, v)               x_array_prepend_vals(a, &(v), 1)
#define x_array_insert_val(a, i, v)             x_array_insert_vals(a, i, &(v), 1)

#define x_array_index(a, t, i)                  (((t *)(void *)(a)->data)[(i)])
#define x_ptr_array_index(array, index_)        ((array)->pdata)[index_]

XLIB_AVAILABLE_IN_ALL
XArray *x_array_new(xboolean zero_terminated, xboolean clear_, xuint element_size);

XLIB_AVAILABLE_IN_2_76
XArray *x_array_new_take(xpointer data, xsize len, xboolean clear, xsize element_size);

XLIB_AVAILABLE_IN_2_76
XArray *x_array_new_take_zero_terminated(xpointer data, xboolean clear, xsize element_size);

XLIB_AVAILABLE_IN_2_64
xpointer x_array_steal(XArray *array, xsize *len);

XLIB_AVAILABLE_IN_ALL
XArray *x_array_sized_new(xboolean zero_terminated, xboolean clear_, xuint element_size, xuint reserved_size);

XLIB_AVAILABLE_IN_2_62
XArray *x_array_copy(XArray *array);

XLIB_AVAILABLE_IN_ALL
xchar *x_array_free(XArray *array, xboolean free_segment);

XLIB_AVAILABLE_IN_ALL
XArray *x_array_ref(XArray *array);

XLIB_AVAILABLE_IN_ALL
void x_array_unref(XArray *array);

XLIB_AVAILABLE_IN_ALL
xuint x_array_get_element_size(XArray *array);

XLIB_AVAILABLE_IN_ALL
XArray *x_array_append_vals(XArray *array, xconstpointer data, xuint len);

XLIB_AVAILABLE_IN_ALL
XArray *x_array_prepend_vals(XArray *array, xconstpointer data, xuint len);

XLIB_AVAILABLE_IN_ALL
XArray *x_array_insert_vals(XArray *array, xuint index_, xconstpointer data, xuint len);

XLIB_AVAILABLE_IN_ALL
XArray *x_array_set_size(XArray *array, xuint length);

XLIB_AVAILABLE_IN_ALL
XArray *x_array_remove_index(XArray *array, xuint index_);

XLIB_AVAILABLE_IN_ALL
XArray *x_array_remove_index_fast(XArray *array, xuint index_);

XLIB_AVAILABLE_IN_ALL
XArray *x_array_remove_range(XArray *array, xuint index_, xuint length);

XLIB_AVAILABLE_IN_ALL
void x_array_sort(XArray *array, XCompareFunc compare_func);

XLIB_AVAILABLE_IN_ALL
void x_array_sort_with_data(XArray *array, XCompareDataFunc compare_func, xpointer user_data);

XLIB_AVAILABLE_IN_2_62
xboolean x_array_binary_search(XArray *array, xconstpointer target, XCompareFunc compare_func, xuint *out_match_index);

XLIB_AVAILABLE_IN_ALL
void x_array_set_clear_func(XArray *array, XDestroyNotify clear_func);

XLIB_AVAILABLE_IN_ALL
XPtrArray *x_ptr_array_new(void);

XLIB_AVAILABLE_IN_ALL
XPtrArray *x_ptr_array_new_with_free_func(XDestroyNotify element_free_func);

XLIB_AVAILABLE_IN_2_76
XPtrArray *x_ptr_array_new_take(xpointer *data, xsize len, XDestroyNotify element_free_func);

XLIB_AVAILABLE_IN_2_76
XPtrArray *x_ptr_array_new_from_array(xpointer *data, xsize len, XCopyFunc copy_func, xpointer copy_func_user_data, XDestroyNotify element_free_func);

XLIB_AVAILABLE_IN_2_64
xpointer *x_ptr_array_steal(XPtrArray *array, xsize *len);

XLIB_AVAILABLE_IN_2_62
XPtrArray *x_ptr_array_copy(XPtrArray *array, XCopyFunc func, xpointer user_data);

XLIB_AVAILABLE_IN_ALL
XPtrArray *x_ptr_array_sized_new(xuint reserved_size);

XLIB_AVAILABLE_IN_ALL
XPtrArray *x_ptr_array_new_full(xuint reserved_size, XDestroyNotify element_free_func);

XLIB_AVAILABLE_IN_2_74
XPtrArray *x_ptr_array_new_null_terminated(xuint reserved_size, XDestroyNotify element_free_func, xboolean null_terminated);

XLIB_AVAILABLE_IN_2_76
XPtrArray *x_ptr_array_new_take_null_terminated(xpointer *data, XDestroyNotify element_free_func);

XLIB_AVAILABLE_IN_2_76
XPtrArray *x_ptr_array_new_from_null_terminated_array(xpointer *data, XCopyFunc copy_func, xpointer copy_func_user_data, XDestroyNotify element_free_func);

XLIB_AVAILABLE_IN_ALL
xpointer *x_ptr_array_free(XPtrArray *array, xboolean free_segment);

XLIB_AVAILABLE_IN_ALL
XPtrArray *x_ptr_array_ref(XPtrArray *array);

XLIB_AVAILABLE_IN_ALL
void x_ptr_array_unref(XPtrArray *array);

XLIB_AVAILABLE_IN_ALL
void x_ptr_array_set_free_func(XPtrArray *array, XDestroyNotify element_free_func);

XLIB_AVAILABLE_IN_ALL
void x_ptr_array_set_size(XPtrArray *array, xint length);

XLIB_AVAILABLE_IN_ALL
xpointer x_ptr_array_remove_index(XPtrArray *array, xuint index_);

XLIB_AVAILABLE_IN_ALL
xpointer x_ptr_array_remove_index_fast(XPtrArray *array, xuint index_);

XLIB_AVAILABLE_IN_2_58
xpointer x_ptr_array_steal_index(XPtrArray *array, xuint index_);

XLIB_AVAILABLE_IN_2_58
xpointer x_ptr_array_steal_index_fast(XPtrArray *array, xuint index_);

XLIB_AVAILABLE_IN_ALL
xboolean x_ptr_array_remove(XPtrArray *array, xpointer data);

XLIB_AVAILABLE_IN_ALL
xboolean x_ptr_array_remove_fast(XPtrArray *array, xpointer data);

XLIB_AVAILABLE_IN_ALL
XPtrArray *x_ptr_array_remove_range(XPtrArray *array, xuint index_, xuint length);

XLIB_AVAILABLE_IN_ALL
void x_ptr_array_add(XPtrArray *array, xpointer data);

XLIB_AVAILABLE_IN_2_62
void x_ptr_array_extend(XPtrArray *array_to_extend, XPtrArray *array, XCopyFunc func, xpointer user_data);

XLIB_AVAILABLE_IN_2_62
void x_ptr_array_extend_and_steal(XPtrArray *array_to_extend, XPtrArray *array);

XLIB_AVAILABLE_IN_2_40
void x_ptr_array_insert(XPtrArray *array, xint index_, xpointer data);

XLIB_AVAILABLE_IN_ALL
void x_ptr_array_sort(XPtrArray *array, XCompareFunc compare_func);

XLIB_AVAILABLE_IN_ALL
void x_ptr_array_sort_with_data(XPtrArray *array, XCompareDataFunc compare_func, xpointer user_data);

XLIB_AVAILABLE_IN_2_76
void x_ptr_array_sort_values(XPtrArray *array, XCompareFunc compare_func);

XLIB_AVAILABLE_IN_2_76
void x_ptr_array_sort_values_with_data(XPtrArray *array, XCompareDataFunc compare_func, xpointer user_data);

XLIB_AVAILABLE_IN_ALL
void x_ptr_array_foreach(XPtrArray *array, XFunc func, xpointer user_data);

XLIB_AVAILABLE_IN_2_54
xboolean x_ptr_array_find(XPtrArray *haystack, xconstpointer needle, xuint *index_);

XLIB_AVAILABLE_IN_2_54
xboolean x_ptr_array_find_with_equal_func(XPtrArray *haystack, xconstpointer needle, XEqualFunc equal_func, xuint *index_);

XLIB_AVAILABLE_IN_2_74
xboolean x_ptr_array_is_null_terminated(XPtrArray *array);

XLIB_AVAILABLE_IN_ALL
XByteArray *x_byte_array_new(void);

XLIB_AVAILABLE_IN_ALL
XByteArray *x_byte_array_new_take(xuint8 *data, xsize len);

XLIB_AVAILABLE_IN_2_64
xuint8 *x_byte_array_steal(XByteArray *array, xsize *len);

XLIB_AVAILABLE_IN_ALL
XByteArray *x_byte_array_sized_new(xuint reserved_size);

XLIB_AVAILABLE_IN_ALL
xuint8 *x_byte_array_free(XByteArray *array, xboolean free_segment);

XLIB_AVAILABLE_IN_ALL
XBytes *x_byte_array_free_to_bytes(XByteArray *array);

XLIB_AVAILABLE_IN_ALL
XByteArray *x_byte_array_ref(XByteArray *array);

XLIB_AVAILABLE_IN_ALL
void x_byte_array_unref(XByteArray *array);

XLIB_AVAILABLE_IN_ALL
XByteArray *x_byte_array_append(XByteArray *array, const xuint8 *data, xuint len);

XLIB_AVAILABLE_IN_ALL
XByteArray *x_byte_array_prepend(XByteArray *array, const xuint8 *data, xuint len);

XLIB_AVAILABLE_IN_ALL
XByteArray *x_byte_array_set_size(XByteArray *array, xuint length);

XLIB_AVAILABLE_IN_ALL
XByteArray *x_byte_array_remove_index(XByteArray *array, xuint index_);

XLIB_AVAILABLE_IN_ALL
XByteArray *x_byte_array_remove_index_fast(XByteArray *array, xuint index_);

XLIB_AVAILABLE_IN_ALL
XByteArray *x_byte_array_remove_range(XByteArray *array, xuint index_, xuint length);

XLIB_AVAILABLE_IN_ALL
void x_byte_array_sort(XByteArray *array, XCompareFunc compare_func);

XLIB_AVAILABLE_IN_ALL
void x_byte_array_sort_with_data(XByteArray *array, XCompareDataFunc compare_func, xpointer user_data);

X_END_DECLS

#endif
