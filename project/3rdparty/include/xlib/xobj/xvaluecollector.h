#ifndef __X_VALUE_COLLECTOR_H__
#define __X_VALUE_COLLECTOR_H__

#include "../xlib/xlib-object.h"

X_BEGIN_DECLS

enum {
    X_VALUE_COLLECT_INT     = 'i',
    X_VALUE_COLLECT_LONG    = 'l',
    X_VALUE_COLLECT_INT64   = 'q',
    X_VALUE_COLLECT_DOUBLE  = 'd',
    X_VALUE_COLLECT_POINTER = 'p'
};

union _XTypeCValue {
    xint     v_int;
    xlong    v_long;
    xint64   v_int64;
    xdouble  v_double;
    xpointer v_pointer;
};

#define X_VALUE_COLLECT_INIT(value, _value_type, var_args, flags, __error)                  \
    X_STMT_START {                                                                          \
        XTypeValueTable *x_vci_vtab;                                                        \
        X_VALUE_COLLECT_INIT2(value, x_vci_vtab, _value_type, var_args, flags, __error);    \
    } X_STMT_END

#define X_VALUE_COLLECT_INIT2(value, x_vci_vtab, _value_type, var_args, flags, __error)                 \
    X_STMT_START {                                                                                      \
        XValue *x_vci_val = (value);                                                                    \
        xuint x_vci_flags = (flags);                                                                    \
        const xchar *x_vci_collect_format;                                                              \
        XTypeCValue x_vci_cvalues[X_VALUE_COLLECT_FORMAT_MAX_LENGTH] = { { 0, }, };                     \
        xuint x_vci_n_values = 0;                                                                       \
        x_vci_vtab = x_type_value_table_peek (_value_type);                                             \
        x_vci_collect_format = x_vci_vtab->collect_format;                                              \
        x_vci_val->x_type = _value_type;                                                                \
        while (*x_vci_collect_format) {                                                                 \
            XTypeCValue *x_vci_cvalue = x_vci_cvalues + x_vci_n_values++;                               \
                                                                                                        \
            switch (*x_vci_collect_format++) {                                                          \
                case X_VALUE_COLLECT_INT:                                                               \
                    x_vci_cvalue->v_int = va_arg((var_args), xint);                                     \
                    break;                                                                              \
                case X_VALUE_COLLECT_LONG:                                                              \
                    x_vci_cvalue->v_long = va_arg((var_args), xlong);                                   \
                    break;                                                                              \
                case X_VALUE_COLLECT_INT64:                                                             \
                    x_vci_cvalue->v_int64 = va_arg((var_args), xint64);                                 \
                    break;                                                                              \
                case X_VALUE_COLLECT_DOUBLE:                                                            \
                    x_vci_cvalue->v_double = va_arg((var_args), xdouble);                               \
                    break;                                                                              \
                case X_VALUE_COLLECT_POINTER:                                                           \
                    x_vci_cvalue->v_pointer = va_arg((var_args), xpointer);                             \
                    break;                                                                              \
                default:                                                                                \
                    x_assert_not_reached();                                                             \
            }                                                                                           \
        }                                                                                               \
        *(__error) = x_vci_vtab->collect_value(x_vci_val, x_vci_n_values, x_vci_cvalues, x_vci_flags);  \
    } X_STMT_END

#define X_VALUE_COLLECT(value, var_args, flags, __error)                                                \
    X_STMT_START {                                                                                      \
        XValue *x_vc_value = (value);                                                                   \
        XType x_vc_value_type = X_VALUE_TYPE(x_vc_value);                                               \
        XTypeValueTable *x_vc_vtable = x_type_value_table_peek(x_vc_value_type);                        \
                                                                                                        \
        if (x_vc_vtable->value_free) {                                                                  \
            x_vc_vtable->value_free(x_vc_value);                                                        \
        }                                                                                               \
        memset(x_vc_value->data, 0, sizeof(x_vc_value->data));                                          \
                                                                                                        \
        X_VALUE_COLLECT_INIT(value, x_vc_value_type, var_args, flags, __error);                         \
    } X_STMT_END

#define X_VALUE_COLLECT_SKIP(_value_type, var_args)                                                     \
    X_STMT_START {                                                                                      \
        XTypeValueTable *x_vcs_vtable = x_type_value_table_peek(_value_type);                           \
        const xchar *x_vcs_collect_format = x_vcs_vtable->collect_format;                               \
                                                                                                        \
        while (*x_vcs_collect_format) {                                                                 \
            switch (*x_vcs_collect_format++) {                                                          \
                case X_VALUE_COLLECT_INT:                                                               \
                    va_arg((var_args), xint);                                                           \
                    break;                                                                              \
                case X_VALUE_COLLECT_LONG:                                                              \
                    va_arg((var_args), xlong);                                                          \
                    break;                                                                              \
                case X_VALUE_COLLECT_INT64:                                                             \
                    va_arg((var_args), xint64);                                                         \
                    break;                                                                              \
                case X_VALUE_COLLECT_DOUBLE:                                                            \
                    va_arg((var_args), xdouble);                                                        \
                    break;                                                                              \
                case X_VALUE_COLLECT_POINTER:                                                           \
                    va_arg((var_args), xpointer);                                                       \
                    break;                                                                              \
                default:                                                                                \
                    x_assert_not_reached();                                                             \
            }                                                                                           \
        }                                                                                               \
    } X_STMT_END

#define X_VALUE_LCOPY(value, var_args, flags, __error)                                                  \
    X_STMT_START {                                                                                      \
        const XValue *x_vl_value = (value);                                                             \
        xuint x_vl_flags = (flags);                                                                     \
        XType x_vl_value_type = X_VALUE_TYPE(x_vl_value);                                               \
        XTypeValueTable *x_vl_vtable = x_type_value_table_peek(x_vl_value_type);                        \
        const xchar *x_vl_lcopy_format = x_vl_vtable->lcopy_format;                                     \
        XTypeCValue x_vl_cvalues[X_VALUE_COLLECT_FORMAT_MAX_LENGTH] = { { 0, }, };                      \
        xuint x_vl_n_values = 0;                                                                        \
                                                                                                        \
        while (*x_vl_lcopy_format) {                                                                    \
            XTypeCValue *x_vl_cvalue = x_vl_cvalues + x_vl_n_values++;                                  \
                                                                                                        \
            switch (*x_vl_lcopy_format++) {                                                             \
                case X_VALUE_COLLECT_INT:                                                               \
                    x_vl_cvalue->v_int = va_arg((var_args), xint);                                      \
                    break;                                                                              \
                case X_VALUE_COLLECT_LONG:                                                              \
                    x_vl_cvalue->v_long = va_arg((var_args), xlong);                                    \
                    break;                                                                              \
                case X_VALUE_COLLECT_INT64:                                                             \
                    x_vl_cvalue->v_int64 = va_arg((var_args), xint64);                                  \
                    break;                                                                              \
                case X_VALUE_COLLECT_DOUBLE:                                                            \
                    x_vl_cvalue->v_double = va_arg((var_args), xdouble);                                \
                    break;                                                                              \
                case X_VALUE_COLLECT_POINTER:                                                           \
                    x_vl_cvalue->v_pointer = va_arg((var_args), xpointer);                              \
                    break;                                                                              \
                default:                                                                                \
                    x_assert_not_reached();                                                             \
            }                                                                                           \
        }                                                                                               \
        *(__error) = x_vl_vtable->lcopy_value(x_vl_value, x_vl_n_values, x_vl_cvalues, x_vl_flags);     \
    } X_STMT_END

#define X_VALUE_COLLECT_FORMAT_MAX_LENGTH   (8)

X_END_DECLS

#endif
