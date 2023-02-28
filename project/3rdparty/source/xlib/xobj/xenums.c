#include <string.h>

#include <xlib/xlib/config.h>
#include <xlib/xobj/xenums.h>
#include <xlib/xobj/xvalue.h>
#include <xlib/xobj/xtype-private.h>
#include <xlib/xobj/xvaluecollector.h>

static void x_enum_class_init(XEnumClass *classt, xpointer class_data);
static void x_flags_class_init(XFlagsClass *classt, xpointer class_data);

static void value_flags_enum_init (XValue *value);
static void value_flags_enum_copy_value(const XValue *src_value, XValue *dest_value);
static xchar *value_flags_enum_collect_value(XValue *value, xuint n_collect_values, XTypeCValue *collect_values, xuint collect_flags);
static xchar *value_flags_enum_lcopy_value(const XValue *value, xuint n_collect_values, XTypeCValue *collect_values, xuint collect_flags);

void _x_enum_types_init(void)
{
    static xboolean initialized = FALSE;
    static const XTypeValueTable flags_enum_value_table = {
        value_flags_enum_init,
        NULL,
        value_flags_enum_copy_value,
        NULL,
        "i",
        value_flags_enum_collect_value,
        "p",
        value_flags_enum_lcopy_value,
    };

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
        &flags_enum_value_table,
    };

    XType type X_GNUC_UNUSED;
    static const XTypeFundamentalInfo finfo = {
        (XTypeFundamentalFlags)(X_TYPE_FLAG_CLASSED | X_TYPE_FLAG_DERIVABLE),
    };

    x_return_if_fail(initialized == FALSE);
    initialized = TRUE;

    info.class_size = sizeof(XEnumClass);
    type = x_type_register_fundamental(X_TYPE_ENUM, x_intern_static_string("XEnum"), &info, &finfo, (XTypeFlags)(X_TYPE_FLAG_ABSTRACT | X_TYPE_FLAG_VALUE_ABSTRACT));
    x_assert(type == X_TYPE_ENUM);

    info.class_size = sizeof(XFlagsClass);
    type = x_type_register_fundamental(X_TYPE_FLAGS, x_intern_static_string("XFlags"), &info, &finfo, (XTypeFlags)(X_TYPE_FLAG_ABSTRACT | X_TYPE_FLAG_VALUE_ABSTRACT));
    x_assert(type == X_TYPE_FLAGS);
}

static void value_flags_enum_init(XValue *value)
{
    value->data[0].v_long = 0;
}

static void value_flags_enum_copy_value(const XValue *src_value, XValue *dest_value)
{
    dest_value->data[0].v_long = src_value->data[0].v_long;
}

static xchar *value_flags_enum_collect_value(XValue *value, xuint n_collect_values, XTypeCValue *collect_values, xuint collect_flags)
{
    if (X_VALUE_HOLDS_ENUM(value)) {
        value->data[0].v_long = collect_values[0].v_int;
    } else {
        value->data[0].v_ulong = (xuint)collect_values[0].v_int;
    }

    return NULL;
}

static xchar *value_flags_enum_lcopy_value(const XValue *value, xuint n_collect_values, XTypeCValue *collect_values, xuint collect_flags)
{
    xint *int_p = (xint *)collect_values[0].v_pointer;

    x_return_val_if_fail(int_p != NULL, x_strdup_printf("value location for '%s' passed as NULL", X_VALUE_TYPE_NAME(value)));
    *int_p = value->data[0].v_long;

    return NULL;
}

XType x_enum_register_static(const xchar *name, const XEnumValue *const_static_values)
{
    XTypeInfo enum_type_info = {
        sizeof(XEnumClass),
        NULL,
        NULL,
        (XClassInitFunc)x_enum_class_init,
        NULL,
        NULL,
        0,
        0,
        NULL,
        NULL,
    };
    XType type;

    x_return_val_if_fail(name != NULL, 0);
    x_return_val_if_fail(const_static_values != NULL, 0);

    enum_type_info.class_data = const_static_values;
    type = x_type_register_static(X_TYPE_ENUM, name, &enum_type_info, (XTypeFlags)0);

    return type;
}

XType x_flags_register_static(const xchar *name, const XFlagsValue *const_static_values)
{
    XTypeInfo flags_type_info = {
        sizeof(XFlagsClass),
        NULL,
        NULL,
        (XClassInitFunc)x_flags_class_init,
        NULL,
        NULL,
        0,
        0,
        NULL,
        NULL,
    };
    XType type;

    x_return_val_if_fail(name != NULL, 0);
    x_return_val_if_fail(const_static_values != NULL, 0);

    flags_type_info.class_data = const_static_values;
    type = x_type_register_static(X_TYPE_FLAGS, name, &flags_type_info, (XTypeFlags)0);

    return type;
}

void x_enum_complete_type_info(XType x_enum_type, XTypeInfo *info, const XEnumValue *const_values)
{
    x_return_if_fail(X_TYPE_IS_ENUM(x_enum_type));
    x_return_if_fail(info != NULL);
    x_return_if_fail(const_values != NULL);

    info->class_size = sizeof(XEnumClass);
    info->base_init = NULL;
    info->base_finalize = NULL;
    info->class_init = (XClassInitFunc)x_enum_class_init;
    info->class_finalize = NULL;
    info->class_data = const_values;
}

void x_flags_complete_type_info(XType x_flags_type, XTypeInfo *info, const XFlagsValue *const_values)
{
    x_return_if_fail(X_TYPE_IS_FLAGS(x_flags_type));
    x_return_if_fail(info != NULL);
    x_return_if_fail(const_values != NULL);

    info->class_size = sizeof(XFlagsClass);
    info->base_init = NULL;
    info->base_finalize = NULL;
    info->class_init = (XClassInitFunc)x_flags_class_init;
    info->class_finalize = NULL;
    info->class_data = const_values;
}

static void x_enum_class_init(XEnumClass *classt, xpointer class_data)
{
    x_return_if_fail(X_IS_ENUM_CLASS(classt));

    classt->minimum = 0;
    classt->maximum = 0;
    classt->n_values = 0;
    classt->values = (XEnumValue *)class_data;

    if (classt->values) {
        XEnumValue *values;

        classt->minimum = classt->values->value;
        classt->maximum = classt->values->value;
        for (values = classt->values; values->value_name; values++) {
            classt->minimum = MIN(classt->minimum, values->value);
            classt->maximum = MAX(classt->maximum, values->value);
            classt->n_values++;
        }
    }
}

static void x_flags_class_init(XFlagsClass *classt, xpointer class_data)
{
    x_return_if_fail(X_IS_FLAGS_CLASS(classt));

    classt->mask = 0;
    classt->n_values = 0;
    classt->values = (XFlagsValue *)class_data;

    if (classt->values) {
        XFlagsValue *values;

        for (values = classt->values; values->value_name; values++) {
            classt->mask |= values->value;
            classt->n_values++;
        }
    }
}

XEnumValue *x_enum_get_value_by_name(XEnumClass *enum_class, const xchar *name)
{
    x_return_val_if_fail(X_IS_ENUM_CLASS(enum_class), NULL);
    x_return_val_if_fail(name != NULL, NULL);

    if (enum_class->n_values) {
        XEnumValue *enum_value;

        for (enum_value = enum_class->values; enum_value->value_name; enum_value++) {
            if (strcmp(name, enum_value->value_name) == 0) {
                return enum_value;
            }
        }
    }

    return NULL;
}

XFlagsValue *x_flags_get_value_by_name(XFlagsClass *flags_class, const xchar *name)
{
    x_return_val_if_fail(X_IS_FLAGS_CLASS(flags_class), NULL);
    x_return_val_if_fail(name != NULL, NULL);

    if (flags_class->n_values) {
        XFlagsValue *flags_value;

        for (flags_value = flags_class->values; flags_value->value_name; flags_value++) {
            if (strcmp(name, flags_value->value_name) == 0) {
                return flags_value;
            }
        }
    }

    return NULL;
}

XEnumValue *x_enum_get_value_by_nick(XEnumClass *enum_class, const xchar *nick)
{
    x_return_val_if_fail(X_IS_ENUM_CLASS(enum_class), NULL);
    x_return_val_if_fail(nick != NULL, NULL);

    if (enum_class->n_values) {
        XEnumValue *enum_value;

        for (enum_value = enum_class->values; enum_value->value_name; enum_value++) {
            if (enum_value->value_nick && strcmp(nick, enum_value->value_nick) == 0) {
                return enum_value;
            }
        }
    }

    return NULL;
}

XFlagsValue *x_flags_get_value_by_nick(XFlagsClass *flags_class, const xchar *nick)
{
    x_return_val_if_fail(X_IS_FLAGS_CLASS(flags_class), NULL);
    x_return_val_if_fail(nick != NULL, NULL);

    if (flags_class->n_values) {
        XFlagsValue *flags_value;

        for (flags_value = flags_class->values; flags_value->value_nick; flags_value++) {
            if (flags_value->value_nick && strcmp(nick, flags_value->value_nick) == 0) {
                return flags_value;
            }
        }
    }

    return NULL;
}

XEnumValue *x_enum_get_value(XEnumClass *enum_class, xint value)
{
    x_return_val_if_fail(X_IS_ENUM_CLASS(enum_class), NULL);

    if (enum_class->n_values) {
        XEnumValue *enum_value;
        
        for (enum_value = enum_class->values; enum_value->value_name; enum_value++) {
            if (enum_value->value == value) {
                return enum_value;
            }
        }
    }

    return NULL;
}

XFlagsValue *x_flags_get_first_value(XFlagsClass *flags_class, xuint value)
{
    x_return_val_if_fail(X_IS_FLAGS_CLASS(flags_class), NULL);

    if (flags_class->n_values) {
        XFlagsValue *flags_value;

        if (value == 0) {
            for (flags_value = flags_class->values; flags_value->value_name; flags_value++) {
                if (flags_value->value == 0) {
                    return flags_value;
                }
            }
        } else {
            for (flags_value = flags_class->values; flags_value->value_name; flags_value++) {
                if (flags_value->value != 0 && (flags_value->value & value) == flags_value->value) {
                    return flags_value;
                }
            }
        }
    }

    return NULL;
}

xchar *x_enum_to_string(XType x_enum_type, xint value)
{
    xchar *result;
    XEnumClass *enum_class;
    XEnumValue *enum_value;

    x_return_val_if_fail(X_TYPE_IS_ENUM(x_enum_type), NULL);

    enum_class = (XEnumClass *)x_type_class_ref(x_enum_type);

    if (enum_class == NULL) {
        return x_strdup_printf("%d", value);
    }

    enum_value = x_enum_get_value(enum_class, value);
    if (enum_value == NULL) {
        result = x_strdup_printf("%d", value);
    } else {
        result = x_strdup(enum_value->value_name);
    }

    x_type_class_unref(enum_class);
    return result;
}

static xchar *x_flags_get_value_string(XFlagsClass *flags_class, xuint value)
{
    XString *str;
    XFlagsValue *flags_value;

    x_return_val_if_fail(X_IS_FLAGS_CLASS(flags_class), NULL);

    str = x_string_new (NULL);

    while ((str->len == 0 || value != 0) && (flags_value = x_flags_get_first_value(flags_class, value)) != NULL) {
        if (str->len > 0) {
            x_string_append(str, " | ");
        }

        x_string_append(str, flags_value->value_name);
        value &= ~flags_value->value;
    }

    if (value != 0 || str->len == 0) {
        if (str->len > 0) {
            x_string_append(str, " | ");
        }

        x_string_append_printf(str, "0x%x", value);
    }

    return x_string_free(str, FALSE);
}

xchar *x_flags_to_string(XType flags_type, xuint value)
{
    xchar *result;
    XFlagsClass *flags_class;

    x_return_val_if_fail(X_TYPE_IS_FLAGS(flags_type), NULL);

    flags_class = (XFlagsClass *)x_type_class_ref(flags_type);
    if (flags_class == NULL) {
        return NULL;
    }

    result = x_flags_get_value_string(flags_class, value);

    x_type_class_unref(flags_class);
    return result;
}

void x_value_set_enum(XValue *value, xint v_enum)
{
    x_return_if_fail(X_VALUE_HOLDS_ENUM(value));
    value->data[0].v_long = v_enum;
}

xint x_value_get_enum(const XValue *value)
{
    x_return_val_if_fail(X_VALUE_HOLDS_ENUM(value), 0);
    return value->data[0].v_long;
}

void x_value_set_flags(XValue *value, xuint v_flags)
{
    x_return_if_fail(X_VALUE_HOLDS_FLAGS(value));
    value->data[0].v_ulong = v_flags;
}

xuint x_value_get_flags(const XValue *value)
{
    x_return_val_if_fail(X_VALUE_HOLDS_FLAGS(value), 0);
    return value->data[0].v_ulong;
}
