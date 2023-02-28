#ifndef __X_HOST_UTILS_H__
#define __X_HOST_UTILS_H__

#include "xtypes.h"

X_BEGIN_DECLS

XLIB_AVAILABLE_IN_ALL
xboolean x_hostname_is_non_ascii(const xchar *hostname);

XLIB_AVAILABLE_IN_ALL
xboolean x_hostname_is_ascii_encoded(const xchar *hostname);

XLIB_AVAILABLE_IN_ALL
xboolean x_hostname_is_ip_address(const xchar *hostname);

XLIB_AVAILABLE_IN_ALL
xchar *x_hostname_to_ascii(const xchar *hostname);

XLIB_AVAILABLE_IN_ALL
xchar *x_hostname_to_unicode(const xchar *hostname);

X_END_DECLS

#endif
