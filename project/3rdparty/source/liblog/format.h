#ifndef FORMAT_H
#define FORMAT_H

#include "thread.h"
#include "logc_defs.h"

struct liblog_format {
    char                  name[LOG_MAXLEN_CFG_LINE + 1];
    char                  pattern[LOG_MAXLEN_CFG_LINE + 1];
    struct logc_arraylist *pattern_specs;
};

void liblog_format_del(struct liblog_format *a_format);
void liblog_format_profile(struct liblog_format *a_format, int flag);
struct liblog_format *liblog_format_new(char *line, int *time_cache_count);
int liblog_format_gen_msg(struct liblog_format *a_format, struct liblog_thread *a_thread);

#define liblog_format_has_name(a_format, fname)     LOG_STRCMP(a_format->name, ==, fname)

#endif
