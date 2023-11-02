#ifndef CONF_H
#define CONF_H

#include "format.h"
#include "rotater.h"
#include "logc_defs.h"

struct liblog_conf {
    char                  file[LOG_MAXLEN_PATH + 1];
    char                  cfg_ptr[LOG_MAXLEN_CFG_LINE * LOG_MAXLINES_NO];
    char                  mtime[20 + 1];

    int                   strict_init;
    size_t                buf_size_min;
    size_t                buf_size_max;

    char                  rotate_lock_file[LOG_MAXLEN_CFG_LINE + 1];
    struct liblog_rotater *rotater;

    char                  default_format_line[LOG_MAXLEN_CFG_LINE + 1];
    struct liblog_format  *default_format;

    unsigned int          file_perms;
    size_t                fsync_period;
    size_t                reload_conf_period;

    struct logc_arraylist *levels;
    struct logc_arraylist *formats;
    struct logc_arraylist *rules;
    int                   time_cache_count;
    char                  log_level[LOG_MAXLEN_CFG_LINE + 1];
    int                   level;
};

extern struct liblog_conf *liblog_env_conf;

void liblog_conf_del(struct liblog_conf *a_conf);
struct liblog_conf *liblog_conf_new(const char *confpath);
void liblog_conf_profile(struct liblog_conf *a_conf, int flag);
struct liblog_conf *liblog_conf_new_from_string(const char *config_string);

#endif
