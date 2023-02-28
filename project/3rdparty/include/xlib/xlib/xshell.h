#ifndef __X_SHELL_H__
#define __X_SHELL_H__

#include "xerror.h"
#include "xtypes.h"

X_BEGIN_DECLS

#define X_SHELL_ERROR       x_shell_error_quark()

typedef enum {
    X_SHELL_ERROR_BAD_QUOTING,
    X_SHELL_ERROR_EMPTY_STRING,
    X_SHELL_ERROR_FAILED
} XShellError;

XLIB_AVAILABLE_IN_ALL
XQuark x_shell_error_quark(void);

XLIB_AVAILABLE_IN_ALL
xchar *x_shell_quote(const xchar *unquoted_string);

XLIB_AVAILABLE_IN_ALL
xchar *x_shell_unquote(const xchar *quoted_string, XError **error);

XLIB_AVAILABLE_IN_ALL
xboolean x_shell_parse_argv(const xchar *command_line, xint *argcp, xchar ***argvp, XError **error);

X_END_DECLS

#endif
