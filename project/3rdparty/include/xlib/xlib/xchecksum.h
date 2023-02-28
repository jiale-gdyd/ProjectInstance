#ifndef __X_CHECKSUM_H__
#define __X_CHECKSUM_H__

#include "xtypes.h"
#include "xbytes.h"

X_BEGIN_DECLS

typedef enum {
    X_CHECKSUM_MD5,
    X_CHECKSUM_SHA1,
    X_CHECKSUM_SHA256,
    X_CHECKSUM_SHA512,
    X_CHECKSUM_SHA384
} XChecksumType;

typedef struct _XChecksum XChecksum;

XLIB_AVAILABLE_IN_ALL
xssize x_checksum_type_get_length(XChecksumType checksum_type);

XLIB_AVAILABLE_IN_ALL
XChecksum *x_checksum_new(XChecksumType checksum_type);

XLIB_AVAILABLE_IN_ALL
void x_checksum_reset(XChecksum *checksum);

XLIB_AVAILABLE_IN_ALL
XChecksum *x_checksum_copy(const XChecksum *checksum);

XLIB_AVAILABLE_IN_ALL
void x_checksum_free(XChecksum *checksum);

XLIB_AVAILABLE_IN_ALL
void x_checksum_update(XChecksum *checksum, const xuchar *data, xssize length);

XLIB_AVAILABLE_IN_ALL
const xchar *x_checksum_get_string(XChecksum *checksum);

XLIB_AVAILABLE_IN_ALL
void x_checksum_get_digest(XChecksum *checksum, xuint8 *buffer, xsize *digest_len);

XLIB_AVAILABLE_IN_ALL
xchar *x_compute_checksum_for_data(XChecksumType checksum_type, const xuchar *data, xsize length);

XLIB_AVAILABLE_IN_ALL
xchar *x_compute_checksum_for_string(XChecksumType checksum_type, const xchar *str, xssize length);

XLIB_AVAILABLE_IN_2_34
xchar *x_compute_checksum_for_bytes(XChecksumType checksum_type, XBytes *data);

X_END_DECLS

#endif
