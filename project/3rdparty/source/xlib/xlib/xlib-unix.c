#include <pwd.h>
#include <string.h>
#include <sys/types.h>

#include <xlib/xlib/config.h>
#include <xlib/xlib/xlibconfig.h>
#include <xlib/xlib/xlib-unix.h>
#include <xlib/xlib/xmain-internal.h>
#include <xlib/xlib/xlib-unixprivate.h>

typedef struct { char a; XPid b; } XPidAlign;
typedef struct { char a; pid_t b; } pid_tAlign;

typedef struct { char a; xssize b; } xssizeAlign;
typedef struct { char a; ssize_t b; } ssize_tAlign;

X_STATIC_ASSERT(sizeof(ssize_t) == XLIB_SIZEOF_SSIZE_T);
// X_STATIC_ASSERT(X_ALIGNOF(xssize) == X_ALIGNOF(ssize_t));
X_STATIC_ASSERT(X_STRUCT_OFFSET(xssizeAlign, b) == X_STRUCT_OFFSET(ssize_tAlign, b));

X_STATIC_ASSERT(sizeof(XPid) == sizeof(pid_t));
// X_STATIC_ASSERT(X_ALIGNOF(XPid) == X_ALIGNOF(pid_t));
X_STATIC_ASSERT(X_STRUCT_OFFSET(XPidAlign, b) == X_STRUCT_OFFSET(pid_tAlign, b));

X_DEFINE_QUARK(x-unix-error-quark, x_unix_error)

static xboolean x_unix_set_error_from_errno(XError **error, xint saved_errno)
{
    x_set_error_literal(error, X_UNIX_ERROR, 0, x_strerror(saved_errno));
    errno = saved_errno;
    return FALSE;
}

xboolean x_unix_open_pipe(int *fds, int flags, XError **error)
{
    x_return_val_if_fail((flags & (FD_CLOEXEC)) == flags, FALSE);

    if (!x_unix_open_pipe_internal(fds, (flags & FD_CLOEXEC) != 0)) {
        return x_unix_set_error_from_errno(error, errno);
    }

    return TRUE;
}

xboolean x_unix_set_fd_nonblocking(xint fd, xboolean nonblock, XError **error)
{
#ifdef F_GETFL
    xlong fcntl_flags;
    fcntl_flags = fcntl(fd, F_GETFL);

    if (fcntl_flags == -1) {
        return x_unix_set_error_from_errno(error, errno);
    }

    if (nonblock) {
#ifdef O_NONBLOCK
        fcntl_flags |= O_NONBLOCK;
#else
        fcntl_flags |= O_NDELAY;
#endif
    } else {
#ifdef O_NONBLOCK
        fcntl_flags &= ~O_NONBLOCK;
#else
        fcntl_flags &= ~O_NDELAY;
#endif
    }

    if (fcntl(fd, F_SETFL, fcntl_flags) == -1) {
        return x_unix_set_error_from_errno(error, errno);
    }

    return TRUE;
#else
    return x_unix_set_error_from_errno(error, EINVAL);
#endif
}

XSource *x_unix_signal_source_new (int signum)
{
    x_return_val_if_fail(signum == SIGHUP || signum == SIGINT || signum == SIGTERM || signum == SIGUSR1 || signum == SIGUSR2 || signum == SIGWINCH, NULL);
    return _x_main_create_unix_signal_watch(signum);
}

xuint x_unix_signal_add_full(int priority, int signum, XSourceFunc handler, xpointer user_data, XDestroyNotify notify)
{
    xuint id;
    XSource *source;

    source = x_unix_signal_source_new(signum);

    if (priority != X_PRIORITY_DEFAULT) {
        x_source_set_priority(source, priority);
    }

    x_source_set_callback(source, handler, user_data, notify);
    id = x_source_attach(source, NULL);
    x_source_unref(source);

    return id;
}

xuint x_unix_signal_add(int signum, XSourceFunc handler, xpointer user_data)
{
    return x_unix_signal_add_full(X_PRIORITY_DEFAULT, signum, handler, user_data, NULL);
}

typedef struct {
    XSource  source;
    xint     fd;
    xpointer tag;
} XUnixFDSource;

static xboolean x_unix_fd_source_dispatch(XSource *source, XSourceFunc callback, xpointer user_data)
{
    XUnixFDSource *fd_source = (XUnixFDSource *)source;
    XUnixFDSourceFunc func = (XUnixFDSourceFunc)callback;

    if (!callback) {
        x_warning("XUnixFDSource dispatched without callback. You must call x_source_set_callback().");
        return FALSE;
    }

    return (*func)(fd_source->fd, x_source_query_unix_fd(source, fd_source->tag), user_data);
}

XSourceFuncs x_unix_fd_source_funcs = { NULL, NULL, x_unix_fd_source_dispatch, NULL, NULL, NULL };

XSource *x_unix_fd_source_new(xint fd, XIOCondition condition)
{
    XSource *source;
    XUnixFDSource *fd_source;

    source = x_source_new(&x_unix_fd_source_funcs, sizeof(XUnixFDSource));
    fd_source = (XUnixFDSource *) source;

    fd_source->fd = fd;
    fd_source->tag = x_source_add_unix_fd(source, fd, condition);

    return source;
}

xuint x_unix_fd_add_full(xint priority, xint fd, XIOCondition condition, XUnixFDSourceFunc function, xpointer user_data, XDestroyNotify notify)
{
    xuint id;
    XSource *source;

    x_return_val_if_fail(function != NULL, 0);

    source = x_unix_fd_source_new(fd, condition);
    if (priority != X_PRIORITY_DEFAULT) {
        x_source_set_priority(source, priority);
    }

    x_source_set_callback(source, (XSourceFunc)function, user_data, notify);
    id = x_source_attach(source, NULL);
    x_source_unref(source);

    return id;
}

xuint x_unix_fd_add(xint fd, XIOCondition condition, XUnixFDSourceFunc function, xpointer user_data)
{
    return x_unix_fd_add_full(X_PRIORITY_DEFAULT, fd, condition, function, user_data, NULL);
}

struct passwd *x_unix_get_passwd_entry(const xchar *user_name, XError **error)
{
    struct localSt {
        struct passwd pwd;
        char   string_buffer[];
    };

    XError *local_error = NULL;
    xsize string_buffer_size = 0;
    struct localSt *buffer = NULL;
    struct passwd *passwd_file_entry;

    x_return_val_if_fail(user_name != NULL, NULL);
    x_return_val_if_fail(error == NULL || *error == NULL, NULL);

#ifdef _SC_GETPW_R_SIZE_MAX
    {
        xlong string_buffer_size_long = sysconf(_SC_GETPW_R_SIZE_MAX);
        if (string_buffer_size_long > 0) {
            string_buffer_size = string_buffer_size_long;
        }
    }
#endif

    if (string_buffer_size == 0) {
        string_buffer_size = 64;
    }

    do {
        int retval;

        x_free(buffer);

        buffer = (struct localSt *)x_malloc0(sizeof(*buffer) + string_buffer_size + 6);
        retval = getpwnam_r(user_name, &buffer->pwd, buffer->string_buffer, string_buffer_size, &passwd_file_entry);
        if (passwd_file_entry != NULL) {
            break;
        } else if (retval == 0 || retval == ENOENT || retval == ESRCH || retval == EBADF || retval == EPERM) {
            x_unix_set_error_from_errno(&local_error, retval);
            break;
        } else if (retval == ERANGE) {
            if (string_buffer_size > 32 * 1024) {
                x_unix_set_error_from_errno(&local_error, retval);
                break;
            }

            string_buffer_size *= 2;
            continue;
        } else {
            x_unix_set_error_from_errno(&local_error, retval);
            break;
        }
    } while (passwd_file_entry == NULL);

    x_assert(passwd_file_entry == NULL || (xpointer)passwd_file_entry == (xpointer)buffer);

    if (local_error != NULL) {
        x_clear_pointer(&buffer, x_free);
        x_propagate_error(error, x_steal_pointer(&local_error));
    }

    return (struct passwd *)x_steal_pointer(&buffer);
}
