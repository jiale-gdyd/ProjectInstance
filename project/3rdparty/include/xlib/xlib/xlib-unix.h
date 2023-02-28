#ifndef __X_UNIX_H__
#define __X_UNIX_H__

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

#include "../xlib.h"

X_BEGIN_DECLS

#define X_UNIX_ERROR                    (x_unix_error_quark())

typedef xboolean (*XUnixFDSourceFunc)(xint fd, XIOCondition condition, xpointer user_data);

XLIB_AVAILABLE_IN_2_30
XQuark x_unix_error_quark(void);

XLIB_AVAILABLE_IN_2_30
xboolean x_unix_open_pipe(xint *fds, xint flags, XError **error);

XLIB_AVAILABLE_IN_2_30
xboolean x_unix_set_fd_nonblocking(xint fd, xboolean nonblock, XError **error);

XLIB_AVAILABLE_IN_2_30
XSource *x_unix_signal_source_new(xint signum);

XLIB_AVAILABLE_IN_2_30
xuint x_unix_signal_add_full(xint priority, xint signum, XSourceFunc handler, xpointer user_data, XDestroyNotify notify);

XLIB_AVAILABLE_IN_2_30
xuint x_unix_signal_add(xint signum, XSourceFunc handler, xpointer user_data);

XLIB_AVAILABLE_IN_2_36
XSource *x_unix_fd_source_new(xint fd, XIOCondition condition);

XLIB_AVAILABLE_IN_2_36
xuint x_unix_fd_add_full(xint priority, xint fd, XIOCondition condition, XUnixFDSourceFunc function, xpointer user_data, XDestroyNotify notify);

XLIB_AVAILABLE_IN_2_36
xuint x_unix_fd_add(xint fd, XIOCondition condition, XUnixFDSourceFunc function, xpointer user_data);

XLIB_AVAILABLE_IN_2_64
struct passwd *x_unix_get_passwd_entry(const xchar *user_name, XError **error);

X_END_DECLS

#endif
