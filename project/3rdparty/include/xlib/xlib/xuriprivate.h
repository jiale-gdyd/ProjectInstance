#ifndef __X_URI_PRIVATE_H__
#define __X_URI_PRIVATE_H__

#include "xtypes.h"

X_BEGIN_DECLS

void _uri_encoder(XString *out, const xuchar *start, xsize length, const xchar *reserved_chars_allowed, xboolean allow_utf8);

X_END_DECLS

#endif
