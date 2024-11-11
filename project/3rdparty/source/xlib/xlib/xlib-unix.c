#include <pwd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>

#include <xlib/xlib/config.h>
#include <xlib/xlib/xlibconfig.h>
#include <xlib/xlib/xlib-unix.h>
#include <xlib/xlib/xmain-internal.h>
#include <xlib/xlib/xlib-unixprivate.h>

#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

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

X_STATIC_ASSERT(O_NONBLOCK != FD_CLOEXEC);

X_DEFINE_QUARK(x-unix-error-quark, x_unix_error)

static xboolean x_unix_set_error_from_errno(XError **error, xint saved_errno)
{
    x_set_error_literal(error, X_UNIX_ERROR, 0, x_strerror(saved_errno));
    errno = saved_errno;
    return FALSE;
}

xboolean x_unix_open_pipe(int *fds, int flags, XError **error)
{
    x_return_val_if_fail((flags & (O_CLOEXEC | FD_CLOEXEC | O_NONBLOCK)) == flags, FALSE);

#if O_CLOEXEC != FD_CLOEXEC && !defined(X_DISABLE_CHECKS)
    if (flags & FD_CLOEXEC) {
        x_debug("x_unix_open_pipe() called with FD_CLOEXEC; please migrate to using O_CLOEXEC instead");
    }
#endif

    if (!x_unix_open_pipe_internal(fds, (flags & (O_CLOEXEC | FD_CLOEXEC)) != 0, (flags & O_NONBLOCK) != 0)) {
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
        fcntl_flags |= O_NONBLOCK;
    } else {
        fcntl_flags &= ~O_NONBLOCK;
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

static int set_cloexec(void *data, xint fd)
{
    if (fd >= XPOINTER_TO_INT(data)) {
        fcntl(fd, F_SETFD, FD_CLOEXEC);
    }

    return 0;
}

X_GNUC_UNUSED static int close_func_with_invalid_fds(void *data, int fd)
{
    if (fd >= XPOINTER_TO_INT(data)) {
        close(fd);
    }

    return 0;
}

struct linux_dirent64 {
    xuint64        d_ino;
    xuint64        d_off;
    unsigned short d_reclen;
    unsigned char  d_type;
    char           d_name[];
};

static xint filename_to_fd(const char *p)
{
    char c;
    int fd = 0;
    const int cutoff = X_MAXINT / 10;
    const int cutlim = X_MAXINT % 10;

    if (*p == '\0') {
        return -1;
    }

    while ((c = *p++) != '\0') {
        if (c < '0' || c > '9') {
            return -1;
        }
        c -= '0';

        if (fd > cutoff || (fd == cutoff && c > cutlim)) {
            return -1;
        }

        fd = fd * 10 + c;
    }

    return fd;
}

static int safe_fdwalk_with_invalid_fds(int (*cb)(void *data, int fd), void *data);

static int safe_fdwalk(int (*cb)(void *data, int fd), void *data)
{
#if 0
    return fdwalk(cb, data);
#else
    xint fd;
    xint res = 0;

    int dir_fd = open("/proc/self/fd", O_RDONLY | O_DIRECTORY);
    if (dir_fd >= 0) {
        union {
            char buf[4096];
            struct linux_dirent64 alignment;
        } u;
        int pos, nread;
        struct linux_dirent64 *de;

        while ((nread = syscall(SYS_getdents64, dir_fd, u.buf, sizeof(u.buf))) > 0) {
            for (pos = 0; pos < nread; pos += de->d_reclen) {
                de = (struct linux_dirent64 *)(u.buf + pos);

                fd = filename_to_fd (de->d_name);
                if (fd < 0 || fd == dir_fd) {
                    continue;
                }

                if ((res = cb(data, fd)) != 0) {
                    break;
                }
            }
        }

        x_close(dir_fd, NULL);
        return res;
    }

#if defined(__sun__) && defined(F_PREVFD) && defined(F_NEXTFD)
    xint fd;
    xint res = 0;
    xint open_max;

    open_max = fcntl(INT_MAX, F_PREVFD);
    if (open_max < 0) {
        return 0;
    }

    for (fd = -1; (fd = fcntl(fd, F_NEXTFD, open_max)) != -1; ) {
        if ((res = cb(data, fd)) != 0 || fd == open_max) {
            break;
        }
    }

    return res;
#endif

    return safe_fdwalk_with_invalid_fds(cb, data);
#endif
}

static int safe_fdwalk_with_invalid_fds(int (*cb)(void *data, int fd), void *data)
{
    xint fd;
    xint res = 0;
    xint open_max = -1;

#if 0 && defined(HAVE_SYS_RESOURCE_H)
    struct rlimit rl;

    if (getrlimit(RLIMIT_NOFILE, &rl) == 0 && rl.rlim_max != RLIM_INFINITY) {
        open_max = rl.rlim_max;
    }
#endif

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__APPLE__)
    if (open_max < 0) {
        open_max = sysconf(_SC_OPEN_MAX);
    }
#endif

    if (open_max < 0) {
        open_max = 4096;
    }

    for (fd = 0; fd < open_max; fd++) {
        if ((res = cb(data, fd)) != 0) {
            break;
        }
    }

    return res;
}

int x_fdwalk_set_cloexec(int lowfd)
{
    int ret;

    x_return_val_if_fail(lowfd >= 0, (errno = EINVAL, -1));

#if defined(HAVE_CLOSE_RANGE) && defined(CLOSE_RANGE_CLOEXEC)
    ret = close_range(lowfd, X_MAXUINT, CLOSE_RANGE_CLOEXEC);
    if (ret == 0 || !(errno == ENOSYS || errno == EINVAL)) {
        return ret;
    }
#endif

    ret = safe_fdwalk(set_cloexec, XINT_TO_POINTER(lowfd));
    return ret;
}

int x_closefrom(int lowfd)
{
    int ret;

    x_return_val_if_fail(lowfd >= 0, (errno = EINVAL, -1));

#if defined(HAVE_CLOSE_RANGE)
    ret = close_range(lowfd, X_MAXUINT, 0);
    if (ret == 0 || errno != ENOSYS) {
        return ret;
    }
#endif

#if defined(__FreeBSD__) || defined(__OpenBSD__) || (defined(__sun__) && defined(F_CLOSEFROM))
    (void)closefrom(lowfd);
    return 0;
#elif defined(__DragonFly__)
    (void)syscall(SYS_closefrom, lowfd);
    return 0;
#elif defined(F_CLOSEM)
    return fcntl(lowfd, F_CLOSEM);
#else
    ret = safe_fdwalk(close_func_with_invalid_fds, XINT_TO_POINTER(lowfd));
    return ret;
#endif
}
