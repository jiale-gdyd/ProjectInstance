#ifndef __X_BYTES_H__
#define __X_BYTES_H__

#include "xtypes.h"
#include "xarray.h"

X_BEGIN_DECLS

XLIB_AVAILABLE_IN_ALL
XBytes *x_bytes_new(xconstpointer data, xsize size);

XLIB_AVAILABLE_IN_ALL
XBytes *x_bytes_new_take(xpointer data, xsize size);

XLIB_AVAILABLE_IN_ALL
XBytes *x_bytes_new_static(xconstpointer data, xsize size);

XLIB_AVAILABLE_IN_ALL
XBytes *x_bytes_new_with_free_func(xconstpointer data, xsize size, XDestroyNotify free_func, xpointer user_data);

XLIB_AVAILABLE_IN_ALL
XBytes *x_bytes_new_from_bytes(XBytes *bytes, xsize offset, xsize length);

XLIB_AVAILABLE_IN_ALL
xconstpointer x_bytes_get_data(XBytes *bytes, xsize *size);

XLIB_AVAILABLE_IN_ALL
xsize x_bytes_get_size(XBytes *bytes);

XLIB_AVAILABLE_IN_ALL
XBytes *x_bytes_ref(XBytes *bytes);

XLIB_AVAILABLE_IN_ALL
void x_bytes_unref(XBytes *bytes);

XLIB_AVAILABLE_IN_ALL
xpointer x_bytes_unref_to_data(XBytes *bytes, xsize *size);

XLIB_AVAILABLE_IN_ALL
XByteArray *x_bytes_unref_to_array(XBytes *bytes);

XLIB_AVAILABLE_IN_ALL
xuint x_bytes_hash(xconstpointer bytes);

XLIB_AVAILABLE_IN_ALL
xboolean x_bytes_equal(xconstpointer bytes1, xconstpointer bytes2);

XLIB_AVAILABLE_IN_ALL
xint x_bytes_compare(xconstpointer bytes1, xconstpointer bytes2);

XLIB_AVAILABLE_IN_2_70
xconstpointer x_bytes_get_region(XBytes *bytes, xsize element_size, xsize offset, xsize n_elements);

X_END_DECLS

#endif
