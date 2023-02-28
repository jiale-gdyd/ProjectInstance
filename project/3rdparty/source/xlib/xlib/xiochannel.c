#include <errno.h>
#include <string.h>
#include <xlib/xlib/config.h>
#include <xlib/xlib/xlibintl.h>
#include <xlib/xlib/xstrfuncs.h>
#include <xlib/xlib/xtestutils.h>
#include <xlib/xlib/xiochannel.h>

#define MAX_CHAR_SIZE                       10
#define X_IO_NICE_BUF_SIZE                  1024

#define USE_BUF(channel)                    ((channel)->encoding ? (channel)->encoded_read_buf : (channel)->read_buf)
#define BUF_LEN(string)                     ((string) ? (string)->len : 0)

static void x_io_channel_purge(XIOChannel *channel);
static XIOError x_io_error_get_from_g_error(XIOStatus status, XError *err);
static XIOStatus x_io_channel_fill_buffer(XIOChannel *channel, XError **err);
static XIOStatus x_io_channel_read_line_backend(XIOChannel *channel, xsize *length, xsize *terminator_pos, XError **error);

void x_io_channel_init(XIOChannel *channel)
{
    channel->ref_count = 1;
    channel->encoding = x_strdup("UTF-8");
    channel->line_term = NULL;
    channel->line_term_len = 0;
    channel->buf_size = X_IO_NICE_BUF_SIZE;
    channel->read_cd =(XIConv)-1;
    channel->write_cd =(XIConv)-1;
    channel->read_buf = NULL;
    channel->encoded_read_buf = NULL;
    channel->write_buf = NULL;
    channel->partial_write_buf[0] = '\0';
    channel->use_buffer = TRUE;
    channel->do_encode = FALSE;
    channel->close_on_unref = FALSE;
}

XIOChannel *x_io_channel_ref(XIOChannel *channel)
{
    x_return_val_if_fail(channel != NULL, NULL);

    x_atomic_int_inc(&channel->ref_count);
    return channel;
}

void x_io_channel_unref(XIOChannel *channel)
{
    xboolean is_zero;

    x_return_if_fail(channel != NULL);

    is_zero = x_atomic_int_dec_and_test(&channel->ref_count);
    if (X_UNLIKELY(is_zero)) {
        if (channel->close_on_unref) {
            x_io_channel_shutdown(channel, TRUE, NULL);
        } else {
            x_io_channel_purge(channel);
        }

        x_free(channel->encoding);

        if (channel->read_cd !=(XIConv)-1) {
            x_iconv_close(channel->read_cd);
        }

        if (channel->write_cd !=(XIConv)-1) {
            x_iconv_close(channel->write_cd);
        }

        x_free(channel->line_term);
    
        if (channel->read_buf) {
            x_string_free(channel->read_buf, TRUE);
        }

        if (channel->write_buf) {
            x_string_free(channel->write_buf, TRUE);
        }

        if (channel->encoded_read_buf) {
            x_string_free(channel->encoded_read_buf, TRUE);
        }

        channel->funcs->io_free(channel);
    }
}

static XIOError x_io_error_get_from_g_error(XIOStatus status, XError *err)
{
    switch (status) {
        case X_IO_STATUS_NORMAL:
        case X_IO_STATUS_EOF:
            return X_IO_ERROR_NONE;

        case X_IO_STATUS_AGAIN:
            return X_IO_ERROR_AGAIN;

        case X_IO_STATUS_ERROR:
            x_return_val_if_fail(err != NULL, X_IO_ERROR_UNKNOWN);
            if (err->domain != X_IO_CHANNEL_ERROR) {
                return X_IO_ERROR_UNKNOWN;
            }

            switch (err->code) {
                case X_IO_CHANNEL_ERROR_INVAL:
                    return X_IO_ERROR_INVAL;

                default:
                    return X_IO_ERROR_UNKNOWN;
            }

        default:
            x_assert_not_reached();
    }
}

XIOError x_io_channel_read(XIOChannel *channel, xchar *buf, xsize count, xsize *bytes_read)
{
    XIOError error;
    XIOStatus status;
    XError *err = NULL;

    x_return_val_if_fail(channel != NULL, X_IO_ERROR_UNKNOWN);
    x_return_val_if_fail(bytes_read != NULL, X_IO_ERROR_UNKNOWN);

    if (count == 0) {
        if (bytes_read) {
            *bytes_read = 0;
        }

        return X_IO_ERROR_NONE;
    }

    x_return_val_if_fail(buf != NULL, X_IO_ERROR_UNKNOWN);

    status = channel->funcs->io_read(channel, buf, count, bytes_read, &err);
    error = x_io_error_get_from_g_error(status, err);
    if(err) {
        x_error_free(err);
    }

    return error;
}

XIOError x_io_channel_write(XIOChannel *channel, const xchar *buf,  xsize count, xsize *bytes_written)
{
    XIOError error;
    XIOStatus status;
    XError *err = NULL;

    x_return_val_if_fail(channel != NULL, X_IO_ERROR_UNKNOWN);
    x_return_val_if_fail(bytes_written != NULL, X_IO_ERROR_UNKNOWN);

    status = channel->funcs->io_write(channel, buf, count, bytes_written, &err);
    error = x_io_error_get_from_g_error(status, err);
    if (err) {
        x_error_free(err);
    }

    return error;
}

XIOError x_io_channel_seek(XIOChannel *channel, xint64 offset, XSeekType type)
{
    XIOError error;
    XIOStatus status;
    XError *err = NULL;

    x_return_val_if_fail(channel != NULL, X_IO_ERROR_UNKNOWN);
    x_return_val_if_fail(channel->is_seekable, X_IO_ERROR_UNKNOWN);

    switch (type) {
        case X_SEEK_CUR:
        case X_SEEK_SET:
        case X_SEEK_END:
            break;

        default:
            x_warning("x_io_channel_seek: unknown seek type");
            return X_IO_ERROR_UNKNOWN;
    }

    status = channel->funcs->io_seek(channel, offset, type, &err);
    error = x_io_error_get_from_g_error(status, err);
    if (err) {
        x_error_free(err);
    }

    return error;
}

void x_io_channel_close(XIOChannel *channel)
{
    XError *err = NULL;

    x_return_if_fail(channel != NULL);
    x_io_channel_purge(channel);

    channel->funcs->io_close(channel, &err);
    if (err) {
        x_warning("Error closing channel: %s", err->message);
        x_error_free(err);
    }

    channel->close_on_unref = FALSE;
    channel->is_readable = FALSE;
    channel->is_writeable = FALSE;
    channel->is_seekable = FALSE;
}

XIOStatus x_io_channel_shutdown(XIOChannel *channel, xboolean flush, XError **err)
{
    XError *tmperr = NULL;
    XIOStatus status, result;

    x_return_val_if_fail(channel != NULL, X_IO_STATUS_ERROR);
    x_return_val_if_fail(err == NULL || *err == NULL, X_IO_STATUS_ERROR);

    if (channel->write_buf && channel->write_buf->len > 0) {
        if (flush) {
            XIOFlags flags;

            flags = x_io_channel_get_flags(channel);
            x_io_channel_set_flags(channel, (XIOFlags)(flags & ~X_IO_FLAG_NONBLOCK), NULL);

            result = x_io_channel_flush(channel, &tmperr);
        } else {
            result = X_IO_STATUS_NORMAL;
        }

        x_string_truncate(channel->write_buf, 0);
    } else {
        result = X_IO_STATUS_NORMAL;
    }

    if (channel->partial_write_buf[0] != '\0') {
        if (flush) {
            x_warning("Partial character at end of write buffer not flushed.");
        }

        channel->partial_write_buf[0] = '\0';
    }

    status = channel->funcs->io_close(channel, err);

    channel->close_on_unref = FALSE;
    channel->is_readable = FALSE;
    channel->is_writeable = FALSE;
    channel->is_seekable = FALSE;

    if (status != X_IO_STATUS_NORMAL) {
        x_clear_error(&tmperr);
        return status;
    } else if (result != X_IO_STATUS_NORMAL) {
        x_propagate_error(err, tmperr);
        return result;
    } else {
        return X_IO_STATUS_NORMAL;
    }
}

static void x_io_channel_purge(XIOChannel *channel)
{
    XError *err = NULL;
    XIOStatus status X_GNUC_UNUSED;

    x_return_if_fail(channel != NULL);

    if (channel->write_buf && channel->write_buf->len > 0) {
        XIOFlags flags;

        flags = x_io_channel_get_flags(channel);
        x_io_channel_set_flags(channel, (XIOFlags)(flags & ~X_IO_FLAG_NONBLOCK), NULL);

        status = x_io_channel_flush(channel, &err);
        if (err) {
            x_warning("Error flushing string: %s", err->message);
            x_error_free(err);
        }
    }

    if (channel->read_buf) {
        x_string_truncate(channel->read_buf, 0);
    }

    if (channel->write_buf) {
        x_string_truncate(channel->write_buf, 0);
    }

    if (channel->encoding) {
        if (channel->encoded_read_buf) {
            x_string_truncate(channel->encoded_read_buf, 0);
        }

        if (channel->partial_write_buf[0] != '\0') {
            x_warning("Partial character at end of write buffer not flushed.");
            channel->partial_write_buf[0] = '\0';
        }
    }
}

XSource *x_io_create_watch(XIOChannel *channel, XIOCondition condition)
{
    x_return_val_if_fail(channel != NULL, NULL);
    return channel->funcs->io_create_watch(channel, condition);
}

xuint x_io_add_watch_full(XIOChannel *channel, xint priority, XIOCondition condition, XIOFunc func, xpointer user_data, XDestroyNotify notify)
{
    xuint id;
    XSource *source;

    x_return_val_if_fail(channel != NULL, 0);

    source = x_io_create_watch(channel, condition);

    if (priority != X_PRIORITY_DEFAULT) {
        x_source_set_priority(source, priority);
    }
    x_source_set_callback(source,(XSourceFunc)func, user_data, notify);

    id = x_source_attach(source, NULL);
    x_source_unref(source);

    return id;
}

xuint x_io_add_watch(XIOChannel *channel, XIOCondition condition, XIOFunc func, xpointer user_data)
{
    return x_io_add_watch_full(channel, X_PRIORITY_DEFAULT, condition, func, user_data, NULL);
}

XIOCondition x_io_channel_get_buffer_condition(XIOChannel *channel)
{
    XIOCondition condition = (XIOCondition)0;

    if (channel->encoding) {
        if (channel->encoded_read_buf &&(channel->encoded_read_buf->len > 0)) {
            condition = (XIOCondition)(condition | X_IO_IN);
        }
    } else {
        if (channel->read_buf &&(channel->read_buf->len > 0)) {
            condition = (XIOCondition)(condition | X_IO_IN);
        }
    }

    if (channel->write_buf &&(channel->write_buf->len < channel->buf_size)) {
        condition = (XIOCondition)(condition | X_IO_OUT);
    }

    return condition;
}

XIOChannelError x_io_channel_error_from_errno(xint en)
{
#ifdef EAGAIN
    x_return_val_if_fail(en != EAGAIN, X_IO_CHANNEL_ERROR_FAILED);
#endif

    switch(en) {
#ifdef EBADF
        case EBADF:
            x_warning("Invalid file descriptor.");
            return X_IO_CHANNEL_ERROR_FAILED;
#endif

#ifdef EFAULT
        case EFAULT:
            x_warning("Buffer outside valid address space.");
            return X_IO_CHANNEL_ERROR_FAILED;
#endif

#ifdef EFBIG
        case EFBIG:
            return X_IO_CHANNEL_ERROR_FBIG;
#endif

#ifdef EINTR
        case EINTR:
            return X_IO_CHANNEL_ERROR_FAILED;
#endif

#ifdef EINVAL
        case EINVAL:
            return X_IO_CHANNEL_ERROR_INVAL;
#endif

#ifdef EIO
        case EIO:
            return X_IO_CHANNEL_ERROR_IO;
#endif

#ifdef EISDIR
        case EISDIR:
            return X_IO_CHANNEL_ERROR_ISDIR;
#endif

#ifdef ENOSPC
        case ENOSPC:
            return X_IO_CHANNEL_ERROR_NOSPC;
#endif

#ifdef ENXIO
        case ENXIO:
            return X_IO_CHANNEL_ERROR_NXIO;
#endif

#ifdef EOVERFLOW
#if EOVERFLOW != EFBIG
        case EOVERFLOW:
            return X_IO_CHANNEL_ERROR_OVERFLOW;
#endif
#endif

#ifdef EPIPE
        case EPIPE:
            return X_IO_CHANNEL_ERROR_PIPE;
#endif

        default:
            return X_IO_CHANNEL_ERROR_FAILED;
    }
}

void x_io_channel_set_buffer_size(XIOChannel *channel, xsize size)
{
    x_return_if_fail(channel != NULL);

    if (size == 0) {
        size = X_IO_NICE_BUF_SIZE;
    }

    if (size < MAX_CHAR_SIZE) {
        size = MAX_CHAR_SIZE;
    }

    channel->buf_size = size;
}

xsize x_io_channel_get_buffer_size(XIOChannel *channel)
{
    x_return_val_if_fail(channel != NULL, 0);
    return channel->buf_size;
}

void x_io_channel_set_line_term(XIOChannel *channel, const xchar *line_term, xint length)
{
    xuint length_unsigned;

    x_return_if_fail(channel != NULL);
    x_return_if_fail(line_term == NULL || length != 0);

    if (line_term == NULL) {
        length_unsigned = 0;
    } else if (length >= 0) {
        length_unsigned =(xuint) length;
    } else {
        xsize length_size = strlen(line_term);
        x_return_if_fail(length_size <= X_MAXUINT);
        length_unsigned =(xuint)length_size;
    }

    x_free(channel->line_term);
    channel->line_term = line_term ? (xchar *)x_memdup2(line_term, length_unsigned) : NULL;
    channel->line_term_len = length_unsigned;
}

const xchar *x_io_channel_get_line_term(XIOChannel *channel, xint *length)
{
    x_return_val_if_fail(channel != NULL, NULL);

    if (length) {
        *length = channel->line_term_len;
    }

    return channel->line_term;
}

XIOStatus x_io_channel_set_flags(XIOChannel *channel, XIOFlags flags, XError **error)
{
    x_return_val_if_fail(channel != NULL, X_IO_STATUS_ERROR);
    x_return_val_if_fail((error == NULL) ||(*error == NULL), X_IO_STATUS_ERROR);

    return (*channel->funcs->io_set_flags)(channel, (XIOFlags)(flags & X_IO_FLAG_SET_MASK), error);
}

XIOFlags x_io_channel_get_flags(XIOChannel *channel)
{
    XIOFlags flags;

    x_return_val_if_fail(channel != NULL, (XIOFlags)0);

    flags =(*channel->funcs->io_get_flags)(channel);

    if (channel->is_seekable) {
        flags = (XIOFlags)(flags | X_IO_FLAG_IS_SEEKABLE);
    }

    if (channel->is_readable) {
        flags = (XIOFlags)(flags | X_IO_FLAG_IS_READABLE);
    }

    if (channel->is_writeable) {
        flags = (XIOFlags)(flags | X_IO_FLAG_IS_WRITABLE);
    }

    return flags;
}

void x_io_channel_set_close_on_unref(XIOChannel *channel, xboolean do_close)
{
    x_return_if_fail(channel != NULL);
    channel->close_on_unref = do_close;
}

xboolean x_io_channel_get_close_on_unref(XIOChannel *channel)
{
    x_return_val_if_fail(channel != NULL, FALSE);
    return channel->close_on_unref;
}

XIOStatus x_io_channel_seek_position(XIOChannel *channel, xint64 offset, XSeekType type, XError **error)
{
    XIOStatus status;

    x_return_val_if_fail(channel != NULL, X_IO_STATUS_ERROR);
    x_return_val_if_fail((error == NULL) ||(*error == NULL), X_IO_STATUS_ERROR);
    x_return_val_if_fail(channel->is_seekable, X_IO_STATUS_ERROR);

    switch (type) {
        case X_SEEK_CUR:
            if (channel->use_buffer) {
                if (channel->do_encode && channel->encoded_read_buf && channel->encoded_read_buf->len > 0) {
                    x_warning("Seek type X_SEEK_CUR not allowed for this channel's encoding.");
                    return X_IO_STATUS_ERROR;
                }

                if (channel->read_buf) {
                    offset -= channel->read_buf->len;
                }

                if (channel->encoded_read_buf) {
                    x_assert(channel->encoded_read_buf->len == 0 || !channel->do_encode);

                    offset -= channel->encoded_read_buf->len;
                }
            }
            break;

        case X_SEEK_SET:
        case X_SEEK_END:
            break;

        default:
            x_warning("x_io_channel_seek_position: unknown seek type");
            return X_IO_STATUS_ERROR;
    }

    if (channel->use_buffer) {
        status = x_io_channel_flush(channel, error);
        if (status != X_IO_STATUS_NORMAL) {
            return status;
        }
    }

    status = channel->funcs->io_seek(channel, offset, type, error);
    if ((status == X_IO_STATUS_NORMAL) &&(channel->use_buffer)) {
        if (channel->read_buf) {
            x_string_truncate(channel->read_buf, 0);
        }

        if (channel->read_cd !=(XIConv) -1) {
            x_iconv(channel->read_cd, NULL, NULL, NULL, NULL);
        }

        if (channel->write_cd !=(XIConv) -1) {
            x_iconv(channel->write_cd, NULL, NULL, NULL, NULL);
        }

        if (channel->encoded_read_buf) {
            x_assert(channel->encoded_read_buf->len == 0 || !channel->do_encode);
            x_string_truncate(channel->encoded_read_buf, 0);
        }

        if (channel->partial_write_buf[0] != '\0') {
            x_warning("Partial character at end of write buffer not flushed.");
            channel->partial_write_buf[0] = '\0';
        }
    }

    return status;
}

XIOStatus x_io_channel_flush(XIOChannel *channel, XError **error)
{
    XIOStatus status;
    xsize this_time = 1, bytes_written = 0;

    x_return_val_if_fail(channel != NULL, X_IO_STATUS_ERROR);
    x_return_val_if_fail((error == NULL) ||(*error == NULL), X_IO_STATUS_ERROR);

    if (channel->write_buf == NULL || channel->write_buf->len == 0) {
        return X_IO_STATUS_NORMAL;
    }

    do {
        x_assert(this_time > 0);

        status = channel->funcs->io_write(channel, channel->write_buf->str + bytes_written, channel->write_buf->len - bytes_written, &this_time, error);
        bytes_written += this_time;
    } while ((bytes_written < channel->write_buf->len) &&(status == X_IO_STATUS_NORMAL));

    x_string_erase(channel->write_buf, 0, bytes_written);
    return status;
}

void x_io_channel_set_buffered(XIOChannel *channel, xboolean buffered)
{
    x_return_if_fail(channel != NULL);

    if (channel->encoding != NULL) {
        x_warning("Need to have NULL encoding to set the buffering state of the channel.");
        return;
    }

    x_return_if_fail(!channel->read_buf || channel->read_buf->len == 0);
    x_return_if_fail(!channel->write_buf || channel->write_buf->len == 0);

    channel->use_buffer = buffered;
}

xboolean x_io_channel_get_buffered(XIOChannel *channel)
{
    x_return_val_if_fail(channel != NULL, FALSE);
    return channel->use_buffer;
}

XIOStatus x_io_channel_set_encoding(XIOChannel *channel, const xchar *encoding, XError **error)
{
    xboolean did_encode;
    XIConv read_cd, write_cd;

    x_return_val_if_fail(channel != NULL, X_IO_STATUS_ERROR);
    x_return_val_if_fail((error == NULL) ||(*error == NULL), X_IO_STATUS_ERROR);
    x_return_val_if_fail(!channel->do_encode || !channel->encoded_read_buf || channel->encoded_read_buf->len == 0, X_IO_STATUS_ERROR);

    if (!channel->use_buffer) {
        x_warning("Need to set the channel buffered before setting the encoding.");
        x_warning("Assuming this is what you meant and acting accordingly.");
        channel->use_buffer = TRUE;
    }

    if (channel->partial_write_buf[0] != '\0') {
        x_warning("Partial character at end of write buffer not flushed.");
        channel->partial_write_buf[0] = '\0';
    }

    did_encode = channel->do_encode;

    if (!encoding || strcmp(encoding, "UTF8") == 0 || strcmp(encoding, "UTF-8") == 0) {
        channel->do_encode = FALSE;
        read_cd = write_cd =(XIConv) -1;
    } else {
        xint err = 0;
        const xchar *from_enc = NULL, *to_enc = NULL;

        if (channel->is_readable) {
            read_cd = x_iconv_open("UTF-8", encoding);
            if (read_cd ==(XIConv) -1) {
                err = errno;
                from_enc = encoding;
                to_enc = "UTF-8";
            }
        } else {
            read_cd =(XIConv) -1;
        }

        if (channel->is_writeable && err == 0) {
            write_cd = x_iconv_open(encoding, "UTF-8");
            if (write_cd ==(XIConv) -1) {
                err = errno;
                from_enc = "UTF-8";
                to_enc = encoding;
            }
        } else {
            write_cd =(XIConv) -1;
        }

        if (err != 0) {
            x_assert(from_enc);
            x_assert(to_enc);

            if (err == EINVAL) {
                x_set_error(error, X_CONVERT_ERROR, X_CONVERT_ERROR_NO_CONVERSION, _("Conversion from character set “%s” to “%s” is not supported"), from_enc, to_enc);
            } else {
                x_set_error(error, X_CONVERT_ERROR, X_CONVERT_ERROR_FAILED, _("Could not open converter from “%s” to “%s”: %s"), from_enc, to_enc, x_strerror(err));
            }

            if (read_cd !=(XIConv) -1) {
                x_iconv_close(read_cd);
            }

            if (write_cd !=(XIConv) -1) {
                x_iconv_close(write_cd);
            }

            return X_IO_STATUS_ERROR;
        }

        channel->do_encode = TRUE;
    }

    if (channel->read_cd !=(XIConv) -1) {
        x_iconv_close(channel->read_cd);
    }

    if (channel->write_cd !=(XIConv) -1) {
        x_iconv_close(channel->write_cd);
    }

    if (channel->encoded_read_buf && channel->encoded_read_buf->len > 0) {
        x_assert(!did_encode);

        x_string_prepend_len(channel->read_buf, channel->encoded_read_buf->str, channel->encoded_read_buf->len);
        x_string_truncate(channel->encoded_read_buf, 0);
    }

    channel->read_cd = read_cd;
    channel->write_cd = write_cd;

    x_free(channel->encoding);
    channel->encoding = x_strdup(encoding);

    return X_IO_STATUS_NORMAL;
}

const xchar *x_io_channel_get_encoding(XIOChannel *channel)
{
    x_return_val_if_fail(channel != NULL, NULL);
    return channel->encoding;
}

static XIOStatus x_io_channel_fill_buffer(XIOChannel *channel, XError **err)
{
    XIOStatus status;
    xsize read_size, cur_len, oldlen;

    if (channel->is_seekable && channel->write_buf && channel->write_buf->len > 0) {
        status = x_io_channel_flush(channel, err);
        if (status != X_IO_STATUS_NORMAL) {
            return status;
        }
    }

    if (channel->is_seekable && channel->partial_write_buf[0] != '\0') {
        x_warning("Partial character at end of write buffer not flushed.");
        channel->partial_write_buf[0] = '\0';
    }

    if (!channel->read_buf) {
        channel->read_buf = x_string_sized_new(channel->buf_size);
    }

    cur_len = channel->read_buf->len;
    x_string_set_size(channel->read_buf, channel->read_buf->len + channel->buf_size);
    status = channel->funcs->io_read(channel, channel->read_buf->str + cur_len, channel->buf_size, &read_size, err);

    x_assert((status == X_IO_STATUS_NORMAL) ||(read_size == 0));

    x_string_truncate(channel->read_buf, read_size + cur_len);

    if ((status != X_IO_STATUS_NORMAL) && ((status != X_IO_STATUS_EOF) ||(channel->read_buf->len == 0))) {
        return status;
    }

    x_assert(channel->read_buf->len > 0);

    if (channel->encoded_read_buf) {
        oldlen = channel->encoded_read_buf->len;
    } else {
        oldlen = 0;
        if (channel->encoding) {
            channel->encoded_read_buf = x_string_sized_new(channel->buf_size);
        }
    }

    if (channel->do_encode) {
        int errval;
        xchar *inbuf, *outbuf;
        xsize errnum, inbytes_left, outbytes_left;

        x_assert(channel->encoded_read_buf);

reencode:
        inbytes_left = channel->read_buf->len;
        outbytes_left = MAX(channel->read_buf->len, channel->encoded_read_buf->allocated_len - channel->encoded_read_buf->len - 1);
        outbytes_left = MAX(outbytes_left, 6);

        inbuf = channel->read_buf->str;
        x_string_set_size(channel->encoded_read_buf, channel->encoded_read_buf->len + outbytes_left);
        outbuf = channel->encoded_read_buf->str + channel->encoded_read_buf->len - outbytes_left;

        errnum = x_iconv(channel->read_cd, &inbuf, &inbytes_left, &outbuf, &outbytes_left);
        errval = errno;

        x_assert(inbuf + inbytes_left == channel->read_buf->str + channel->read_buf->len);
        x_assert(outbuf + outbytes_left == channel->encoded_read_buf->str + channel->encoded_read_buf->len);

        x_string_erase(channel->read_buf, 0, channel->read_buf->len - inbytes_left);
        x_string_truncate(channel->encoded_read_buf, channel->encoded_read_buf->len - outbytes_left);

        if (errnum ==(xsize) -1) {
            switch (errval) {
                case EINVAL:
                    if((oldlen == channel->encoded_read_buf->len) &&(status == X_IO_STATUS_EOF)) {
                        status = X_IO_STATUS_EOF;
                    } else {
                        status = X_IO_STATUS_NORMAL;
                    }
                    break;

                case E2BIG:
                    x_assert(inbuf != channel->read_buf->str);
                    goto reencode;

                case EILSEQ:
                    if (oldlen < channel->encoded_read_buf->len) {
                        status = X_IO_STATUS_NORMAL;
                    } else {
                        x_set_error_literal(err, X_CONVERT_ERROR,
                        X_CONVERT_ERROR_ILLEGAL_SEQUENCE,
                        _("Invalid byte sequence in conversion input"));
                        return X_IO_STATUS_ERROR;
                    }
                    break;

                default:
                    x_assert(errval != EBADF);
                    x_set_error(err, X_CONVERT_ERROR, X_CONVERT_ERROR_FAILED,
                    _("Error during conversion: %s"), x_strerror(errval));
                    return X_IO_STATUS_ERROR;
                }
            }

        x_assert((status != X_IO_STATUS_NORMAL) ||(channel->encoded_read_buf->len > 0));
    } else if (channel->encoding) {
        xchar *nextchar, *lastchar;

        x_assert(channel->encoded_read_buf);

        nextchar = channel->read_buf->str;
        lastchar = channel->read_buf->str + channel->read_buf->len;

        while (nextchar < lastchar) {
            xunichar val_char;

            val_char = x_utf8_get_char_validated(nextchar, lastchar - nextchar);
            switch (val_char) {
                case -2:
                    lastchar = nextchar;
                    break;

                case -1:
                    if (oldlen < channel->encoded_read_buf->len) {
                        status = X_IO_STATUS_NORMAL;
                    } else {
                        x_set_error_literal(err, X_CONVERT_ERROR, X_CONVERT_ERROR_ILLEGAL_SEQUENCE, _("Invalid byte sequence in conversion input")); status = X_IO_STATUS_ERROR;
                    }
                    lastchar = nextchar;
                    break;

                default:
                    nextchar = x_utf8_next_char(nextchar);
                    break;
            }
        }

        if (lastchar > channel->read_buf->str) {
            xint copy_len = lastchar - channel->read_buf->str;

            x_string_append_len(channel->encoded_read_buf, channel->read_buf->str, copy_len);
            x_string_erase(channel->read_buf, 0, copy_len);
        }
    }

    return status;
}

XIOStatus x_io_channel_read_line(XIOChannel *channel, xchar **str_return, xsize *length, xsize *terminator_pos, XError **error)
{
    XIOStatus status;
    xsize got_length;

    x_return_val_if_fail(channel != NULL, X_IO_STATUS_ERROR);
    x_return_val_if_fail(str_return != NULL, X_IO_STATUS_ERROR);
    x_return_val_if_fail((error == NULL) ||(*error == NULL), X_IO_STATUS_ERROR);
    x_return_val_if_fail(channel->is_readable, X_IO_STATUS_ERROR);

    status = x_io_channel_read_line_backend(channel, &got_length, terminator_pos, error);

    if (length && status != X_IO_STATUS_ERROR) {
        *length = got_length;
    }

    if (status == X_IO_STATUS_NORMAL) {
        xchar *line;

        x_assert(USE_BUF(channel));
        line = (xchar *)x_memdup2(USE_BUF(channel)->str, got_length + 1);
        line[got_length] = '\0';
        *str_return = x_steal_pointer(&line);
        x_string_erase(USE_BUF(channel), 0, got_length);
    } else {
        *str_return = NULL;
    }

    return status;
}

XIOStatus x_io_channel_read_line_string(XIOChannel *channel, XString *buffer, xsize *terminator_pos, XError **error)
{
    xsize length;
    XIOStatus status;

    x_return_val_if_fail(channel != NULL, X_IO_STATUS_ERROR);
    x_return_val_if_fail(buffer != NULL, X_IO_STATUS_ERROR);
    x_return_val_if_fail((error == NULL) ||(*error == NULL), X_IO_STATUS_ERROR);
    x_return_val_if_fail(channel->is_readable, X_IO_STATUS_ERROR);

    if (buffer->len > 0) {
        x_string_truncate(buffer, 0);
    }

    status = x_io_channel_read_line_backend(channel, &length, terminator_pos, error);
    if (status == X_IO_STATUS_NORMAL) {
        x_assert(USE_BUF(channel));
        x_string_append_len(buffer, USE_BUF(channel)->str, length);
        x_string_erase(USE_BUF(channel), 0, length);
    }

    return status;
}

static XIOStatus x_io_channel_read_line_backend(XIOChannel *channel, xsize *length, xsize *terminator_pos, XError **error)
{
    XIOStatus status;
    xboolean first_time = TRUE;
    xsize checked_to, line_term_len, line_length, got_term_len;

    if (!channel->use_buffer) {
        x_set_error_literal(error, X_CONVERT_ERROR, X_CONVERT_ERROR_FAILED, _("Can’t do a raw read in x_io_channel_read_line_string"));
        return X_IO_STATUS_ERROR;
    }

    status = X_IO_STATUS_NORMAL;
    if (channel->line_term) {
        line_term_len = channel->line_term_len;
    } else {
        line_term_len = 3;
    }

    checked_to = 0;

    while (TRUE) {
        XString *use_buf;
        xchar *nextchar, *lastchar;

        if (!first_time ||(BUF_LEN(USE_BUF(channel)) == 0)) {
read_again:
            status = x_io_channel_fill_buffer(channel, error);
            switch (status) {
                case X_IO_STATUS_NORMAL:
                    if (BUF_LEN(USE_BUF(channel)) == 0) {
                        first_time = FALSE;
                        continue;
                    }
                    break;

                case X_IO_STATUS_EOF:
                    if (BUF_LEN(USE_BUF(channel)) == 0) {
                        if (length) {
                            *length = 0;
                        }

                        if (channel->encoding && channel->read_buf->len != 0) {
                            x_set_error_literal(error, X_CONVERT_ERROR, X_CONVERT_ERROR_PARTIAL_INPUT, _("Leftover unconverted data in " "read buffer"));
                            return X_IO_STATUS_ERROR;
                        } else {
                            return X_IO_STATUS_EOF;
                        }
                    }
                    break;

                default:
                    if(length)
                    *length = 0;
                    return status;
            }
        }

        x_assert(BUF_LEN(USE_BUF(channel)) != 0);

        use_buf = USE_BUF(channel);
        first_time = FALSE;
        lastchar = use_buf->str + use_buf->len;

        for (nextchar = use_buf->str + checked_to; nextchar < lastchar; channel->encoding ? nextchar = x_utf8_next_char(nextchar) : nextchar++) {
            if (channel->line_term) {
                if (memcmp(channel->line_term, nextchar, line_term_len) == 0) {
                    line_length = nextchar - use_buf->str;
                    got_term_len = line_term_len;
                    goto done;
                }
            } else {
                switch (*nextchar) {
                    case '\n':
                        line_length = nextchar - use_buf->str;
                        got_term_len = 1;
                        goto done;

                    case '\r':
                        line_length = nextchar - use_buf->str;
                        if((nextchar == lastchar - 1) &&(status != X_IO_STATUS_EOF) &&(lastchar == use_buf->str + use_buf->len)) {
                            goto read_again;
                        }

                        if ((nextchar < lastchar - 1) &&(*(nextchar + 1) == '\n')) {
                            got_term_len = 2;
                        } else {
                            got_term_len = 1;
                        }
                        goto done;

                    case '\xe2':
                        if (strncmp("\xe2\x80\xa9", nextchar, 3) == 0) {
                            line_length = nextchar - use_buf->str;
                            got_term_len = 3;
                            goto done;
                        }
                        break;

                    case '\0':
                        line_length = nextchar - use_buf->str;
                        got_term_len = 1;
                        goto done;

                    default:
                        break;
                }
            }
        }

        x_assert(nextchar == lastchar);

        if (status == X_IO_STATUS_EOF) {
            if (channel->encoding && channel->read_buf->len > 0) {
                x_set_error_literal(error, X_CONVERT_ERROR, X_CONVERT_ERROR_PARTIAL_INPUT, _("Channel terminates in a partial character"));
                return X_IO_STATUS_ERROR;
            }

            line_length = use_buf->len;
            got_term_len = 0;
            break;
        }

        if (use_buf->len > line_term_len - 1) {
            checked_to = use_buf->len -(line_term_len - 1);
        } else {
            checked_to = 0;
        }
    }

done:
    if (terminator_pos) {
        *terminator_pos = line_length;
    }

    if (length) {
        *length = line_length + got_term_len;
    }

    return X_IO_STATUS_NORMAL;
}

XIOStatus x_io_channel_read_to_end(XIOChannel *channel, xchar **str_return, xsize *length, XError **error)
{
    XIOStatus status;

    x_return_val_if_fail(channel != NULL, X_IO_STATUS_ERROR);
    x_return_val_if_fail((error == NULL) ||(*error == NULL), X_IO_STATUS_ERROR);
    x_return_val_if_fail(channel->is_readable, X_IO_STATUS_ERROR);

    if (str_return) {
        *str_return = NULL;
    }

    if (length) {
        *length = 0;
    }

    if (!channel->use_buffer) {
        x_set_error_literal(error, X_CONVERT_ERROR, X_CONVERT_ERROR_FAILED, _("Can’t do a raw read in x_io_channel_read_to_end"));
        return X_IO_STATUS_ERROR;
    }

    do {
        status = x_io_channel_fill_buffer(channel, error);
    } while (status == X_IO_STATUS_NORMAL);

    if (status != X_IO_STATUS_EOF) {
        return status;
    }

    if (channel->encoding && channel->read_buf->len > 0) {
        x_set_error_literal(error, X_CONVERT_ERROR, X_CONVERT_ERROR_PARTIAL_INPUT, _("Channel terminates in a partial character"));
        return X_IO_STATUS_ERROR;
    }

    if (USE_BUF(channel) == NULL) {
        if (str_return) {
            *str_return = x_strdup("");
        }
    } else {
        if (length) {
            *length = USE_BUF(channel)->len;
        }

        if (str_return) {
            *str_return = x_string_free(USE_BUF(channel), FALSE);
        } else {
            x_string_free(USE_BUF(channel), TRUE);
        }

        if (channel->encoding) {
            channel->encoded_read_buf = NULL;
        } else {
            channel->read_buf = NULL;
        }
    }

    return X_IO_STATUS_NORMAL;
}

XIOStatus x_io_channel_read_chars(XIOChannel *channel, xchar *buf, xsize count, xsize *bytes_read, XError **error)
{
    xsize got_bytes;
    XIOStatus status;

    x_return_val_if_fail(channel != NULL, X_IO_STATUS_ERROR);
    x_return_val_if_fail((error == NULL) ||(*error == NULL), X_IO_STATUS_ERROR);
    x_return_val_if_fail(channel->is_readable, X_IO_STATUS_ERROR);

    if (count == 0) {
        if (bytes_read) {
            *bytes_read = 0;
        }

        return X_IO_STATUS_NORMAL;
    }

    x_return_val_if_fail(buf != NULL, X_IO_STATUS_ERROR);

    if (!channel->use_buffer) {
        xsize tmp_bytes;

        x_assert(!channel->read_buf || channel->read_buf->len == 0);

        status = channel->funcs->io_read(channel, buf, count, &tmp_bytes, error);
        if (bytes_read) {
            *bytes_read = tmp_bytes;
        }

        return status;
    }

    status = X_IO_STATUS_NORMAL;

    while (BUF_LEN(USE_BUF(channel)) < count && status == X_IO_STATUS_NORMAL) {
        status = x_io_channel_fill_buffer(channel, error);
    }

    if (BUF_LEN(USE_BUF(channel)) == 0) {
        x_assert(status != X_IO_STATUS_NORMAL);

        if (status == X_IO_STATUS_EOF && channel->encoding && BUF_LEN(channel->read_buf) > 0) {
            x_set_error_literal(error, X_CONVERT_ERROR, X_CONVERT_ERROR_PARTIAL_INPUT, _("Leftover unconverted data in read buffer"));
            status = X_IO_STATUS_ERROR;
        }

        if (bytes_read) {
            *bytes_read = 0;
        }

        return status;
    }

    if (status == X_IO_STATUS_ERROR) {
        x_clear_error(error);
    }

    got_bytes = MIN(count, BUF_LEN(USE_BUF(channel)));

    x_assert(got_bytes > 0);

    if (channel->encoding) {
        xchar *nextchar, *prevchar;

        x_assert(USE_BUF(channel) == channel->encoded_read_buf);
        nextchar = channel->encoded_read_buf->str;

        do {
            prevchar = nextchar;
            nextchar = x_utf8_next_char(nextchar);
            x_assert(nextchar != prevchar);
        } while(nextchar < channel->encoded_read_buf->str + got_bytes);

        if (nextchar > channel->encoded_read_buf->str + got_bytes) {
            got_bytes = prevchar - channel->encoded_read_buf->str;
        }

        x_assert(got_bytes > 0 || count < 6);
    }

    memcpy(buf, USE_BUF(channel)->str, got_bytes);
    x_string_erase(USE_BUF(channel), 0, got_bytes);

    if (bytes_read) {
        *bytes_read = got_bytes;
    }

    return X_IO_STATUS_NORMAL;
}

XIOStatus x_io_channel_read_unichar(XIOChannel *channel, xunichar *thechar, XError **error)
{
    XIOStatus status = X_IO_STATUS_NORMAL;

    x_return_val_if_fail(channel != NULL, X_IO_STATUS_ERROR);
    x_return_val_if_fail(channel->encoding != NULL, X_IO_STATUS_ERROR);
    x_return_val_if_fail((error == NULL) ||(*error == NULL), X_IO_STATUS_ERROR);
    x_return_val_if_fail(channel->is_readable, X_IO_STATUS_ERROR);

    while (BUF_LEN(channel->encoded_read_buf) == 0 && status == X_IO_STATUS_NORMAL) {
        status = x_io_channel_fill_buffer(channel, error);
    }

    if (BUF_LEN(USE_BUF(channel)) == 0) {
        x_assert(status != X_IO_STATUS_NORMAL);

        if (status == X_IO_STATUS_EOF && BUF_LEN(channel->read_buf) > 0) {
            x_set_error_literal(error, X_CONVERT_ERROR, X_CONVERT_ERROR_PARTIAL_INPUT, _("Leftover unconverted data in read buffer"));
            status = X_IO_STATUS_ERROR;
        }

        if (thechar) {
            *thechar =(xunichar) -1;
        }

        return status;
    }

    if (status == X_IO_STATUS_ERROR) {
        x_clear_error(error);
    }

    if (thechar) {
        *thechar = x_utf8_get_char(channel->encoded_read_buf->str);
    }

    x_string_erase(channel->encoded_read_buf, 0, x_utf8_next_char(channel->encoded_read_buf->str) - channel->encoded_read_buf->str);
    return X_IO_STATUS_NORMAL;
}

XIOStatus x_io_channel_write_chars(XIOChannel *channel, const xchar *buf, xssize count, xsize *bytes_written, XError **error)
{
    XIOStatus status;
    xsize count_unsigned;
    xsize wrote_bytes = 0;

    x_return_val_if_fail(channel != NULL, X_IO_STATUS_ERROR);
    x_return_val_if_fail(buf != NULL || count == 0, X_IO_STATUS_ERROR);
    x_return_val_if_fail((error == NULL) ||(*error == NULL), X_IO_STATUS_ERROR);
    x_return_val_if_fail(channel->is_writeable, X_IO_STATUS_ERROR);

    if (count < 0) {
        count_unsigned = strlen(buf);
    } else {
        count_unsigned = count;
    }

    if (count_unsigned == 0) {
        if (bytes_written) {
            *bytes_written = 0;
        }

        return X_IO_STATUS_NORMAL;
    }

    x_assert(count_unsigned > 0);

    if (!channel->use_buffer) {
        xsize tmp_bytes;

        x_assert(!channel->write_buf || channel->write_buf->len == 0);
        x_assert(channel->partial_write_buf[0] == '\0');

        status = channel->funcs->io_write(channel, buf, count_unsigned, &tmp_bytes, error);

        if (bytes_written) {
            *bytes_written = tmp_bytes;
        }

        return status;
    }

    if (channel->is_seekable &&(( BUF_LEN(channel->read_buf) > 0) ||(BUF_LEN(channel->encoded_read_buf) > 0))) {
        if (channel->do_encode && BUF_LEN(channel->encoded_read_buf) > 0) {
            x_warning("Mixed reading and writing not allowed on encoded files");
            return X_IO_STATUS_ERROR;
        }

        status = x_io_channel_seek_position(channel, 0, X_SEEK_CUR, error);
        if (status != X_IO_STATUS_NORMAL) {
            if (bytes_written) {
                *bytes_written = 0;
            }

            return status;
        }
    }

    if (!channel->write_buf) {
        channel->write_buf = x_string_sized_new(channel->buf_size);
    }

    while (wrote_bytes < count_unsigned) {
        xsize space_in_buf;

        if (channel->write_buf->len >= channel->buf_size - MAX_CHAR_SIZE) {
            xsize did_write = 0, this_time;

            do {
                status = channel->funcs->io_write(channel, channel->write_buf->str + did_write, channel->write_buf->len - did_write, &this_time, error);
                did_write += this_time;
            } while(status == X_IO_STATUS_NORMAL && did_write < MIN(channel->write_buf->len, MAX_CHAR_SIZE));

            x_string_erase(channel->write_buf, 0, did_write);

            if (status != X_IO_STATUS_NORMAL) {
                if (status == X_IO_STATUS_AGAIN && wrote_bytes > 0) {
                    status = X_IO_STATUS_NORMAL;
                }

                if (bytes_written) {
                    *bytes_written = wrote_bytes;
                }

                return status;
             }
        }

        space_in_buf = MAX(channel->buf_size, channel->write_buf->allocated_len - 1) - channel->write_buf->len;

        x_assert(space_in_buf >= MAX_CHAR_SIZE);

        if (!channel->encoding) {
            xsize write_this = MIN(space_in_buf, count_unsigned - wrote_bytes);
            if (write_this > X_MAXSSIZE) {
                write_this = X_MAXSSIZE;
            }

            x_string_append_len(channel->write_buf, buf, write_this);
            buf += write_this;
            wrote_bytes += write_this;
        } else {
            xsize err;
            xint errnum;
            const xchar *from_buf;
            xsize from_buf_len, from_buf_old_len, left_len;

            if (channel->partial_write_buf[0] != '\0') {
                x_assert(wrote_bytes == 0);

                from_buf = channel->partial_write_buf;
                from_buf_old_len = strlen(channel->partial_write_buf);
                x_assert(from_buf_old_len > 0);
                from_buf_len = MIN(6, from_buf_old_len + count_unsigned);

                memcpy(channel->partial_write_buf + from_buf_old_len, buf,
                        from_buf_len - from_buf_old_len);
            } else {
                from_buf = buf;
                from_buf_len = count_unsigned - wrote_bytes;
                from_buf_old_len = 0;
            }
reconvert:
            if (!channel->do_encode) {
                const xchar *badchar;
                xsize try_len = MIN(from_buf_len, space_in_buf);

                if (!x_utf8_validate_len(from_buf, try_len, &badchar)) {
                    xunichar try_char;
                    xsize incomplete_len = from_buf + try_len - badchar;

                    left_len = from_buf + from_buf_len - badchar;
                    try_char = x_utf8_get_char_validated(badchar, incomplete_len);

                    switch (try_char) {
                        case -2:
                            x_assert(incomplete_len < 6);
                            if (try_len == from_buf_len) {
                                errnum = EINVAL;
                                err =(xsize) -1;
                            } else {
                                errnum = 0;
                                err =(xsize) 0;
                            }
                            break;

                        case -1:
                            x_warning("Invalid UTF-8 passed to x_io_channel_write_chars().");
                            errnum = EILSEQ;
                            err =(xsize) -1;
                            break;

                        default:
                            x_assert_not_reached();
                            err =(xsize) -1;
                            errnum = 0;
                    }
                } else {
                    err =(xsize) 0;
                    errnum = 0;
                    left_len = from_buf_len - try_len;
                }

                x_string_append_len(channel->write_buf, from_buf, from_buf_len - left_len);
                from_buf += from_buf_len - left_len;
            } else {
                xchar *outbuf;

                left_len = from_buf_len;
                x_string_set_size(channel->write_buf, channel->write_buf->len + space_in_buf);
                outbuf = channel->write_buf->str + channel->write_buf->len - space_in_buf;
                err = x_iconv(channel->write_cd,(xchar **) &from_buf, &left_len, &outbuf, &space_in_buf);
                errnum = errno;
                x_string_truncate(channel->write_buf, channel->write_buf->len - space_in_buf);
            }

            if (err ==(xsize) -1) {
                switch (errnum) {
                    case EINVAL:
                        x_assert(left_len < 6);

                        if (from_buf_old_len == 0) {
                            memcpy(channel->partial_write_buf, from_buf, left_len);
                            channel->partial_write_buf[left_len] = '\0';
                            if (bytes_written) {
                                *bytes_written = count_unsigned;
                            }

                            return X_IO_STATUS_NORMAL;
                        }

                        if (left_len == from_buf_len) {
                            x_assert(count_unsigned == from_buf_len - from_buf_old_len);

                            channel->partial_write_buf[from_buf_len] = '\0';
                            if (bytes_written) {
                                *bytes_written = count_unsigned;
                            }

                            return X_IO_STATUS_NORMAL;
                        }

                        x_assert(from_buf_len - left_len >= from_buf_old_len);
                        break;

                    case E2BIG:
                        if (from_buf_len == left_len) {
                            space_in_buf += MAX_CHAR_SIZE;
                            goto reconvert;
                        }
                        break;

                    case EILSEQ:
                        x_set_error_literal(error, X_CONVERT_ERROR, X_CONVERT_ERROR_ILLEGAL_SEQUENCE, _("Invalid byte sequence in conversion input"));
                        if (from_buf_old_len > 0 && from_buf_len == left_len) {
                            x_warning("Illegal sequence due to partial character at the end of a previous write.");
                        } else {
                            x_assert(from_buf_len >= left_len + from_buf_old_len);
                            wrote_bytes += from_buf_len - left_len - from_buf_old_len;
                        }

                        if (bytes_written) {
                            *bytes_written = wrote_bytes;
                        }
                        channel->partial_write_buf[0] = '\0';
                        return X_IO_STATUS_ERROR;

                    default:
                        x_set_error(error, X_CONVERT_ERROR, X_CONVERT_ERROR_FAILED, _("Error during conversion: %s"), x_strerror(errnum));
                        if (from_buf_len >= left_len + from_buf_old_len) {
                            wrote_bytes += from_buf_len - left_len - from_buf_old_len;
                        }

                        if (bytes_written) {
                            *bytes_written = wrote_bytes;
                        }
                        channel->partial_write_buf[0] = '\0';
                        return X_IO_STATUS_ERROR;
                   }
            }

            x_assert(from_buf_len - left_len >= from_buf_old_len);

            wrote_bytes += from_buf_len - left_len - from_buf_old_len;
            if (from_buf_old_len > 0) {
                buf += from_buf_len - left_len - from_buf_old_len;
                channel->partial_write_buf[0] = '\0';
            } else
                buf = from_buf;
        }
    }

    if (bytes_written) {
        *bytes_written = count_unsigned;
    }

    return X_IO_STATUS_NORMAL;
}

XIOStatus x_io_channel_write_unichar(XIOChannel *channel, xunichar thechar, XError **error)
{
    XIOStatus status;
    xchar static_buf[6];
    xsize char_len, wrote_len;

    x_return_val_if_fail(channel != NULL, X_IO_STATUS_ERROR);
    x_return_val_if_fail(channel->encoding != NULL, X_IO_STATUS_ERROR);
    x_return_val_if_fail((error == NULL) ||(*error == NULL), X_IO_STATUS_ERROR);
    x_return_val_if_fail(channel->is_writeable, X_IO_STATUS_ERROR);

    char_len = x_unichar_to_utf8(thechar, static_buf);
    if (channel->partial_write_buf[0] != '\0') {
        x_warning("Partial character written before writing unichar.");
        channel->partial_write_buf[0] = '\0';
    }

    status = x_io_channel_write_chars(channel, static_buf, char_len, &wrote_len, error);
    x_assert(wrote_len == char_len || status != X_IO_STATUS_NORMAL);

    return status;
}

X_DEFINE_QUARK(x-io-channel-error-quark, x_io_channel_error)
