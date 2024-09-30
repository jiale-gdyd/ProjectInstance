#ifndef __X_VARIANT_CORE_H__
#define __X_VARIANT_CORE_H__

#include "xbytes.h"
#include "xvariant.h"
#include "xvarianttypeinfo.h"

#if XLIB_SIZEOF_VOID_P == 8
# define X_VARIANT_MAX_PREALLOCATED         64
#else
# define X_VARIANT_MAX_PREALLOCATED         32
#endif

XVariant *x_variant_new_preallocated_trusted(const XVariantType *type, xconstpointer data, xsize size);

XVariant *x_variant_new_take_bytes(const XVariantType *type, XBytes *bytes, xboolean trusted);

XVariant *x_variant_new_from_children(const XVariantType *type, XVariant **children, xsize n_children, xboolean trusted);

xboolean x_variant_is_trusted(XVariant *value);

XVariantTypeInfo *x_variant_get_type_info(XVariant *value);

xsize x_variant_get_depth(XVariant *value);

XVariant *x_variant_maybe_get_child_value(XVariant *value, xsize index_);

#endif
