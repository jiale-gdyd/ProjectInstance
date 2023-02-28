#ifndef __X_VALUETYPES_H__
#define __X_VALUETYPES_H__

#include "xvalue.h"

X_BEGIN_DECLS

#define X_VALUE_HOLDS_CHAR(value)           (X_TYPE_CHECK_VALUE_TYPE((value), X_TYPE_CHAR))
#define X_VALUE_HOLDS_UCHAR(value)          (X_TYPE_CHECK_VALUE_TYPE((value), X_TYPE_UCHAR))
#define X_VALUE_HOLDS_BOOLEAN(value)        (X_TYPE_CHECK_VALUE_TYPE((value), X_TYPE_BOOLEAN))
#define X_VALUE_HOLDS_INT(value)            (X_TYPE_CHECK_VALUE_TYPE((value), X_TYPE_INT))
#define X_VALUE_HOLDS_UINT(value)           (X_TYPE_CHECK_VALUE_TYPE((value), X_TYPE_UINT))
#define X_VALUE_HOLDS_LONG(value)           (X_TYPE_CHECK_VALUE_TYPE((value), X_TYPE_LONG))
#define X_VALUE_HOLDS_ULONG(value)          (X_TYPE_CHECK_VALUE_TYPE((value), X_TYPE_ULONG))
#define X_VALUE_HOLDS_INT64(value)          (X_TYPE_CHECK_VALUE_TYPE((value), X_TYPE_INT64))
#define X_VALUE_HOLDS_UINT64(value)         (X_TYPE_CHECK_VALUE_TYPE((value), X_TYPE_UINT64))
#define X_VALUE_HOLDS_FLOAT(value)          (X_TYPE_CHECK_VALUE_TYPE((value), X_TYPE_FLOAT))
#define X_VALUE_HOLDS_DOUBLE(value)         (X_TYPE_CHECK_VALUE_TYPE((value), X_TYPE_DOUBLE))
#define X_VALUE_HOLDS_STRING(value)         (X_TYPE_CHECK_VALUE_TYPE((value), X_TYPE_STRING))

#define X_VALUE_IS_INTERNED_STRING(value)   (X_VALUE_HOLDS_STRING(value) && ((value)->data[1].v_uint & X_VALUE_INTERNED_STRING)) XLIB_AVAILABLE_MACRO_IN_2_66

#define X_VALUE_HOLDS_POINTER(value)        (X_TYPE_CHECK_VALUE_TYPE((value), X_TYPE_POINTER))
#define X_TYPE_GTYPE                        (x_gtype_get_type())
#define X_VALUE_HOLDS_GTYPE(value)          (X_TYPE_CHECK_VALUE_TYPE((value), X_TYPE_GTYPE))
#define X_VALUE_HOLDS_VARIANT(value)        (X_TYPE_CHECK_VALUE_TYPE((value), X_TYPE_VARIANT))

XLIB_DEPRECATED_IN_2_32_FOR(x_value_set_schar)
void x_value_set_char(XValue *value, xchar v_char);

XLIB_DEPRECATED_IN_2_32_FOR(x_value_get_schar)
xchar x_value_get_char(const XValue *value);

XLIB_AVAILABLE_IN_ALL
void x_value_set_schar(XValue *value, xint8 v_char);

XLIB_AVAILABLE_IN_ALL
xint8 x_value_get_schar(const XValue *value);

XLIB_AVAILABLE_IN_ALL
void x_value_set_uchar(XValue *value, xuchar v_uchar);

XLIB_AVAILABLE_IN_ALL
xuchar x_value_get_uchar(const XValue *value);

XLIB_AVAILABLE_IN_ALL
void x_value_set_boolean(XValue *value, xboolean v_boolean);

XLIB_AVAILABLE_IN_ALL
xboolean x_value_get_boolean(const XValue *value);

XLIB_AVAILABLE_IN_ALL
void x_value_set_int(XValue *value, xint v_int);

XLIB_AVAILABLE_IN_ALL
xint x_value_get_int(const XValue *value);

XLIB_AVAILABLE_IN_ALL
void x_value_set_uint(XValue *value, xuint v_uint);

XLIB_AVAILABLE_IN_ALL
xuint x_value_get_uint(const XValue *value);

XLIB_AVAILABLE_IN_ALL
void x_value_set_long(XValue *value, xlong v_long);

XLIB_AVAILABLE_IN_ALL
xlong x_value_get_long(const XValue *value);

XLIB_AVAILABLE_IN_ALL
void x_value_set_ulong(XValue *value, xulong v_ulong);

XLIB_AVAILABLE_IN_ALL
xulong x_value_get_ulong(const XValue *value);

XLIB_AVAILABLE_IN_ALL
void x_value_set_int64(XValue *value, xint64 v_int64);

XLIB_AVAILABLE_IN_ALL
xint64 x_value_get_int64(const XValue *value);

XLIB_AVAILABLE_IN_ALL
void x_value_set_uint64(XValue *value, xuint64 v_uint64);

XLIB_AVAILABLE_IN_ALL
xuint64 x_value_get_uint64(const XValue *value);

XLIB_AVAILABLE_IN_ALL
void x_value_set_float(XValue *value, xfloat v_float);

XLIB_AVAILABLE_IN_ALL
xfloat x_value_get_float(const XValue *value);

XLIB_AVAILABLE_IN_ALL
void x_value_set_double(XValue *value, xdouble v_double);

XLIB_AVAILABLE_IN_ALL
xdouble x_value_get_double(const XValue *value);

XLIB_AVAILABLE_IN_ALL
void x_value_set_string(XValue *value, const xchar *v_string);

XLIB_AVAILABLE_IN_ALL
void x_value_set_static_string(XValue *value, const xchar *v_string);

XLIB_AVAILABLE_IN_2_66
void x_value_set_interned_string(XValue *value, const xchar *v_string);

XLIB_AVAILABLE_IN_ALL
const xchar *x_value_get_string(const XValue *value);

XLIB_AVAILABLE_IN_ALL
xchar *x_value_dup_string(const XValue *value);

XLIB_AVAILABLE_IN_ALL
void x_value_set_pointer(XValue *value, xpointer v_pointer);

XLIB_AVAILABLE_IN_ALL
xpointer x_value_get_pointer(const XValue *value);

XLIB_AVAILABLE_IN_ALL
XType x_gtype_get_type(void);

XLIB_AVAILABLE_IN_ALL
void x_value_set_gtype(XValue *value, XType v_gtype);

XLIB_AVAILABLE_IN_ALL
XType x_value_get_gtype(const XValue *value);

XLIB_AVAILABLE_IN_ALL
void x_value_set_variant(XValue *value, XVariant *variant);

XLIB_AVAILABLE_IN_ALL
void x_value_take_variant(XValue *value, XVariant *variant);

XLIB_AVAILABLE_IN_ALL
XVariant *x_value_get_variant(const XValue *value);

XLIB_AVAILABLE_IN_ALL
XVariant *x_value_dup_variant(const XValue *value);

XLIB_AVAILABLE_IN_ALL
XType x_pointer_type_register_static(const xchar *name);

XLIB_AVAILABLE_IN_ALL
xchar *x_strdup_value_contents(const XValue *value);

XLIB_AVAILABLE_IN_ALL
void x_value_take_string(XValue *value, xchar *v_string);

XLIB_DEPRECATED_FOR(x_value_take_string)
void x_value_set_string_take_ownership(XValue *value, xchar *v_string);

typedef xchar* xchararray;

X_END_DECLS

#endif
