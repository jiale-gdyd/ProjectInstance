#ifndef __X_VARIANT_H__
#define __X_VARIANT_H__

#include "xvarianttype.h"
#include "xstring.h"
#include "xbytes.h"

X_BEGIN_DECLS

typedef struct _XVariant XVariant;

typedef enum {
    X_VARIANT_CLASS_BOOLEAN     = 'b',
    X_VARIANT_CLASS_BYTE        = 'y',
    X_VARIANT_CLASS_INT16       = 'n',
    X_VARIANT_CLASS_UINT16      = 'q',
    X_VARIANT_CLASS_INT32       = 'i',
    X_VARIANT_CLASS_UINT32      = 'u',
    X_VARIANT_CLASS_INT64       = 'x',
    X_VARIANT_CLASS_UINT64      = 't',
    X_VARIANT_CLASS_HANDLE      = 'h',
    X_VARIANT_CLASS_DOUBLE      = 'd',
    X_VARIANT_CLASS_STRING      = 's',
    X_VARIANT_CLASS_OBJECT_PATH = 'o',
    X_VARIANT_CLASS_SIGNATURE   = 'g',
    X_VARIANT_CLASS_VARIANT     = 'v',
    X_VARIANT_CLASS_MAYBE       = 'm',
    X_VARIANT_CLASS_ARRAY       = 'a',
    X_VARIANT_CLASS_TUPLE       = '(',
    X_VARIANT_CLASS_DICT_ENTRY  = '{'
} XVariantClass;

XLIB_AVAILABLE_IN_ALL
void x_variant_unref(XVariant *value);

XLIB_AVAILABLE_IN_ALL
XVariant *x_variant_ref(XVariant *value);

XLIB_AVAILABLE_IN_ALL
XVariant *x_variant_ref_sink(XVariant *value);

XLIB_AVAILABLE_IN_ALL
xboolean x_variant_is_floating(XVariant *value);

XLIB_AVAILABLE_IN_ALL
XVariant *x_variant_take_ref(XVariant *value);

XLIB_AVAILABLE_IN_ALL
const XVariantType *x_variant_get_type(XVariant *value);

XLIB_AVAILABLE_IN_ALL
const xchar *x_variant_get_type_string(XVariant *value);

XLIB_AVAILABLE_IN_ALL
xboolean x_variant_is_of_type(XVariant *value, const XVariantType *type);

XLIB_AVAILABLE_IN_ALL
xboolean x_variant_is_container(XVariant *value);

XLIB_AVAILABLE_IN_ALL
XVariantClass x_variant_classify(XVariant *value);

XLIB_AVAILABLE_IN_ALL
XVariant *x_variant_new_boolean(xboolean value);

XLIB_AVAILABLE_IN_ALL
XVariant *x_variant_new_byte(xuint8 value);

XLIB_AVAILABLE_IN_ALL
XVariant *x_variant_new_int16(xint16 value);

XLIB_AVAILABLE_IN_ALL
XVariant *x_variant_new_uint16(xuint16 value);

XLIB_AVAILABLE_IN_ALL
XVariant *x_variant_new_int32(xint32 value);

XLIB_AVAILABLE_IN_ALL
XVariant *x_variant_new_uint32(xuint32 value);

XLIB_AVAILABLE_IN_ALL
XVariant *x_variant_new_int64(xint64 value);

XLIB_AVAILABLE_IN_ALL
XVariant *x_variant_new_uint64(xuint64 value);

XLIB_AVAILABLE_IN_ALL
XVariant *x_variant_new_handle(xint32 value);

XLIB_AVAILABLE_IN_ALL
XVariant *x_variant_new_double(xdouble value);

XLIB_AVAILABLE_IN_ALL
XVariant *x_variant_new_string(const xchar *string);

XLIB_AVAILABLE_IN_2_38
XVariant *x_variant_new_take_string(xchar *string);

XLIB_AVAILABLE_IN_2_38
XVariant *x_variant_new_printf(const xchar *format_string, ...) X_GNUC_PRINTF (1, 2);

XLIB_AVAILABLE_IN_ALL
XVariant *x_variant_new_object_path(const xchar *object_path);

XLIB_AVAILABLE_IN_ALL
xboolean x_variant_is_object_path(const xchar *string);

XLIB_AVAILABLE_IN_ALL
XVariant *x_variant_new_signature(const xchar *signature);

XLIB_AVAILABLE_IN_ALL
xboolean x_variant_is_signature(const xchar *string);

XLIB_AVAILABLE_IN_ALL
XVariant *x_variant_new_variant(XVariant *value);

XLIB_AVAILABLE_IN_ALL
XVariant *x_variant_new_strv(const xchar *const *strv, xssize length);

XLIB_AVAILABLE_IN_2_30
XVariant *x_variant_new_objv(const xchar *const *strv, xssize length);

XLIB_AVAILABLE_IN_ALL
XVariant *x_variant_new_bytestring(const xchar *string);

XLIB_AVAILABLE_IN_ALL
XVariant *x_variant_new_bytestring_array(const xchar *const *strv, xssize length);

XLIB_AVAILABLE_IN_ALL
XVariant *x_variant_new_fixed_array(const XVariantType *element_type, xconstpointer elements, xsize n_elements, xsize element_size);

XLIB_AVAILABLE_IN_ALL
xboolean x_variant_get_boolean(XVariant *value);

XLIB_AVAILABLE_IN_ALL
xuint8 x_variant_get_byte(XVariant *value);

XLIB_AVAILABLE_IN_ALL
xint16 x_variant_get_int16(XVariant *value);

XLIB_AVAILABLE_IN_ALL
xuint16 x_variant_get_uint16(XVariant *value);

XLIB_AVAILABLE_IN_ALL
xint32 x_variant_get_int32(XVariant *value);

XLIB_AVAILABLE_IN_ALL
xuint32 x_variant_get_uint32(XVariant *value);

XLIB_AVAILABLE_IN_ALL
xint64 x_variant_get_int64(XVariant *value);

XLIB_AVAILABLE_IN_ALL
xuint64 x_variant_get_uint64(XVariant *value);

XLIB_AVAILABLE_IN_ALL
xint32 x_variant_get_handle(XVariant *value);

XLIB_AVAILABLE_IN_ALL
xdouble x_variant_get_double(XVariant *value);

XLIB_AVAILABLE_IN_ALL
XVariant *x_variant_get_variant(XVariant *value);

XLIB_AVAILABLE_IN_ALL
const xchar *x_variant_get_string(XVariant *value, xsize *length);

XLIB_AVAILABLE_IN_ALL
xchar *x_variant_dup_string(XVariant *value, xsize *length);

XLIB_AVAILABLE_IN_ALL
const xchar **x_variant_get_strv(XVariant *value, xsize *length);

XLIB_AVAILABLE_IN_ALL
xchar **x_variant_dup_strv(XVariant *value, xsize *length);

XLIB_AVAILABLE_IN_2_30
const xchar **x_variant_get_objv(XVariant *value, xsize *length);

XLIB_AVAILABLE_IN_ALL
xchar **x_variant_dup_objv(XVariant *value, xsize *length);

XLIB_AVAILABLE_IN_ALL
const xchar *x_variant_get_bytestring(XVariant *value);

XLIB_AVAILABLE_IN_ALL
xchar *x_variant_dup_bytestring(XVariant *value, xsize *length);

XLIB_AVAILABLE_IN_ALL
const xchar **x_variant_get_bytestring_array(XVariant *value, xsize *length);

XLIB_AVAILABLE_IN_ALL
xchar **x_variant_dup_bytestring_array(XVariant *value, xsize *length);

XLIB_AVAILABLE_IN_ALL
XVariant *x_variant_new_maybe(const XVariantType *child_type, XVariant *child);

XLIB_AVAILABLE_IN_ALL
XVariant *x_variant_new_array(const XVariantType *child_type, XVariant *const *children, xsize n_children);

XLIB_AVAILABLE_IN_ALL
XVariant *x_variant_new_tuple(XVariant *const *children, xsize n_children);

XLIB_AVAILABLE_IN_ALL
XVariant *x_variant_new_dict_entry(XVariant *key, XVariant *value);

XLIB_AVAILABLE_IN_ALL
XVariant *x_variant_get_maybe(XVariant *value);

XLIB_AVAILABLE_IN_ALL
xsize x_variant_n_children(XVariant *value);

XLIB_AVAILABLE_IN_ALL
void x_variant_get_child(XVariant *value, xsize index_, const xchar *format_string, ...);

XLIB_AVAILABLE_IN_ALL
XVariant *x_variant_get_child_value(XVariant *value, xsize index_);

XLIB_AVAILABLE_IN_ALL
xboolean x_variant_lookup(XVariant *dictionary, const xchar *key, const xchar *format_string, ...);

XLIB_AVAILABLE_IN_ALL
XVariant *x_variant_lookup_value(XVariant *dictionary, const xchar *key, const XVariantType *expected_type);

XLIB_AVAILABLE_IN_ALL
xconstpointer x_variant_get_fixed_array(XVariant *value, xsize *n_elements, xsize element_size);

XLIB_AVAILABLE_IN_ALL
xsize x_variant_get_size(XVariant *value);

XLIB_AVAILABLE_IN_ALL
xconstpointer x_variant_get_data(XVariant *value);

XLIB_AVAILABLE_IN_2_36
XBytes *x_variant_get_data_as_bytes(XVariant *value);

XLIB_AVAILABLE_IN_ALL
void x_variant_store(XVariant *value, xpointer data);

XLIB_AVAILABLE_IN_ALL
xchar *x_variant_print(XVariant *value, xboolean type_annotate);

XLIB_AVAILABLE_IN_ALL
XString *x_variant_print_string(XVariant *value, XString *string, xboolean type_annotate);

XLIB_AVAILABLE_IN_ALL
xuint x_variant_hash(xconstpointer value);

XLIB_AVAILABLE_IN_ALL
xboolean x_variant_equal(xconstpointer one, xconstpointer two);

XLIB_AVAILABLE_IN_ALL
XVariant *x_variant_get_normal_form(XVariant *value);

XLIB_AVAILABLE_IN_ALL
xboolean x_variant_is_normal_form(XVariant *value);

XLIB_AVAILABLE_IN_ALL
XVariant *x_variant_byteswap(XVariant *value);

XLIB_AVAILABLE_IN_2_36
XVariant *x_variant_new_from_bytes(const XVariantType *type, XBytes *bytes, xboolean trusted);

XLIB_AVAILABLE_IN_ALL
XVariant *x_variant_new_from_data(const XVariantType *type, xconstpointer data, xsize size, xboolean trusted, XDestroyNotify notify, xpointer user_data);

typedef struct _XVariantIter XVariantIter;
struct _XVariantIter {
    xuintptr x[16];
};

XLIB_AVAILABLE_IN_ALL
XVariantIter *x_variant_iter_new(XVariant *value);

XLIB_AVAILABLE_IN_ALL
xsize x_variant_iter_init(XVariantIter *iter, XVariant *value);

XLIB_AVAILABLE_IN_ALL
XVariantIter *x_variant_iter_copy(XVariantIter *iter);

XLIB_AVAILABLE_IN_ALL
xsize x_variant_iter_n_children(XVariantIter *iter);

XLIB_AVAILABLE_IN_ALL
void x_variant_iter_free(XVariantIter *iter);

XLIB_AVAILABLE_IN_ALL
XVariant *x_variant_iter_next_value(XVariantIter *iter);

XLIB_AVAILABLE_IN_ALL
xboolean x_variant_iter_next(XVariantIter *iter, const xchar *format_string, ...);

XLIB_AVAILABLE_IN_ALL
xboolean x_variant_iter_loop(XVariantIter *iter, const xchar *format_string, ...);

typedef struct _XVariantBuilder XVariantBuilder;
struct _XVariantBuilder {
    union {
        struct {
            xsize              partial_magic;
            const XVariantType *type;
            xuintptr           y[14];
        } s;

        xuintptr               x[16];
    } u;
};

typedef enum {
    X_VARIANT_PARSE_ERROR_FAILED,
    X_VARIANT_PARSE_ERROR_BASIC_TYPE_EXPECTED,
    X_VARIANT_PARSE_ERROR_CANNOT_INFER_TYPE,
    X_VARIANT_PARSE_ERROR_DEFINITE_TYPE_EXPECTED,
    X_VARIANT_PARSE_ERROR_INPUT_NOT_AT_END,
    X_VARIANT_PARSE_ERROR_INVALID_CHARACTER,
    X_VARIANT_PARSE_ERROR_INVALID_FORMAT_STRING,
    X_VARIANT_PARSE_ERROR_INVALID_OBJECT_PATH,
    X_VARIANT_PARSE_ERROR_INVALID_SIGNATURE,
    X_VARIANT_PARSE_ERROR_INVALID_TYPE_STRING,
    X_VARIANT_PARSE_ERROR_NO_COMMON_TYPE,
    X_VARIANT_PARSE_ERROR_NUMBER_OUT_OF_RANGE,
    X_VARIANT_PARSE_ERROR_NUMBER_TOO_BIG,
    X_VARIANT_PARSE_ERROR_TYPE_ERROR,
    X_VARIANT_PARSE_ERROR_UNEXPECTED_TOKEN,
    X_VARIANT_PARSE_ERROR_UNKNOWN_KEYWORD,
    X_VARIANT_PARSE_ERROR_UNTERMINATED_STRING_CONSTANT,
    X_VARIANT_PARSE_ERROR_VALUE_EXPECTED,
    X_VARIANT_PARSE_ERROR_RECURSION
} XVariantParseError;

#define X_VARIANT_PARSE_ERROR               (x_variant_parse_error_quark())

XLIB_DEPRECATED_IN_2_38_FOR(x_variant_parse_error_quark)
XQuark x_variant_parser_get_error_quark(void);

XLIB_AVAILABLE_IN_ALL
XQuark x_variant_parse_error_quark(void);

#define X_VARIANT_BUILDER_INIT(variant_type)        \
    {                                               \
        {                                           \
            {                                       \
                2942751021u, variant_type, { 0, }   \
            }                                       \
        }                                           \
    }

XLIB_AVAILABLE_IN_ALL
XVariantBuilder *x_variant_builder_new(const XVariantType *type);

XLIB_AVAILABLE_IN_ALL
void x_variant_builder_unref(XVariantBuilder *builder);

XLIB_AVAILABLE_IN_ALL
XVariantBuilder *x_variant_builder_ref(XVariantBuilder *builder);

XLIB_AVAILABLE_IN_ALL
void x_variant_builder_init(XVariantBuilder *builder, const XVariantType *type);

XLIB_AVAILABLE_IN_2_84
void x_variant_builder_init_static(XVariantBuilder *builder, const XVariantType *type);

XLIB_AVAILABLE_IN_ALL
XVariant *x_variant_builder_end(XVariantBuilder *builder);

XLIB_AVAILABLE_IN_ALL
void x_variant_builder_clear(XVariantBuilder *builder);

XLIB_AVAILABLE_IN_ALL
void x_variant_builder_open(XVariantBuilder *builder, const XVariantType *type);

XLIB_AVAILABLE_IN_ALL
void x_variant_builder_close(XVariantBuilder *builder);

XLIB_AVAILABLE_IN_ALL
void x_variant_builder_add_value(XVariantBuilder *builder, XVariant *value);

XLIB_AVAILABLE_IN_ALL
void x_variant_builder_add(XVariantBuilder *builder, const xchar *format_string, ...);

XLIB_AVAILABLE_IN_ALL
void x_variant_builder_add_parsed(XVariantBuilder *builder, const xchar *format, ...);

XLIB_AVAILABLE_IN_ALL
XVariant *x_variant_new(const xchar *format_string, ...);

XLIB_AVAILABLE_IN_ALL
void x_variant_get(XVariant *value, const xchar *format_string, ...);

XLIB_AVAILABLE_IN_ALL
XVariant *x_variant_new_va(const xchar *format_string, const xchar **endptr, va_list *app);

XLIB_AVAILABLE_IN_ALL
void x_variant_get_va(XVariant *value, const xchar *format_string, const xchar **endptr, va_list *app);

XLIB_AVAILABLE_IN_2_34
xboolean x_variant_check_format_string(XVariant *value, const xchar *format_string, xboolean copy_only);

XLIB_AVAILABLE_IN_ALL
XVariant *x_variant_parse(const XVariantType *type, const xchar *text, const xchar *limit, const xchar **endptr, XError **error);

XLIB_AVAILABLE_IN_ALL
XVariant *x_variant_new_parsed(const xchar *format, ...);

XLIB_AVAILABLE_IN_ALL
XVariant *x_variant_new_parsed_va(const xchar *format, va_list *app);

XLIB_AVAILABLE_IN_2_40
xchar *x_variant_parse_error_print_context(XError *error, const xchar *source_str);

XLIB_AVAILABLE_IN_ALL
xint x_variant_compare(xconstpointer one, xconstpointer two);

typedef struct _XVariantDict XVariantDict;
struct _XVariantDict {
    union {
        struct {
            XVariant *asv;
            xsize    partial_magic;
            xuintptr y[14];
        } s;

        xuintptr     x[16];
    } u;
};

#define X_VARIANT_DICT_INIT(asv)            \
    {                                       \
        {                                   \
            {                               \
                asv, 3488698669u, { 0, }    \
            }                               \
        }                                   \
    }

XLIB_AVAILABLE_IN_2_40
XVariantDict *x_variant_dict_new(XVariant *from_asv);

XLIB_AVAILABLE_IN_2_40
void x_variant_dict_init(XVariantDict *dict, XVariant *from_asv);

XLIB_AVAILABLE_IN_2_40
xboolean x_variant_dict_lookup(XVariantDict *dict, const xchar *key, const xchar *format_string, ...);

XLIB_AVAILABLE_IN_2_40
XVariant *x_variant_dict_lookup_value(XVariantDict *dict, const xchar *key, const XVariantType *expected_type);

XLIB_AVAILABLE_IN_2_40
xboolean x_variant_dict_contains(XVariantDict *dict, const xchar *key);

XLIB_AVAILABLE_IN_2_40
void x_variant_dict_insert(XVariantDict *dict, const xchar *key, const xchar *format_string, ...);

XLIB_AVAILABLE_IN_2_40
void x_variant_dict_insert_value(XVariantDict *dict, const xchar *key, XVariant *value);

XLIB_AVAILABLE_IN_2_40
xboolean x_variant_dict_remove(XVariantDict *dict, const xchar *key);

XLIB_AVAILABLE_IN_2_40
void x_variant_dict_clear(XVariantDict *dict);

XLIB_AVAILABLE_IN_2_40
XVariant *x_variant_dict_end(XVariantDict *dict);

XLIB_AVAILABLE_IN_2_40
XVariantDict *x_variant_dict_ref(XVariantDict *dict);

XLIB_AVAILABLE_IN_2_40
void x_variant_dict_unref(XVariantDict *dict);

X_END_DECLS

#endif
