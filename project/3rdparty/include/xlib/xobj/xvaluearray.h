#ifndef __X_VALUE_ARRAY_H__
#define __X_VALUE_ARRAY_H__

#include "xvalue.h"

X_BEGIN_DECLS

#define X_TYPE_VALUE_ARRAY              (x_value_array_get_type()) XLIB_DEPRECATED_MACRO_IN_2_32_FOR(X_TYPE_ARRAY)

typedef struct _XValueArray XValueArray;

struct _XValueArray {
    xuint  n_values;
    XValue *values;
    xuint  n_prealloced;
};

XLIB_DEPRECATED_IN_2_32_FOR(XArray)
XType x_value_array_get_type(void) X_GNUC_CONST;

XLIB_DEPRECATED_IN_2_32_FOR(XArray)
XValue *x_value_array_get_nth(XValueArray *value_array, xuint index_);

XLIB_DEPRECATED_IN_2_32_FOR(XArray)
XValueArray *x_value_array_new(xuint n_prealloced);

XLIB_DEPRECATED_IN_2_32_FOR(XArray)
void x_value_array_free(XValueArray *value_array);

XLIB_DEPRECATED_IN_2_32_FOR(XArray)
XValueArray *x_value_array_copy(const XValueArray *value_array);

XLIB_DEPRECATED_IN_2_32_FOR(XArray)
XValueArray *x_value_array_prepend(XValueArray *value_array, const XValue *value);

XLIB_DEPRECATED_IN_2_32_FOR(XArray)
XValueArray *x_value_array_append(XValueArray *value_array, const XValue *value);

XLIB_DEPRECATED_IN_2_32_FOR(XArray)
XValueArray *x_value_array_insert(XValueArray *value_array, xuint index_, const XValue *value);

XLIB_DEPRECATED_IN_2_32_FOR(XArray)
XValueArray *x_value_array_remove(XValueArray *value_array, xuint index_);

XLIB_DEPRECATED_IN_2_32_FOR(XArray)
XValueArray *x_value_array_sort(XValueArray *value_array, XCompareFunc compare_func);

XLIB_DEPRECATED_IN_2_32_FOR(XArray)
XValueArray *x_value_array_sort_with_data(XValueArray *value_array, XCompareDataFunc compare_func, xpointer user_data);

X_END_DECLS

#endif
