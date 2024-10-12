#ifndef __X_UNIX_H__
#define __X_UNIX_H__

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

#include "../xlib.h"
#include "xstdio.h"

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

XLIB_AVAILABLE_TYPE_IN_2_80
typedef struct {
    int fds[2];
} XUnixPipe;

XLIB_AVAILABLE_TYPE_IN_2_80
typedef enum {
    X_UNIX_PIPE_END_READ = 0,
    X_UNIX_PIPE_END_WRITE = 1
} XUnixPipeEnd;

#define X_UNIX_PIPE_INIT        { { -1, -1 } } XLIB_AVAILABLE_MACRO_IN_2_80

X_GNUC_BEGIN_IGNORE_DEPRECATIONS

XLIB_AVAILABLE_STATIC_INLINE_IN_2_80
static inline xboolean x_unix_pipe_open(XUnixPipe *self, int flags, XError **error);

XLIB_AVAILABLE_STATIC_INLINE_IN_2_80
static inline xboolean x_unix_pipe_open(XUnixPipe *self, int flags, XError **error)
{
    return x_unix_open_pipe(self->fds, flags, error);
}

XLIB_AVAILABLE_STATIC_INLINE_IN_2_80
static inline int x_unix_pipe_get(XUnixPipe *self, XUnixPipeEnd end);

XLIB_AVAILABLE_STATIC_INLINE_IN_2_80
static inline int x_unix_pipe_get(XUnixPipe *self, XUnixPipeEnd end)
{
    return self->fds[end];
}

XLIB_AVAILABLE_STATIC_INLINE_IN_2_80
static inline int x_unix_pipe_steal(XUnixPipe *self, XUnixPipeEnd end);

XLIB_AVAILABLE_STATIC_INLINE_IN_2_80
static inline int x_unix_pipe_steal(XUnixPipe *self, XUnixPipeEnd end)
{
    return x_steal_fd(&self->fds[end]);
}

XLIB_AVAILABLE_STATIC_INLINE_IN_2_80
static inline xboolean x_unix_pipe_close(XUnixPipe *self, XUnixPipeEnd end, XError **error);

XLIB_AVAILABLE_STATIC_INLINE_IN_2_80
static inline xboolean x_unix_pipe_close(XUnixPipe *self, XUnixPipeEnd end, XError **error)
{
    return x_clear_fd(&self->fds[end], error);
}

XLIB_AVAILABLE_STATIC_INLINE_IN_2_80
static inline void x_unix_pipe_clear(XUnixPipe *self);

XLIB_AVAILABLE_STATIC_INLINE_IN_2_80
static inline void x_unix_pipe_clear(XUnixPipe *self)
{
    int errsv = errno;

    if (!x_unix_pipe_close(self, X_UNIX_PIPE_END_READ, NULL)) {

    }

    if (!x_unix_pipe_close(self, X_UNIX_PIPE_END_WRITE, NULL)) {

    }

    errno = errsv;
}

X_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(XUnixPipe, x_unix_pipe_clear)

XLIB_AVAILABLE_IN_2_80
int x_closefrom(int lowfd);

XLIB_AVAILABLE_IN_2_80
int x_fdwalk_set_cloexec(int lowfd);

X_GNUC_END_IGNORE_DEPRECATIONS

X_END_DECLS

#endif
