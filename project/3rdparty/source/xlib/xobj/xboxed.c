#include <string.h>

#include <xlib/xlib/config.h>
#ifndef XLIB_DISABLE_DEPRECATION_WARNINGS
#define XLIB_DISABLE_DEPRECATION_WARNINGS
#endif

#include <xlib/xobj/xvalue.h>
#include <xlib/xobj/xboxed.h>
#include <xlib/xobj/xclosure.h>
#include <xlib/xobj/xvaluearray.h>
#include <xlib/xobj/xtype-private.h>
#include <xlib/xobj/xvaluecollector.h>

static inline void value_meminit(XValue *value, XType value_type)
{
    value->x_type = value_type;
    memset(value->data, 0, sizeof(value->data));
}

static XValue *value_copy(XValue *src_value)
{
    XValue *dest_value = x_new0(XValue, 1);

    if (X_VALUE_TYPE(src_value)) {
        x_value_init(dest_value, X_VALUE_TYPE(src_value));
        x_value_copy(src_value, dest_value);
    }

    return dest_value;
}

static void value_free(XValue *value)
{
    if (X_VALUE_TYPE(value)) {
        x_value_unset(value);
    }

    x_free(value);
}

static XPollFD *pollfd_copy(XPollFD *src)
{
    XPollFD *dest = x_new0(XPollFD, 1);

    memcpy(dest, src, sizeof(XPollFD));
    return dest;
}

void _x_boxed_type_init(void)
{
    const XTypeInfo info = {
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

    type = x_type_register_fundamental(X_TYPE_BOXED, x_intern_static_string("GBoxed"), &info, &finfo, (XTypeFlags)(X_TYPE_FLAG_ABSTRACT | X_TYPE_FLAG_VALUE_ABSTRACT));
    x_assert(type == X_TYPE_BOXED);
}

static XString *xstring_copy(XString *src_gstring)
{
    return x_string_new_len(src_gstring->str, src_gstring->len);
}

static void xstring_free(XString *gstring)
{
    x_string_free(gstring, TRUE);
}

X_DEFINE_BOXED_TYPE(XClosure, x_closure, x_closure_ref, x_closure_unref)
X_DEFINE_BOXED_TYPE(XValue, x_value, value_copy, value_free)
X_DEFINE_BOXED_TYPE(XValueArray, x_value_array, x_value_array_copy, x_value_array_free)
X_DEFINE_BOXED_TYPE(XDate, x_date, x_date_copy, x_date_free)

X_DEFINE_BOXED_TYPE(XString, x_gstring, xstring_copy, xstring_free)
X_DEFINE_BOXED_TYPE(XHashTable, x_hash_table, x_hash_table_ref, x_hash_table_unref)
X_DEFINE_BOXED_TYPE(XArray, x_array, x_array_ref, x_array_unref)
X_DEFINE_BOXED_TYPE(XPtrArray, x_ptr_array, x_ptr_array_ref, x_ptr_array_unref)
X_DEFINE_BOXED_TYPE(XByteArray, x_byte_array, x_byte_array_ref, x_byte_array_unref)
X_DEFINE_BOXED_TYPE(XBytes, x_bytes, x_bytes_ref, x_bytes_unref)
X_DEFINE_BOXED_TYPE(XTree, x_tree, x_tree_ref, x_tree_unref)

X_DEFINE_BOXED_TYPE(XRegex, x_regex, x_regex_ref, x_regex_unref)
X_DEFINE_BOXED_TYPE(XMatchInfo, x_match_info, x_match_info_ref, x_match_info_unref)

#define x_variant_type_get_type x_variant_type_get_gtype
X_DEFINE_BOXED_TYPE(XVariantType, x_variant_type, x_variant_type_copy, x_variant_type_free)
#undef x_variant_type_get_type

X_DEFINE_BOXED_TYPE(XVariantBuilder, x_variant_builder, x_variant_builder_ref, x_variant_builder_unref)
X_DEFINE_BOXED_TYPE(XVariantDict, x_variant_dict, x_variant_dict_ref, x_variant_dict_unref)

X_DEFINE_BOXED_TYPE(XError, x_error, x_error_copy, x_error_free)

X_DEFINE_BOXED_TYPE(XDateTime, x_date_time, x_date_time_ref, x_date_time_unref)
X_DEFINE_BOXED_TYPE(XTimeZone, x_time_zone, x_time_zone_ref, x_time_zone_unref)
X_DEFINE_BOXED_TYPE(XKeyFile, x_key_file, x_key_file_ref, x_key_file_unref)
X_DEFINE_BOXED_TYPE(XMappedFile, x_mapped_file, x_mapped_file_ref, x_mapped_file_unref)
X_DEFINE_BOXED_TYPE(XBookmarkFile, x_bookmark_file, x_bookmark_file_copy, x_bookmark_file_free)

X_DEFINE_BOXED_TYPE(XMainLoop, x_main_loop, x_main_loop_ref, x_main_loop_unref)
X_DEFINE_BOXED_TYPE(XMainContext, x_main_context, x_main_context_ref, x_main_context_unref)
X_DEFINE_BOXED_TYPE(XSource, x_source, x_source_ref, x_source_unref)
X_DEFINE_BOXED_TYPE(XPollFD, x_pollfd, pollfd_copy, x_free)
X_DEFINE_BOXED_TYPE(XMarkupParseContext, x_markup_parse_context, x_markup_parse_context_ref, x_markup_parse_context_unref)

X_DEFINE_BOXED_TYPE(XThread, x_thread, x_thread_ref, x_thread_unref)
X_DEFINE_BOXED_TYPE(XChecksum, x_checksum, x_checksum_copy, x_checksum_free)
X_DEFINE_BOXED_TYPE(XUri, x_uri, x_uri_ref, x_uri_unref)

X_DEFINE_BOXED_TYPE(XOptionGroup, x_option_group, x_option_group_ref, x_option_group_unref)
X_DEFINE_BOXED_TYPE(XPatternSpec, x_pattern_spec, x_pattern_spec_copy, x_pattern_spec_free);

XType x_strv_get_type(void)
{
    static XType static_x_define_type_id = 0;

    if (x_once_init_enter_pointer(&static_x_define_type_id)) {
        XType x_define_type_id = x_boxed_type_register_static(x_intern_static_string("GStrv"), (XBoxedCopyFunc)x_strdupv, (XBoxedFreeFunc)x_strfreev);
        x_once_init_leave_pointer(&static_x_define_type_id, x_define_type_id);
    }

    return static_x_define_type_id;
}

XType x_variant_get_gtype(void)
{
    return X_TYPE_VARIANT;
}

static void boxed_proxy_value_init(XValue *value)
{
    value->data[0].v_pointer = NULL;
}

static void boxed_proxy_value_free(XValue *value)
{
    if (value->data[0].v_pointer && !(value->data[1].v_uint & X_VALUE_NOCOPY_CONTENTS)) {
        _x_type_boxed_free(X_VALUE_TYPE(value), value->data[0].v_pointer);
    }
}

static void boxed_proxy_value_copy(const XValue *src_value, XValue *dest_value)
{
    if (src_value->data[0].v_pointer) {
        dest_value->data[0].v_pointer = _x_type_boxed_copy(X_VALUE_TYPE(src_value), src_value->data[0].v_pointer);
    } else {
        dest_value->data[0].v_pointer = src_value->data[0].v_pointer;
    }
}

static xpointer boxed_proxy_value_peek_pointer(const XValue *value)
{
    return value->data[0].v_pointer;
}

static xchar *boxed_proxy_collect_value(XValue *value, xuint n_collect_values, XTypeCValue *collect_values, xuint collect_flags)
{
    if (!collect_values[0].v_pointer) {
        value->data[0].v_pointer = NULL;
    } else {
        if (collect_flags & X_VALUE_NOCOPY_CONTENTS) {
            value->data[0].v_pointer = collect_values[0].v_pointer;
            value->data[1].v_uint = X_VALUE_NOCOPY_CONTENTS;
        } else {
            value->data[0].v_pointer = _x_type_boxed_copy(X_VALUE_TYPE(value), collect_values[0].v_pointer);
        }
    }

    return NULL;
}

static xchar *boxed_proxy_lcopy_value(const XValue *value, xuint n_collect_values, XTypeCValue *collect_values, xuint collect_flags)
{
    xpointer *boxed_p = (xpointer *)collect_values[0].v_pointer;

    x_return_val_if_fail(boxed_p != NULL, x_strdup_printf("value location for '%s' passed as NULL", X_VALUE_TYPE_NAME(value)));

    if (!value->data[0].v_pointer) {
        *boxed_p = NULL;
    } else if (collect_flags & X_VALUE_NOCOPY_CONTENTS) {
        *boxed_p = value->data[0].v_pointer;
    } else {
        *boxed_p = _x_type_boxed_copy(X_VALUE_TYPE(value), value->data[0].v_pointer);
    }

    return NULL;
}

XType x_boxed_type_register_static(const xchar *name, XBoxedCopyFunc boxed_copy, XBoxedFreeFunc boxed_free)
{
    static const XTypeValueTable vtable = {
        boxed_proxy_value_init,
        boxed_proxy_value_free,
        boxed_proxy_value_copy,
        boxed_proxy_value_peek_pointer,
        "p",
        boxed_proxy_collect_value,
        "p",
        boxed_proxy_lcopy_value,
    };

    XTypeInfo type_info = {
        0,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        0,
        0,
        NULL,
        &vtable,
    };

    XType type;

    x_return_val_if_fail(name != NULL, 0);
    x_return_val_if_fail(boxed_copy != NULL, 0);
    x_return_val_if_fail(boxed_free != NULL, 0);
    x_return_val_if_fail(x_type_from_name(name) == 0, 0);

    type = x_type_register_static(X_TYPE_BOXED, name, &type_info, (XTypeFlags)0);
    if (type) {
        _x_type_boxed_init(type, boxed_copy, boxed_free);
    }

    return type;
}

xpointer x_boxed_copy(XType boxed_type, xconstpointer src_boxed)
{
    xpointer dest_boxed;
    XTypeValueTable *value_table;

    x_return_val_if_fail(X_TYPE_IS_BOXED(boxed_type), NULL);
    x_return_val_if_fail(X_TYPE_IS_ABSTRACT(boxed_type) == FALSE, NULL);
    x_return_val_if_fail(src_boxed != NULL, NULL);

    value_table = x_type_value_table_peek(boxed_type);
    x_assert(value_table != NULL);

    if (value_table->value_copy == boxed_proxy_value_copy) {
        dest_boxed = _x_type_boxed_copy(boxed_type, (xpointer)src_boxed);
    } else {
        XValue src_value, dest_value;

        value_meminit(&src_value, boxed_type);
        src_value.data[0].v_pointer = (xpointer)src_boxed;
        src_value.data[1].v_uint = X_VALUE_NOCOPY_CONTENTS;

        value_meminit(&dest_value, boxed_type);
        value_table->value_copy (&src_value, &dest_value);

        if (dest_value.data[1].v_ulong) {
            x_warning("the copy_value() implementation of type '%s' seems to make use of reserved XValue fields", x_type_name(boxed_type));
        }

        dest_boxed = dest_value.data[0].v_pointer;
    }

    return dest_boxed;
}

void x_boxed_free(XType boxed_type, xpointer boxed)
{
    XTypeValueTable *value_table;

    x_return_if_fail(X_TYPE_IS_BOXED(boxed_type));
    x_return_if_fail(X_TYPE_IS_ABSTRACT(boxed_type) == FALSE);
    x_return_if_fail(boxed != NULL);

    value_table = x_type_value_table_peek(boxed_type);
    x_assert(value_table != NULL);

    if (value_table->value_free == boxed_proxy_value_free) {
        _x_type_boxed_free(boxed_type, boxed);
    } else {
        XValue value;

        value_meminit(&value, boxed_type);
        value.data[0].v_pointer = boxed;
        value_table->value_free(&value);
    }
}

xpointer x_value_get_boxed(const XValue *value)
{
    x_return_val_if_fail(X_VALUE_HOLDS_BOXED(value), NULL);
    x_return_val_if_fail(X_TYPE_IS_VALUE(X_VALUE_TYPE(value)), NULL);

    return value->data[0].v_pointer;
}

xpointer x_value_dup_boxed(const XValue *value)
{
    x_return_val_if_fail(X_VALUE_HOLDS_BOXED(value), NULL);
    x_return_val_if_fail(X_TYPE_IS_VALUE(X_VALUE_TYPE(value)), NULL);

    return value->data[0].v_pointer ? x_boxed_copy(X_VALUE_TYPE(value), value->data[0].v_pointer) : NULL;
}

static inline void value_set_boxed_internal(XValue *value, xconstpointer boxed, xboolean need_copy, xboolean need_free)
{
    if (!boxed) {
        x_value_reset(value);
        return;
    }

    if (value->data[0].v_pointer && !(value->data[1].v_uint & X_VALUE_NOCOPY_CONTENTS)) {
        x_boxed_free(X_VALUE_TYPE(value), value->data[0].v_pointer);
    }

    value->data[1].v_uint = need_free ? 0 : X_VALUE_NOCOPY_CONTENTS;
    value->data[0].v_pointer = need_copy ? x_boxed_copy(X_VALUE_TYPE(value), boxed) : (xpointer)boxed;
}

void x_value_set_boxed(XValue *value, xconstpointer boxed)
{
    x_return_if_fail(X_VALUE_HOLDS_BOXED(value));
    x_return_if_fail(X_TYPE_IS_VALUE(X_VALUE_TYPE(value)));

    value_set_boxed_internal(value, boxed, TRUE, TRUE);
}

void x_value_set_static_boxed(XValue *value, xconstpointer boxed)
{
    x_return_if_fail(X_VALUE_HOLDS_BOXED(value));
    x_return_if_fail(X_TYPE_IS_VALUE(X_VALUE_TYPE(value)));

    value_set_boxed_internal(value, boxed, FALSE, FALSE);
}

void x_value_set_boxed_take_ownership(XValue *value, xconstpointer boxed)
{
    x_value_take_boxed(value, boxed);
}

void x_value_take_boxed(XValue *value, xconstpointer boxed)
{
    x_return_if_fail(X_VALUE_HOLDS_BOXED(value));
    x_return_if_fail(X_TYPE_IS_VALUE(X_VALUE_TYPE(value)));

    value_set_boxed_internal(value, boxed, FALSE, TRUE);
}
