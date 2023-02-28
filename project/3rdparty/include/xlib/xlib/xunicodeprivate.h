
#ifndef __X_UNICODE_PRIVATE_H__
#define __X_UNICODE_PRIVATE_H__

#include "xtypes.h"
#include "xunicode.h"

X_BEGIN_DECLS

xunichar *_x_utf8_normalize_wc(const xchar *str, xssize max_len, XNormalizeMode mode);

X_END_DECLS

#endif
