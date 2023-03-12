#ifndef EVENT_H
#define EVENT_H

#include <stdarg.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/types.h>

#include "logc_defs.h"

enum liblog_event_cmd {
    LIBLOG_FMT = 0,
    LIBLOG_HEX = 1,
};

struct liblog_time_cache {
    char   str[LOG_MAXLEN_CFG_LINE + 1];
    size_t len;
    time_t sec;
};

struct liblog_event {
    char                     *category_name;
    size_t                   category_name_len;
    char                     host_name[256 + 1];
    size_t                   host_name_len;

    const char               *file;
    size_t                   file_len;
    const char               *func;
    size_t                   func_len;
    long                     line;
    int                      level;

    const void               *hex_buf;
    size_t                   hex_buf_len;
    const char               *str_format;
    va_list                  str_args;
    enum liblog_event_cmd    generate_cmd;

    struct timeval           time_stamp;

    time_t                   time_utc_sec;
    struct tm                time_utc;
    time_t                   time_local_sec;
    struct tm                time_local;

    struct liblog_time_cache *time_caches;
    int                      time_cache_count;

    pid_t                    pid;
    pid_t                    last_pid;
    char                     pid_str[30 + 1];
    size_t                   pid_str_len;

    pthread_t                tid;
    char                     tid_str[30 + 1];
    size_t                   tid_str_len;

    char                     tid_hex_str[30 + 1];
    size_t                   tid_hex_str_len;

    pid_t                    ktid;
    char                     ktid_str[30 + 1];
    size_t                   ktid_str_len;
};

void liblog_event_del(struct liblog_event *a_event);
struct liblog_event *liblog_event_new(int time_cache_count);
void liblog_event_profile(struct liblog_event *a_event, int flag);

void liblog_event_set_hex(struct liblog_event *a_event, char *category_name, size_t category_name_len, const char *file, size_t file_len, const char *func, size_t func_len, long line, int level, const void *hex_buf, size_t hex_buf_len);
void liblog_event_set_fmt(struct liblog_event *a_event, char *category_name, size_t category_name_len, const char *file, size_t file_len, const char *func, size_t func_len, long line, int level, const char *str_format, va_list str_args);

#endif
