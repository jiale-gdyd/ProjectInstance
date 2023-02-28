#ifndef __X_BASE64_H__
#define __X_BASE64_H__

#include "xtypes.h"

X_BEGIN_DECLS

XLIB_AVAILABLE_IN_ALL
xsize x_base64_encode_step(const xuchar *in, xsize len, xboolean break_lines, xchar *out, xint *state, xint *save);

XLIB_AVAILABLE_IN_ALL
xsize x_base64_encode_close(xboolean break_lines, xchar *out, xint *state, xint *save);

XLIB_AVAILABLE_IN_ALL
xchar *x_base64_encode(const xuchar *data, xsize len) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_ALL
xsize x_base64_decode_step(const xchar *in, xsize len, xuchar *out, xint *state, xuint *save);

XLIB_AVAILABLE_IN_ALL
xuchar *x_base64_decode(const xchar *text, xsize *out_len) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_ALL
xuchar *x_base64_decode_inplace(xchar *text, xsize *out_len);

X_END_DECLS

#endif
