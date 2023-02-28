#define _POSIX_SOURCE

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifndef O_CLOEXEC
#define O_CLOEXEC       0
#endif

#include <xlib/xlib/config.h>
#include <xlib/xlib/xstdio.h>
#include <xlib/xlib/xerror.h>
#include <xlib/xlib/xstrfuncs.h>
#include <xlib/xlib/xfileutils.h>
#include <xlib/xlib/xiochannel.h>
#include <xlib/xlib/xtestutils.h>

typedef struct _XIOUnixWatch XIOUnixWatch;
typedef struct _XIOUnixChannel XIOUnixChannel;

struct _XIOUnixChannel {
    XIOChannel channel;
    xint       fd;
};

struct _XIOUnixWatch {
    XSource      source;
    XPollFD      pollfd;
    XIOChannel   *channel;
    XIOCondition condition;
};

static XIOStatus x_io_unix_read(XIOChannel *channel, xchar *buf, xsize count, xsize *bytes_read, XError **err);
static XIOStatus x_io_unix_write(XIOChannel *channel, const xchar *buf, xsize count, xsize *bytes_written, XError **err);

static XIOStatus x_io_unix_close(XIOChannel *channel, XError **err);
static XIOStatus x_io_unix_seek(XIOChannel *channel, xint64 offset, XSeekType type, XError **err);

static void x_io_unix_free(XIOChannel *channel);
static XSource *x_io_unix_create_watch(XIOChannel *channel, XIOCondition condition);

static XIOFlags x_io_unix_get_flags(XIOChannel *channel);
static XIOStatus x_io_unix_set_flags(XIOChannel *channel, XIOFlags flags, XError **err);

static void x_io_unix_finalize(XSource *source);
static xboolean x_io_unix_check(XSource *source);
static xboolean x_io_unix_prepare(XSource *source, xint *timeout);
static xboolean x_io_unix_dispatch(XSource *source, XSourceFunc callback, xpointer user_data);

XSourceFuncs x_io_watch_funcs = {
    x_io_unix_prepare,
    x_io_unix_check,
    x_io_unix_dispatch,
    x_io_unix_finalize,
    NULL, NULL
};

static XIOFuncs unix_channel_funcs = {
    x_io_unix_read,
    x_io_unix_write,
    x_io_unix_seek,
    x_io_unix_close,
    x_io_unix_create_watch,
    x_io_unix_free,
    x_io_unix_set_flags,
    x_io_unix_get_flags,
};

static xboolean x_io_unix_prepare(XSource *source, xint *timeout)
{
    XIOUnixWatch *watch = (XIOUnixWatch *)source;
    XIOCondition buffer_condition = x_io_channel_get_buffer_condition(watch->channel);

    *timeout = -1;
    return ((watch->condition & buffer_condition) == watch->condition);
}

static xboolean x_io_unix_check(XSource *source)
{
    XIOUnixWatch *watch = (XIOUnixWatch *)source;
    XIOCondition buffer_condition = x_io_channel_get_buffer_condition(watch->channel);
    XIOCondition poll_condition = (XIOCondition)watch->pollfd.revents;

    return ((poll_condition | buffer_condition) & watch->condition);
}

static xboolean x_io_unix_dispatch (XSource *source, XSourceFunc callback, xpointer user_data)
{
    XIOFunc func = (XIOFunc)callback;
    XIOUnixWatch *watch = (XIOUnixWatch *)source;
    XIOCondition buffer_condition = x_io_channel_get_buffer_condition(watch->channel);

    if (!func) {
        x_warning("IO watch dispatched without callback. You must call x_source_connect().");
        return FALSE;
    }

    return (*func)(watch->channel, (XIOCondition)((watch->pollfd.revents | buffer_condition) & watch->condition), user_data);
}

static void x_io_unix_finalize(XSource *source)
{
    XIOUnixWatch *watch = (XIOUnixWatch *)source;
    x_io_channel_unref(watch->channel);
}

static XIOStatus x_io_unix_read(XIOChannel *channel, xchar *buf,  xsize count, xsize *bytes_read, XError **err)
{
    xssize result;
    XIOUnixChannel *unix_channel = (XIOUnixChannel *)channel;

    if (count > SSIZE_MAX) {
        count = SSIZE_MAX;
    }

retry:
    result = read(unix_channel->fd, buf, count);
    if (result < 0) {
        int errsv = errno;
        *bytes_read = 0;

        switch (errsv) {
#ifdef EINTR
            case EINTR:
                goto retry;
#endif
#ifdef EAGAIN
            case EAGAIN:
                return X_IO_STATUS_AGAIN;
#endif
            default:
                x_set_error_literal(err, X_IO_CHANNEL_ERROR, x_io_channel_error_from_errno(errsv), x_strerror(errsv));
                return X_IO_STATUS_ERROR;
        }
    }

    *bytes_read = result;
    return (result > 0) ? X_IO_STATUS_NORMAL : X_IO_STATUS_EOF;
}

static XIOStatus x_io_unix_write(XIOChannel *channel, const xchar *buf,  xsize count, xsize *bytes_written, XError **err)
{
    xssize result;
    XIOUnixChannel *unix_channel = (XIOUnixChannel *)channel;

retry:
    result = write (unix_channel->fd, buf, count);
    if (result < 0) {
        int errsv = errno;
        *bytes_written = 0;

        switch (errsv) {
#ifdef EINTR
            case EINTR:
                goto retry;
#endif
#ifdef EAGAIN
            case EAGAIN:
                return X_IO_STATUS_AGAIN;
#endif
            default:
                x_set_error_literal(err, X_IO_CHANNEL_ERROR, x_io_channel_error_from_errno(errsv), x_strerror(errsv));
                return X_IO_STATUS_ERROR;
        }
    }

    *bytes_written = result;
    return X_IO_STATUS_NORMAL;
}

static XIOStatus x_io_unix_seek(XIOChannel *channel, xint64 offset, XSeekType type, XError **err)
{
    int whence;
    off_t result;
    off_t tmp_offset;
    XIOUnixChannel *unix_channel = (XIOUnixChannel *)channel;

    switch (type) {
        case X_SEEK_SET:
            whence = SEEK_SET;
            break;

        case X_SEEK_CUR:
            whence = SEEK_CUR;
            break;

        case X_SEEK_END:
            whence = SEEK_END;
            break;

        default:
            whence = -1;
            x_assert_not_reached();
    }

    tmp_offset = offset;
    if (tmp_offset != offset) {
        x_set_error_literal(err, X_IO_CHANNEL_ERROR, x_io_channel_error_from_errno(EINVAL), x_strerror(EINVAL));
        return X_IO_STATUS_ERROR;
    }

    result = lseek(unix_channel->fd, tmp_offset, whence);
    if (result < 0) {
        int errsv = errno;
        x_set_error_literal(err, X_IO_CHANNEL_ERROR, x_io_channel_error_from_errno(errsv), x_strerror(errsv));
        return X_IO_STATUS_ERROR;
    }

    return X_IO_STATUS_NORMAL;
}

static XIOStatus x_io_unix_close(XIOChannel *channel, XError **err)
{
    XIOUnixChannel *unix_channel = (XIOUnixChannel *)channel;

    if (close(unix_channel->fd) < 0) {
        int errsv = errno;
        x_set_error_literal(err, X_IO_CHANNEL_ERROR, x_io_channel_error_from_errno(errsv), x_strerror(errsv));
        return X_IO_STATUS_ERROR;
    }

    return X_IO_STATUS_NORMAL;
}

static void x_io_unix_free(XIOChannel *channel)
{
    XIOUnixChannel *unix_channel = (XIOUnixChannel *)channel;
    x_free(unix_channel);
}

static XSource *x_io_unix_create_watch (XIOChannel *channel, XIOCondition condition)
{
    XSource *source;
    XIOUnixWatch *watch;
    XIOUnixChannel *unix_channel = (XIOUnixChannel *)channel;

    source = x_source_new(&x_io_watch_funcs, sizeof(XIOUnixWatch));
    x_source_set_static_name(source, "XIOChannel (Unix)");
    watch = (XIOUnixWatch *)source;

    watch->channel = channel;
    x_io_channel_ref(channel);

    watch->condition = condition;
    watch->pollfd.fd = unix_channel->fd;
    watch->pollfd.events = condition;

    x_source_add_poll(source, &watch->pollfd);

    return source;
}

static XIOStatus x_io_unix_set_flags(XIOChannel *channel, XIOFlags flags, XError **err)
{
    xlong fcntl_flags;
    XIOUnixChannel *unix_channel = (XIOUnixChannel *)channel;

    fcntl_flags = 0;

    if (flags & X_IO_FLAG_APPEND) {
        fcntl_flags |= O_APPEND;
    }

    if (flags & X_IO_FLAG_NONBLOCK) {
#ifdef O_NONBLOCK
        fcntl_flags |= O_NONBLOCK;
#else
        fcntl_flags |= O_NDELAY;
#endif
    }

    if (fcntl(unix_channel->fd, F_SETFL, fcntl_flags) == -1) {
        int errsv = errno;
        x_set_error_literal(err, X_IO_CHANNEL_ERROR, x_io_channel_error_from_errno(errsv), x_strerror(errsv));
        return X_IO_STATUS_ERROR;
    }

    return X_IO_STATUS_NORMAL;
}

static XIOFlags x_io_unix_get_flags(XIOChannel *channel)
{
    xlong fcntl_flags;
    XIOFlags flags = (XIOFlags)X_IO_FLAG_NONE;
    XIOUnixChannel *unix_channel = (XIOUnixChannel *)channel;

    fcntl_flags = fcntl(unix_channel->fd, F_GETFL);
    if (fcntl_flags == -1) {
        int err = errno;
        x_warning(X_STRLOC "Error while getting flags for FD: %s (%d)", x_strerror(err), err);
        return (XIOFlags)0;
    }

    if (fcntl_flags & O_APPEND) {
        flags = (XIOFlags)(flags | X_IO_FLAG_APPEND);
    }

#ifdef O_NONBLOCK
    if (fcntl_flags & O_NONBLOCK) {
#else
    if (fcntl_flags & O_NDELAY) {
#endif
        flags = (XIOFlags)(flags | X_IO_FLAG_NONBLOCK);
    }

    switch (fcntl_flags & (O_RDONLY | O_WRONLY | O_RDWR)) {
        case O_RDONLY:
            channel->is_readable = TRUE;
            channel->is_writeable = FALSE;
            break;

        case O_WRONLY:
            channel->is_readable = FALSE;
            channel->is_writeable = TRUE;
            break;

        case O_RDWR:
            channel->is_readable = TRUE;
            channel->is_writeable = TRUE;
            break;

        default:
            x_assert_not_reached();
    }

    return flags;
}

XIOChannel *x_io_channel_new_file(const xchar *filename, const xchar *mode, XError **error)
{
    int fid, flags;
    mode_t create_mode;
    XIOChannel *channel;

    typedef enum {
        MODE_R      = 1 << 0,
        MODE_W      = 1 << 1,
        MODE_A      = 1 << 2,
        MODE_PLUS   = 1 << 3,
        MODE_R_PLUS = MODE_R | MODE_PLUS,
        MODE_W_PLUS = MODE_W | MODE_PLUS,
        MODE_A_PLUS = MODE_A | MODE_PLUS
    } local_enum_e;;
    struct stat buffer;
    local_enum_e mode_num;

    x_return_val_if_fail(filename != NULL, NULL);
    x_return_val_if_fail(mode != NULL, NULL);
    x_return_val_if_fail((error == NULL) || (*error == NULL), NULL);

    switch (mode[0]) {
        case 'r':
            mode_num = (local_enum_e)MODE_R;
            break;

        case 'w':
            mode_num = (local_enum_e)MODE_W;
            break;

        case 'a':
            mode_num = (local_enum_e)MODE_A;
            break;

        default:
            x_warning("Invalid XIOFileMode %s.", mode);
            return NULL;
    }

    switch (mode[1]) {
        case '\0':
            break;

        case '+':
            if (mode[2] == '\0') {
                mode_num = (local_enum_e)(mode_num | MODE_PLUS);
                break;
            }
            X_GNUC_FALLTHROUGH;

        default:
            x_warning("Invalid XIOFileMode %s.", mode);
            return NULL;
    }

    switch (mode_num) {
        case MODE_R:
            flags = O_RDONLY;
            break;

        case MODE_W:
            flags = O_WRONLY | O_TRUNC | O_CREAT;
            break;

        case MODE_A:
            flags = O_WRONLY | O_APPEND | O_CREAT;
            break;

        case MODE_R_PLUS:
            flags = O_RDWR;
            break;

        case MODE_W_PLUS:
            flags = O_RDWR | O_TRUNC | O_CREAT;
            break;

        case MODE_A_PLUS:
            flags = O_RDWR | O_APPEND | O_CREAT;
            break;

        case MODE_PLUS:
        default:
            x_assert_not_reached();
            flags = 0;
    }

    create_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
    fid = x_open(filename, flags | O_CLOEXEC, create_mode);
    if (fid == -1) {
        int err = errno;
        x_set_error_literal(error, X_FILE_ERROR, x_file_error_from_errno(err), x_strerror(err));
        return (XIOChannel *)NULL;
    }

    if (fstat(fid, &buffer) == -1)  {
        int err = errno;
        close(fid);
        x_set_error_literal(error, X_FILE_ERROR, x_file_error_from_errno(err), x_strerror(err));
        return (XIOChannel *)NULL;
    }

    channel = (XIOChannel *)x_new(XIOUnixChannel, 1);
    channel->is_seekable = S_ISREG(buffer.st_mode) || S_ISCHR(buffer.st_mode) || S_ISBLK(buffer.st_mode);

    switch (mode_num) {
        case MODE_R:
            channel->is_readable = TRUE;
            channel->is_writeable = FALSE;
            break;

        case MODE_W:
        case MODE_A:
            channel->is_readable = FALSE;
            channel->is_writeable = TRUE;
            break;

        case MODE_R_PLUS:
        case MODE_W_PLUS:
        case MODE_A_PLUS:
            channel->is_readable = TRUE;
            channel->is_writeable = TRUE;
            break;

        case MODE_PLUS:
        default:
            x_assert_not_reached();
    }

    x_io_channel_init(channel);
    channel->close_on_unref = TRUE;
    channel->funcs = &unix_channel_funcs;

    ((XIOUnixChannel *)channel)->fd = fid;
    return channel;
}

XIOChannel *x_io_channel_unix_new(xint fd)
{
    struct stat buffer;
    XIOUnixChannel *unix_channel = x_new(XIOUnixChannel, 1);
    XIOChannel *channel = (XIOChannel *)unix_channel;

    x_io_channel_init(channel);
    channel->funcs = &unix_channel_funcs;

    unix_channel->fd = fd;

    if (fstat(unix_channel->fd, &buffer) == 0) {
        channel->is_seekable = S_ISREG(buffer.st_mode) || S_ISCHR(buffer.st_mode) || S_ISBLK(buffer.st_mode);
    } else {
        channel->is_seekable = FALSE;
    }

    x_io_unix_get_flags(channel);
    return channel;
}

xint x_io_channel_unix_get_fd(XIOChannel *channel)
{
    XIOUnixChannel *unix_channel = (XIOUnixChannel *)channel;
    return unix_channel->fd;
}
