#ifndef __X_ENVIRON_H__
#define __X_ENVIRON_H__

#include "xtypes.h"

X_BEGIN_DECLS

XLIB_AVAILABLE_IN_ALL
const xchar *x_getenv(const xchar *variable);

XLIB_AVAILABLE_IN_ALL
xboolean x_setenv(const xchar *variable, const xchar *value, xboolean overwrite);

XLIB_AVAILABLE_IN_ALL
void x_unsetenv(const xchar *variable);

XLIB_AVAILABLE_IN_ALL
xchar **x_listenv(void);

XLIB_AVAILABLE_IN_ALL
xchar **x_get_environ(void);

XLIB_AVAILABLE_IN_ALL
const xchar *x_environ_getenv(xchar **envp, const xchar *variable);

XLIB_AVAILABLE_IN_ALL
xchar **x_environ_setenv(xchar **envp, const xchar *variable, const xchar *value, xboolean overwrite) X_GNUC_WARN_UNUSED_RESULT;

XLIB_AVAILABLE_IN_ALL
xchar **x_environ_unsetenv(xchar **envp, const xchar *variable) X_GNUC_WARN_UNUSED_RESULT;

X_END_DECLS

#endif
