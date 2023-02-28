#ifndef __X_VALUE_H__
#define __X_VALUE_H__

#include "xtype.h"

X_BEGIN_DECLS

#define X_TYPE_IS_VALUE(type)               (x_type_check_is_value_type(type))
#define X_IS_VALUE(value)                   (X_TYPE_CHECK_VALUE(value))
#define X_VALUE_TYPE(value)                 (((XValue *)(value))->x_type)
#define X_VALUE_TYPE_NAME(value)            (x_type_name(X_VALUE_TYPE(value)))
#define X_VALUE_HOLDS(value, type)          (X_TYPE_CHECK_VALUE_TYPE((value), (type)))

typedef void (*XValueTransform)(const XValue *src_value, XValue *dest_value);

struct _XValue {
    XType        x_type;

    union {
        xint     v_int;
        xuint    v_uint;
        xlong    v_long;
        xulong   v_ulong;
        xint64   v_int64;
        xuint64  v_uint64;
        xfloat   v_float;
        xdouble  v_double;
        xpointer v_pointer;
    } data[2];
};

XLIB_AVAILABLE_IN_ALL
XValue *x_value_init(XValue *value, XType x_type);

XLIB_AVAILABLE_IN_ALL
void x_value_copy(const XValue *src_value, XValue *dest_value);

XLIB_AVAILABLE_IN_ALL
XValue *x_value_reset(XValue *value);

XLIB_AVAILABLE_IN_ALL
void x_value_unset(XValue *value);

XLIB_AVAILABLE_IN_ALL
void x_value_set_instance(XValue *value, xpointer instance);

XLIB_AVAILABLE_IN_2_42
void x_value_init_from_instance(XValue *value, xpointer instance);

XLIB_AVAILABLE_IN_ALL
xboolean x_value_fits_pointer(const XValue *value);

XLIB_AVAILABLE_IN_ALL
xpointer x_value_peek_pointer(const XValue *value);

XLIB_AVAILABLE_IN_ALL
xboolean x_value_type_compatible(XType src_type, XType dest_type);

XLIB_AVAILABLE_IN_ALL
xboolean x_value_type_transformable(XType src_type, XType dest_type);

XLIB_AVAILABLE_IN_ALL
xboolean x_value_transform(const XValue *src_value, XValue *dest_value);

XLIB_AVAILABLE_IN_ALL
void x_value_register_transform_func(XType src_type, XType dest_type, XValueTransform transform_func);

#define X_VALUE_NOCOPY_CONTENTS         (1 << 27)
#define X_VALUE_INTERNED_STRING         (1 << 28) XLIB_AVAILABLE_MACRO_IN_2_66

#define X_VALUE_INIT                    { 0, { { 0 } } }

X_END_DECLS

#endif
