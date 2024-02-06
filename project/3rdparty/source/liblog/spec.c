#include <time.h>
#include <errno.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <sys/time.h>
#include <sys/types.h>

#include "conf.h"
#include "spec.h"
#include "fmacros.h"
#include "logc_defs.h"
#include "level_list.h"

#define LOG_DEFAULT_TIME_FMT       "%F %T"
#define LOG_HEX_HEAD               "\n             0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F    0123456789ABCDEF"

void liblog_spec_profile(struct liblog_spec *a_spec, int flag)
{
    logc_assert(a_spec, );
    logc_profile(flag, "----spec[%p][%.*s][%s|%d][%s,%ld,%ld,%s][%s]----", a_spec,
        a_spec->len, a_spec->str, a_spec->time_fmt, a_spec->time_cache_index,
        a_spec->print_fmt, (long)a_spec->max_width, (long)a_spec->min_width, a_spec->left_fill_zeros ? "true" : "false", a_spec->mdc_key);
}

static int liblog_spec_write_time_internal(struct liblog_spec *a_spec, struct liblog_thread *a_thread, struct liblog_buf *a_buf, short use_utc)
{
    time_t now_sec = a_thread->event->time_stamp.tv_sec;
    struct liblog_time_cache *a_cache = a_thread->event->time_caches + a_spec->time_cache_index;

    struct tm *tim;
    time_t *time_sec;
    typedef struct tm *(*zlog_spec_time_fn) (const time_t *, struct tm *);
    zlog_spec_time_fn time_stamp_convert_function;

    if (use_utc) {
        tim = &(a_thread->event->time_utc);
        time_sec = &(a_thread->event->time_utc_sec);
        time_stamp_convert_function = gmtime_r;
    } else {
        tim = &(a_thread->event->time_local);
        time_sec = &(a_thread->event->time_local_sec);
        time_stamp_convert_function = localtime_r;
    }

    // 该事件符合他生命周期中的第一个time_spec
    if (!now_sec) {
        gettimeofday(&(a_thread->event->time_stamp), NULL);
        now_sec = a_thread->event->time_stamp.tv_sec;
    }

    // 当这个事件的最后一次缓存time_local不是现在
    if (*time_sec != now_sec) {
        time_stamp_convert_function(&(now_sec), tim);
        *time_sec = now_sec;
    }

    // 当这个规范的最后一个缓存时间字符串不是现在
    if (a_cache->sec != now_sec) {
        a_cache->len = strftime(a_cache->str, sizeof(a_cache->str), a_spec->time_fmt, tim);
        a_cache->sec = now_sec;
    }

    return liblog_buf_append(a_buf, a_cache->str, a_cache->len);
}

static int liblog_spec_write_time_UTC(struct liblog_spec *a_spec, struct liblog_thread *a_thread, struct liblog_buf *a_buf)
{
    return liblog_spec_write_time_internal(a_spec, a_thread, a_buf, 1);
}

static int liblog_spec_write_time_local(struct liblog_spec *a_spec, struct liblog_thread *a_thread, struct liblog_buf *a_buf)
{
    return liblog_spec_write_time_internal(a_spec, a_thread, a_buf, 0);
}

static int liblog_spec_write_ms(struct liblog_spec *a_spec, struct liblog_thread *a_thread, struct liblog_buf *a_buf)
{
    if (!a_thread->event->time_stamp.tv_sec) {
        gettimeofday(&(a_thread->event->time_stamp), NULL);
    }

    return liblog_buf_printf_dec32(a_buf, (a_thread->event->time_stamp.tv_usec / 1000), 3);
}

static int liblog_spec_write_us(struct liblog_spec *a_spec, struct liblog_thread *a_thread, struct liblog_buf *a_buf)
{
    if (!a_thread->event->time_stamp.tv_sec) {
        gettimeofday(&(a_thread->event->time_stamp), NULL);
    }

    return liblog_buf_printf_dec32(a_buf, a_thread->event->time_stamp.tv_usec, 6);
}

static int liblog_spec_write_mdc(struct liblog_spec *a_spec, struct liblog_thread *a_thread, struct liblog_buf *a_buf)
{
    struct liblog_mdc_kv *a_mdc_kv;

    a_mdc_kv = liblog_mdc_get_kv(a_thread->mdc, a_spec->mdc_key);
    if (!a_mdc_kv) {
        logc_error("liblog_mdc_get_kv key[%s], fail", a_spec->mdc_key);
        return 0;
    }

    return liblog_buf_append(a_buf, a_mdc_kv->value, a_mdc_kv->value_len);
}

static int liblog_spec_write_str(struct liblog_spec *a_spec, struct liblog_thread *a_thread, struct liblog_buf *a_buf)
{
    return liblog_buf_append(a_buf, a_spec->str, a_spec->len);
}

static int liblog_spec_write_category(struct liblog_spec *a_spec, struct liblog_thread *a_thread, struct liblog_buf *a_buf)
{
    return liblog_buf_append(a_buf, a_thread->event->category_name, a_thread->event->category_name_len);
}

static int liblog_spec_write_srcfile(struct liblog_spec *a_spec, struct liblog_thread *a_thread, struct liblog_buf *a_buf)
{
    if (!a_thread->event->file) {
        return liblog_buf_append(a_buf, "(file=null)", sizeof("(file=null)") - 1);
    } else {
        return liblog_buf_append(a_buf, a_thread->event->file, a_thread->event->file_len);
    }
}

static int liblog_spec_write_srcfile_neat(struct liblog_spec *a_spec, struct liblog_thread *a_thread, struct liblog_buf *a_buf)
{
    char *p;

    if ((p = (char *)strrchr(a_thread->event->file, '/')) != NULL) {
        return liblog_buf_append(a_buf, p + 1, (char *)a_thread->event->file + a_thread->event->file_len - p - 1);
    } else {
        if (!a_thread->event->file) {
            return liblog_buf_append(a_buf, "(file=null)", sizeof("(file=null)") - 1);
        } else {
            return liblog_buf_append(a_buf, a_thread->event->file, a_thread->event->file_len);
        }
    }
}

static int liblog_spec_write_srcline(struct liblog_spec *a_spec, struct liblog_thread *a_thread, struct liblog_buf *a_buf)
{
    return liblog_buf_printf_dec64(a_buf, a_thread->event->line, 0);
}

static int liblog_spec_write_srcfunc(struct liblog_spec *a_spec, struct liblog_thread *a_thread, struct liblog_buf *a_buf)
{
    if (!a_thread->event->func) {
        return liblog_buf_append(a_buf, "(func=null)", sizeof("(func=null)") - 1);
    } else {
        return liblog_buf_append(a_buf, a_thread->event->func, a_thread->event->func_len);
    }
}

static int liblog_spec_write_hostname(struct liblog_spec *a_spec, struct liblog_thread *a_thread, struct liblog_buf *a_buf)
{
    return liblog_buf_append(a_buf, a_thread->event->host_name, a_thread->event->host_name_len);
}

static int liblog_spec_write_newline(struct liblog_spec *a_spec, struct liblog_thread *a_thread, struct liblog_buf *a_buf)
{
    return liblog_buf_append(a_buf, LOG_FILE_NEWLINE, LOG_FILE_NEWLINE_LEN);
}

static int liblog_spec_write_cr(struct liblog_spec *a_spec, struct liblog_thread *a_thread, struct liblog_buf *a_buf)
{
    return liblog_buf_append(a_buf, "\r", 1);
}

static int liblog_spec_write_percent(struct liblog_spec *a_spec, struct liblog_thread *a_thread, struct liblog_buf *a_buf)
{
    return liblog_buf_append(a_buf, "%", 1);
}

static int liblog_spec_write_pid(struct liblog_spec *a_spec, struct liblog_thread *a_thread, struct liblog_buf *a_buf)
{
    if (!a_thread->event->pid) {
        a_thread->event->pid = getpid();

        if (a_thread->event->pid != a_thread->event->last_pid) {
            a_thread->event->last_pid = a_thread->event->pid;
            a_thread->event->pid_str_len = sprintf(a_thread->event->pid_str, "%u", a_thread->event->pid);
        }
    }

    return liblog_buf_append(a_buf, a_thread->event->pid_str, a_thread->event->pid_str_len);
}

static int liblog_spec_write_tid_hex(struct liblog_spec *a_spec, struct liblog_thread *a_thread, struct liblog_buf *a_buf)
{
    return liblog_buf_append(a_buf, a_thread->event->tid_hex_str, a_thread->event->tid_hex_str_len);
}

static int liblog_spec_write_tid_long(struct liblog_spec *a_spec, struct liblog_thread *a_thread, struct liblog_buf *a_buf)
{
    return liblog_buf_append(a_buf, a_thread->event->tid_str, a_thread->event->tid_str_len);
}

static int liblog_spec_write_ktid(struct liblog_spec *a_spec, struct liblog_thread *a_thread, struct liblog_buf *a_buf)
{
    return liblog_buf_append(a_buf, a_thread->event->ktid_str, a_thread->event->ktid_str_len);
}

static int liblog_spec_write_level_lowercase(struct liblog_spec *a_spec, struct liblog_thread *a_thread, struct liblog_buf *a_buf)
{
    struct liblog_level *a_level;

    a_level = liblog_level_list_get(liblog_env_conf->levels, a_thread->event->level);
    return liblog_buf_append(a_buf, a_level->str_lowercase, a_level->str_len);
}

static int liblog_spec_write_level_uppercase(struct liblog_spec *a_spec, struct liblog_thread *a_thread, struct liblog_buf *a_buf)
{
    struct liblog_level *a_level;

    a_level = liblog_level_list_get(liblog_env_conf->levels, a_thread->event->level);
    return liblog_buf_append(a_buf, a_level->str_uppercase, a_level->str_len);
}

static int liblog_spec_write_usrmsg(struct liblog_spec *a_spec, struct liblog_thread *a_thread, struct liblog_buf *a_buf)
{
    if (a_thread->event->generate_cmd == LIBLOG_FMT) {
        if (a_thread->event->str_format) {
            return liblog_buf_vprintf(a_buf, a_thread->event->str_format, a_thread->event->str_args);
        } else {
            return liblog_buf_append(a_buf, "format=(null)", sizeof("format=(null)") - 1);
        }
    } else if (a_thread->event->generate_cmd == LIBLOG_HEX) {
        int rc;
        long line_offset;
        long byte_offset;

        if (a_thread->event->hex_buf == NULL) {
            rc = liblog_buf_append(a_buf, "buf=(null)", sizeof("buf=(null)") - 1);
            goto liblog_hex_exit;
        }

        rc = liblog_buf_append(a_buf, LOG_HEX_HEAD, sizeof(LOG_HEX_HEAD) - 1);
        if (rc) {
            goto liblog_hex_exit;
        }

        line_offset = 0;

        while (1) {
            unsigned char ch;

            rc = liblog_buf_append(a_buf, "\n", 1);
            if (rc) {
                goto liblog_hex_exit;
            }

            rc = liblog_buf_printf_dec64(a_buf, line_offset + 1, 10);
            if (rc) {
                goto liblog_hex_exit;
            }

            rc = liblog_buf_append(a_buf, "   ", 3);
            if (rc) {
                goto liblog_hex_exit;
            }

            for (byte_offset = 0; byte_offset < 16; byte_offset++) {
                if ((line_offset * 16 + byte_offset) < (long)a_thread->event->hex_buf_len) {
                    ch = *((unsigned char *)a_thread->event->hex_buf + line_offset * 16 + byte_offset);
                    rc = liblog_buf_printf_hex(a_buf, ch, 2);
                    if (rc) {
                        goto liblog_hex_exit;
                    }

                    rc = liblog_buf_append(a_buf, " ", 1);
                    if (rc) {
                        goto liblog_hex_exit;
                    }
                } else {
                    rc = liblog_buf_append(a_buf, "   ", 3);
                    if (rc) {
                        goto liblog_hex_exit;
                    }
                }
            }

            rc = liblog_buf_append(a_buf, "  ", 2);
            if (rc) {
                goto liblog_hex_exit;
            }

            for (byte_offset = 0; byte_offset < 16; byte_offset++) {
                if ((line_offset * 16 + byte_offset) < (long)a_thread->event->hex_buf_len) {
                    ch  = *((unsigned char *)a_thread->event->hex_buf + line_offset * 16 + byte_offset);
                    if ((ch >= 32) && (ch <= 126)) {
                        rc = liblog_buf_append(a_buf, (char *)&ch, 1);
                        if (rc) {
                            goto liblog_hex_exit;
                        }
                    } else {
                        rc = liblog_buf_append(a_buf, ".", 1);
                        if (rc) {
                            goto liblog_hex_exit;
                        }
                    }
                } else {
                    rc = liblog_buf_append(a_buf, " ", 1);
                    if (rc) {
                        goto liblog_hex_exit;
                    }
                }
            }

            if ((line_offset * 16 + byte_offset) >= (long)a_thread->event->hex_buf_len) {
                break;
            }
            line_offset++;
        }

liblog_hex_exit:
        if (rc < 0) {
            logc_error("write hex msg fail");
            return -1;
        } else if (rc > 0) {
            logc_error("write hex msg, buf is full");
            return -1;
        }

        return 0;
    }

    return 0;
}

static int liblog_spec_gen_msg_direct(struct liblog_spec *a_spec, struct liblog_thread *a_thread)
{
    return a_spec->write_buf(a_spec, a_thread, a_thread->msg_buf);
}

static int liblog_spec_gen_msg_reformat(struct liblog_spec *a_spec, struct liblog_thread *a_thread)
{
    int rc;

    liblog_buf_restart(a_thread->pre_msg_buf);

    rc = a_spec->write_buf(a_spec, a_thread, a_thread->pre_msg_buf);
    if (rc < 0) {
        logc_error("a_spec->gen_buf fail");
        return -1;
    } else if (rc > 0) {

    }

    return liblog_buf_adjust_append(a_thread->msg_buf, liblog_buf_str(a_thread->pre_msg_buf), liblog_buf_len(a_thread->pre_msg_buf), a_spec->left_adjust, a_spec->left_fill_zeros, a_spec->min_width, a_spec->max_width);
}

static int liblog_spec_gen_path_direct(struct liblog_spec *a_spec, struct liblog_thread *a_thread)
{
    return a_spec->write_buf(a_spec, a_thread, a_thread->path_buf);
}

static int liblog_spec_gen_path_reformat(struct liblog_spec *a_spec, struct liblog_thread *a_thread)
{
    int rc;

    liblog_buf_restart(a_thread->pre_path_buf);

    rc = a_spec->write_buf(a_spec, a_thread, a_thread->pre_path_buf);
    if (rc < 0) {
        logc_error("a_spec->gen_buf fail");
        return -1;
    } else if (rc > 0) {

    }

    return liblog_buf_adjust_append(a_thread->path_buf, liblog_buf_str(a_thread->pre_path_buf), liblog_buf_len(a_thread->pre_path_buf), a_spec->left_adjust, a_spec->left_fill_zeros, a_spec->min_width, a_spec->max_width);
}

static int liblog_spec_gen_archive_path_direct(struct liblog_spec *a_spec, struct liblog_thread *a_thread)
{
    return a_spec->write_buf(a_spec, a_thread, a_thread->archive_path_buf);
}

static int liblog_spec_gen_archive_path_reformat(struct liblog_spec *a_spec, struct liblog_thread *a_thread)
{
    int rc;

    liblog_buf_restart(a_thread->pre_path_buf);

    rc = a_spec->write_buf(a_spec, a_thread, a_thread->pre_path_buf);
    if (rc < 0) {
        logc_error("a_spec->gen_buf fail");
        return -1;
    } else if (rc > 0) {

    }

    return liblog_buf_adjust_append(a_thread->archive_path_buf, liblog_buf_str(a_thread->pre_path_buf), liblog_buf_len(a_thread->pre_path_buf), a_spec->left_adjust, a_spec->left_fill_zeros, a_spec->min_width, a_spec->max_width);
}

static int liblog_spec_parse_print_fmt(struct liblog_spec *a_spec)
{
    long i, j;
    char *p, *q;

    p = a_spec->print_fmt;
    if (*p == '-') {
        a_spec->left_adjust = 1;
        p++;
    } else {
        if (*p == '0') {
            a_spec->left_fill_zeros = 1;
        }

        a_spec->left_adjust = 0;
    }

    i = j = 0;
    sscanf(p, "%ld.", &i);
    q = strchr(p, '.');
    if (q) {
        sscanf(q, ".%ld", &j);
    }

    a_spec->min_width = (size_t)i;
    a_spec->max_width = (size_t)j;

    return 0;
}

void liblog_spec_del(struct liblog_spec *a_spec)
{
    logc_assert(a_spec, );
    logc_debug("liblog_spec_del[%p]", a_spec);
    free(a_spec);
}

struct liblog_spec *liblog_spec_new(char *pattern_start, char **pattern_next, int *time_cache_count)
{
    char *p;
    int nscan = 0;
    int nread = 0;
    struct liblog_spec *a_spec;

    logc_assert(pattern_start, NULL);
    logc_assert(pattern_next, NULL);

    a_spec = (struct liblog_spec *)calloc(1, sizeof(struct liblog_spec));
    if (!a_spec) {
        logc_error("calloc fail, errno[%d]", errno);
        return NULL;
    }

    a_spec->str = p = pattern_start;

    switch (*p) {
        case '%':
            nread = 0;
            nscan = sscanf(p, "%%%[.0-9-]%n", a_spec->print_fmt, &nread);
            if (nscan == 1) {
                a_spec->gen_msg = liblog_spec_gen_msg_reformat;
                a_spec->gen_path = liblog_spec_gen_path_reformat;
                a_spec->gen_archive_path = liblog_spec_gen_archive_path_reformat;
                if (liblog_spec_parse_print_fmt(a_spec)) {
                    logc_error("liblog_spec_parse_print_fmt fail");
                    goto err;
                }
            } else {
                nread = 1;
                a_spec->gen_msg = liblog_spec_gen_msg_direct;
                a_spec->gen_path = liblog_spec_gen_path_direct;
                a_spec->gen_archive_path = liblog_spec_gen_archive_path_direct;
            }

            p += nread;

            if ((*p == 'd') || (*p == 'g')) {
                short use_utc = *p == 'g';

                if (*(p + 1) != '(') {
                    strcpy(a_spec->time_fmt, LOG_DEFAULT_TIME_FMT);
                    p++;
                } else if (LOG_STRNCMP(p, ==, "d()", 3)) {
                    strcpy(a_spec->time_fmt, LOG_DEFAULT_TIME_FMT);
                    p += 3;
                } else {
                    nread = 0;
                    p++;
                    nscan = sscanf(p, "(%[^)])%n", a_spec->time_fmt, &nread);
                    if (nscan != 1) {
                        nread = 0;
                    }

                    p += nread;
                    if (*(p - 1) != ')') {
                        logc_error("in string[%s] can't find match \')\'", a_spec->str);
                        goto err;
                    }
                }

                a_spec->time_cache_index = *time_cache_count;
                (*time_cache_count)++;
                if (use_utc) {
                    a_spec->write_buf = liblog_spec_write_time_UTC;
                } else {
                    a_spec->write_buf = liblog_spec_write_time_local;
                }

                *pattern_next = p;
                a_spec->len = p - a_spec->str;
                break;
            }

            if (*p == 'M') {
                nread = 0;
                nscan = sscanf(p, "M(%[^)])%n", a_spec->mdc_key, &nread);
                if (nscan != 1) {
                    nread = 0;
                    if (LOG_STRNCMP(p, ==, "M()", 3)) {
                        nread = 3;
                    }
                }

                p += nread;
                if (*(p - 1) != ')') {
                    logc_error("in string[%s] can't find match \')\'", a_spec->str);
                    goto err;
                }

                *pattern_next = p;
                a_spec->len = p - a_spec->str;
                a_spec->write_buf = liblog_spec_write_mdc;
                break;
            }

            if (LOG_STRNCMP(p, ==, "ms", 2)) {
                p += 2;
                *pattern_next = p;
                a_spec->len = p - a_spec->str;
                a_spec->write_buf = liblog_spec_write_ms;
                break;
            } else if (LOG_STRNCMP(p, ==, "us", 2)) {
                p += 2;
                *pattern_next = p;
                a_spec->len = p - a_spec->str;
                a_spec->write_buf = liblog_spec_write_us;
                break;
            }

            *pattern_next = p + 1;
            a_spec->len = p - a_spec->str + 1;

            switch (*p) {
                case 'c':
                    a_spec->write_buf = liblog_spec_write_category;
                    break;

                case 'D':
                    strcpy(a_spec->time_fmt, LOG_DEFAULT_TIME_FMT);
                    a_spec->time_cache_index = *time_cache_count;
                    (*time_cache_count)++;
                    a_spec->write_buf = liblog_spec_write_time_local;
                    break;

                case 'F':
                    a_spec->write_buf = liblog_spec_write_srcfile;
                    break;

                case 'f':
                    a_spec->write_buf = liblog_spec_write_srcfile_neat;
                    break;

                case 'G':
                    strcpy(a_spec->time_fmt, LOG_DEFAULT_TIME_FMT);
                    a_spec->time_cache_index = *time_cache_count;
                    (*time_cache_count)++;
                    a_spec->write_buf = liblog_spec_write_time_UTC;
                    break;

                case 'H':
                    a_spec->write_buf = liblog_spec_write_hostname;
                    break;

                case 'k':
                    a_spec->write_buf = liblog_spec_write_ktid;
                    break;

                case 'L':
                    a_spec->write_buf = liblog_spec_write_srcline;
                    break;

                case 'm':
                    a_spec->write_buf = liblog_spec_write_usrmsg;
                    break;

                case 'n':
                    a_spec->write_buf = liblog_spec_write_newline;
                    break;

                case 'r':
                    a_spec->write_buf = liblog_spec_write_cr;
                    break;

                case 'p':
                    a_spec->write_buf = liblog_spec_write_pid;
                    break;

                case 'U':
                    a_spec->write_buf = liblog_spec_write_srcfunc;
                    break;

                case 'v':
                    a_spec->write_buf = liblog_spec_write_level_lowercase;
                    break;

                case 'V':
                    a_spec->write_buf = liblog_spec_write_level_uppercase;
                    break;

                case 't':
                    a_spec->write_buf = liblog_spec_write_tid_hex;
                    break;

                case 'T':
                    a_spec->write_buf = liblog_spec_write_tid_long;
                    break;

                case '%':
                    a_spec->write_buf = liblog_spec_write_percent;
                    break;

                default:
                    logc_error("str[%s] in wrong format, p[%c]", a_spec->str, *p);
                    goto err;
            }
            break;

        default:
            *pattern_next = strchr(p, '%');
            if (*pattern_next) {
                a_spec->len = *pattern_next - p;
            } else {
                a_spec->len = strlen(p);
                *pattern_next = p + a_spec->len;
            }
            a_spec->write_buf = liblog_spec_write_str;
            a_spec->gen_msg = liblog_spec_gen_msg_direct;
            a_spec->gen_path = liblog_spec_gen_path_direct;
            a_spec->gen_archive_path = liblog_spec_gen_archive_path_direct;
    }

    liblog_spec_profile(a_spec, LOGC_DEBUG);
    return a_spec;

err:
    liblog_spec_del(a_spec);
    return NULL;
}
