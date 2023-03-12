#ifndef BUF_H
#define BUF_H

#include <stdarg.h>
#include <stdint.h>
#include <sys/types.h>

#include "logc_xplatform.h"

struct liblog_buf {
    char   *start;
    char   *tail;
    char   *end;
    char   *end_plus_1;

    size_t size_min;
    size_t size_max;
    size_t size_real;

    char   truncate_str[LOG_MAXLEN_PATH + 1];
    size_t truncate_str_len;
};

struct liblog_buf *liblog_buf_new(size_t buf_size_min, size_t buf_size_max, const char *truncate_str);

void liblog_buf_del(struct liblog_buf *a_buf);
void liblog_buf_profile(struct liblog_buf *a_buf, int flag);

int liblog_buf_vprintf(struct liblog_buf *a_buf, const char *format, va_list args);

int liblog_buf_append(struct liblog_buf *a_buf, const char *str, size_t str_len);
int liblog_buf_adjust_append(struct liblog_buf *a_buf, const char *str, size_t str_len, int left_adjust, int zero_pad, size_t in_width, size_t out_width);

int liblog_buf_printf_hex(struct liblog_buf *a_buf, uint32_t ui32, int width);
int liblog_buf_printf_dec32(struct liblog_buf *a_buf, uint32_t ui32, int width);
int liblog_buf_printf_dec64(struct liblog_buf *a_buf, uint64_t ui64, int width);

#define liblog_buf_restart(a_buf)   do {    \
        a_buf->tail = a_buf->start;         \
    } while (0)

#define liblog_buf_len(a_buf)       (a_buf->tail - a_buf->start)
#define liblog_buf_str(a_buf)       (a_buf->start)
#define liblog_buf_seal(a_buf)      do {*(a_buf)->tail = '\0';} while (0)

#endif
