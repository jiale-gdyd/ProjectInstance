#ifndef SPEC_H
#define SPEC_H

#include <stddef.h>
#include <sys/types.h>

#include "buf.h"
#include "event.h"
#include "thread.h"

struct liblog_spec;
typedef int (*spec_gen_func)(struct liblog_spec *a_spec, struct liblog_thread *a_thread);
typedef int (*spec_write_func)(struct liblog_spec *a_spec, struct liblog_thread *a_thread, struct liblog_buf *a_buf);

struct liblog_spec {
    char                 *str;
    int                  len;

    char                 time_fmt[LOG_MAXLEN_CFG_LINE + 1];
    int                  time_cache_index;
    char                 mdc_key[LOG_MAXLEN_PATH + 1];

    char                 print_fmt[LOG_MAXLEN_CFG_LINE + 1];
    int                  left_adjust;
    int                  left_fill_zeros;
    size_t               max_width;
    size_t               min_width;

    spec_write_func      write_buf;
    spec_gen_func        gen_msg;
    spec_gen_func        gen_path;
    spec_gen_func        gen_archive_path;
};

void liblog_spec_del(struct liblog_spec *a_spec);
void liblog_spec_profile(struct liblog_spec *a_spec, int flag);
struct liblog_spec *liblog_spec_new(char *pattern_start, char **pattern_next, int *time_cache_count);

#define liblog_spec_gen_msg(a_spec, a_thread)             a_spec->gen_msg(a_spec, a_thread)
#define liblog_spec_gen_path(a_spec, a_thread)            a_spec->gen_path(a_spec, a_thread)
#define liblog_spec_gen_archive_path(a_spec, a_thread)    a_spec->gen_archive_path(a_spec, a_thread)

#endif
