#ifndef __X_VARIANT_INTERNAL_H__
#define __X_VARIANT_INTERNAL_H__

#define __XLIB_H_INSIDE__
#include "xtypes.h"
#include "xvarianttype.h"
#include "xvarianttypeinfo.h"
#include "xvariant-serialiser.h"
#undef __XLIB_H_INSIDE__

#define X_VARIANT_MAX_RECURSION_DEPTH       ((xsize)128)

XLIB_AVAILABLE_IN_ALL
xboolean x_variant_format_string_scan(const xchar *string, const xchar *limit, const xchar **endptr);

XLIB_AVAILABLE_IN_ALL
XVariantType *x_variant_format_string_scan_type(const xchar *string, const xchar *limit, const xchar **endptr);

#endif
