#include <string.h>
#include <stdlib.h>
#include <xlib/xlib/config.h>
#include <xlib/xobj/xparam.h>
#include <xlib/xobj/xboxed.h>
#include <xlib/xobj/xenums.h>
#include <xlib/xobj/xobject.h>
#include <xlib/xobj/xvaluetypes.h>
#include <xlib/xobj/xtype-private.h>
#include <xlib/xobj/xvaluecollector.h>

static void value_init_long0(XValue *value)
{
    value->data[0].v_long = 0;
}

static void value_copy_long0(const XValue *src_value, XValue *dest_value)
{
    dest_value->data[0].v_long = src_value->data[0].v_long;
}

static xchar *value_lcopy_char(const XValue *value, xuint n_collect_values, XTypeCValue *collect_values, xuint collect_flags)
{
    xint8 *int8_p = (xint8 *)collect_values[0].v_pointer;

    x_return_val_if_fail(int8_p != NULL, x_strdup_printf("value location for '%s' passed as NULL", X_VALUE_TYPE_NAME(value)));

    *int8_p = value->data[0].v_int;
    return NULL;
}

static xchar *value_lcopy_boolean(const XValue *value, xuint n_collect_values, XTypeCValue *collect_values, xuint collect_flags)
{
    xboolean *bool_p = (xboolean *)collect_values[0].v_pointer;

    x_return_val_if_fail(bool_p != NULL, x_strdup_printf("value location for '%s' passed as NULL", X_VALUE_TYPE_NAME(value)));
    *bool_p = value->data[0].v_int;

    return NULL;
}

static xchar *value_collect_int(XValue *value, xuint n_collect_values, XTypeCValue *collect_values, xuint collect_flags)
{
    value->data[0].v_int = collect_values[0].v_int;
    return NULL;
}

static xchar *value_lcopy_int(const XValue *value, xuint n_collect_values, XTypeCValue *collect_values, xuint collect_flags)
{
    xint *int_p = (xint *)collect_values[0].v_pointer;

    x_return_val_if_fail(int_p != NULL, x_strdup_printf("value location for '%s' passed as NULL", X_VALUE_TYPE_NAME(value)));
    *int_p = value->data[0].v_int;

    return NULL;
}

static xchar *value_collect_long(XValue *value, xuint n_collect_values, XTypeCValue *collect_values, xuint collect_flags)
{
    value->data[0].v_long = collect_values[0].v_long;
    return NULL;
}

static xchar *value_lcopy_long(const XValue *value, xuint n_collect_values, XTypeCValue *collect_values, xuint collect_flags)
{
    xlong *long_p = (xlong *)collect_values[0].v_pointer;

    x_return_val_if_fail(long_p != NULL, x_strdup_printf("value location for '%s' passed as NULL", X_VALUE_TYPE_NAME(value)));
    *long_p = value->data[0].v_long;

    return NULL;
}

static void value_init_int64(XValue *value)
{
    value->data[0].v_int64 = 0;
}

static void value_copy_int64(const XValue *src_value, XValue *dest_value)
{
    dest_value->data[0].v_int64 = src_value->data[0].v_int64;
}

static xchar *value_collect_int64(XValue *value, xuint n_collect_values, XTypeCValue *collect_values, xuint collect_flags)
{
    value->data[0].v_int64 = collect_values[0].v_int64;
    return NULL;
}

static xchar *value_lcopy_int64(const XValue *value, xuint n_collect_values, XTypeCValue *collect_values, xuint collect_flags)
{
    xint64 *int64_p = (xint64 *)collect_values[0].v_pointer;

    x_return_val_if_fail(int64_p != NULL, x_strdup_printf("value location for '%s' passed as NULL", X_VALUE_TYPE_NAME(value)));
    *int64_p = value->data[0].v_int64;

    return NULL;
}

static void value_init_float(XValue *value)
{
    value->data[0].v_float = 0.0;
}

static void value_copy_float(const XValue *src_value, XValue *dest_value)
{
    dest_value->data[0].v_float = src_value->data[0].v_float;
}

static xchar *value_collect_float(XValue *value, xuint n_collect_values, XTypeCValue *collect_values, xuint collect_flags)
{
    value->data[0].v_float = collect_values[0].v_double;
    return NULL;
}

static xchar *value_lcopy_float(const XValue *value, xuint n_collect_values, XTypeCValue *collect_values, xuint collect_flags)
{
    xfloat *float_p = (xfloat *)collect_values[0].v_pointer;

    x_return_val_if_fail(float_p != NULL, x_strdup_printf("value location for '%s' passed as NULL", X_VALUE_TYPE_NAME(value)));
    *float_p = value->data[0].v_float;

    return NULL;
}

static void value_init_double(XValue *value)
{
    value->data[0].v_double = 0.0;
}

static void value_copy_double(const XValue *src_value, XValue *dest_value)
{
    dest_value->data[0].v_double = src_value->data[0].v_double;
}

static xchar *value_collect_double(XValue *value, xuint n_collect_values, XTypeCValue *collect_values, xuint collect_flags)
{
    value->data[0].v_double = collect_values[0].v_double;
    return NULL;
}

static xchar *value_lcopy_double(const XValue *value, xuint n_collect_values, XTypeCValue *collect_values, xuint collect_flags)
{
    xdouble *double_p = (xdouble *)collect_values[0].v_pointer;

    x_return_val_if_fail(double_p != NULL, x_strdup_printf("value location for '%s' passed as NULL", X_VALUE_TYPE_NAME(value)));
    *double_p = value->data[0].v_double;

    return NULL;
}

static void value_init_string(XValue *value)
{
    value->data[0].v_pointer = NULL;
}

static void value_free_string(XValue *value)
{
    if (!(value->data[1].v_uint & X_VALUE_NOCOPY_CONTENTS)) {
        x_free(value->data[0].v_pointer);
    }
}

static void value_copy_string(const XValue *src_value, XValue *dest_value)
{
    if (src_value->data[1].v_uint & X_VALUE_INTERNED_STRING) {
        dest_value->data[0].v_pointer = src_value->data[0].v_pointer;
        dest_value->data[1].v_uint = src_value->data[1].v_uint;
    } else {
        dest_value->data[0].v_pointer = x_strdup((const xchar *)src_value->data[0].v_pointer);
    }
}

static xchar *value_collect_string(XValue *value, xuint n_collect_values, XTypeCValue *collect_values, xuint collect_flags)
{
    if (!collect_values[0].v_pointer) {
        value->data[0].v_pointer = NULL;
    } else if (collect_flags & X_VALUE_NOCOPY_CONTENTS) {
        value->data[0].v_pointer = collect_values[0].v_pointer;
        value->data[1].v_uint = X_VALUE_NOCOPY_CONTENTS;
    } else {
        value->data[0].v_pointer = x_strdup((const xchar *)collect_values[0].v_pointer);
    }

    return NULL;
}

static xchar *value_lcopy_string(const XValue *value, xuint n_collect_values, XTypeCValue *collect_values, xuint collect_flags)
{
    xchar **string_p = (xchar **)collect_values[0].v_pointer;

    x_return_val_if_fail(string_p != NULL, x_strdup_printf("value location for '%s' passed as NULL", X_VALUE_TYPE_NAME(value)));

    if (!value->data[0].v_pointer) {
        *string_p = NULL;
    } else if (collect_flags & X_VALUE_NOCOPY_CONTENTS) {
        *string_p = (xchar *)value->data[0].v_pointer;
    } else {
        *string_p = x_strdup((const xchar *)value->data[0].v_pointer);
    }

    return NULL;
}

static void value_init_pointer(XValue *value)
{
    value->data[0].v_pointer = NULL;
}

static void value_copy_pointer(const XValue *src_value, XValue *dest_value)
{
    dest_value->data[0].v_pointer = src_value->data[0].v_pointer;
}

static xpointer value_peek_pointer0(const XValue *value)
{
    return value->data[0].v_pointer;
}

static xchar *value_collect_pointer(XValue *value, xuint n_collect_values, XTypeCValue *collect_values, xuint collect_flags)
{
    value->data[0].v_pointer = collect_values[0].v_pointer;
    return NULL;
}

static xchar *value_lcopy_pointer(const XValue *value, xuint n_collect_values, XTypeCValue *collect_values, xuint collect_flags)
{
    xpointer *pointer_p = (xpointer *)collect_values[0].v_pointer;

    x_return_val_if_fail(pointer_p != NULL, x_strdup_printf("value location for '%s' passed as NULL", X_VALUE_TYPE_NAME(value)));
    *pointer_p = value->data[0].v_pointer;

    return NULL;
}

static void value_free_variant(XValue *value)
{
    if (!(value->data[1].v_uint & X_VALUE_NOCOPY_CONTENTS) && value->data[0].v_pointer) {
        x_variant_unref((XVariant *)value->data[0].v_pointer);
    }
}

static void value_copy_variant(const XValue *src_value, XValue *dest_value)
{
    if (src_value->data[0].v_pointer) {
        dest_value->data[0].v_pointer = x_variant_ref_sink((XVariant *)src_value->data[0].v_pointer);
    } else {
        dest_value->data[0].v_pointer = NULL;
    }
}

static xchar *value_collect_variant(XValue *value, xuint n_collect_values, XTypeCValue *collect_values, xuint collect_flags)
{
    if (!collect_values[0].v_pointer) {
        value->data[0].v_pointer = NULL;
    } else {
        value->data[0].v_pointer = x_variant_ref_sink((XVariant *)collect_values[0].v_pointer);
    }

    return NULL;
}

static xchar *value_lcopy_variant(const XValue *value, xuint n_collect_values, XTypeCValue *collect_values, xuint collect_flags)
{
    XVariant **variant_p = (XVariant **)collect_values[0].v_pointer;

    x_return_val_if_fail(variant_p != NULL, x_strdup_printf("value location for '%s' passed as NULL", X_VALUE_TYPE_NAME(value)));

    if (!value->data[0].v_pointer) {
        *variant_p = NULL;
    } else if (collect_flags & X_VALUE_NOCOPY_CONTENTS) {
        *variant_p = (XVariant *)value->data[0].v_pointer;
    } else {
        *variant_p = x_variant_ref_sink((XVariant *)value->data[0].v_pointer);
    }

    return NULL;
}

void _x_value_types_init(void)
{
    XTypeInfo info = {
        0,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        0,
        0,
        NULL,
        NULL,
    };

    XType type X_GNUC_UNUSED;
    const XTypeFundamentalInfo finfo = { X_TYPE_FLAG_DERIVABLE, };

    {
        static const XTypeValueTable value_table = {
            value_init_long0,
            NULL,
            value_copy_long0,
            NULL,
            "i",
            value_collect_int,
            "p",
            value_lcopy_char,
        };

        info.value_table = &value_table;
        type = x_type_register_fundamental(X_TYPE_CHAR, x_intern_static_string("xchar"), &info, &finfo, (XTypeFlags)0);
        x_assert(type == X_TYPE_CHAR);
        type = x_type_register_fundamental(X_TYPE_UCHAR, x_intern_static_string("xuchar"), &info, &finfo, (XTypeFlags)0);
        x_assert(type == X_TYPE_UCHAR);
    }

    {
        static const XTypeValueTable value_table = {
            value_init_long0,
            NULL,
            value_copy_long0,
            NULL,
            "i",
            value_collect_int,
            "p",
            value_lcopy_boolean,
        };

        info.value_table = &value_table;
        type = x_type_register_fundamental(X_TYPE_BOOLEAN, x_intern_static_string("xboolean"), &info, &finfo, (XTypeFlags)0);
        x_assert(type == X_TYPE_BOOLEAN);
    }

    {
        static const XTypeValueTable value_table = {
            value_init_long0,
            NULL,
            value_copy_long0,
            NULL,
            "i",
            value_collect_int,
            "p",
            value_lcopy_int,
        };

        info.value_table = &value_table;
        type = x_type_register_fundamental(X_TYPE_INT, x_intern_static_string("xint"), &info, &finfo, (XTypeFlags)0);
        x_assert(type == X_TYPE_INT);
        type = x_type_register_fundamental(X_TYPE_UINT, x_intern_static_string("xuint"), &info, &finfo, (XTypeFlags)0);
        x_assert(type == X_TYPE_UINT);
    }

    {
        static const XTypeValueTable value_table = {
            value_init_long0,
            NULL,
            value_copy_long0,
            NULL,
            "l",
            value_collect_long,
            "p",
            value_lcopy_long,
        };

        info.value_table = &value_table;
        type = x_type_register_fundamental(X_TYPE_LONG, x_intern_static_string("xlong"), &info, &finfo, (XTypeFlags)0);
        x_assert(type == X_TYPE_LONG);
        type = x_type_register_fundamental(X_TYPE_ULONG, x_intern_static_string("xulong"), &info, &finfo, (XTypeFlags)0);
        x_assert(type == X_TYPE_ULONG);
    }

    {
        static const XTypeValueTable value_table = {
            value_init_int64,
            NULL,
            value_copy_int64,
            NULL,
            "q",
            value_collect_int64,
            "p",
            value_lcopy_int64,
        };

        info.value_table = &value_table;
        type = x_type_register_fundamental(X_TYPE_INT64, x_intern_static_string("xint64"), &info, &finfo, (XTypeFlags)0);
        x_assert(type == X_TYPE_INT64);
        type = x_type_register_fundamental(X_TYPE_UINT64, x_intern_static_string("xuint64"), &info, &finfo, (XTypeFlags)0);
        x_assert(type == X_TYPE_UINT64);
    }

    {
        static const XTypeValueTable value_table = {
            value_init_float,
            NULL,
            value_copy_float,
            NULL,
            "d",
            value_collect_float,
            "p",
            value_lcopy_float,
        };

        info.value_table = &value_table;
        type = x_type_register_fundamental(X_TYPE_FLOAT, x_intern_static_string("xfloat"), &info, &finfo, (XTypeFlags)0);
        x_assert(type == X_TYPE_FLOAT);
    }

    {
        static const XTypeValueTable value_table = {
            value_init_double,
            NULL,
            value_copy_double,
            NULL,
            "d",
            value_collect_double,
            "p",
            value_lcopy_double,
        };

        info.value_table = &value_table;
        type = x_type_register_fundamental(X_TYPE_DOUBLE, x_intern_static_string("xdouble"), &info, &finfo, (XTypeFlags)0);
        x_assert(type == X_TYPE_DOUBLE);
    }

    {
        static const XTypeValueTable value_table = {
            value_init_string,
            value_free_string,
            value_copy_string,
            value_peek_pointer0,
            "p",
            value_collect_string,
            "p",
            value_lcopy_string,
        };

        info.value_table = &value_table;
        type = x_type_register_fundamental(X_TYPE_STRING, x_intern_static_string("xchararray"), &info, &finfo, (XTypeFlags)0);
        x_assert(type == X_TYPE_STRING);
    }

    {
        static const XTypeValueTable value_table = {
            value_init_pointer,
            NULL,
            value_copy_pointer,
            value_peek_pointer0,
            "p",
            value_collect_pointer,
            "p",
            value_lcopy_pointer,
        };

        info.value_table = &value_table;
        type = x_type_register_fundamental(X_TYPE_POINTER, x_intern_static_string("xpointer"), &info, &finfo, (XTypeFlags)0);
        x_assert(type == X_TYPE_POINTER);
    }

    {
        static const XTypeValueTable value_table = {
            value_init_pointer,
            value_free_variant,
            value_copy_variant,
            value_peek_pointer0,
            "p",
            value_collect_variant,
            "p",
            value_lcopy_variant,
        };

        info.value_table = &value_table;
        type = x_type_register_fundamental(X_TYPE_VARIANT, x_intern_static_string("XVariant"), &info, &finfo, (XTypeFlags)0);
        x_assert(type == X_TYPE_VARIANT);
    }
}

void x_value_set_char(XValue *value, xchar v_char)
{
    x_return_if_fail(X_VALUE_HOLDS_CHAR(value));
    value->data[0].v_int = v_char;
}

xchar x_value_get_char(const XValue *value)
{
    x_return_val_if_fail(X_VALUE_HOLDS_CHAR(value), 0);
    return value->data[0].v_int;
}

void x_value_set_schar(XValue *value, xint8 v_char)
{
    x_return_if_fail(X_VALUE_HOLDS_CHAR(value));
    value->data[0].v_int = v_char;
}

xint8 x_value_get_schar(const XValue *value)
{
    x_return_val_if_fail(X_VALUE_HOLDS_CHAR(value), 0);
    return value->data[0].v_int;
}

void x_value_set_uchar(XValue *value, xuchar v_uchar)
{
    x_return_if_fail(X_VALUE_HOLDS_UCHAR(value));
    value->data[0].v_uint = v_uchar;
}

xuchar x_value_get_uchar(const XValue *value)
{
    x_return_val_if_fail(X_VALUE_HOLDS_UCHAR(value), 0);
    return value->data[0].v_uint;
}

void x_value_set_boolean(XValue *value, xboolean v_boolean)
{
    x_return_if_fail(X_VALUE_HOLDS_BOOLEAN(value));
    value->data[0].v_int = v_boolean != FALSE;
}

xboolean x_value_get_boolean(const XValue *value)
{
    x_return_val_if_fail(X_VALUE_HOLDS_BOOLEAN(value), 0);
    return value->data[0].v_int;
}

void x_value_set_int(XValue *value, xint v_int)
{
    x_return_if_fail(X_VALUE_HOLDS_INT(value));
    value->data[0].v_int = v_int;
}

xint x_value_get_int(const XValue *value)
{
    x_return_val_if_fail(X_VALUE_HOLDS_INT(value), 0);
    return value->data[0].v_int;
}

void x_value_set_uint(XValue *value, xuint v_uint)
{
    x_return_if_fail(X_VALUE_HOLDS_UINT(value));
    value->data[0].v_uint = v_uint;
}

xuint x_value_get_uint(const XValue *value)
{
    x_return_val_if_fail(X_VALUE_HOLDS_UINT(value), 0);
    return value->data[0].v_uint;
}

void x_value_set_long(XValue *value, xlong v_long)
{
    x_return_if_fail(X_VALUE_HOLDS_LONG(value));
    value->data[0].v_long = v_long;
}

xlong x_value_get_long(const XValue *value)
{
    x_return_val_if_fail(X_VALUE_HOLDS_LONG(value), 0);
    return value->data[0].v_long;
}

void x_value_set_ulong(XValue *value, xulong v_ulong)
{
    x_return_if_fail(X_VALUE_HOLDS_ULONG(value));
    value->data[0].v_ulong = v_ulong;
}

xulong x_value_get_ulong(const XValue *value)
{
    x_return_val_if_fail(X_VALUE_HOLDS_ULONG(value), 0);
    return value->data[0].v_ulong;
}

void x_value_set_int64(XValue *value, xint64 v_int64)
{
    x_return_if_fail(X_VALUE_HOLDS_INT64(value));
    value->data[0].v_int64 = v_int64;
}

xint64 x_value_get_int64(const XValue *value)
{
    x_return_val_if_fail(X_VALUE_HOLDS_INT64(value), 0);
    return value->data[0].v_int64;
}

void x_value_set_uint64(XValue *value, xuint64 v_uint64)
{
    x_return_if_fail(X_VALUE_HOLDS_UINT64(value));
    value->data[0].v_uint64 = v_uint64;
}

xuint64 x_value_get_uint64(const XValue *value)
{
    x_return_val_if_fail(X_VALUE_HOLDS_UINT64(value), 0);
    return value->data[0].v_uint64;
}

void x_value_set_float(XValue *value, xfloat v_float)
{
    x_return_if_fail(X_VALUE_HOLDS_FLOAT(value));
    value->data[0].v_float = v_float;
}

xfloat x_value_get_float(const XValue *value)
{
    x_return_val_if_fail(X_VALUE_HOLDS_FLOAT(value), 0);
    return value->data[0].v_float;
}

void x_value_set_double(XValue *value, xdouble v_double)
{
    x_return_if_fail(X_VALUE_HOLDS_DOUBLE(value));
    value->data[0].v_double = v_double;
}

xdouble x_value_get_double(const XValue *value)
{
    x_return_val_if_fail(X_VALUE_HOLDS_DOUBLE(value), 0);
    return value->data[0].v_double;
}

void x_value_set_string(XValue *value, const xchar *v_string)
{
    xchar *new_val;

    x_return_if_fail(X_VALUE_HOLDS_STRING(value));

    new_val = x_strdup(v_string);

    if (value->data[1].v_uint & X_VALUE_NOCOPY_CONTENTS) {
        value->data[1].v_uint = 0;
    } else {
        x_free(value->data[0].v_pointer);
    }

    value->data[0].v_pointer = new_val;
}

void x_value_set_static_string(XValue *value, const xchar *v_string)
{
    x_return_if_fail(X_VALUE_HOLDS_STRING(value));

    if (!(value->data[1].v_uint & X_VALUE_NOCOPY_CONTENTS)) {
        x_free(value->data[0].v_pointer);
    }

    value->data[1].v_uint = X_VALUE_NOCOPY_CONTENTS;
    value->data[0].v_pointer = (xchar *)v_string;
}

void x_value_set_interned_string(XValue *value, const xchar *v_string)
{
    x_return_if_fail(X_VALUE_HOLDS_STRING(value));

    if (!(value->data[1].v_uint & X_VALUE_NOCOPY_CONTENTS)) {
        x_free(value->data[0].v_pointer);
    }

    value->data[1].v_uint = X_VALUE_NOCOPY_CONTENTS | X_VALUE_INTERNED_STRING;
    value->data[0].v_pointer = (xchar *)v_string;
}

void x_value_set_string_take_ownership(XValue *value, xchar *v_string)
{
    x_value_take_string(value, v_string);
}

void x_value_take_string(XValue *value, xchar *v_string)
{
    x_return_if_fail(X_VALUE_HOLDS_STRING(value));

    if (value->data[1].v_uint & X_VALUE_NOCOPY_CONTENTS) {
        value->data[1].v_uint = 0;
    } else {
        x_free(value->data[0].v_pointer);
    }

    value->data[0].v_pointer = v_string;
}

const xchar *x_value_get_string(const XValue *value)
{
    x_return_val_if_fail(X_VALUE_HOLDS_STRING(value), NULL);
    return (const xchar *)value->data[0].v_pointer;
}

xchar *x_value_dup_string(const XValue *value)
{
    x_return_val_if_fail(X_VALUE_HOLDS_STRING(value), NULL);
    return x_strdup((const xchar *)value->data[0].v_pointer);
}

xchar *x_value_steal_string(XValue *value)
{
    xchar *ret;

    x_return_val_if_fail(X_VALUE_HOLDS_STRING(value), NULL);

    ret = value->data[0].v_pointer;
    value->data[0].v_pointer = NULL;

    if (value->data[1].v_uint & X_VALUE_NOCOPY_CONTENTS) {
        return x_strdup(ret);
    }

    return ret;
}

void x_value_set_pointer(XValue *value, xpointer v_pointer)
{
    x_return_if_fail(X_VALUE_HOLDS_POINTER(value));
    value->data[0].v_pointer = v_pointer;
}

xpointer x_value_get_pointer(const XValue *value)
{
    x_return_val_if_fail(X_VALUE_HOLDS_POINTER(value), NULL);
    return value->data[0].v_pointer;
}

X_DEFINE_POINTER_TYPE(XType, x_gtype)

void x_value_set_gtype(XValue *value, XType v_gtype)
{
    x_return_if_fail(X_VALUE_HOLDS_GTYPE(value));
    value->data[0].v_pointer = XTYPE_TO_POINTER(v_gtype);
}

XType x_value_get_gtype(const XValue *value)
{
    x_return_val_if_fail(X_VALUE_HOLDS_GTYPE(value), 0);
    return XPOINTER_TO_TYPE(value->data[0].v_pointer);
}

void x_value_set_variant(XValue *value, XVariant *variant)
{
    XVariant *old_variant;

    x_return_if_fail(X_VALUE_HOLDS_VARIANT(value));

    old_variant = (XVariant *)value->data[0].v_pointer;

    if (variant) {
        value->data[0].v_pointer = x_variant_ref_sink(variant);
    } else {
        value->data[0].v_pointer = NULL;
    }

    if (old_variant) {
        x_variant_unref(old_variant);
    }
}

void x_value_take_variant(XValue *value, XVariant *variant)
{
    XVariant *old_variant;

    x_return_if_fail(X_VALUE_HOLDS_VARIANT(value));

    old_variant = (XVariant *)value->data[0].v_pointer;

    if (variant) {
        value->data[0].v_pointer = x_variant_take_ref(variant);
    } else {
        value->data[0].v_pointer = NULL;
    }

    if (old_variant) {
        x_variant_unref(old_variant);
    }
}

XVariant *x_value_get_variant(const XValue *value)
{
    x_return_val_if_fail(X_VALUE_HOLDS_VARIANT(value), NULL);
    return (XVariant *)value->data[0].v_pointer;
}

XVariant *x_value_dup_variant(const XValue *value)
{
    XVariant *variant;

    x_return_val_if_fail(X_VALUE_HOLDS_VARIANT(value), NULL);

    variant = (XVariant *)value->data[0].v_pointer;
    if (variant) {
        x_variant_ref_sink(variant);
    }

    return variant;
}

xchar *x_strdup_value_contents(const XValue *value)
{
    xchar *contents;
    const xchar *src;

    x_return_val_if_fail(X_IS_VALUE(value), NULL);

    if (X_VALUE_HOLDS_STRING(value)) {
        src = x_value_get_string(value);

        if (!src) {
            contents = x_strdup("NULL");
        } else {
            xchar *s = x_strescape(src, NULL);
            contents = x_strdup_printf("\"%s\"", s);
            x_free(s);
        }
    } else if (x_value_type_transformable(X_VALUE_TYPE(value), X_TYPE_STRING)) {
        xchar *s;
        XValue tmp_value = X_VALUE_INIT;

        x_value_init(&tmp_value, X_TYPE_STRING);
        x_value_transform(value, &tmp_value);
        s = x_strescape(x_value_get_string(&tmp_value), NULL);
        x_value_unset(&tmp_value);

        if (X_VALUE_HOLDS_ENUM(value) || X_VALUE_HOLDS_FLAGS(value)) {
            contents = x_strdup_printf("((%s) %s)", x_type_name(X_VALUE_TYPE(value)), s);
        } else {
            contents = x_strdup(s ? s : "NULL");
        }

        x_free(s);
    } else if (x_value_fits_pointer(value)) {
        xpointer p = x_value_peek_pointer(value);

        if (!p) {
            contents = x_strdup("NULL");
        } else if (X_VALUE_HOLDS_OBJECT(value)) {
            contents = x_strdup_printf("((%s*) %p)", X_OBJECT_TYPE_NAME(p), p);
        } else if (X_VALUE_HOLDS_PARAM(value)) {
            contents = x_strdup_printf("((%s*) %p)", X_PARAM_SPEC_TYPE_NAME(p), p);
        } else if (X_VALUE_HOLDS(value, X_TYPE_STRV)) {
            XStrv strv = (XStrv)x_value_get_boxed(value);
            XString *tmp = x_string_new("[");

            while (*strv != NULL) {
                xchar *escaped = x_strescape(*strv, NULL);

                x_string_append_printf(tmp, "\"%s\"", escaped);
                x_free(escaped);

                if (*++strv != NULL) {
                    x_string_append(tmp, ", ");
                }
            }

            x_string_append(tmp, "]");
            contents = x_string_free(tmp, FALSE);
        } else if (X_VALUE_HOLDS_BOXED(value)) {
            contents = x_strdup_printf("((%s*) %p)", x_type_name(X_VALUE_TYPE(value)), p);
        } else if (X_VALUE_HOLDS_POINTER(value)) {
            contents = x_strdup_printf("((xpointer) %p)", p);
        } else {
            contents = x_strdup("???");
        }
    } else {
        contents = x_strdup("???");
    }

    return contents;
}

XType x_pointer_type_register_static(const xchar *name)
{
    const XTypeInfo type_info = {
        0,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        0,
        0,
        NULL,
        NULL
    };

    XType type;
    x_return_val_if_fail(name != NULL, 0);
    x_return_val_if_fail(x_type_from_name(name) == 0, 0);

    type = x_type_register_static(X_TYPE_POINTER, name, &type_info, (XTypeFlags)0);
    return type;
}
