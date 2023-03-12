#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/syscall.h>

#include "event.h"
#include "fmacros.h"
#include "logc_defs.h"

void liblog_event_profile(struct liblog_event *a_event, int flag)
{
    logc_assert(a_event, );
    logc_profile(flag, "---event[%p][%s,%s][%s(%ld),%s(%ld),%ld,%d][%p,%s][%ld,%ld][%ld,%ld][%d]---",
        a_event, a_event->category_name, a_event->host_name,
        a_event->file, a_event->file_len, a_event->func, a_event->func_len,
        a_event->line, a_event->level, a_event->hex_buf, a_event->str_format,
        a_event->time_stamp.tv_sec, a_event->time_stamp.tv_usec,
        (long)a_event->pid, (long)a_event->tid, a_event->time_cache_count);
}

void liblog_event_del(struct liblog_event *a_event)
{
    logc_assert(a_event, );
    if (a_event->time_caches) {
        free(a_event->time_caches);
    }

    logc_debug("liblog_event_del[%p]", a_event);
    free(a_event);
}

struct liblog_event *liblog_event_new(int time_cache_count)
{
    struct liblog_event *a_event;

    a_event = (struct liblog_event *)calloc(1, sizeof(struct liblog_event));
    if (!a_event) {
        logc_error("calloc fail, errno[%d]", errno);
        return NULL;
    }

    a_event->time_caches = (struct liblog_time_cache *)calloc(time_cache_count, sizeof(struct liblog_time_cache));
    if (!a_event->time_caches) {
        logc_error("calloc fail, errno[%d]", errno);
        free(a_event);

        return NULL;
    }

    a_event->time_cache_count = time_cache_count;

    if (gethostname(a_event->host_name, sizeof(a_event->host_name) - 1)) {
        logc_error("gethostname fail, errno[%d]", errno);
        goto err;
    }

    a_event->host_name_len = strlen(a_event->host_name);
    a_event->tid = pthread_self();
    a_event->tid_str_len = sprintf(a_event->tid_str, "%lu", (unsigned long)a_event->tid);
    a_event->tid_hex_str_len = sprintf(a_event->tid_hex_str, "%X", (unsigned int)a_event->tid);

    a_event->ktid = syscall(SYS_gettid);
    a_event->ktid_str_len = sprintf(a_event->ktid_str, "%u", (unsigned int)a_event->ktid);

    return a_event;

err:
    liblog_event_del(a_event);
    return NULL;
}

void liblog_event_set_fmt(struct liblog_event *a_event, char *category_name, size_t category_name_len, const char *file, size_t file_len, const char *func, size_t func_len, long line, int level, const char *str_format, va_list str_args)
{
    a_event->category_name = category_name;
    a_event->category_name_len = category_name_len;

    a_event->file = (char *)file;
    a_event->file_len = file_len;
    a_event->func = (char *)func;
    a_event->func_len = func_len;
    a_event->line = line;
    a_event->level = level;

    a_event->generate_cmd = LIBLOG_FMT;
    a_event->str_format = str_format;
    va_copy(a_event->str_args, str_args);

    a_event->pid = (pid_t)0;
    a_event->time_stamp.tv_sec = 0;
}

void liblog_event_set_hex(struct liblog_event *a_event, char *category_name, size_t category_name_len, const char *file, size_t file_len, const char *func, size_t func_len, long line, int level, const void *hex_buf, size_t hex_buf_len)
{
    a_event->category_name = category_name;
    a_event->category_name_len = category_name_len;

    a_event->file = (char *)file;
    a_event->file_len = file_len;
    a_event->func = (char *)func;
    a_event->func_len = func_len;
    a_event->line = line;
    a_event->level = level;

    a_event->generate_cmd = LIBLOG_HEX;
    a_event->hex_buf = hex_buf;
    a_event->hex_buf_len = hex_buf_len;

    a_event->pid = (pid_t)0;
    a_event->time_stamp.tv_sec = 0;
}
