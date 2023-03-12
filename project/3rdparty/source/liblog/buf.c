#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>

#include "buf.h"
#include "logc_defs.h"

void liblog_buf_profile(struct liblog_buf *a_buf, int flag)
{
    logc_profile(flag, "---buf[%p][%ld-%ld][%ld][%s][%p:%ld]---",
        a_buf, a_buf->size_min, a_buf->size_max, a_buf->size_real, a_buf->truncate_str, a_buf->start, a_buf->tail - a_buf->start);
}

void liblog_buf_del(struct liblog_buf *a_buf)
{
    if (a_buf->start) {
        free(a_buf->start);
    }

    logc_debug("liblog_buf_del[%p]", a_buf);
    free(a_buf);
}

struct liblog_buf *liblog_buf_new(size_t buf_size_min, size_t buf_size_max, const char *truncate_str)
{
    struct liblog_buf *a_buf;

    if (buf_size_min == 0) {
        logc_error("buf_size_min == 0, not allowed");
        return NULL;
    }

    if ((buf_size_max != 0) && (buf_size_max < buf_size_min)) {
        logc_error("buf_size_max[%lu] < buf_size_min[%lu]  && buf_size_max != 0", (unsigned long)buf_size_max, (unsigned long)buf_size_min);
        return NULL;
    }

    a_buf = (struct liblog_buf *)calloc(1, sizeof(*a_buf));
    if (!a_buf) {
        logc_error("calloc fail, errno[%d]", errno);
        return NULL;
    }

    if (truncate_str) {
        if (strlen(truncate_str) > (sizeof(a_buf->truncate_str) - 1)) {
            logc_error("truncate_str[%s] overflow", truncate_str);
            goto err;
        } else {
            strcpy(a_buf->truncate_str, truncate_str);
        }
        
        a_buf->truncate_str_len = strlen(truncate_str);
    }

    a_buf->size_min = buf_size_min;
    a_buf->size_max = buf_size_max;
    a_buf->size_real = a_buf->size_min;

    a_buf->start = (char *)calloc(1, a_buf->size_real);
    if (!a_buf->start) {
        logc_error("calloc fail, errno[%d]", errno);
        goto err;
    }

    a_buf->tail = a_buf->start;
    a_buf->end_plus_1 = a_buf->start + a_buf->size_real;
    a_buf->end = a_buf->end_plus_1 - 1;

    return a_buf;

err:
    liblog_buf_del(a_buf);
    return NULL;
}

static void liblog_buf_truncate(struct liblog_buf *a_buf)
{
    char *p;
    size_t len;

    if ((a_buf->truncate_str)[0] == '\0') {
        return;
    }

    p = (a_buf->tail - a_buf->truncate_str_len);
    if (p < a_buf->start) {
        p = a_buf->start;
    }

    len = a_buf->tail - p;
    memcpy(p, a_buf->truncate_str, len);
}

static int liblog_buf_resize(struct liblog_buf *a_buf, size_t increment)
{
    int rc = 0;
    size_t len = 0;
    char *p = NULL;
    size_t new_size = 0;

    if ((a_buf->size_max != 0) && (a_buf->size_real >= a_buf->size_max)) {
        logc_error("a_buf->size_real[%ld] >= a_buf->size_max[%ld]", a_buf->size_real, a_buf->size_max);
        return 1;
    }

    if (a_buf->size_max == 0) {
        new_size = a_buf->size_real + 1.5 * increment;
    } else {
        if ((a_buf->size_real + increment) <= a_buf->size_max) {
            new_size = a_buf->size_real + increment;
        } else {
            new_size = a_buf->size_max;
            rc = 1;
        }
    }

    len = a_buf->tail - a_buf->start;
    p = (char *)realloc(a_buf->start, new_size);
    if (!p) {
        logc_error("realloc fail, errno[%d]", errno);

        free(a_buf->start);
        a_buf->start = NULL;
        a_buf->tail = NULL;
        a_buf->end = NULL;
        a_buf->end_plus_1 = NULL;

        return -1;
    } else {
        a_buf->start = p;
        a_buf->tail = p + len;
        a_buf->size_real = new_size;
        a_buf->end_plus_1 = a_buf->start + new_size;
        a_buf->end = a_buf->end_plus_1 - 1;
    }

    return rc;
}

int liblog_buf_vprintf(struct liblog_buf *a_buf, const char *format, va_list args)
{
    int nwrite;
    va_list ap;
    size_t size_left;

    if (!a_buf->start) {
        logc_error("pre-use of liblog_buf_resize fail, so can't convert");
        return -1;
    }

    va_copy(ap, args);
    size_left = a_buf->end_plus_1 - a_buf->tail;
    nwrite = vsnprintf(a_buf->tail, size_left, format, ap);
    if ((nwrite >= 0) && (nwrite < (int)size_left)) {
        a_buf->tail += nwrite;
        return 0;
    } else if (nwrite < 0) {
        logc_error("vsnprintf fail, errno[%d]", errno);
        logc_error("nwrite[%d], size_left[%ld], format[%s]", nwrite, size_left, format);

        return -1;
    } else if (nwrite >= (int)size_left) {
        int rc;

        rc = liblog_buf_resize(a_buf, nwrite - size_left + 1);
        if (rc > 0) {
            logc_error("conf limit to %ld, can't extend, so truncate", a_buf->size_max);

            va_copy(ap, args);
            size_left = a_buf->end_plus_1 - a_buf->tail;
            vsnprintf(a_buf->tail, size_left, format, ap);
            a_buf->tail += size_left - 1;

            liblog_buf_truncate(a_buf);
            return 1;
        } else if (rc < 0) {
            logc_error("liblog_buf_resize fail");
            return -1;
        } else {
            va_copy(ap, args);
            size_left = a_buf->end_plus_1 - a_buf->tail;
            nwrite = vsnprintf(a_buf->tail, size_left, format, ap);
            if (nwrite < 0) {
                logc_error("vsnprintf fail, errno[%d]", errno);
                logc_error("nwrite[%d], size_left[%ld], format[%s]", nwrite, size_left, format);

                return -1;
            } else {
                a_buf->tail += nwrite;
                return 0;
            }
        }
    }

    return 0;
}

int liblog_buf_printf_dec32(struct liblog_buf *a_buf, uint32_t ui32, int width)
{
    char *q;
    unsigned char *p;
    size_t num_len, zero_len, out_len;
    unsigned char tmp[LOG_INT32_LEN + 1];

    if (!a_buf->start) {
        logc_error("pre-use of liblog_buf_resize fail, so can't convert");
        return -1;
    }

    p = tmp + LOG_INT32_LEN;
    do {
        *--p = (unsigned char)(ui32 % 10 + '0');
    } while (ui32 /= 10);

    num_len = (tmp + LOG_INT32_LEN) - p;
    if (width > (int)num_len) {
        zero_len = width - num_len;
        out_len = width;
    } else {
        zero_len = 0;
        out_len = num_len;
    }

    if ((q = a_buf->tail + out_len) > a_buf->end) {
        int rc;

        rc = liblog_buf_resize(a_buf, out_len - (a_buf->end - a_buf->tail));
        if (rc > 0) {
            size_t len_left;

            logc_error("conf limit to %ld, can't extend, so ouput", a_buf->size_max);

            len_left = a_buf->end - a_buf->tail;
            if (len_left <= zero_len) {
                zero_len = len_left;
                num_len = 0;
            } else if (len_left > zero_len) {
                num_len = len_left - zero_len;
            }

            if (zero_len) {
                memset(a_buf->tail, '0', zero_len);
            }

            memcpy(a_buf->tail + zero_len, p, num_len);
            a_buf->tail += len_left;

            liblog_buf_truncate(a_buf);
            return 1;
        } else if (rc < 0) {
            logc_error("liblog_buf_resize fail");
            return -1;
        } else {
            q = a_buf->tail + out_len;
        }
    }

    if (zero_len) {
        memset(a_buf->tail, '0', zero_len);
    }

    memcpy(a_buf->tail + zero_len, p, num_len);
    a_buf->tail = q;

    return 0;
}

int liblog_buf_printf_dec64(struct liblog_buf *a_buf, uint64_t ui64, int width)
{
    char *q;
    uint32_t ui32;
    unsigned char *p;
    size_t num_len, zero_len, out_len;
    unsigned char tmp[LOG_INT64_LEN + 1];

    if (!a_buf->start) {
        logc_error("pre-use of liblog_buf_resize fail, so can't convert");
        return -1;
    }

    p = tmp + LOG_INT64_LEN;
    if (ui64 <= LOG_MAX_UINT32_VALUE) {
        ui32 = (uint32_t)ui64;
        do {
            *--p = (unsigned char)(ui32 % 10 + '0');
        } while (ui32 /= 10);
    } else {
        do {
            *--p = (unsigned char)(ui64 % 10 + '0');
        } while (ui64 /= 10);
    }

    num_len = (tmp + LOG_INT64_LEN) - p;
    if (width > (int)num_len) {
        zero_len = width - num_len;
        out_len = width;
    } else {
        zero_len = 0;
        out_len = num_len;
    }

    if ((q = a_buf->tail + out_len) > a_buf->end) {
        int rc;

        rc = liblog_buf_resize(a_buf, out_len - (a_buf->end - a_buf->tail));
        if (rc > 0) {
            size_t len_left;

            logc_error("conf limit to %ld, can't extend, so output", a_buf->size_max);

            len_left = a_buf->end - a_buf->tail;
            if (len_left <= zero_len) {
                zero_len = len_left;
                num_len = 0;
            } else if (len_left > zero_len) {
                num_len = len_left - zero_len;
            }

            if (zero_len) {
                memset(a_buf->tail, '0', zero_len);
            }

            memcpy(a_buf->tail + zero_len, p, num_len);
            a_buf->tail += len_left;

            liblog_buf_truncate(a_buf);
            return 1;
        } else if (rc < 0) {
            logc_error("liblog_buf_resize fail");
            return -1;
        } else {
            q = a_buf->tail + out_len;
        }
    }

    if (zero_len) {
        memset(a_buf->tail, '0', zero_len);
    }

    memcpy(a_buf->tail + zero_len, p, num_len);
    a_buf->tail = q;

    return 0;
}

int liblog_buf_printf_hex(struct liblog_buf *a_buf, uint32_t ui32, int width)
{
    char *q;
    unsigned char *p;
    size_t num_len, zero_len, out_len;
    unsigned char tmp[LOG_INT32_LEN + 1];
    static unsigned char hex[] = "0123456789abcdef";

    if (!a_buf->start) {
        logc_error("pre-use of liblog_buf_resize fail, so can't convert");
        return -1;
    }

    p = tmp + LOG_INT32_LEN;
    do {
        *--p = hex[(uint32_t)(ui32 & 0xF)];
    } while (ui32 >>= 4);

    num_len = (tmp + LOG_INT32_LEN) - p;
    if (width > (int)num_len) {
        zero_len = width - num_len;
        out_len = width;
    } else {
        zero_len = 0;
        out_len = num_len;
    }

    if ((q = a_buf->tail + out_len) > a_buf->end) {
        int rc;

        rc = liblog_buf_resize(a_buf, out_len - (a_buf->end - a_buf->tail));
        if (rc > 0) {
            size_t len_left;

            logc_error("conf limit to %ld, can't extend, so output", a_buf->size_max);

            len_left = a_buf->end - a_buf->tail;
            if (len_left <= zero_len) {
                zero_len = len_left;
                num_len = 0;
            } else if (len_left > zero_len) {
                num_len = len_left - zero_len;
            }

            if (zero_len) {
                memset(a_buf->tail, '0', zero_len);
            }

            memcpy(a_buf->tail + zero_len, p, num_len);
            a_buf->tail += len_left;

            liblog_buf_truncate(a_buf);
            return 1;
        } else if (rc < 0) {
            logc_error("liblog_buf_resize fail");
            return -1;
        } else {
            q = a_buf->tail + out_len;
        }
    }

    if (zero_len) {
        memset(a_buf->tail, '0', zero_len);
    }

    memcpy(a_buf->tail + zero_len, p, num_len);
    a_buf->tail = q;

    return 0;
}

int liblog_buf_append(struct liblog_buf *a_buf, const char *str, size_t str_len)
{
    char *p;

    if ((p = a_buf->tail + str_len) > a_buf->end) {
        int rc;

        rc = liblog_buf_resize(a_buf, str_len - (a_buf->end - a_buf->tail));
        if (rc > 0) {
            size_t len_left;

            logc_error("conf limit to %ld, can't extend, so output", a_buf->size_max);

            len_left = a_buf->end - a_buf->tail;
            memcpy(a_buf->tail, str, len_left);
            a_buf->tail += len_left;

            liblog_buf_truncate(a_buf);
            return 1;
        } else if (rc < 0) {
            logc_error("liblog_buf_resize fail");
            return -1;
        } else {
            p = a_buf->tail + str_len;
        }
    }

    memcpy(a_buf->tail, str, str_len);
    a_buf->tail = p;

    return 0;
}

int liblog_buf_adjust_append(struct liblog_buf *a_buf, const char *str, size_t str_len, int left_adjust, int zero_pad, size_t in_width, size_t out_width)
{
    size_t space_len = 0;
    size_t source_len = 0;
    size_t append_len = 0;

    if (!a_buf->start) {
        logc_error("pre-use of liblog_buf_resize fail, so can't convert");
        return -1;
    }

    if ((out_width == 0) || (str_len < out_width)) {
        source_len = str_len;
    } else {
        source_len = out_width;
    }

    if ((in_width == 0) || (source_len >= in_width)) {
        append_len = source_len;
        space_len = 0;
    } else {
        append_len = in_width;
        space_len = in_width - source_len;
    }

    if (append_len > (size_t)(a_buf->end - a_buf->tail)) {
        int rc = 0;

        rc = liblog_buf_resize(a_buf, append_len - (a_buf->end - a_buf->tail));
        if (rc > 0) {
            logc_error("conf limit to %ld, can't extend, so output", a_buf->size_max);

            append_len = (a_buf->end - a_buf->tail);
            if (left_adjust) {
                if (source_len < append_len) {
                    space_len = append_len - source_len;
                } else {
                    source_len = append_len;
                    space_len = 0;
                }

                if (space_len) {
                    memset(a_buf->tail + source_len, ' ', space_len);
                }
                memcpy(a_buf->tail, str, source_len);
            } else {
                if (space_len < append_len) {
                    source_len = append_len - space_len;
                } else {
                    space_len = append_len;
                    source_len = 0;
                }

                if (space_len) {
                    if (zero_pad) {
                        memset(a_buf->tail, '0', space_len);
                    } else {
                        memset(a_buf->tail, ' ', space_len);
                    }
                }
                memcpy(a_buf->tail + space_len, str, source_len);
            }

            a_buf->tail += append_len;
            liblog_buf_truncate(a_buf);

            return 1;
        } else if (rc < 0) {
            logc_error("liblog_buf_resize fail");
            return -1;
        } else {

        }
    }

    if (left_adjust) {
        if (space_len) {
            memset(a_buf->tail + source_len, ' ', space_len);
        }

        memcpy(a_buf->tail, str, source_len);
    } else {
        if (space_len) {
            if (zero_pad) {
                memset(a_buf->tail, '0', space_len);
            } else {
                memset(a_buf->tail, ' ', space_len);
            }
        }
        memcpy(a_buf->tail + space_len, str, source_len);
    }

    a_buf->tail += append_len;
    return 0;
}
