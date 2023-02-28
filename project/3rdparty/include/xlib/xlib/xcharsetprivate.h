#ifndef __X_CHARSET_PRIVATE_H__
#define __X_CHARSET_PRIVATE_H__

#include "xcharset.h"

X_BEGIN_DECLS

const char **_x_charset_get_aliases(const char *canonical_name);

xboolean _x_get_time_charset(const char **charset);
xboolean _x_get_ctype_charset(const char **charset);

X_END_DECLS

#endif
