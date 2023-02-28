#ifndef __X_HMAC_H__
#define __X_HMAC_H__

#include "xtypes.h"
#include "xchecksum.h"

X_BEGIN_DECLS

typedef struct _XHmac XHmac;

XLIB_AVAILABLE_IN_2_30
XHmac *x_hmac_new(XChecksumType digest_type, const xuchar *key, xsize key_len);

XLIB_AVAILABLE_IN_2_30
XHmac *x_hmac_copy(const XHmac *hmac);

XLIB_AVAILABLE_IN_2_30
XHmac *x_hmac_ref(XHmac *hmac);

XLIB_AVAILABLE_IN_2_30
void x_hmac_unref(XHmac *hmac);

XLIB_AVAILABLE_IN_2_30
void x_hmac_update(XHmac *hmac, const xuchar *data, xssize length);

XLIB_AVAILABLE_IN_2_30
const xchar *x_hmac_get_string(XHmac *hmac);

XLIB_AVAILABLE_IN_2_30
void x_hmac_get_digest(XHmac *hmac, xuint8 *buffer, xsize *digest_len);

XLIB_AVAILABLE_IN_2_30
xchar *x_compute_hmac_for_data(XChecksumType digest_type, const xuchar *key, xsize key_len, const xuchar *data, xsize length);

XLIB_AVAILABLE_IN_2_30
xchar *x_compute_hmac_for_string(XChecksumType digest_type, const xuchar *key, xsize key_len, const xchar *str, xssize length);

XLIB_AVAILABLE_IN_2_50
xchar *x_compute_hmac_for_bytes(XChecksumType digest_type, XBytes *key, XBytes *data);

X_END_DECLS

#endif
