#ifndef LEVEL_H
#define LEVEL_H

#include "logc_defs.h"

struct liblog_level {
    int    int_level;
    char   str_uppercase[LOG_MAXLEN_PATH + 1];
    char   str_lowercase[LOG_MAXLEN_PATH + 1];
    size_t str_len;
    int    syslog_level;
};

struct liblog_level *liblog_level_new(char *line);
void liblog_level_del(struct liblog_level *a_level);
void liblog_level_profile(struct liblog_level *a_level, int flag);

#endif
