#ifndef __X_ENUMS_H__
#define __X_ENUMS_H__

#include "xtype.h"

X_BEGIN_DECLS

#define X_TYPE_IS_ENUM(type)                (X_TYPE_FUNDAMENTAL(type) == X_TYPE_ENUM)
#define X_ENUM_CLASS(classt)                (X_TYPE_CHECK_CLASS_CAST((classt), X_TYPE_ENUM, XEnumClass))
#define X_IS_ENUM_CLASS(classt)             (X_TYPE_CHECK_CLASS_TYPE((classt), X_TYPE_ENUM))
#define X_ENUM_CLASS_TYPE(classt)           (X_TYPE_FROM_CLASS(classt))
#define X_ENUM_CLASS_TYPE_NAME(classt)      (x_type_name(X_ENUM_CLASS_TYPE(classt)))

#define X_TYPE_IS_FLAGS(type)               (X_TYPE_FUNDAMENTAL(type) == X_TYPE_FLAGS)
#define X_FLAGS_CLASS(classt)               (X_TYPE_CHECK_CLASS_CAST((classt), X_TYPE_FLAGS, XFlagsClass))
#define X_IS_FLAGS_CLASS(classt)            (X_TYPE_CHECK_CLASS_TYPE((classt), X_TYPE_FLAGS))
#define X_FLAGS_CLASS_TYPE(classt)          (X_TYPE_FROM_CLASS(classt))
#define X_FLAGS_CLASS_TYPE_NAME(classt)     (x_type_name(X_FLAGS_CLASS_TYPE(classt)))

#define X_VALUE_HOLDS_ENUM(value)           (X_TYPE_CHECK_VALUE_TYPE((value), X_TYPE_ENUM))
#define X_VALUE_HOLDS_FLAGS(value)          (X_TYPE_CHECK_VALUE_TYPE((value), X_TYPE_FLAGS))

typedef struct _XEnumClass XEnumClass;
typedef struct _XEnumValue XEnumValue;
typedef struct _XFlagsClass XFlagsClass;
typedef struct _XFlagsValue XFlagsValue;

struct _XEnumClass {
    XTypeClass  x_type_class;

    xint        minimum;
    xint        maximum;
    xuint       n_values;
    XEnumValue  *values;
};

struct _XFlagsClass {
    XTypeClass   x_type_class;

    xuint        mask;
    xuint        n_values;
    XFlagsValue  *values;
};

struct _XEnumValue {
    xint        value;
    const xchar *value_name;
    const xchar *value_nick;
};

struct _XFlagsValue {
    xuint       value;
    const xchar *value_name;
    const xchar *value_nick;
};

XLIB_AVAILABLE_IN_ALL
XEnumValue *x_enum_get_value(XEnumClass *enum_class, xint value);

XLIB_AVAILABLE_IN_ALL
XEnumValue *x_enum_get_value_by_name(XEnumClass *enum_class, const xchar *name);

XLIB_AVAILABLE_IN_ALL
XEnumValue *x_enum_get_value_by_nick(XEnumClass *enum_class, const xchar *nick);

XLIB_AVAILABLE_IN_ALL
XFlagsValue *x_flags_get_first_value(XFlagsClass *flags_class, xuint value);

XLIB_AVAILABLE_IN_ALL
XFlagsValue *x_flags_get_value_by_name(XFlagsClass *flags_class, const xchar *name);

XLIB_AVAILABLE_IN_ALL
XFlagsValue *x_flags_get_value_by_nick(XFlagsClass *flags_class, const xchar *nick);

XLIB_AVAILABLE_IN_2_54
xchar *x_enum_to_string(XType x_enum_type, xint value);

XLIB_AVAILABLE_IN_2_54
xchar *x_flags_to_string(XType flags_type, xuint value);

XLIB_AVAILABLE_IN_ALL
void x_value_set_enum(XValue *value, xint v_enum);

XLIB_AVAILABLE_IN_ALL
xint x_value_get_enum(const XValue *value);

XLIB_AVAILABLE_IN_ALL
void x_value_set_flags(XValue *value, xuint v_flags);

XLIB_AVAILABLE_IN_ALL
xuint x_value_get_flags(const XValue *value);

XLIB_AVAILABLE_IN_ALL
XType x_enum_register_static(const xchar *name, const XEnumValue *const_static_values);

XLIB_AVAILABLE_IN_ALL
XType x_flags_register_static(const xchar *name, const XFlagsValue *const_static_values);

XLIB_AVAILABLE_IN_ALL
void x_enum_complete_type_info(XType x_enum_type, XTypeInfo *info, const XEnumValue *const_values);

XLIB_AVAILABLE_IN_ALL
void x_flags_complete_type_info(XType x_flags_type, XTypeInfo *info, const XFlagsValue *const_values);

#define X_DEFINE_ENUM_VALUE(EnumValue, EnumNick)        { EnumValue, #EnumValue, EnumNick } XLIB_AVAILABLE_MACRO_IN_2_74

#define X_DEFINE_ENUM_TYPE(TypeName, type_name, ...)                                                        \
    XType type_name ## _get_type(void)                                                                      \
    {                                                                                                       \
        static xsize x_define_type__static = 0;                                                             \
        if (x_once_init_enter(&x_define_type__static)) {                                                    \
            static const XEnumValue enum_values[] = {                                                       \
                __VA_ARGS__ ,                                                                               \
                { 0, NULL, NULL },                                                                          \
            };                                                                                              \
            XType x_define_type = x_enum_register_static(x_intern_static_string(#TypeName), enum_values);   \
            x_once_init_leave(&x_define_type__static, x_define_type);                                       \
        }                                                                                                   \
        return x_define_type__static;                                                                       \
    } XLIB_AVAILABLE_MACRO_IN_2_74

#define X_DEFINE_FLAGS_TYPE(TypeName, type_name, ...)                                                       \
    XType type_name ## _get_type(void)                                                                      \
    {                                                                                                       \
        static xsize x_define_type__static = 0;                                                             \
        if (X_once_init_enter(&x_define_type__static)) {                                                    \
            static const XFlagsValue flags_values[] = {                                                     \
                __VA_ARGS__ ,                                                                               \
                { 0, NULL, NULL },                                                                          \
            };                                                                                              \
            XType x_define_type = x_flags_register_static(x_intern_static_string(#TypeName), flags_values); \
            x_once_init_leave(&x_define_type__static, x_define_type);                                       \
        }                                                                                                   \
        return x_define_type__static;                                                                       \
    } XLIB_AVAILABLE_MACRO_IN_2_74

X_END_DECLS

#endif
