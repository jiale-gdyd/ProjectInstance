#ifndef __X_VARIANT_TYPE_H__
#define __X_VARIANT_TYPE_H__

#include "xtypes.h"

X_BEGIN_DECLS

typedef struct _XVariantType XVariantType;

#define X_VARIANT_TYPE_BOOLEAN              ((const XVariantType *) "b")
#define X_VARIANT_TYPE_BYTE                 ((const XVariantType *) "y")
#define X_VARIANT_TYPE_INT16                ((const XVariantType *) "n")
#define X_VARIANT_TYPE_UINT16               ((const XVariantType *) "q")
#define X_VARIANT_TYPE_INT32                ((const XVariantType *) "i")
#define X_VARIANT_TYPE_UINT32               ((const XVariantType *) "u")
#define X_VARIANT_TYPE_INT64                ((const XVariantType *) "x")
#define X_VARIANT_TYPE_UINT64               ((const XVariantType *) "t")
#define X_VARIANT_TYPE_DOUBLE               ((const XVariantType *) "d")
#define X_VARIANT_TYPE_STRING               ((const XVariantType *) "s")
#define X_VARIANT_TYPE_OBJECT_PATH          ((const XVariantType *) "o")
#define X_VARIANT_TYPE_SIGNATURE            ((const XVariantType *) "g")
#define X_VARIANT_TYPE_VARIANT              ((const XVariantType *) "v")
#define X_VARIANT_TYPE_HANDLE               ((const XVariantType *) "h")
#define X_VARIANT_TYPE_UNIT                 ((const XVariantType *) "()")
#define X_VARIANT_TYPE_ANY                  ((const XVariantType *) "*")
#define X_VARIANT_TYPE_BASIC                ((const XVariantType *) "?")
#define X_VARIANT_TYPE_MAYBE                ((const XVariantType *) "m*")
#define X_VARIANT_TYPE_ARRAY                ((const XVariantType *) "a*")
#define X_VARIANT_TYPE_TUPLE                ((const XVariantType *) "r")
#define X_VARIANT_TYPE_DICT_ENTRY           ((const XVariantType *) "{?*}")
#define X_VARIANT_TYPE_DICTIONARY           ((const XVariantType *) "a{?*}")
#define X_VARIANT_TYPE_STRING_ARRAY         ((const XVariantType *) "as")
#define X_VARIANT_TYPE_OBJECT_PATH_ARRAY    ((const XVariantType *) "ao")
#define X_VARIANT_TYPE_BYTESTRING           ((const XVariantType *) "ay")
#define X_VARIANT_TYPE_BYTESTRING_ARRAY     ((const XVariantType *) "aay")
#define X_VARIANT_TYPE_VARDICT              ((const XVariantType *) "a{sv}")

#ifndef X_DISABLE_CHECKS
#define X_VARIANT_TYPE(type_string)         (x_variant_type_checked_((type_string)))
#else
#define X_VARIANT_TYPE(type_string)         ((const XVariantType *)(type_string))
#endif

XLIB_AVAILABLE_IN_ALL
xboolean x_variant_type_string_is_valid(const xchar *type_string);

XLIB_AVAILABLE_IN_ALL
xboolean x_variant_type_string_scan(const xchar *string, const xchar *limit, const xchar **endptr);

XLIB_AVAILABLE_IN_ALL
void x_variant_type_free(XVariantType *type);

XLIB_AVAILABLE_IN_ALL
XVariantType *x_variant_type_copy(const XVariantType *type);

XLIB_AVAILABLE_IN_ALL
XVariantType *x_variant_type_new(const xchar *type_string);

XLIB_AVAILABLE_IN_ALL
xsize x_variant_type_get_string_length(const XVariantType *type);

XLIB_AVAILABLE_IN_ALL
const xchar *x_variant_type_peek_string(const XVariantType *type);

XLIB_AVAILABLE_IN_ALL
xchar *x_variant_type_dup_string(const XVariantType *type);

XLIB_AVAILABLE_IN_ALL
xboolean x_variant_type_is_definite(const XVariantType *type);

XLIB_AVAILABLE_IN_ALL
xboolean x_variant_type_is_container(const XVariantType *type);

XLIB_AVAILABLE_IN_ALL
xboolean x_variant_type_is_basic(const XVariantType *type);

XLIB_AVAILABLE_IN_ALL
xboolean x_variant_type_is_maybe(const XVariantType *type);

XLIB_AVAILABLE_IN_ALL
xboolean x_variant_type_is_array(const XVariantType *type);

XLIB_AVAILABLE_IN_ALL
xboolean x_variant_type_is_tuple(const XVariantType *type);

XLIB_AVAILABLE_IN_ALL
xboolean x_variant_type_is_dict_entry(const XVariantType *type);

XLIB_AVAILABLE_IN_ALL
xboolean x_variant_type_is_variant(const XVariantType *type);

XLIB_AVAILABLE_IN_ALL
xuint x_variant_type_hash(xconstpointer type);

XLIB_AVAILABLE_IN_ALL
xboolean x_variant_type_equal(xconstpointer type1, xconstpointer type2);

XLIB_AVAILABLE_IN_ALL
xboolean x_variant_type_is_subtype_of(const XVariantType *type, const XVariantType *supertype);

XLIB_AVAILABLE_IN_ALL
const XVariantType *x_variant_type_element(const XVariantType *type);

XLIB_AVAILABLE_IN_ALL
const XVariantType *x_variant_type_first(const XVariantType *type);

XLIB_AVAILABLE_IN_ALL
const XVariantType *x_variant_type_next(const XVariantType *type);

XLIB_AVAILABLE_IN_ALL
xsize x_variant_type_n_items(const XVariantType *type);

XLIB_AVAILABLE_IN_ALL
const XVariantType *x_variant_type_key(const XVariantType *type);

XLIB_AVAILABLE_IN_ALL
const XVariantType *x_variant_type_value(const XVariantType *type);

XLIB_AVAILABLE_IN_ALL
XVariantType *x_variant_type_new_array(const XVariantType *element);

XLIB_AVAILABLE_IN_ALL
XVariantType *x_variant_type_new_maybe(const XVariantType *element);

XLIB_AVAILABLE_IN_ALL
XVariantType *x_variant_type_new_tuple(const XVariantType *const *items, xint length);

XLIB_AVAILABLE_IN_ALL
XVariantType *x_variant_type_new_dict_entry(const XVariantType *key, const XVariantType *value);

XLIB_AVAILABLE_IN_ALL
const XVariantType *x_variant_type_checked_(const xchar *type_string);

XLIB_AVAILABLE_IN_2_60
xsize x_variant_type_string_get_depth_(const xchar *type_string);

X_END_DECLS

#endif
