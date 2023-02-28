#ifndef __X_IOCHANNEL_H__
#define __X_IOCHANNEL_H__

#include "xmain.h"
#include "xstring.h"
#include "xconvert.h"

X_BEGIN_DECLS

#define X_IO_CHANNEL_ERROR          x_io_channel_error_quark()

typedef struct _XIOFuncs XIOFuncs;
typedef struct _XIOChannel XIOChannel;

typedef enum {
    X_IO_ERROR_NONE,
    X_IO_ERROR_AGAIN,
    X_IO_ERROR_INVAL,
    X_IO_ERROR_UNKNOWN
} XIOError;

typedef enum {
    X_IO_CHANNEL_ERROR_FBIG,
    X_IO_CHANNEL_ERROR_INVAL,
    X_IO_CHANNEL_ERROR_IO,
    X_IO_CHANNEL_ERROR_ISDIR,
    X_IO_CHANNEL_ERROR_NOSPC,
    X_IO_CHANNEL_ERROR_NXIO,
    X_IO_CHANNEL_ERROR_OVERFLOW,
    X_IO_CHANNEL_ERROR_PIPE,
    X_IO_CHANNEL_ERROR_FAILED
} XIOChannelError;

typedef enum {
    X_IO_STATUS_ERROR,
    X_IO_STATUS_NORMAL,
    X_IO_STATUS_EOF,
    X_IO_STATUS_AGAIN
} XIOStatus;

typedef enum {
    X_SEEK_CUR,
    X_SEEK_SET,
    X_SEEK_END
} XSeekType;

typedef enum {
    X_IO_FLAG_NONE XLIB_AVAILABLE_ENUMERATOR_IN_2_74 = 0,
    X_IO_FLAG_APPEND        = 1 << 0,
    X_IO_FLAG_NONBLOCK     = 1 << 1,
    X_IO_FLAG_IS_READABLE  = 1 << 2,
    X_IO_FLAG_IS_WRITABLE  = 1 << 3,
    X_IO_FLAG_IS_WRITEABLE = 1 << 3,
    X_IO_FLAG_IS_SEEKABLE  = 1 << 4,
    X_IO_FLAG_MASK         = (1 << 5) - 1,
    X_IO_FLAG_GET_MASK     = X_IO_FLAG_MASK,
    X_IO_FLAG_SET_MASK     = X_IO_FLAG_APPEND | X_IO_FLAG_NONBLOCK
} XIOFlags;

struct _XIOChannel {
    xint     ref_count;
    XIOFuncs *funcs;

    xchar    *encoding;
    XIConv   read_cd;
    XIConv   write_cd;
    xchar    *line_term;
    xuint    line_term_len;

    xsize    buf_size;
    XString  *read_buf;
    XString  *encoded_read_buf;
    XString  *write_buf;
    xchar    partial_write_buf[6];

    xuint    use_buffer     : 1;
    xuint    do_encode      : 1;
    xuint    close_on_unref : 1;
    xuint    is_readable    : 1;
    xuint    is_writeable   : 1;
    xuint    is_seekable    : 1;

    xpointer reserved1;
    xpointer reserved2;
};

typedef xboolean (*XIOFunc)(XIOChannel *source, XIOCondition condition, xpointer data);

struct _XIOFuncs {
    XIOStatus (*io_read)(XIOChannel *channel, xchar *buf, xsize count, xsize *bytes_read, XError **err);
    XIOStatus (*io_write)(XIOChannel *channel, const xchar *buf, xsize count, xsize *bytes_written, XError **err);
    XIOStatus (*io_seek)(XIOChannel *channel, xint64 offset, XSeekType type, XError **err);
    XIOStatus (*io_close)(XIOChannel *channel, XError **err);
    XSource *(*io_create_watch)(XIOChannel *channel, XIOCondition condition);
    void (*io_free)(XIOChannel *channel);
    XIOStatus (*io_set_flags)(XIOChannel *channel, XIOFlags flags, XError **err);
    XIOFlags (*io_get_flags)(XIOChannel *channel);
};

XLIB_AVAILABLE_IN_ALL
void x_io_channel_init(XIOChannel *channel);

XLIB_AVAILABLE_IN_ALL
XIOChannel *x_io_channel_ref(XIOChannel *channel);

XLIB_AVAILABLE_IN_ALL
void x_io_channel_unref(XIOChannel *channel);

XLIB_DEPRECATED_FOR(x_io_channel_read_chars)
XIOError x_io_channel_read(XIOChannel *channel, xchar *buf, xsize count, xsize *bytes_read);

XLIB_DEPRECATED_FOR(x_io_channel_write_chars)
XIOError x_io_channel_write(XIOChannel *channel, const xchar *buf, xsize count, xsize *bytes_written);

XLIB_DEPRECATED_FOR(x_io_channel_seek_position)
XIOError x_io_channel_seek(XIOChannel *channel, xint64 offset, XSeekType type);

XLIB_DEPRECATED_FOR(x_io_channel_shutdown)
void x_io_channel_close(XIOChannel *channel);

XLIB_AVAILABLE_IN_ALL
XIOStatus x_io_channel_shutdown(XIOChannel *channel, xboolean flush, XError **err);

XLIB_AVAILABLE_IN_ALL
xuint x_io_add_watch_full(XIOChannel *channel, xint priority, XIOCondition condition, XIOFunc func, xpointer user_data, XDestroyNotify notify);

XLIB_AVAILABLE_IN_ALL
XSource *x_io_create_watch(XIOChannel *channel, XIOCondition condition);

XLIB_AVAILABLE_IN_ALL
xuint x_io_add_watch(XIOChannel *channel, XIOCondition condition, XIOFunc func, xpointer user_data);

XLIB_AVAILABLE_IN_ALL
void x_io_channel_set_buffer_size(XIOChannel *channel, xsize size);

XLIB_AVAILABLE_IN_ALL
xsize x_io_channel_get_buffer_size(XIOChannel *channel);

XLIB_AVAILABLE_IN_ALL
XIOCondition x_io_channel_get_buffer_condition(XIOChannel *channel);

XLIB_AVAILABLE_IN_ALL
XIOStatus x_io_channel_set_flags(XIOChannel *channel, XIOFlags flags, XError **error);

XLIB_AVAILABLE_IN_ALL
XIOFlags x_io_channel_get_flags(XIOChannel *channel);

XLIB_AVAILABLE_IN_ALL
void x_io_channel_set_line_term(XIOChannel *channel, const xchar *line_term, xint length);

XLIB_AVAILABLE_IN_ALL
const xchar *x_io_channel_get_line_term(XIOChannel *channel, xint *length);

XLIB_AVAILABLE_IN_ALL
void x_io_channel_set_buffered(XIOChannel *channel, xboolean buffered);

XLIB_AVAILABLE_IN_ALL
xboolean x_io_channel_get_buffered(XIOChannel *channel);

XLIB_AVAILABLE_IN_ALL
XIOStatus x_io_channel_set_encoding(XIOChannel *channel, const xchar *encoding, XError **error);

XLIB_AVAILABLE_IN_ALL
const xchar *x_io_channel_get_encoding(XIOChannel *channel);

XLIB_AVAILABLE_IN_ALL
void x_io_channel_set_close_on_unref(XIOChannel *channel, xboolean do_close);

XLIB_AVAILABLE_IN_ALL
xboolean x_io_channel_get_close_on_unref(XIOChannel *channel);

XLIB_AVAILABLE_IN_ALL
XIOStatus x_io_channel_flush(XIOChannel *channel, XError **error);

XLIB_AVAILABLE_IN_ALL
XIOStatus x_io_channel_read_line(XIOChannel *channel, xchar **str_return, xsize *length, xsize *terminator_pos, XError **error);

XLIB_AVAILABLE_IN_ALL
XIOStatus x_io_channel_read_line_string(XIOChannel *channel, XString *buffer, xsize *terminator_pos, XError **error);

XLIB_AVAILABLE_IN_ALL
XIOStatus x_io_channel_read_to_end(XIOChannel *channel, xchar **str_return, xsize *length, XError **error);

XLIB_AVAILABLE_IN_ALL
XIOStatus x_io_channel_read_chars(XIOChannel *channel, xchar *buf, xsize count, xsize *bytes_read, XError **error);

XLIB_AVAILABLE_IN_ALL
XIOStatus x_io_channel_read_unichar(XIOChannel *channel, xunichar *thechar, XError **error);

XLIB_AVAILABLE_IN_ALL
XIOStatus x_io_channel_write_chars(XIOChannel *channel, const xchar *buf, xssize count, xsize *bytes_written, XError **error);

XLIB_AVAILABLE_IN_ALL
XIOStatus x_io_channel_write_unichar(XIOChannel *channel, xunichar thechar, XError **error);

XLIB_AVAILABLE_IN_ALL
XIOStatus x_io_channel_seek_position(XIOChannel *channel, xint64 offset, XSeekType type, XError **error);

XLIB_AVAILABLE_IN_ALL
XIOChannel *x_io_channel_new_file(const xchar *filename, const xchar *mode, XError **error);

XLIB_AVAILABLE_IN_ALL
XQuark x_io_channel_error_quark(void);

XLIB_AVAILABLE_IN_ALL
XIOChannelError x_io_channel_error_from_errno(xint en);

XLIB_AVAILABLE_IN_ALL
XIOChannel *x_io_channel_unix_new(int fd);

XLIB_AVAILABLE_IN_ALL
xint x_io_channel_unix_get_fd(XIOChannel *channel);

XLIB_VAR XSourceFuncs x_io_watch_funcs;

X_END_DECLS

#endif
