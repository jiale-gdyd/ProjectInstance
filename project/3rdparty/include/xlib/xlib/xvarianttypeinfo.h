#ifndef __X_VARIANT_TYPE_INFO_H__
#define __X_VARIANT_TYPE_INFO_H__

#include "xvarianttype.h"

#define X_VARIANT_TYPE_INFO_CHAR_MAYBE                  'm'
#define X_VARIANT_TYPE_INFO_CHAR_ARRAY                  'a'
#define X_VARIANT_TYPE_INFO_CHAR_TUPLE                  '('
#define X_VARIANT_TYPE_INFO_CHAR_DICT_ENTRY             '{'
#define X_VARIANT_TYPE_INFO_CHAR_VARIANT                'v'
#define x_variant_type_info_get_type_char(info)         (x_variant_type_info_get_type_string(info)[0])

typedef struct _XVariantTypeInfo XVariantTypeInfo;

typedef struct {
    XVariantTypeInfo *type_info;
    xsize            i, a;
    xint8            b, c;
    xuint8           ending_type;
} XVariantMemberInfo;

#define X_VARIANT_MEMBER_ENDING_FIXED                   0
#define X_VARIANT_MEMBER_ENDING_LAST                    1
#define X_VARIANT_MEMBER_ENDING_OFFSET                  2

XLIB_AVAILABLE_IN_ALL
const xchar *x_variant_type_info_get_type_string(XVariantTypeInfo *typeinfo);

XLIB_AVAILABLE_IN_ALL
void x_variant_type_info_query(XVariantTypeInfo *typeinfo, xuint *alignment, xsize *size);

XLIB_AVAILABLE_IN_2_60
xsize x_variant_type_info_query_depth(XVariantTypeInfo *typeinfo);

XLIB_AVAILABLE_IN_ALL
XVariantTypeInfo *x_variant_type_info_element(XVariantTypeInfo *typeinfo);

XLIB_AVAILABLE_IN_ALL
void x_variant_type_info_query_element(XVariantTypeInfo *typeinfo, xuint *alignment, xsize *size);

XLIB_AVAILABLE_IN_ALL
xsize x_variant_type_info_n_members(XVariantTypeInfo *typeinfo);

XLIB_AVAILABLE_IN_ALL
const XVariantMemberInfo *x_variant_type_info_member_info(XVariantTypeInfo *typeinfo, xsize index);

XLIB_AVAILABLE_IN_ALL
XVariantTypeInfo *x_variant_type_info_get(const XVariantType *type);

XLIB_AVAILABLE_IN_ALL
XVariantTypeInfo *x_variant_type_info_ref(XVariantTypeInfo *typeinfo);

XLIB_AVAILABLE_IN_ALL
void x_variant_type_info_unref(XVariantTypeInfo *typeinfo);

XLIB_AVAILABLE_IN_ALL
void x_variant_type_info_assert_no_infos(void);

#endif
