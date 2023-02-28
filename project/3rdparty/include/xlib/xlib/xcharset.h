#ifndef __X_CHARSET_H__
#define __X_CHARSET_H__

#include "xtypes.h"

X_BEGIN_DECLS

XLIB_AVAILABLE_IN_ALL
xboolean x_get_charset(const char **charset);

XLIB_AVAILABLE_IN_ALL
xchar *x_get_codeset(void);

XLIB_AVAILABLE_IN_2_62
xboolean x_get_console_charset(const char **charset);

XLIB_AVAILABLE_IN_ALL
const xchar *const *x_get_language_names(void);

XLIB_AVAILABLE_IN_2_58
const xchar *const *x_get_language_names_with_category(const xchar *category_name);

XLIB_AVAILABLE_IN_ALL
xchar **x_get_locale_variants(const xchar *locale);

X_END_DECLS

#endif
