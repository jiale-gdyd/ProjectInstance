#include <string.h>
#include <xlib/xlib/config.h>
#include <xlib/xlib/xmem.h>
#include <xlib/xlib/xhash.h>
#include <xlib/xlib/xslice.h>
#include <xlib/xlib/xstrfuncs.h>
#include <xlib/xlib/xtestutils.h>
#include <xlib/xlib/xvariant-core.h>
#include <xlib/xlib/xvariant-internal.h>
#include <xlib/xlib/xvariant-serialiser.h>

#define TYPE_CHECK(value, TYPE, val)                                                                       \
    if X_UNLIKELY(!x_variant_is_of_type(value, TYPE)) {                                                    \
        x_return_if_fail_warning(X_LOG_DOMAIN, X_STRFUNC, "x_variant_is_of_type(" #value ", " #TYPE ")");  \
        return val;                                                                                        \
    }

static XVariant *x_variant_new_from_trusted(const XVariantType *type, xconstpointer data, xsize size)
{

    XBytes *bytes;
    XVariant *value;

    bytes = x_bytes_new(data, size);
    value = x_variant_new_from_bytes(type, bytes, TRUE);
    x_bytes_unref(bytes);

    return value;
}

XVariant *x_variant_new_boolean(xboolean value)
{
    xuchar v = value;
    return x_variant_new_from_trusted(X_VARIANT_TYPE_BOOLEAN, &v, 1);
}

xboolean x_variant_get_boolean(XVariant *value)
{
    const xuchar *data;

    TYPE_CHECK(value, X_VARIANT_TYPE_BOOLEAN, FALSE);

    data = (const xuchar *)x_variant_get_data(value);
    return data != NULL ? *data != 0 : FALSE;
}

#define NUMERIC_TYPE(TYPE, type, ctype)                                                 \
    XVariant *x_variant_new_##type(ctype value) {                                       \
        return x_variant_new_from_trusted(X_VARIANT_TYPE_##TYPE, &value, sizeof value); \
    }                                                                                   \
    ctype x_variant_get_##type(XVariant *value) {                                       \
        const ctype *data;                                                              \
        TYPE_CHECK(value, X_VARIANT_TYPE_ ## TYPE, 0);                                  \
        data = (const ctype *)x_variant_get_data(value);                                \
        return data != NULL ? *data : 0;                                                \
    }

NUMERIC_TYPE(BYTE, byte, xuint8)

NUMERIC_TYPE(INT16, int16, xint16)

NUMERIC_TYPE(UINT16, uint16, xuint16)

NUMERIC_TYPE(INT32, int32, xint32)

NUMERIC_TYPE(UINT32, uint32, xuint32)

NUMERIC_TYPE(INT64, int64, xint64)

NUMERIC_TYPE(UINT64, uint64, xuint64)

NUMERIC_TYPE(HANDLE, handle, xint32)

NUMERIC_TYPE(DOUBLE, double, xdouble)

XVariant *x_variant_new_maybe(const XVariantType *child_type, XVariant *child)
{
    XVariant *value;
    XVariantType *maybe_type;

    x_return_val_if_fail(child_type == NULL || x_variant_type_is_definite(child_type), 0);
    x_return_val_if_fail(child_type != NULL || child != NULL, NULL);
    x_return_val_if_fail(child_type == NULL || child == NULL || x_variant_is_of_type(child, child_type), NULL);

    if(child_type == NULL) {
        child_type = x_variant_get_type(child);
    }

    maybe_type = x_variant_type_new_maybe(child_type);
    if(child != NULL) {
        xboolean trusted;
        XVariant **children;

        children = x_new(XVariant *, 1);
        children[0] = x_variant_ref_sink(child);
        trusted = x_variant_is_trusted(children[0]);

        value = x_variant_new_from_children(maybe_type, children, 1, trusted);
    } else {
        value = x_variant_new_from_children(maybe_type, NULL, 0, TRUE);
    }
    x_variant_type_free(maybe_type);

    return value;
}

XVariant *x_variant_get_maybe(XVariant *value)
{
    TYPE_CHECK(value, X_VARIANT_TYPE_MAYBE, NULL);

    if(x_variant_n_children(value)) {
        return x_variant_get_child_value(value, 0);
    }

    return NULL;
}

XVariant *x_variant_new_variant(XVariant *value)
{
    x_return_val_if_fail(value != NULL, NULL);

    x_variant_ref_sink(value);
    return x_variant_new_from_children(X_VARIANT_TYPE_VARIANT, (XVariant **)x_memdup2(&value, sizeof value), 1, x_variant_is_trusted(value));
}

XVariant *x_variant_get_variant(XVariant *value)
{
    TYPE_CHECK(value, X_VARIANT_TYPE_VARIANT, NULL);
    return x_variant_get_child_value(value, 0);
}

XVariant *x_variant_new_array(const XVariantType *child_type, XVariant *const *children, xsize n_children)
{
    xsize i;
    XVariant *value;
    xboolean trusted;
    XVariant **my_children;
    XVariantType *array_type;

    x_return_val_if_fail(n_children > 0 || child_type != NULL, NULL);
    x_return_val_if_fail(n_children == 0 || children != NULL, NULL);
    x_return_val_if_fail(child_type == NULL || x_variant_type_is_definite(child_type), NULL);

    my_children = x_new(XVariant *, n_children);
    trusted = TRUE;

    if (child_type == NULL) {
        child_type = x_variant_get_type(children[0]);
    }
    array_type = x_variant_type_new_array(child_type);

    for(i = 0; i < n_children; i++)  {
        xboolean is_of_child_type = x_variant_is_of_type(children[i], child_type);
        if X_UNLIKELY(!is_of_child_type) {
            while (i != 0) {
                x_variant_unref(my_children[--i]);
            }

            x_free(my_children);
            x_return_val_if_fail(is_of_child_type, NULL);
        }

        my_children[i] = x_variant_ref_sink(children[i]);
        trusted &= x_variant_is_trusted(children[i]);
    }

    value = x_variant_new_from_children(array_type, my_children, n_children, trusted);
    x_variant_type_free(array_type);

    return value;
}

static XVariantType *x_variant_make_tuple_type(XVariant *const *children, xsize n_children)
{
    xsize i;
    XVariantType *type;
    const XVariantType **types;

    types = x_new(const XVariantType *, n_children);
    for (i = 0; i < n_children; i++) {
        types[i] = x_variant_get_type(children[i]);
    }

    type = x_variant_type_new_tuple(types, n_children);
    x_free(types);

    return type;
}

XVariant *x_variant_new_tuple(XVariant *const *children, xsize n_children)
{
    xsize i;
    XVariant *value;
    xboolean trusted;
    XVariant **my_children;
    XVariantType *tuple_type;

    x_return_val_if_fail(n_children == 0 || children != NULL, NULL);

    my_children = x_new(XVariant *, n_children);
    trusted = TRUE;

    for (i = 0; i < n_children; i++) {
        my_children[i] = x_variant_ref_sink(children[i]);
        trusted &= x_variant_is_trusted(children[i]);
    }

    tuple_type = x_variant_make_tuple_type(children, n_children);
    value = x_variant_new_from_children(tuple_type, my_children, n_children, trusted);
    x_variant_type_free(tuple_type);

    return value;
}

static XVariantType *x_variant_make_dict_entry_type(XVariant *key, XVariant *val)
{
    return x_variant_type_new_dict_entry(x_variant_get_type(key), x_variant_get_type(val));
}

XVariant *x_variant_new_dict_entry(XVariant *key, XVariant *value)
{
    xboolean trusted;
    XVariant **children;
    XVariantType *dict_type;

    x_return_val_if_fail(key != NULL && value != NULL, NULL);
    x_return_val_if_fail(!x_variant_is_container(key), NULL);

    children = x_new(XVariant *, 2);
    children[0] = x_variant_ref_sink(key);
    children[1] = x_variant_ref_sink(value);
    trusted = x_variant_is_trusted(key) && x_variant_is_trusted(value);

    dict_type = x_variant_make_dict_entry_type(key, value);
    value = x_variant_new_from_children(dict_type, children, 2, trusted);
    x_variant_type_free(dict_type);

    return value;
}

xboolean x_variant_lookup(XVariant *dictionary, const xchar *key, const xchar *format_string, ...)
{
    XVariant *value;
    XVariantType *type;

    x_variant_get_data(dictionary);

    type = x_variant_format_string_scan_type(format_string, NULL, NULL);
    value = x_variant_lookup_value(dictionary, key, type);
    x_variant_type_free(type);

    if (value) {
        va_list ap;

        va_start(ap, format_string);
        x_variant_get_va(value, format_string, NULL, &ap);
        x_variant_unref(value);
        va_end(ap);

        return TRUE;
    } else {
        return FALSE;
    }
}

XVariant *x_variant_lookup_value(XVariant *dictionary, const xchar *key, const XVariantType *expected_type)
{
    XVariant *entry;
    XVariant *value;
    XVariantIter iter;

    x_return_val_if_fail(x_variant_is_of_type(dictionary, X_VARIANT_TYPE("a{s*}")) || x_variant_is_of_type(dictionary, X_VARIANT_TYPE("a{o*}")), NULL);
    x_variant_iter_init(&iter, dictionary);

    while((entry = x_variant_iter_next_value(&iter))) {
        XVariant *entry_key;
        xboolean matches;

        entry_key = x_variant_get_child_value(entry, 0);
        matches = strcmp(x_variant_get_string(entry_key, NULL), key) == 0;
        x_variant_unref(entry_key);

        if (matches) {
            break;
        }

        x_variant_unref(entry);
    }

    if (entry == NULL) {
        return NULL;
    }

    value = x_variant_get_child_value(entry, 1);
    x_variant_unref(entry);

    if (x_variant_is_of_type(value, X_VARIANT_TYPE_VARIANT)) {
        XVariant *tmp;

        tmp = x_variant_get_variant(value);
        x_variant_unref(value);

        if (expected_type && !x_variant_is_of_type(tmp, expected_type)) {
            x_variant_unref(tmp);
            tmp = NULL;
        }

        value = tmp;
    }

    x_return_val_if_fail(expected_type == NULL || value == NULL || x_variant_is_of_type(value, expected_type), NULL);
    return value;
}

xconstpointer x_variant_get_fixed_array(XVariant *value, xsize *n_elements, xsize element_size)
{
    xsize size;
    xconstpointer data;
    xsize array_element_size;
    XVariantTypeInfo *array_info;

    TYPE_CHECK(value, X_VARIANT_TYPE_ARRAY, NULL);

    x_return_val_if_fail(n_elements != NULL, NULL);
    x_return_val_if_fail(element_size > 0, NULL);

    array_info = x_variant_get_type_info(value);
    x_variant_type_info_query_element(array_info, NULL, &array_element_size);

    x_return_val_if_fail(array_element_size, NULL);

    if X_UNLIKELY(array_element_size != element_size) {
        if (array_element_size) {
            x_critical("x_variant_get_fixed_array: assertion "
                        "'x_variant_array_has_fixed_size(value, element_size)' "
                        "failed: array size %" X_XSIZE_FORMAT " does not match "
                        "given element_size %" X_XSIZE_FORMAT ".",
                        array_element_size, element_size);
        } else {
            x_critical("x_variant_get_fixed_array: assertion "
                        "'x_variant_array_has_fixed_size(value, element_size)' "
                        "failed: array does not have fixed size.");
        }
    }

    data = x_variant_get_data(value);
    size = x_variant_get_size(value);

    if (size % element_size) {
        *n_elements = 0;
    } else {
        *n_elements = size / element_size;
    }

    if (*n_elements) {
        return data;
    }

    return NULL;
}

XVariant *x_variant_new_fixed_array(const XVariantType *element_type, xconstpointer elements, xsize n_elements, xsize element_size)
{
    xpointer data;
    XVariant *value;
    XVariantType *array_type;
    xsize array_element_size;
    XVariantTypeInfo *array_info;

    x_return_val_if_fail(x_variant_type_is_definite(element_type), NULL);
    x_return_val_if_fail(element_size > 0, NULL);

    array_type = x_variant_type_new_array(element_type);
    array_info = x_variant_type_info_get(array_type);
    x_variant_type_info_query_element(array_info, NULL, &array_element_size);
    if X_UNLIKELY(array_element_size != element_size) {
        if (array_element_size) {
            x_critical("x_variant_new_fixed_array: array size %" X_XSIZE_FORMAT
                        " does not match given element_size %" X_XSIZE_FORMAT ".",
                        array_element_size, element_size);
        } else {
            x_critical("x_variant_get_fixed_array: array does not have fixed size.");
        }

        return NULL;
    }

    data = x_memdup2(elements, n_elements * element_size);
    value = x_variant_new_from_data(array_type, data, n_elements * element_size, FALSE, x_free, data);

    x_variant_type_free(array_type);
    x_variant_type_info_unref(array_info);

    return value;
}

XVariant *x_variant_new_string(const xchar *string)
{
    x_return_val_if_fail(string != NULL, NULL);
    x_return_val_if_fail(x_utf8_validate(string, -1, NULL), NULL);

    return x_variant_new_from_trusted(X_VARIANT_TYPE_STRING, string, strlen(string) + 1);
}

XVariant *x_variant_new_take_string(xchar *string)
{
    XBytes *bytes;
    XVariant *value;

    x_return_val_if_fail(string != NULL, NULL);
    x_return_val_if_fail(x_utf8_validate(string, -1, NULL), NULL);

    bytes = x_bytes_new_take(string, strlen(string) + 1);
    value = x_variant_new_from_bytes(X_VARIANT_TYPE_STRING, bytes, TRUE);
    x_bytes_unref(bytes);

    return value;
}

XVariant *x_variant_new_printf(const xchar *format_string, ...)
{
    va_list ap;
    XBytes *bytes;
    xchar *string;
    XVariant *value;

    x_return_val_if_fail(format_string != NULL, NULL);

    va_start(ap, format_string);
    string = x_strdup_vprintf(format_string, ap);
    va_end(ap);

    bytes = x_bytes_new_take(string, strlen(string) + 1);
    value = x_variant_new_from_bytes(X_VARIANT_TYPE_STRING, bytes, TRUE);
    x_bytes_unref(bytes);

    return value;
}

XVariant *x_variant_new_object_path(const xchar *object_path)
{
    x_return_val_if_fail(x_variant_is_object_path(object_path), NULL);
    return x_variant_new_from_trusted(X_VARIANT_TYPE_OBJECT_PATH, object_path, strlen(object_path) + 1);
}

xboolean x_variant_is_object_path(const xchar *string)
{
    x_return_val_if_fail(string != NULL, FALSE);
    return x_variant_serialiser_is_object_path(string, strlen(string) + 1);
}

XVariant *x_variant_new_signature(const xchar *signature)
{
    x_return_val_if_fail(x_variant_is_signature(signature), NULL);
    return x_variant_new_from_trusted(X_VARIANT_TYPE_SIGNATURE, signature, strlen(signature) + 1);
}

xboolean x_variant_is_signature(const xchar *string)
{
    x_return_val_if_fail(string != NULL, FALSE);
    return x_variant_serialiser_is_signature(string, strlen(string) + 1);
}

const xchar *x_variant_get_string(XVariant *value, xsize *length)
{
    xsize size;
    xconstpointer data;

    x_return_val_if_fail(value != NULL, NULL);
    x_return_val_if_fail(x_variant_is_of_type(value, X_VARIANT_TYPE_STRING) || x_variant_is_of_type(value, X_VARIANT_TYPE_OBJECT_PATH) || x_variant_is_of_type(value, X_VARIANT_TYPE_SIGNATURE), NULL);

    data = x_variant_get_data(value);
    size = x_variant_get_size(value);

    if (!x_variant_is_trusted(value)) {
        switch (x_variant_classify(value)) {
            case X_VARIANT_CLASS_STRING:
                if (x_variant_serialiser_is_string(data, size)) {
                    break;
                }
                data = "";
                size = 1;
                break;

            case X_VARIANT_CLASS_OBJECT_PATH:
                if (x_variant_serialiser_is_object_path(data, size)) {
                    break;
                }
                data = "/";
                size = 2;
                break;

            case X_VARIANT_CLASS_SIGNATURE:
                if (x_variant_serialiser_is_signature(data, size)) {
                    break;
                }
                data = "";
                size = 1;
                break;

            default:
                x_assert_not_reached();
            }
        }

    if (length) {
        *length = size - 1;
    }

    return (const xchar *)data;
}

xchar *x_variant_dup_string(XVariant *value, xsize *length)
{
    return x_strdup(x_variant_get_string(value, length));
}

XVariant *x_variant_new_strv(const xchar *const *strv, xssize length)
{
    XVariant **strings;
    xsize i, length_unsigned;

    x_return_val_if_fail(length == 0 || strv != NULL, NULL);

    if(length < 0) {
        length = x_strv_length((xchar **)strv);
    }
    length_unsigned = length;

    strings = x_new(XVariant *, length_unsigned);
    for (i = 0; i < length_unsigned; i++) {
        strings[i] = x_variant_ref_sink(x_variant_new_string(strv[i]));
    }

    return x_variant_new_from_children(X_VARIANT_TYPE_STRING_ARRAY, strings, length_unsigned, TRUE);
}

const xchar **x_variant_get_strv(XVariant *value, xsize *length)
{
    xsize n;
    xsize i;
    const xchar **strv;

    TYPE_CHECK(value, X_VARIANT_TYPE_STRING_ARRAY, NULL);

    x_variant_get_data(value);
    n = x_variant_n_children(value);
    strv = x_new(const xchar *, n + 1);

    for (i = 0; i < n; i++) {
        XVariant *string;

        string = x_variant_get_child_value(value, i);
        strv[i] = x_variant_get_string(string, NULL);
        x_variant_unref(string);
    }
    strv[i] = NULL;

    if (length) {
        *length = n;
    }

    return strv;
}

xchar **x_variant_dup_strv(XVariant *value, xsize *length)
{
    xsize n;
    xsize i;
    xchar **strv;

    TYPE_CHECK(value, X_VARIANT_TYPE_STRING_ARRAY, NULL);

    n = x_variant_n_children(value);
    strv = x_new(xchar *, n + 1);

    for (i = 0; i < n; i++) {
        XVariant *string;

        string = x_variant_get_child_value(value, i);
        strv[i] = x_variant_dup_string(string, NULL);
        x_variant_unref(string);
    }
    strv[i] = NULL;

    if (length) {
        *length = n;
    }

    return strv;
}

XVariant *x_variant_new_objv(const xchar *const *strv, xssize length)
{
    XVariant **strings;
    xsize i, length_unsigned;

    x_return_val_if_fail(length == 0 || strv != NULL, NULL);

    if (length < 0) {
        length = x_strv_length((xchar **)strv);
    }
    length_unsigned = length;

    strings = x_new(XVariant *, length_unsigned);
    for (i = 0; i < length_unsigned; i++) {
        strings[i] = x_variant_ref_sink(x_variant_new_object_path(strv[i]));
    }

    return x_variant_new_from_children(X_VARIANT_TYPE_OBJECT_PATH_ARRAY, strings, length_unsigned, TRUE);
}

const xchar **x_variant_get_objv(XVariant *value, xsize *length)
{
    xsize n;
    xsize i;
    const xchar **strv;

    TYPE_CHECK(value, X_VARIANT_TYPE_OBJECT_PATH_ARRAY, NULL);

    x_variant_get_data(value);
    n = x_variant_n_children(value);
    strv = x_new(const xchar *, n + 1);

    for (i = 0; i < n; i++) {
        XVariant *string;

        string = x_variant_get_child_value(value, i);
        strv[i] = x_variant_get_string(string, NULL);
        x_variant_unref(string);
    }
    strv[i] = NULL;

    if (length) {
        *length = n;
    }

    return strv;
}

xchar **x_variant_dup_objv(XVariant *value, xsize *length)
{
    xsize n;
    xsize i;
    xchar **strv;

    TYPE_CHECK(value, X_VARIANT_TYPE_OBJECT_PATH_ARRAY, NULL);

    n = x_variant_n_children(value);
    strv = x_new(xchar *, n + 1);

    for (i = 0; i < n; i++) {
        XVariant *string;

        string = x_variant_get_child_value(value, i);
        strv[i] = x_variant_dup_string(string, NULL);
        x_variant_unref(string);
    }
    strv[i] = NULL;

    if (length) {
        *length = n;
    }

    return strv;
}

XVariant *x_variant_new_bytestring(const xchar *string)
{
    x_return_val_if_fail(string != NULL, NULL);
    return x_variant_new_from_trusted(X_VARIANT_TYPE_BYTESTRING, string, strlen(string) + 1);
}

const xchar *x_variant_get_bytestring(XVariant *value)
{
    xsize size;
    const xchar *string;

    TYPE_CHECK(value, X_VARIANT_TYPE_BYTESTRING, NULL);

    string = (const xchar *)x_variant_get_data(value);
    size = x_variant_get_size(value);

    if (size && string[size - 1] == '\0') {
        return string;
    } else {
        return "";
    }
}

xchar *x_variant_dup_bytestring(XVariant *value, xsize *length)
{
    xsize size;
    const xchar *original = x_variant_get_bytestring(value);

    if (original == NULL) {
        return NULL;
    }

    size = strlen(original);
    if (length) {
        *length = size;
    }

    return (xchar *)x_memdup2(original, size + 1);
}

XVariant *x_variant_new_bytestring_array(const xchar *const *strv, xssize length)
{
    XVariant **strings;
    xsize i, length_unsigned;

    x_return_val_if_fail(length == 0 || strv != NULL, NULL);

    if (length < 0) {
        length = x_strv_length((xchar **)strv);
    }
    length_unsigned = length;

    strings = x_new(XVariant *, length_unsigned);
    for (i = 0; i < length_unsigned; i++)
        strings[i] = x_variant_ref_sink(x_variant_new_bytestring(strv[i]));

    return x_variant_new_from_children(X_VARIANT_TYPE_BYTESTRING_ARRAY, strings, length_unsigned, TRUE);
}

const xchar **x_variant_get_bytestring_array(XVariant *value, xsize *length)
{
    xsize n;
    xsize i;
    const xchar **strv;

    TYPE_CHECK(value, X_VARIANT_TYPE_BYTESTRING_ARRAY, NULL);

    x_variant_get_data(value);
    n = x_variant_n_children(value);
    strv = x_new(const xchar *, n + 1);

    for (i = 0; i < n; i++) {
        XVariant *string;

        string = x_variant_get_child_value(value, i);
        strv[i] = x_variant_get_bytestring(string);
        x_variant_unref(string);
    }
    strv[i] = NULL;

    if (length) {
        *length = n;
    }

    return strv;
}

xchar **x_variant_dup_bytestring_array(XVariant *value, xsize *length)
{
    xsize n;
    xsize i;
    xchar **strv;

    TYPE_CHECK(value, X_VARIANT_TYPE_BYTESTRING_ARRAY, NULL);

    x_variant_get_data(value);
    n = x_variant_n_children(value);
    strv = x_new(xchar *, n + 1);

    for (i = 0; i < n; i++) {
        XVariant *string;

        string = x_variant_get_child_value(value, i);
        strv[i] = x_variant_dup_bytestring(string, NULL);
        x_variant_unref(string);
    }
    strv[i] = NULL;

    if (length) {
        *length = n;
    }

    return strv;
}

const XVariantType *x_variant_get_type(XVariant *value)
{
    XVariantTypeInfo *type_info;
    x_return_val_if_fail(value != NULL, NULL);

    type_info = x_variant_get_type_info(value);

    return(XVariantType *)x_variant_type_info_get_type_string(type_info);
}

const xchar *x_variant_get_type_string(XVariant *value)
{
    XVariantTypeInfo *type_info;
    x_return_val_if_fail(value != NULL, NULL);

    type_info = x_variant_get_type_info(value);
    return x_variant_type_info_get_type_string(type_info);
}

xboolean x_variant_is_of_type(XVariant *value, const XVariantType *type)
{
    return x_variant_type_is_subtype_of(x_variant_get_type(value), type);
}

xboolean x_variant_is_container(XVariant *value)
{
    return x_variant_type_is_container(x_variant_get_type(value));
}

XVariantClass x_variant_classify(XVariant *value)
{
    x_return_val_if_fail(value != NULL, (XVariantClass)0);
    return (XVariantClass)*x_variant_get_type_string(value);
}

XString *x_variant_print_string(XVariant *value, XString *string, xboolean type_annotate)
{
    const xchar *value_type_string = x_variant_get_type_string(value);

    if X_UNLIKELY(string == NULL) {
        string = x_string_new(NULL);
    }

    switch (value_type_string[0]) {
        case X_VARIANT_CLASS_MAYBE:
            if (type_annotate) {
                x_string_append_printf(string, "@%s ", value_type_string);
            }

            if (x_variant_n_children(value)) {
                xuint i, depth;
                XVariant *element = NULL;
                const XVariantType *base_type;

                for (depth = 0, base_type = x_variant_get_type(value); x_variant_type_is_maybe(base_type); depth++, base_type = x_variant_type_element(base_type));

                element = x_variant_ref(value);
                for (i = 0; i < depth && element != NULL; i++) {
                    XVariant *new_element = x_variant_n_children(element) ? x_variant_get_child_value(element, 0) : NULL;
                    x_variant_unref(element);
                    element = x_steal_pointer(&new_element);
                }

                if (element == NULL) {
                    for (; i > 1; i--) {
                        x_string_append(string, "just ");
                    }
                    x_string_append(string, "nothing");
                } else {
                    x_variant_print_string(element, string, FALSE);
                }

                x_clear_pointer(&element, x_variant_unref);
            } else {
                x_string_append(string, "nothing");
            }
            break;

        case X_VARIANT_CLASS_ARRAY:
            if (value_type_string[1] == 'y') {
                xsize i;
                xsize size;
                const xchar *str;

                str = (const xchar *)x_variant_get_data(value);
                size = x_variant_get_size(value);

                for (i = 0; i < size; i++) {
                    if (str[i] == '\0') {
                        break;
                    }
                }

                if (i == size - 1) {
                    xchar *escaped = x_strescape(str, NULL);

                    if (strchr(str, '\'')) {
                        x_string_append_printf(string, "b\"%s\"", escaped);
                    } else {
                        x_string_append_printf(string, "b'%s'", escaped);
                    }

                    x_free(escaped);
                    break;
                } else {

                }
            }

            if (value_type_string[1] == '{') {
                xsize n, i;
                const xchar *comma = "";

                if ((n = x_variant_n_children(value)) == 0) {
                    if (type_annotate) {
                        x_string_append_printf(string, "@%s ", value_type_string);
                    }

                    x_string_append(string, "{}");
                    break;
                }

                x_string_append_c(string, '{');
                for (i = 0; i < n; i++) {
                    XVariant *entry, *key, *val;

                    x_string_append(string, comma);
                    comma = ", ";

                    entry = x_variant_get_child_value(value, i);
                    key = x_variant_get_child_value(entry, 0);
                    val = x_variant_get_child_value(entry, 1);
                    x_variant_unref(entry);

                    x_variant_print_string(key, string, type_annotate);
                    x_variant_unref(key);
                    x_string_append(string, ": ");
                    x_variant_print_string(val, string, type_annotate);
                    x_variant_unref(val);
                    type_annotate = FALSE;
                }

                x_string_append_c(string, '}');
            } else {
                xsize n, i;
                const xchar *comma = "";

                if ((n = x_variant_n_children(value)) == 0) {
                    if (type_annotate) {
                        x_string_append_printf(string, "@%s ", value_type_string);
                    }

                    x_string_append(string, "[]");
                    break;
                }

                x_string_append_c(string, '[');
                for (i = 0; i < n; i++) {
                    XVariant *element;

                    x_string_append(string, comma);
                    comma = ", ";

                    element = x_variant_get_child_value(value, i);

                    x_variant_print_string(element, string, type_annotate);
                    x_variant_unref(element);
                    type_annotate = FALSE;
                }

                x_string_append_c(string, ']');
            }
            break;

        case X_VARIANT_CLASS_TUPLE: {
            xsize n, i;

            n = x_variant_n_children(value);

            x_string_append_c(string, '(');
            for (i = 0; i < n; i++) {
                XVariant *element;

                element = x_variant_get_child_value(value, i);
                x_variant_print_string(element, string, type_annotate);
                x_string_append(string, ", ");
                x_variant_unref(element);
            }

            x_string_truncate(string, string->len -(n > 0) -(n > 1));
            x_string_append_c(string, ')');
        }
        break;

        case X_VARIANT_CLASS_DICT_ENTRY: {
            XVariant *element;

            x_string_append_c(string, '{');

            element = x_variant_get_child_value(value, 0);
            x_variant_print_string(element, string, type_annotate);
            x_variant_unref(element);

            x_string_append(string, ", ");

            element = x_variant_get_child_value(value, 1);
            x_variant_print_string(element, string, type_annotate);
            x_variant_unref(element);

            x_string_append_c(string, '}');
        }
        break;

        case X_VARIANT_CLASS_VARIANT: {
            XVariant *child = x_variant_get_variant(value);

            x_string_append_c(string, '<');
            x_variant_print_string(child, string, TRUE);
            x_string_append_c(string, '>');

            x_variant_unref(child);
        }
        break;

        case X_VARIANT_CLASS_BOOLEAN:
            if (x_variant_get_boolean(value)) {
                x_string_append(string, "true");
            } else {
                x_string_append(string, "false");
            }
            break;

        case X_VARIANT_CLASS_STRING: {
            const xchar *str = x_variant_get_string(value, NULL);
            xunichar quote = strchr(str, '\'') ? '"' : '\'';

            x_string_append_c(string, quote);

            while (*str) {
                xunichar c = x_utf8_get_char(str);

                if (c == quote || c == '\\') {
                    x_string_append_c(string, '\\');
                }

                if (x_unichar_isprint(c)) {
                    x_string_append_unichar(string, c);
                } else {
                    x_string_append_c(string, '\\');
                    if (c < 0x10000) {
                        switch (c) {
                            case '\a':
                                x_string_append_c(string, 'a');
                                break;

                            case '\b':
                                x_string_append_c(string, 'b');
                                break;

                            case '\f':
                                x_string_append_c(string, 'f');
                                break;

                            case '\n':
                                x_string_append_c(string, 'n');
                                break;

                            case '\r':
                                x_string_append_c(string, 'r');
                                break;

                            case '\t':
                                x_string_append_c(string, 't');
                                break;

                            case '\v':
                                x_string_append_c(string, 'v');
                                break;

                            default:
                                x_string_append_printf(string, "u%04x", c);
                                break;
                        }
                    } else {
                        x_string_append_printf(string, "U%08x", c);
                    }
                }

                str = x_utf8_next_char(str);
            }

            x_string_append_c(string, quote);
        }
        break;

        case X_VARIANT_CLASS_BYTE:
            if (type_annotate) {
                x_string_append(string, "byte ");
            }
            x_string_append_printf(string, "0x%02x", x_variant_get_byte(value));
            break;

        case X_VARIANT_CLASS_INT16:
            if (type_annotate) {
                x_string_append(string, "int16 ");
            }
            x_string_append_printf(string, "%" X_XINT16_FORMAT, x_variant_get_int16(value));
            break;

        case X_VARIANT_CLASS_UINT16:
            if (type_annotate) {
                x_string_append(string, "uint16 ");
            }
            x_string_append_printf(string, "%" X_XUINT16_FORMAT, x_variant_get_uint16(value));
            break;

        case X_VARIANT_CLASS_INT32:
            x_string_append_printf(string, "%" X_XINT32_FORMAT, x_variant_get_int32(value));
            break;

        case X_VARIANT_CLASS_HANDLE:
            if (type_annotate) {
                x_string_append(string, "handle ");
            }
            x_string_append_printf(string, "%" X_XINT32_FORMAT, x_variant_get_handle(value));
            break;

        case X_VARIANT_CLASS_UINT32:
            if (type_annotate) {
                x_string_append(string, "uint32 ");
            }
            x_string_append_printf(string, "%" X_XUINT32_FORMAT, x_variant_get_uint32(value));
            break;

        case X_VARIANT_CLASS_INT64:
            if (type_annotate) {
                x_string_append(string, "int64 ");
            }
            x_string_append_printf(string, "%" X_XINT64_FORMAT, x_variant_get_int64(value));
            break;

        case X_VARIANT_CLASS_UINT64:
            if (type_annotate) {
                x_string_append(string, "uint64 ");
            }
            x_string_append_printf(string, "%" X_XUINT64_FORMAT, x_variant_get_uint64(value));
            break;

        case X_VARIANT_CLASS_DOUBLE: {
            xint i;
            xchar buffer[100];

            x_ascii_dtostr(buffer, sizeof buffer, x_variant_get_double(value));

            for (i = 0; buffer[i]; i++) {
                if (buffer[i] == '.' || buffer[i] == 'e' || buffer[i] == 'n' || buffer[i] == 'N') {
                    break;
                }
            }

            if (buffer[i] == '\0') {
                buffer[i++] = '.';
                buffer[i++] = '0';
                buffer[i++] = '\0';
            }

            x_string_append(string, buffer);
        }
        break;

        case X_VARIANT_CLASS_OBJECT_PATH:
            if (type_annotate) {
                x_string_append(string, "objectpath ");
            }
            x_string_append_printf(string, "\'%s\'", x_variant_get_string(value, NULL));
            break;

        case X_VARIANT_CLASS_SIGNATURE:
            if (type_annotate) {
                x_string_append(string, "signature ");
            }
            x_string_append_printf(string, "\'%s\'", x_variant_get_string(value, NULL));
            break;

        default:
            x_assert_not_reached();
    }

    return string;
}

xchar *x_variant_print(XVariant *value, xboolean type_annotate)
{
    return x_string_free(x_variant_print_string(value, NULL, type_annotate), FALSE);
}

xuint x_variant_hash(xconstpointer value_)
{
    XVariant *value =(XVariant *)value_;

    switch (x_variant_classify(value)) {
        case X_VARIANT_CLASS_STRING:
        case X_VARIANT_CLASS_OBJECT_PATH:
        case X_VARIANT_CLASS_SIGNATURE:
            return x_str_hash(x_variant_get_string(value, NULL));

        case X_VARIANT_CLASS_BOOLEAN:
            return x_variant_get_boolean(value);

        case X_VARIANT_CLASS_BYTE:
            return x_variant_get_byte(value);

        case X_VARIANT_CLASS_INT16:
        case X_VARIANT_CLASS_UINT16: {
            const xuint16 *ptr;

            ptr = (const xuint16 *)x_variant_get_data(value);
            if (ptr) {
                return *ptr;
            } else {
                return 0;
            }
        }

        case X_VARIANT_CLASS_INT32:
        case X_VARIANT_CLASS_UINT32:
        case X_VARIANT_CLASS_HANDLE: {
            const xuint *ptr;

            ptr = (const xuint *)x_variant_get_data(value);
            if (ptr) {
                return *ptr;
            } else {
                return 0;
            }
        }

        case X_VARIANT_CLASS_INT64:
        case X_VARIANT_CLASS_UINT64:
        case X_VARIANT_CLASS_DOUBLE: {
            const xuint *ptr;

            ptr = (const xuint *)x_variant_get_data(value);
            if (ptr) {
                return ptr[0] + ptr[1];
            } else {
                return 0;
            }
        }

        default:
            x_return_val_if_fail(!x_variant_is_container(value), 0);
            x_assert_not_reached();
    }
}

xboolean x_variant_equal(xconstpointer one, xconstpointer two)
{
    xboolean equal;

    x_return_val_if_fail(one != NULL && two != NULL, FALSE);

    if (x_variant_get_type_info((XVariant *)one) != x_variant_get_type_info((XVariant *)two)) {
        return FALSE;
    }

    if (x_variant_is_trusted((XVariant *)one) && x_variant_is_trusted((XVariant *)two)) {
        xsize size_one, size_two;
        xconstpointer data_one, data_two;

        size_one = x_variant_get_size((XVariant *)one);
        size_two = x_variant_get_size((XVariant *)two);

        if (size_one != size_two) {
            return FALSE;
        }

        data_one = x_variant_get_data((XVariant *) one);
        data_two = x_variant_get_data((XVariant *) two);

        if (size_one) {
            equal = memcmp(data_one, data_two, size_one) == 0;
        } else {
            equal = TRUE;
        }
    } else {
        xchar *strone, *strtwo;

        strone = x_variant_print((XVariant *)one, FALSE);
        strtwo = x_variant_print((XVariant *)two, FALSE);
        equal = strcmp(strone, strtwo) == 0;
        x_free(strone);
        x_free(strtwo);
    }

    return equal;
}

xint x_variant_compare(xconstpointer one, xconstpointer two)
{
    XVariant *a =(XVariant *)one;
    XVariant *b =(XVariant *)two;

    x_return_val_if_fail(x_variant_classify(a) == x_variant_classify(b), 0);

    switch (x_variant_classify(a)) {
        case X_VARIANT_CLASS_BOOLEAN:
            return x_variant_get_boolean(a) - x_variant_get_boolean(b);

        case X_VARIANT_CLASS_BYTE:
            return ((xint)x_variant_get_byte(a)) - ((xint)x_variant_get_byte(b));

        case X_VARIANT_CLASS_INT16:
            return ((xint)x_variant_get_int16(a)) - ((xint)x_variant_get_int16(b));

        case X_VARIANT_CLASS_UINT16:
            return((xint)x_variant_get_uint16(a)) - ((xint)x_variant_get_uint16(b));

        case X_VARIANT_CLASS_INT32: {
            xint32 a_val = x_variant_get_int32(a);
            xint32 b_val = x_variant_get_int32(b);

            return (a_val == b_val) ? 0 :(a_val > b_val) ? 1 : -1;
        }

        case X_VARIANT_CLASS_UINT32: {
            xuint32 a_val = x_variant_get_uint32(a);
            xuint32 b_val = x_variant_get_uint32(b);

            return (a_val == b_val) ? 0 :(a_val > b_val) ? 1 : -1;
        }

        case X_VARIANT_CLASS_INT64: {
            xint64 a_val = x_variant_get_int64(a);
            xint64 b_val = x_variant_get_int64(b);

            return (a_val == b_val) ? 0 :(a_val > b_val) ? 1 : -1;
        }

        case X_VARIANT_CLASS_UINT64: {
            xuint64 a_val = x_variant_get_uint64(a);
            xuint64 b_val = x_variant_get_uint64(b);

            return (a_val == b_val) ? 0 :(a_val > b_val) ? 1 : -1;
        }

        case X_VARIANT_CLASS_DOUBLE: {
            xdouble a_val = x_variant_get_double(a);
            xdouble b_val = x_variant_get_double(b);

            return (a_val == b_val) ? 0 :(a_val > b_val) ? 1 : -1;
        }

        case X_VARIANT_CLASS_STRING:
        case X_VARIANT_CLASS_OBJECT_PATH:
        case X_VARIANT_CLASS_SIGNATURE:
            return strcmp(x_variant_get_string(a, NULL), x_variant_get_string(b, NULL));

        default:
            x_return_val_if_fail(!x_variant_is_container(a), 0);
            x_assert_not_reached();
    }
}

struct stack_iter {
    XVariant    *value;
    xssize      n, i;
    const xchar *loop_format;
    xsize       padding[3];
    xsize       magic;
};

X_STATIC_ASSERT(sizeof(struct stack_iter) <= sizeof(XVariantIter));

struct heap_iter {
    struct stack_iter iter;
    XVariant          *value_ref;
    xsize             magic;
};

X_STATIC_ASSERT(sizeof(struct heap_iter) <= sizeof(XVariantIter));

#define GVSI(i)                     ((struct stack_iter *)(i))
#define GVHI(i)                     ((struct heap_iter *)(i))
#define GVSI_MAGIC                  ((xsize)3579507750u)
#define GVHI_MAGIC                  ((xsize)1450270775u)
#define is_valid_iter(i)            (i != NULL && GVSI(i)->magic == GVSI_MAGIC)
#define is_valid_heap_iter(i)       (is_valid_iter(i) && GVHI(i)->magic == GVHI_MAGIC)

XVariantIter *x_variant_iter_new(XVariant *value)
{
    XVariantIter *iter;

    iter =(XVariantIter *)x_slice_new(struct heap_iter);
    GVHI(iter)->value_ref = x_variant_ref(value);
    GVHI(iter)->magic = GVHI_MAGIC;

    x_variant_iter_init(iter, value);
    return iter;
}

xsize x_variant_iter_init(XVariantIter *iter, XVariant *value)
{
    GVSI(iter)->magic = GVSI_MAGIC;
    GVSI(iter)->value = value;
    GVSI(iter)->n = x_variant_n_children(value);
    GVSI(iter)->i = -1;
    GVSI(iter)->loop_format = NULL;

    return GVSI(iter)->n;
}

XVariantIter *x_variant_iter_copy(XVariantIter *iter)
{
    XVariantIter *copy;

    x_return_val_if_fail(is_valid_iter(iter), 0);

    copy = x_variant_iter_new(GVSI(iter)->value);
    GVSI(copy)->i = GVSI(iter)->i;

    return copy;
}

xsize x_variant_iter_n_children(XVariantIter *iter)
{
    x_return_val_if_fail(is_valid_iter(iter), 0);
    return GVSI(iter)->n;
}

void x_variant_iter_free(XVariantIter *iter)
{
    x_return_if_fail(is_valid_heap_iter(iter));
    x_variant_unref(GVHI(iter)->value_ref);
    GVHI(iter)->magic = 0;
    x_slice_free(struct heap_iter, GVHI(iter));
}

XVariant *x_variant_iter_next_value(XVariantIter *iter)
{
    x_return_val_if_fail(is_valid_iter(iter), FALSE);

    if X_UNLIKELY(GVSI(iter)->i >= GVSI(iter)->n) {
        x_critical("x_variant_iter_next_value: must not be called again after NULL has already been returned.");
        return NULL;
    }

    GVSI(iter)->i++;

    if (GVSI(iter)->i < GVSI(iter)->n) {
        return x_variant_get_child_value(GVSI(iter)->value, GVSI(iter)->i);
    }

    return NULL;
}

struct stack_builder {
    XVariantBuilder    *parent;
    XVariantType       *type;
    const XVariantType *expected_type;
    const XVariantType *prev_item_type;

    xsize              min_items;
    xsize              max_items;

    XVariant           **children;
    xsize              allocated_children;
    xsize              offset;

    xuint              uniform_item_types : 1;
    xuint              trusted : 1;
    xsize              magic;
};

X_STATIC_ASSERT(sizeof(struct stack_builder) <= sizeof(XVariantBuilder));

struct heap_builder {
    XVariantBuilder builder;
    xsize           magic;
    xint            ref_count;
};

#define GVSB(b)                             ((struct stack_builder *)(b))
#define GVHB(b)                             ((struct heap_builder *)(b))
#define GVSB_MAGIC                          ((xsize)1033660112u)
#define GVSB_MAGIC_PARTIAL                  ((xsize)2942751021u)
#define GVHB_MAGIC                          ((xsize)3087242682u)
#define is_valid_builder(b)                 (GVSB(b)->magic == GVSB_MAGIC)
#define is_valid_heap_builder(b)            (GVHB(b)->magic == GVHB_MAGIC)

X_STATIC_ASSERT(sizeof(XVariantBuilder) == sizeof(xuintptr[16]));

static xboolean ensure_valid_builder(XVariantBuilder *builder)
{
    if (builder == NULL) {
        return FALSE;
    } else if (is_valid_builder(builder)) {
        return TRUE;
    }

    if (builder->u.s.partial_magic == GVSB_MAGIC_PARTIAL) {
        static XVariantBuilder cleared_builder;
        if (memcmp(cleared_builder.u.s.y, builder->u.s.y, sizeof cleared_builder.u.s.y)) {
            return FALSE;
        }

        x_variant_builder_init(builder, builder->u.s.type);
    }

    return is_valid_builder(builder);
}

#define return_if_invalid_builder(b)                                    \
    X_STMT_START {                                                      \
        xboolean valid_builder X_GNUC_UNUSED = ensure_valid_builder(b); \
        x_return_if_fail(valid_builder);                                \
    } X_STMT_END

#define return_val_if_invalid_builder(b, val)                           \
    X_STMT_START {                                                      \
        xboolean valid_builder X_GNUC_UNUSED = ensure_valid_builder(b); \
        x_return_val_if_fail(valid_builder, val);                       \
    } X_STMT_END

XVariantBuilder *x_variant_builder_new(const XVariantType *type)
{
    XVariantBuilder *builder;

    builder = (XVariantBuilder *)x_slice_new(struct heap_builder);
    x_variant_builder_init(builder, type);
    GVHB(builder)->magic = GVHB_MAGIC;
    GVHB(builder)->ref_count = 1;

    return builder;
}

void x_variant_builder_unref(XVariantBuilder *builder)
{
    x_return_if_fail(is_valid_heap_builder(builder));
    if (--GVHB(builder)->ref_count) {
        return;
    }

    x_variant_builder_clear(builder);
    GVHB(builder)->magic = 0;

    x_slice_free(struct heap_builder, GVHB(builder));
}

XVariantBuilder *x_variant_builder_ref(XVariantBuilder *builder)
{
    x_return_val_if_fail(is_valid_heap_builder(builder), NULL);

    GVHB(builder)->ref_count++;
    return builder;
}

void x_variant_builder_clear(XVariantBuilder *builder)
{
    xsize i;

    if (GVSB(builder)->magic == 0) {
        return;
    }

    return_if_invalid_builder(builder);
    x_variant_type_free(GVSB(builder)->type);

    for (i = 0; i < GVSB(builder)->offset; i++) {
        x_variant_unref(GVSB(builder)->children[i]);
    }
    x_free(GVSB(builder)->children);

    if (GVSB(builder)->parent) {
        x_variant_builder_clear(GVSB(builder)->parent);
        x_slice_free(XVariantBuilder, GVSB(builder)->parent);
    }

    memset(builder, 0, sizeof(XVariantBuilder));
}

void x_variant_builder_init(XVariantBuilder *builder, const XVariantType *type)
{
    x_return_if_fail(type != NULL);
    x_return_if_fail(x_variant_type_is_container(type));

    memset(builder, 0, sizeof(XVariantBuilder));

    GVSB(builder)->type = x_variant_type_copy(type);
    GVSB(builder)->magic = GVSB_MAGIC;
    GVSB(builder)->trusted = TRUE;

    switch (*(const xchar *) type) {
        case X_VARIANT_CLASS_VARIANT:
            GVSB(builder)->uniform_item_types = TRUE;
            GVSB(builder)->allocated_children = 1;
            GVSB(builder)->expected_type = NULL;
            GVSB(builder)->min_items = 1;
            GVSB(builder)->max_items = 1;
            break;

        case X_VARIANT_CLASS_ARRAY:
            GVSB(builder)->uniform_item_types = TRUE;
            GVSB(builder)->allocated_children = 8;
            GVSB(builder)->expected_type = x_variant_type_element(GVSB(builder)->type);
            GVSB(builder)->min_items = 0;
            GVSB(builder)->max_items = -1;
            break;

        case X_VARIANT_CLASS_MAYBE:
            GVSB(builder)->uniform_item_types = TRUE;
            GVSB(builder)->allocated_children = 1;
            GVSB(builder)->expected_type = x_variant_type_element(GVSB(builder)->type);
            GVSB(builder)->min_items = 0;
            GVSB(builder)->max_items = 1;
            break;

        case X_VARIANT_CLASS_DICT_ENTRY:
            GVSB(builder)->uniform_item_types = FALSE;
            GVSB(builder)->allocated_children = 2;
            GVSB(builder)->expected_type = x_variant_type_key(GVSB(builder)->type);
            GVSB(builder)->min_items = 2;
            GVSB(builder)->max_items = 2;
            break;

        case 'r':
            GVSB(builder)->uniform_item_types = FALSE;
            GVSB(builder)->allocated_children = 8;
            GVSB(builder)->expected_type = NULL;
            GVSB(builder)->min_items = 0;
            GVSB(builder)->max_items = -1;
            break;

        case X_VARIANT_CLASS_TUPLE:
            GVSB(builder)->allocated_children = x_variant_type_n_items(type);
            GVSB(builder)->expected_type = x_variant_type_first(GVSB(builder)->type);
            GVSB(builder)->min_items = GVSB(builder)->allocated_children;
            GVSB(builder)->max_items = GVSB(builder)->allocated_children;
            GVSB(builder)->uniform_item_types = FALSE;
            break;

        default:
            x_assert_not_reached();
    }

#ifdef X_ANALYZER_ANALYZING
    GVSB(builder)->children = x_new0(XVariant *, GVSB(builder)->allocated_children);
#else
    GVSB(builder)->children = X_new(XVariant *, GVSB(builder)->allocated_children);
#endif
}

static void x_variant_builder_make_room(struct stack_builder *builder)
{
    if (builder->offset == builder->allocated_children) {
        builder->allocated_children *= 2;
        builder->children = x_renew(XVariant *, builder->children, builder->allocated_children);
    }
}

void x_variant_builder_add_value(XVariantBuilder *builder, XVariant *value)
{
    return_if_invalid_builder(builder);

    x_return_if_fail(GVSB(builder)->offset < GVSB(builder)->max_items);
    x_return_if_fail(!GVSB(builder)->expected_type || x_variant_is_of_type(value, GVSB(builder)->expected_type));
    x_return_if_fail(!GVSB(builder)->prev_item_type || x_variant_is_of_type(value, GVSB(builder)->prev_item_type));

    GVSB(builder)->trusted &= x_variant_is_trusted(value);

    if (!GVSB(builder)->uniform_item_types) {
        if (GVSB(builder)->expected_type) {
            GVSB(builder)->expected_type = x_variant_type_next(GVSB(builder)->expected_type);
        }

        if (GVSB(builder)->prev_item_type) {
            GVSB(builder)->prev_item_type = x_variant_type_next(GVSB(builder)->prev_item_type);
        }
    } else {
        GVSB(builder)->prev_item_type = x_variant_get_type(value);
    }

    x_variant_builder_make_room(GVSB(builder));

    GVSB(builder)->children[GVSB(builder)->offset++] = x_variant_ref_sink(value);
}

void x_variant_builder_open(XVariantBuilder *builder, const XVariantType *type)
{
    XVariantBuilder *parent;

    return_if_invalid_builder(builder);
    x_return_if_fail(GVSB(builder)->offset < GVSB(builder)->max_items);
    x_return_if_fail(!GVSB(builder)->expected_type || x_variant_type_is_subtype_of(type, GVSB(builder)->expected_type));
    x_return_if_fail(!GVSB(builder)->prev_item_type || x_variant_type_is_subtype_of(GVSB(builder)->prev_item_type, type));

    parent = x_slice_dup(XVariantBuilder, builder);
    x_variant_builder_init(builder, type);
    GVSB(builder)->parent = parent;

    if (GVSB(parent)->prev_item_type) {
        if (!GVSB(builder)->uniform_item_types) {
            GVSB(builder)->prev_item_type = x_variant_type_first(GVSB(parent)->prev_item_type);
        } else if (!x_variant_type_is_variant(GVSB(builder)->type)) {
            GVSB(builder)->prev_item_type = x_variant_type_element(GVSB(parent)->prev_item_type);
        }
    }
}

void x_variant_builder_close(XVariantBuilder *builder)
{
    XVariantBuilder *parent;

    return_if_invalid_builder(builder);
    x_return_if_fail(GVSB(builder)->parent != NULL);

    parent = GVSB(builder)->parent;
    GVSB(builder)->parent = NULL;

    x_variant_builder_add_value(parent, x_variant_builder_end(builder));
    *builder = *parent;

    x_slice_free(XVariantBuilder, parent);
}

static XVariantType *x_variant_make_maybe_type(XVariant *element)
{
    return x_variant_type_new_maybe(x_variant_get_type(element));
}

static XVariantType *x_variant_make_array_type(XVariant *element)
{
    return x_variant_type_new_array(x_variant_get_type(element));
}

XVariant *x_variant_builder_end(XVariantBuilder *builder)
{
    XVariant *value;
    XVariantType *my_type;

    return_val_if_invalid_builder(builder, NULL);
    x_return_val_if_fail(GVSB(builder)->offset >= GVSB(builder)->min_items, NULL);
    x_return_val_if_fail(!GVSB(builder)->uniform_item_types || GVSB(builder)->prev_item_type != NULL || x_variant_type_is_definite(GVSB(builder)->type), NULL);

    if (x_variant_type_is_definite(GVSB(builder)->type)) {
        my_type = x_variant_type_copy(GVSB(builder)->type);
    } else if (x_variant_type_is_maybe(GVSB(builder)->type)) {
        my_type = x_variant_make_maybe_type(GVSB(builder)->children[0]);
    } else if (x_variant_type_is_array(GVSB(builder)->type)) {
        my_type = x_variant_make_array_type(GVSB(builder)->children[0]);
    } else if (x_variant_type_is_tuple(GVSB(builder)->type)) {
        my_type = x_variant_make_tuple_type(GVSB(builder)->children, GVSB(builder)->offset);
    } else if (x_variant_type_is_dict_entry(GVSB(builder)->type)) {
        my_type = x_variant_make_dict_entry_type(GVSB(builder)->children[0], GVSB(builder)->children[1]);
    } else {
        x_assert_not_reached();
    }

    value = x_variant_new_from_children(my_type, x_renew(XVariant *, GVSB(builder)->children, GVSB(builder)->offset), GVSB(builder)->offset, GVSB(builder)->trusted);
    GVSB(builder)->children = NULL;
    GVSB(builder)->offset = 0;

    x_variant_builder_clear(builder);
    x_variant_type_free(my_type);

    return value;
}

struct stack_dict {
    XHashTable *values;
    xsize      magic;
};

X_STATIC_ASSERT(sizeof(struct stack_dict) <= sizeof(XVariantDict));

struct heap_dict {
    struct stack_dict dict;
    xint              ref_count;
    xsize             magic;
};

#define GVSD(d)                         ((struct stack_dict *)(d))
#define GVHD(d)                         ((struct heap_dict *)(d))
#define GVSD_MAGIC                      ((xsize)2579507750u)
#define GVSD_MAGIC_PARTIAL              ((xsize)3488698669u)
#define GVHD_MAGIC                      ((xsize)2450270775u)
#define is_valid_dict(d)                (GVSD(d)->magic == GVSD_MAGIC)
#define is_valid_heap_dict(d)           (GVHD(d)->magic == GVHD_MAGIC)

X_STATIC_ASSERT(sizeof(XVariantDict) == sizeof(xuintptr[16]));

static xboolean ensure_valid_dict(XVariantDict *dict)
{
    if (dict == NULL) {
        return FALSE;
    } else if (is_valid_dict(dict)) {
        return TRUE;
    }

    if (dict->u.s.partial_magic == GVSD_MAGIC_PARTIAL) {
        static XVariantDict cleared_dict;

        if (memcmp(cleared_dict.u.s.y, dict->u.s.y, sizeof cleared_dict.u.s.y)) {
            return FALSE;
        }

        x_variant_dict_init(dict, dict->u.s.asv);
    }

    return is_valid_dict(dict);
}

#define return_if_invalid_dict(d)                                    \
    X_STMT_START {                                                   \
        xboolean valid_dict X_GNUC_UNUSED = ensure_valid_dict(d);    \
        x_return_if_fail(valid_dict);                                \
    } X_STMT_END

#define return_val_if_invalid_dict(d, val)                           \
    X_STMT_START {                                                   \
        xboolean valid_dict X_GNUC_UNUSED = ensure_valid_dict(d);    \
        x_return_val_if_fail(valid_dict, val);                       \
    } X_STMT_END

XVariantDict *x_variant_dict_new(XVariant *from_asv)
{
    XVariantDict *dict;

    dict = (XVariantDict *)x_slice_alloc(sizeof(struct heap_dict));
    x_variant_dict_init(dict, from_asv);
    GVHD(dict)->magic = GVHD_MAGIC;
    GVHD(dict)->ref_count = 1;

    return dict;
}

void x_variant_dict_init(XVariantDict *dict, XVariant *from_asv)
{
    xchar *key;
    XVariant *value;
    XVariantIter iter;

    GVSD(dict)->values = x_hash_table_new_full(x_str_hash, x_str_equal, x_free, (XDestroyNotify)x_variant_unref);
    GVSD(dict)->magic = GVSD_MAGIC;

    if (from_asv) {
        x_variant_iter_init(&iter, from_asv);
        while (x_variant_iter_next(&iter, "{sv}", &key, &value)) {
            x_hash_table_insert(GVSD(dict)->values, key, value);
        }
    }
}

xboolean x_variant_dict_lookup(XVariantDict *dict, const xchar *key, const xchar *format_string, ...)
{
    va_list ap;
    XVariant *value;

    return_val_if_invalid_dict(dict, FALSE);
    x_return_val_if_fail(key != NULL, FALSE);
    x_return_val_if_fail(format_string != NULL, FALSE);

    value = (XVariant *)x_hash_table_lookup(GVSD(dict)->values, key);

    if (value == NULL || !x_variant_check_format_string(value, format_string, FALSE)) {
        return FALSE;
    }

    va_start(ap, format_string);
    x_variant_get_va(value, format_string, NULL, &ap);
    va_end(ap);

    return TRUE;
}

XVariant *x_variant_dict_lookup_value(XVariantDict *dict, const xchar *key, const XVariantType *expected_type)
{
    XVariant *result;

    return_val_if_invalid_dict(dict, NULL);
    x_return_val_if_fail(key != NULL, NULL);

    result = (XVariant *)x_hash_table_lookup(GVSD(dict)->values, key);

    if (result &&(!expected_type || x_variant_is_of_type(result, expected_type))) {
        return x_variant_ref(result);
    }

    return NULL;
}

xboolean x_variant_dict_contains(XVariantDict *dict, const xchar *key)
{
    return_val_if_invalid_dict(dict, FALSE);
    x_return_val_if_fail(key != NULL, FALSE);

    return x_hash_table_contains(GVSD(dict)->values, key);
}

void x_variant_dict_insert(XVariantDict *dict, const xchar *key, const xchar *format_string, ...)
{
    va_list ap;

    return_if_invalid_dict(dict);
    x_return_if_fail(key != NULL);
    x_return_if_fail(format_string != NULL);

    va_start(ap, format_string);
    x_variant_dict_insert_value(dict, key, x_variant_new_va(format_string, NULL, &ap));
    va_end(ap);
}

void x_variant_dict_insert_value(XVariantDict *dict, const xchar *key, XVariant *value)
{
    return_if_invalid_dict(dict);
    x_return_if_fail(key != NULL);
    x_return_if_fail(value != NULL);

    x_hash_table_insert(GVSD(dict)->values, x_strdup(key), x_variant_ref_sink(value));
}

xboolean x_variant_dict_remove(XVariantDict *dict, const xchar *key)
{
    return_val_if_invalid_dict(dict, FALSE);
    x_return_val_if_fail(key != NULL, FALSE);

    return x_hash_table_remove(GVSD(dict)->values, key);
}

void x_variant_dict_clear(XVariantDict *dict)
{
    if (GVSD(dict)->magic == 0) {
        return;
    }

    return_if_invalid_dict(dict);

    x_hash_table_unref(GVSD(dict)->values);
    GVSD(dict)->values = NULL;

    GVSD(dict)->magic = 0;
}

XVariant *x_variant_dict_end(XVariantDict *dict)
{
    XHashTableIter iter;
    xpointer key, value;
    XVariantBuilder builder;

    return_val_if_invalid_dict(dict, NULL);

    x_variant_builder_init(&builder, X_VARIANT_TYPE_VARDICT);

    x_hash_table_iter_init(&iter, GVSD(dict)->values);
    while (x_hash_table_iter_next(&iter, &key, &value)) {
        x_variant_builder_add(&builder, "{sv}",(const xchar *)key,(XVariant *)value);
    }
    x_variant_dict_clear(dict);

    return x_variant_builder_end(&builder);
}

XVariantDict *x_variant_dict_ref(XVariantDict *dict)
{
    x_return_val_if_fail(is_valid_heap_dict(dict), NULL);
    GVHD(dict)->ref_count++;

    return dict;
}

void x_variant_dict_unref(XVariantDict *dict)
{
    x_return_if_fail(is_valid_heap_dict(dict));

    if (--GVHD(dict)->ref_count == 0) {
        x_variant_dict_clear(dict);
        x_slice_free(struct heap_dict,(struct heap_dict *)dict);
    }
}

xboolean x_variant_format_string_scan(const xchar *string, const xchar *limit, const xchar **endptr)
{
#define next_char()     (string == limit ? '\0' : *(string++))
#define peek_char()     (string == limit ? '\0' : *string)
    char c;

    switch (next_char()) {
        case 'b': case 'y': case 'n': case 'q': case 'i': case 'u':
        case 'x': case 't': case 'h': case 'd': case 's': case 'o':
        case 'g': case 'v': case '*': case '?': case 'r':
            break;

        case 'm':
            return x_variant_format_string_scan(string, limit, endptr);

        case 'a':
        case '@':
            return x_variant_type_string_scan(string, limit, endptr);

        case '(':
            while (peek_char() != ')') {
                if (!x_variant_format_string_scan(string, limit, &string)) {
                    return FALSE;
                }
            }
            next_char();
            break;

        case '{':
            c = next_char();
            if (c == '&') {
                c = next_char();
                if (c != 's' && c != 'o' && c != 'g') {
                    return FALSE;
                }
            } else {
                if (c == '@') {
                    c = next_char();
                }

                if (c != '\0' && strchr("bynqiuxthdsog?", c) == NULL) {
                    return FALSE;
                }
            }

            if (!x_variant_format_string_scan(string, limit, &string)) {
                return FALSE;
            }

            if (next_char() != '}') {
                return FALSE;
            }
            break;

        case '^':
            if ((c = next_char()) == 'a') {
                if ((c = next_char()) == '&') {
                    if ((c = next_char()) == 'a') {
                        if ((c = next_char()) == 'y') {
                            break;
                        }
                    } else if (c == 's' || c == 'o') {
                        break;
                    }
                } else if (c == 'a') {
                    if ((c = next_char()) == 'y') {
                        break;
                    }
                } else if (c == 's' || c == 'o') {
                    break;
                } else if (c == 'y') {
                    break;
                }
            } else if (c == '&') {
                if ((c = next_char()) == 'a') {
                    if ((c = next_char()) == 'y') {
                        break;
                    }
                }
            }
            return FALSE;

        case '&':
            c = next_char();
            if (c != 's' && c != 'o' && c != 'g') {
                return FALSE;
            }
            break;

        default:
            return FALSE;
    }

    if (endptr != NULL) {
        *endptr = string;
    }

#undef next_char
#undef peek_char

    return TRUE;
}

xboolean x_variant_check_format_string(XVariant *value, const xchar *format_string, xboolean copy_only)
{
    const xchar *type_string;
    const xchar *original_format = format_string;

    type_string = x_variant_get_type_string(value);

    while (*type_string || *format_string) {
        xchar format = *format_string++;

        switch (format) {
            case '&':
                if X_UNLIKELY(copy_only) {
                    x_critical("x_variant_check_format_string() is being called by a function with a XVariant varargs "
                                "interface to validate the passed format string for type safety.  The passed format "
                                "(%s) contains a '&' character which would result in a pointer being returned to the "
                                "data inside of a XVariant instance that may no longer exist by the time the function "
                                "returns.  Modify your code to use a format string without '&'.", original_format);
                    return FALSE;
                }

            X_GNUC_FALLTHROUGH;
            case '^':
            case '@':
                continue;

            case '?': {
                char s = *type_string++;
                if (s == '\0' || strchr("bynqiuxthdsog", s) == NULL) {
                    return FALSE;
                }
            }
            continue;

            case 'r':
                if (*type_string != '(') {
                    return FALSE;
                }

            X_GNUC_FALLTHROUGH;
            case '*':
                if (!x_variant_type_string_scan(type_string, NULL, &type_string)) {
                    return FALSE;
                }
                continue;

            default:
                if (format != *type_string++) {
                    return FALSE;
                }
        }
    }

    return TRUE;
}

XVariantType *x_variant_format_string_scan_type(const xchar *string, const xchar *limit, const xchar **endptr)
{
    xchar *newt;
    xchar *dest;
    const xchar *my_end;

    if (endptr == NULL) {
        endptr = &my_end;
    }

    if (!x_variant_format_string_scan(string, limit, endptr)) {
        return NULL;
    }

    dest = newt = (xchar *)x_malloc(*endptr - string + 1);
    while (string != *endptr) {
        if (*string != '@' && *string != '&' && *string != '^') {
            *dest++ = *string;
        }
        string++;
    }
    *dest = '\0';

    return(XVariantType *)X_VARIANT_TYPE(newt);
}

static xboolean valid_format_string(const xchar *format_string, xboolean single, XVariant *value)
{
    XVariantType *type;
    const xchar *endptr;

    type = x_variant_format_string_scan_type(format_string, NULL, &endptr);

    if X_UNLIKELY(type == NULL ||(single && *endptr != '\0')) {
        if (single) {
            x_critical("'%s' is not a valid XVariant format string", format_string);
        } else {
            x_critical("'%s' does not have a valid XVariant format string as a prefix", format_string);
        }

        if (type != NULL) {
            x_variant_type_free(type);
        }

        return FALSE;
    }

    if X_UNLIKELY(value && !x_variant_is_of_type(value, type)) {
        xchar *typestr;
        xchar *fragment;

        fragment = x_strndup(format_string, endptr - format_string);
        typestr = x_variant_type_dup_string(type);

        x_critical("the XVariant format string '%s' has a type of '%s' but the given value has a type of '%s'", fragment, typestr, x_variant_get_type_string(value));

        x_variant_type_free(type);
        x_free(fragment);
        x_free(typestr);

        return FALSE;
    }

    x_variant_type_free(type);
    return TRUE;
}

static xboolean x_variant_format_string_is_leaf(const xchar *str)
{
    return str[0] != 'm' && str[0] != '(' && str[0] != '{';
}

static xboolean x_variant_format_string_is_nnp(const xchar *str)
{
    return str[0] == 'a' || str[0] == 's' || str[0] == 'o' || str[0] == 'g' || str[0] == '^' || str[0] == '@' || str[0] == '*' || str[0] == '?' || str[0] == 'r' || str[0] == 'v' || str[0] == '&';
}

static void x_variant_valist_free_nnp(const xchar *str, xpointer ptr)
{
    switch (*str) {
        case 'a':
            x_variant_iter_free((XVariantIter *)ptr);
            break;

        case '^':
            if (x_str_has_suffix(str, "y")) {
                if (str[2] != 'a') {
                    x_free(ptr);
                } else if (str[1] == 'a') {
                    x_strfreev((xchar **)ptr);
                }
                break;
            } else if (str[2] != '&') {
                x_strfreev((xchar **)ptr);
            } else {
                x_free(ptr);
            }
            break;

        case 's':
        case 'o':
        case 'g':
            x_free(ptr);
            break;

        case '@':
        case '*':
        case '?':
        case 'v':
            x_variant_unref((XVariant *)ptr);
            break;

        case '&':
            break;

        default:
        x_assert_not_reached();
    }
}

static xchar x_variant_scan_convenience(const xchar **str, xboolean *constant, xuint *arrays)
{
    *arrays = 0;
    *constant = FALSE;

    for (; ;) {
        char c = *(*str)++;
        if (c == '&') {
            *constant = TRUE;
        } else if (c == 'a') {
            (*arrays)++;
        } else  {
            return c;
        }
    }
}

static XVariant *x_variant_valist_new_nnp(const xchar **str, xpointer ptr)
{
    if(**str == '&') {
        (*str)++;
    }

    switch (*(*str)++) {
        case 'a':
            if (ptr != NULL) {
                XVariant *value;
                const XVariantType *type;

                value = x_variant_builder_end((XVariantBuilder *)ptr);
                type = x_variant_get_type(value);

                if X_UNLIKELY(!x_variant_type_is_array(type)) {
                    x_error("x_variant_new: expected array XVariantBuilder but the built value has type '%s'", x_variant_get_type_string(value));
                }

                type = x_variant_type_element(type);
                if X_UNLIKELY(!x_variant_type_is_subtype_of(type,(XVariantType *)*str)) {
                    xchar *type_string = x_variant_type_dup_string((XVariantType *)*str);
                    x_error("x_variant_new: expected XVariantBuilder array element type '%s' but the built value has element type '%s'", type_string, x_variant_get_type_string(value) + 1);
                    x_free(type_string);
                }

                x_variant_type_string_scan(*str, NULL, str);
                return value;
            } else {
                const XVariantType *type =(XVariantType *)*str;

                x_variant_type_string_scan(*str, NULL, str);

                if X_UNLIKELY(!x_variant_type_is_definite(type)) {
                    x_error("x_variant_new: NULL pointer given with indefinite array type; unable to determine which type of empty array to construct.");
                }

                return x_variant_new_array(type, NULL, 0);
            }

        case 's': {
            XVariant *value;

            value = x_variant_new_string((const xchar *)ptr);
            if (value == NULL) {
                value = x_variant_new_string("[Invalid UTF-8]");
            }

            return value;
        }

        case 'o':
            return x_variant_new_object_path((const xchar *)ptr);

        case 'g':
            return x_variant_new_signature((const xchar *)ptr);

        case '^': {
            xchar type;
            xuint arrays;
            xboolean constant;

            type = x_variant_scan_convenience(str, &constant, &arrays);

            if (type == 's') {
                return x_variant_new_strv((const xchar *const *)ptr, -1);
            }

            if (type == 'o') {
                return x_variant_new_objv((const xchar *const *)ptr, -1);
            }

            if (arrays > 1) {
                return x_variant_new_bytestring_array((const xchar *const *)ptr, -1);
            }

            return x_variant_new_bytestring((const xchar *)ptr);
        }

        case '@':
            if X_UNLIKELY(!x_variant_is_of_type((XVariant *)ptr, (const XVariantType *)*str)) {
                xchar *type_string = x_variant_type_dup_string((const XVariantType *)*str);
                x_error("x_variant_new: expected XVariant of type '%s' but received value has type '%s'", type_string, x_variant_get_type_string((XVariant *)ptr));
                x_free(type_string);
            }

            x_variant_type_string_scan(*str, NULL, str);
            return (XVariant *)ptr;

        case '*':
            return (XVariant *)ptr;

        case '?':
            if X_UNLIKELY(!x_variant_type_is_basic(x_variant_get_type((XVariant *)ptr))) {
                x_error("x_variant_new: format string '?' expects basic-typed XVariant, but received value has type '%s'", x_variant_get_type_string((XVariant *)ptr));
            }
            return (XVariant *)ptr;

        case 'r':
            if X_UNLIKELY(!x_variant_type_is_tuple(x_variant_get_type((XVariant *)ptr))) {
                x_error("x_variant_new: format string 'r' expects tuple-typed XVariant, but received value has type '%s'", x_variant_get_type_string((XVariant *)ptr));
            }
            return (XVariant *)ptr;

        case 'v':
            return x_variant_new_variant((XVariant *)ptr);

        default:
            x_assert_not_reached();
    }
}

static xpointer x_variant_valist_get_nnp(const xchar **str, XVariant *value)
{
    switch(*(*str)++) {
        case 'a':
            x_variant_type_string_scan(*str, NULL, str);
            return x_variant_iter_new(value);

        case '&':
            (*str)++;
            return(xchar *)x_variant_get_string(value, NULL);

        case 's':
        case 'o':
        case 'g':
            return x_variant_dup_string(value, NULL);

        case '^': {
            xchar type;
            xuint arrays;
            xboolean constant;

            type = x_variant_scan_convenience(str, &constant, &arrays);
            if (type == 's') {
                if(constant) {
                    return x_variant_get_strv(value, NULL);
                } else {
                    return x_variant_dup_strv(value, NULL);
                }
            } else if (type == 'o') {
                if (constant) {
                    return x_variant_get_objv(value, NULL);
                } else {
                    return x_variant_dup_objv(value, NULL);
                }
            } else if (arrays > 1) {
                if (constant) {
                    return x_variant_get_bytestring_array(value, NULL);
                } else {
                    return x_variant_dup_bytestring_array(value, NULL);
                }
            } else {
                if (constant) {
                    return(xchar *)x_variant_get_bytestring(value);
                } else {
                    return x_variant_dup_bytestring(value, NULL);
                }
            }
        }

        case '@':
            x_variant_type_string_scan(*str, NULL, str);
            X_GNUC_FALLTHROUGH;

        case '*':
        case '?':
        case 'r':
            return x_variant_ref(value);

        case 'v':
            return x_variant_get_variant(value);

        default:
            x_assert_not_reached();
    }
}

static void x_variant_valist_skip_leaf(const xchar **str, va_list *app)
{
    if (x_variant_format_string_is_nnp(*str)) {
        x_variant_format_string_scan(*str, NULL, str);
        va_arg(*app, xpointer);
        return;
    }

    switch (*(*str)++) {
        case 'b':
        case 'y':
        case 'n':
        case 'q':
        case 'i':
        case 'u':
        case 'h':
            va_arg(*app, int);
            return;

        case 'x':
        case 't':
            va_arg(*app, xuint64);
            return;

        case 'd':
            va_arg(*app, xdouble);
            return;

        default:
            x_assert_not_reached();
    }
}

static XVariant *x_variant_valist_new_leaf(const xchar **str, va_list *app)
{
    if (x_variant_format_string_is_nnp(*str)) {
        return x_variant_valist_new_nnp(str, va_arg(*app, xpointer));
    }

    switch (*(*str)++) {
        case 'b':
            return x_variant_new_boolean(va_arg(*app, xboolean));

        case 'y':
            return x_variant_new_byte(va_arg(*app, xuint));

        case 'n':
            return x_variant_new_int16(va_arg(*app, xint));

        case 'q':
            return x_variant_new_uint16(va_arg(*app, xuint));

        case 'i':
            return x_variant_new_int32(va_arg(*app, xint));

        case 'u':
            return x_variant_new_uint32(va_arg(*app, xuint));

        case 'x':
            return x_variant_new_int64(va_arg(*app, xint64));

        case 't':
            return x_variant_new_uint64(va_arg(*app, xuint64));

        case 'h':
            return x_variant_new_handle(va_arg(*app, xint));

        case 'd':
            return x_variant_new_double(va_arg(*app, xdouble));

        default:
            x_assert_not_reached();
    }
}

X_STATIC_ASSERT(sizeof(xboolean) == sizeof(xuint32));
X_STATIC_ASSERT(sizeof(xdouble) == sizeof(xuint64));

static void x_variant_valist_get_leaf(const xchar **str, XVariant *value, xboolean free, va_list *app)
{
    xpointer ptr = va_arg(*app, xpointer);

    if (ptr == NULL) {
        x_variant_format_string_scan(*str, NULL, str);
        return;
    }

    if (x_variant_format_string_is_nnp(*str)) {
        xpointer *nnp =(xpointer *)ptr;

        if (free && *nnp != NULL) {
            x_variant_valist_free_nnp(*str, *nnp);
        }

        *nnp = NULL;
        if (value != NULL) {
            *nnp = x_variant_valist_get_nnp(str, value);
        } else {
            x_variant_format_string_scan(*str, NULL, str);
        }

        return;
    }

    if (value != NULL) {
        switch (*(*str)++) {
            case 'b':
                *(xboolean *)ptr = x_variant_get_boolean(value);
                return;

            case 'y':
                *(xuint8 *)ptr = x_variant_get_byte(value);
                return;

            case 'n':
                *(xint16 *)ptr = x_variant_get_int16(value);
                return;

            case 'q':
                *(xuint16 *)ptr = x_variant_get_uint16(value);
                return;

            case 'i':
                *(xint32 *)ptr = x_variant_get_int32(value);
                return;

            case 'u':
                *(xuint32 *)ptr = x_variant_get_uint32(value);
                return;

            case 'x':
                *(xint64 *)ptr = x_variant_get_int64(value);
                return;

            case 't':
                *(xuint64 *)ptr = x_variant_get_uint64(value);
                return;

            case 'h':
                *(xint32 *)ptr = x_variant_get_handle(value);
                return;

            case 'd':
                *(xdouble *)ptr = x_variant_get_double(value);
                return;
        }
    } else {
        switch (*(*str)++) {
            case 'y':
                *(xuint8 *)ptr = 0;
                return;

            case 'n':
            case 'q':
                *(xuint16 *)ptr = 0;
                return;

            case 'i':
            case 'u':
            case 'h':
            case 'b':
                *(xuint32 *)ptr = 0;
                return;

            case 'x':
            case 't':
            case 'd':
                *(xuint64 *)ptr = 0;
                return;
        }
    }

    x_assert_not_reached();
}

static void x_variant_valist_skip(const xchar **str, va_list *app)
{
    if (x_variant_format_string_is_leaf(*str)) {
        x_variant_valist_skip_leaf(str, app);
    } else if (**str == 'm') {
        (*str)++;

        if (!x_variant_format_string_is_nnp(*str)) {
            va_arg(*app, xboolean);
        }

        x_variant_valist_skip(str, app);
    } else  {
        x_assert(**str == '(' || **str == '{');

        (*str)++;
        while (**str != ')' && **str != '}') {
            x_variant_valist_skip(str, app);
        }
        (*str)++;
    }
}

static XVariant *x_variant_valist_new(const xchar **str, va_list *app)
{
    if (x_variant_format_string_is_leaf(*str)) {
        return x_variant_valist_new_leaf(str, app);
    }

    if (**str == 'm') {
        XVariant *value = NULL;
        XVariantType *type = NULL;

        (*str)++;

        if (x_variant_format_string_is_nnp(*str)) {
            xpointer nnp = va_arg(*app, xpointer);

            if (nnp != NULL) {
                value = x_variant_valist_new_nnp(str, nnp);
            } else {
                type = x_variant_format_string_scan_type(*str, NULL, str);
            }
        } else {
            xboolean just = va_arg(*app, xboolean);
            if (just) {
                value = x_variant_valist_new(str, app);
            } else {
                type = x_variant_format_string_scan_type(*str, NULL, NULL);
                x_variant_valist_skip(str, app);
            }
        }

        value = x_variant_new_maybe(type, value);
        if (type != NULL) {
            x_variant_type_free(type);
        }

        return value;
    } else {
        XVariantBuilder b;

        if (**str == '(') {
            x_variant_builder_init(&b, X_VARIANT_TYPE_TUPLE);
        } else {
            x_assert(**str == '{');
            x_variant_builder_init(&b, X_VARIANT_TYPE_DICT_ENTRY);
        }

        (*str)++;
        while (**str != ')' && **str != '}') {
            x_variant_builder_add_value(&b, x_variant_valist_new(str, app));
        }
        (*str)++;

        return x_variant_builder_end(&b);
    }
}

static void x_variant_valist_get(const xchar **str, XVariant *value, xboolean free, va_list *app)
{
    if (x_variant_format_string_is_leaf(*str)) {
        x_variant_valist_get_leaf(str, value, free, app);
    } else if (**str == 'm') {
        (*str)++;

        if (value != NULL) {
            value = x_variant_get_maybe(value);
        }

        if (!x_variant_format_string_is_nnp(*str)) {
            xboolean *ptr = va_arg(*app, xboolean *);
            if (ptr != NULL) {
                *ptr = value != NULL;
            }
        }

        x_variant_valist_get(str, value, free, app);

        if (value != NULL) {
            x_variant_unref(value);
        }
    } else {
        xint index = 0;

        x_assert(**str == '(' || **str == '{');

        (*str)++;
        while (**str != ')' && **str != '}') {
            if (value != NULL) {
                XVariant *child = x_variant_get_child_value(value, index++);
                x_variant_valist_get(str, child, free, app);
                x_variant_unref(child);
            } else {
                x_variant_valist_get(str, NULL, free, app);
            }
        }
        (*str)++;
    }
}

XVariant *x_variant_new(const xchar *format_string, ...)
{
    va_list ap;
    XVariant *value;

    x_return_val_if_fail(valid_format_string(format_string, TRUE, NULL) && format_string[0] != '?' && format_string[0] != '@' && format_string[0] != '*' && format_string[0] != 'r', NULL);

    va_start(ap, format_string);
    value = x_variant_new_va(format_string, NULL, &ap);
    va_end(ap);

    return value;
}

XVariant *x_variant_new_va(const xchar *format_string, const xchar **endptr, va_list *app)
{
    XVariant *value;

    x_return_val_if_fail(valid_format_string(format_string, !endptr, NULL), NULL);
    x_return_val_if_fail(app != NULL, NULL);

    value = x_variant_valist_new(&format_string, app);
    if (endptr != NULL) {
        *endptr = format_string;
    }

    return value;
}

void x_variant_get(XVariant *value, const xchar *format_string, ...)
{
    va_list ap;

    x_return_if_fail(value != NULL);
    x_return_if_fail(valid_format_string(format_string, TRUE, value));

    if (strchr(format_string, '&')) {
        x_variant_get_data(value);
    }

    va_start(ap, format_string);
    x_variant_get_va(value, format_string, NULL, &ap);
    va_end(ap);
}

void x_variant_get_va(XVariant *value, const xchar *format_string, const xchar **endptr, va_list *app)
{
    x_return_if_fail(valid_format_string(format_string, !endptr, value));
    x_return_if_fail(value != NULL);
    x_return_if_fail(app != NULL);

    if (strchr(format_string, '&')) {
        x_variant_get_data(value);
    }

    x_variant_valist_get(&format_string, value, FALSE, app);
    if (endptr != NULL) {
        *endptr = format_string;
    }
}

void x_variant_builder_add(XVariantBuilder *builder, const xchar *format_string, ...)
{
    va_list ap;
    XVariant *variant;

    va_start(ap, format_string);
    variant = x_variant_new_va(format_string, NULL, &ap);
    va_end(ap);

    x_variant_builder_add_value(builder, variant);
}

void x_variant_get_child(XVariant *value, xsize index_, const xchar *format_string, ...)
{
    va_list ap;
    XVariant *child;

    if (strchr(format_string, '&')) {
        x_variant_get_data(value);
    }

    child = x_variant_get_child_value(value, index_);
    x_return_if_fail(valid_format_string(format_string, TRUE, child));

    va_start(ap, format_string);
    x_variant_get_va(child, format_string, NULL, &ap);
    va_end(ap);

    x_variant_unref(child);
}

xboolean x_variant_iter_next(XVariantIter *iter, const xchar *format_string, ...)
{
    XVariant *value;

    value = x_variant_iter_next_value(iter);
    x_return_val_if_fail(valid_format_string(format_string, TRUE, value), FALSE);

    if (value != NULL) {
        va_list ap;

        va_start(ap, format_string);
        x_variant_valist_get(&format_string, value, FALSE, &ap);
        va_end(ap);

        x_variant_unref(value);
    }

    return value != NULL;
}

xboolean x_variant_iter_loop(XVariantIter *iter, const xchar *format_string, ...)
{
    va_list ap;
    XVariant *value;
    xboolean first_time = GVSI(iter)->loop_format == NULL;

    x_return_val_if_fail(first_time || format_string == GVSI(iter)->loop_format, FALSE);

    if (first_time) {
        TYPE_CHECK(GVSI(iter)->value, X_VARIANT_TYPE_ARRAY, FALSE);
        GVSI(iter)->loop_format = format_string;

        if (strchr(format_string, '&')) {
            x_variant_get_data(GVSI(iter)->value);
        }
    }

    value = x_variant_iter_next_value(iter);

    x_return_val_if_fail(!first_time || valid_format_string(format_string, TRUE, value), FALSE);

    va_start(ap, format_string);
    x_variant_valist_get(&format_string, value, !first_time, &ap);
    va_end(ap);

    if (value != NULL) {
        x_variant_unref(value);
    }

    return value != NULL;
}

static XVariant *x_variant_deep_copy(XVariant *value, xboolean byteswap)
{
    switch (x_variant_classify(value)) {
        case X_VARIANT_CLASS_MAYBE:
        case X_VARIANT_CLASS_TUPLE:
        case X_VARIANT_CLASS_DICT_ENTRY:
        case X_VARIANT_CLASS_VARIANT: {
            xsize i, n_children;
            XVariantBuilder builder;

            x_variant_builder_init(&builder, x_variant_get_type(value));

            for (i = 0, n_children = x_variant_n_children(value); i < n_children; i++) {
                XVariant *child = x_variant_get_child_value(value, i);
                x_variant_builder_add_value(&builder, x_variant_deep_copy(child, byteswap));
                x_variant_unref(child);
            }

            return x_variant_builder_end(&builder);
        }

        case X_VARIANT_CLASS_ARRAY: {
            xsize i, n_children;
            XVariantBuilder builder;
            XVariant *first_invalid_child_deep_copy = NULL;

            x_variant_builder_init(&builder, x_variant_get_type(value));

            for (i = 0, n_children = x_variant_n_children(value); i < n_children; i++) {
                XVariant *child = x_variant_maybe_get_child_value(value, i);
                if (child != NULL) {
                    x_variant_builder_add_value(&builder, x_variant_deep_copy(child, byteswap));
                } else if (child == NULL && first_invalid_child_deep_copy != NULL) {
                    x_variant_builder_add_value(&builder, first_invalid_child_deep_copy);
                } else if (child == NULL) {
                    child = x_variant_get_child_value(value, i);
                    first_invalid_child_deep_copy = x_variant_ref_sink(x_variant_deep_copy(child, byteswap));
                    x_variant_builder_add_value(&builder, first_invalid_child_deep_copy);
                }

                x_clear_pointer(&child, x_variant_unref);
            }

            x_clear_pointer(&first_invalid_child_deep_copy, x_variant_unref);
            return x_variant_builder_end(&builder);
        }

        case X_VARIANT_CLASS_BOOLEAN:
            return x_variant_new_boolean(x_variant_get_boolean(value));

        case X_VARIANT_CLASS_BYTE:
            return x_variant_new_byte(x_variant_get_byte(value));

        case X_VARIANT_CLASS_INT16:
            if (byteswap) {
                return x_variant_new_int16(XUINT16_SWAP_LE_BE(x_variant_get_int16(value)));
            } else {
                return x_variant_new_int16(x_variant_get_int16(value));
            }

        case X_VARIANT_CLASS_UINT16:
            if (byteswap) {
                return x_variant_new_uint16(XUINT16_SWAP_LE_BE(x_variant_get_uint16(value)));
            } else {
                return x_variant_new_uint16(x_variant_get_uint16(value));
            }

        case X_VARIANT_CLASS_INT32:
            if (byteswap) {
                return x_variant_new_int32(XUINT32_SWAP_LE_BE(x_variant_get_int32(value)));
            } else {
                return x_variant_new_int32(x_variant_get_int32(value));
            }

        case X_VARIANT_CLASS_UINT32:
            if (byteswap) {
                return x_variant_new_uint32(XUINT32_SWAP_LE_BE(x_variant_get_uint32(value)));
            } else {
                return x_variant_new_uint32(x_variant_get_uint32(value));
            }

        case X_VARIANT_CLASS_INT64:
            if (byteswap) {
                return x_variant_new_int64(XUINT64_SWAP_LE_BE(x_variant_get_int64(value)));
            } else {
                return x_variant_new_int64(x_variant_get_int64(value));
            }

        case X_VARIANT_CLASS_UINT64:
            if (byteswap) {
                return x_variant_new_uint64(XUINT64_SWAP_LE_BE(x_variant_get_uint64(value)));
            } else {
                return x_variant_new_uint64(x_variant_get_uint64(value));
            }

        case X_VARIANT_CLASS_HANDLE:
            if (byteswap) {
                return x_variant_new_handle(XUINT32_SWAP_LE_BE(x_variant_get_handle(value)));
            } else {
                return x_variant_new_handle(x_variant_get_handle(value));
            }

        case X_VARIANT_CLASS_DOUBLE:
            if (byteswap) {
                union {
                    xuint64 u64;
                    xdouble dbl;
                } u1, u2;

                u1.dbl = x_variant_get_double(value);
                u2.u64 = XUINT64_SWAP_LE_BE(u1.u64);
                return x_variant_new_double(u2.dbl);
            } else {
                return x_variant_new_double(x_variant_get_double(value));
            }

        case X_VARIANT_CLASS_STRING:
            return x_variant_new_string(x_variant_get_string(value, NULL));

        case X_VARIANT_CLASS_OBJECT_PATH:
            return x_variant_new_object_path(x_variant_get_string(value, NULL));

        case X_VARIANT_CLASS_SIGNATURE:
            return x_variant_new_signature(x_variant_get_string(value, NULL));
    }

    x_assert_not_reached();
}

XVariant *x_variant_get_normal_form(XVariant *value)
{
    XVariant *trusted;

    if (x_variant_is_normal_form(value)) {
        return x_variant_ref(value);
    }

    trusted = x_variant_deep_copy(value, FALSE);
    x_assert(x_variant_is_trusted(trusted));

    return x_variant_ref_sink(trusted);
}

XVariant *x_variant_byteswap(XVariant *value)
{
    XVariant *newt;
    xuint alignment;
    XVariantTypeInfo *type_info;

    type_info = x_variant_get_type_info(value);
    x_variant_type_info_query(type_info, &alignment, NULL);

    if (alignment && x_variant_is_normal_form(value)) {
        XBytes *bytes;
        XVariantSerialised serialised = { 0, };

        serialised.type_info = x_variant_get_type_info(value);
        serialised.size = x_variant_get_size(value);
        serialised.data = (xuchar *)x_malloc(serialised.size);
        serialised.depth = x_variant_get_depth(value);
        serialised.ordered_offsets_up_to = X_MAXSIZE;
        serialised.checked_offsets_up_to = X_MAXSIZE;
        x_variant_store(value, serialised.data);

        x_variant_serialised_byteswap(serialised);

        bytes = x_bytes_new_take(serialised.data, serialised.size);
        newt = x_variant_ref_sink(x_variant_new_from_bytes(x_variant_get_type(value), bytes, TRUE));
        x_bytes_unref(bytes);
    } else if (alignment) {
        newt = x_variant_ref_sink(x_variant_deep_copy(value, TRUE)); 
    } else {
        newt = x_variant_get_normal_form(value);
    }

    x_assert(x_variant_is_trusted(newt));

    return x_steal_pointer(&newt);
}

XVariant *x_variant_new_from_data(const XVariantType *type, xconstpointer data, xsize size, xboolean trusted, XDestroyNotify notify, xpointer user_data)
{
    XBytes *bytes;
    XVariant *value;

    x_return_val_if_fail(x_variant_type_is_definite(type), NULL);
    x_return_val_if_fail(data != NULL || size == 0, NULL);

    if (notify) {
        bytes = x_bytes_new_with_free_func(data, size, notify, user_data);
    } else {
        bytes = x_bytes_new_static(data, size);
    }

    value = x_variant_new_from_bytes(type, bytes, trusted);
    x_bytes_unref(bytes);

    return value;
}
