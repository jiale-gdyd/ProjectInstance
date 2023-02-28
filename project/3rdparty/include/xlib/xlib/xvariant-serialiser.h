#ifndef __X_VARIANT_SERIALISER_H__
#define __X_VARIANT_SERIALISER_H__

#include "xvarianttypeinfo.h"

typedef struct {
    XVariantTypeInfo *type_info;
    xuchar           *data;
    xsize            size;
    xsize            depth;
    xsize            ordered_offsets_up_to;
    xsize            checked_offsets_up_to;
} XVariantSerialised;

typedef void (*XVariantSerialisedFiller)(XVariantSerialised *serialised, xpointer data);

XLIB_AVAILABLE_IN_ALL
xsize x_variant_serialised_n_children(XVariantSerialised container);

XLIB_AVAILABLE_IN_ALL
XVariantSerialised x_variant_serialised_get_child(XVariantSerialised container, xsize index);

XLIB_AVAILABLE_IN_ALL
xsize x_variant_serialiser_needed_size(XVariantTypeInfo *info, XVariantSerialisedFiller gsv_filler, const xpointer *children, xsize n_children);

XLIB_AVAILABLE_IN_ALL
void x_variant_serialiser_serialise(XVariantSerialised container, XVariantSerialisedFiller gsv_filler, const xpointer *children, xsize n_children);

XLIB_AVAILABLE_IN_2_60
xboolean x_variant_serialised_check(XVariantSerialised serialised);

XLIB_AVAILABLE_IN_ALL
xboolean x_variant_serialised_is_normal(XVariantSerialised value);

XLIB_AVAILABLE_IN_ALL
void x_variant_serialised_byteswap(XVariantSerialised value);

XLIB_AVAILABLE_IN_ALL
xboolean x_variant_serialiser_is_string(xconstpointer data, xsize size);

XLIB_AVAILABLE_IN_ALL
xboolean x_variant_serialiser_is_object_path(xconstpointer data, xsize size);

XLIB_AVAILABLE_IN_ALL
xboolean x_variant_serialiser_is_signature(xconstpointer data, xsize size);

#endif
