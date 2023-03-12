#ifndef LIBLOG_RULE_H
#define LIBLOG_RULE_H

#include <stdio.h>
#include <pthread.h>

#include "format.h"
#include "thread.h"
#include "record.h"
#include "rotater.h"
#include "logc_defs.h"

struct liblog_rule;
typedef int (*rule_output_func)(struct liblog_rule *a_rule, struct liblog_thread *a_thread);

struct liblog_rule {
    char                  category[LOG_MAXLEN_CFG_LINE + 1];
    char                  compare_char;

    int                   level;
    unsigned char         level_bitmap[32];

    unsigned int          file_perms;
    int                   file_open_flags;

    char                  file_path[LOG_MAXLEN_PATH + 1];
    struct logc_arraylist *dynamic_specs;
    int                   static_fd;
    dev_t                 static_dev;
    ino_t                 static_ino;

    long                  archive_max_size;
    int                   archive_max_count;
    char                  archive_path[LOG_MAXLEN_PATH + 1];
    struct logc_arraylist *archive_specs;

    FILE                  *pipe_fp;
    int                   pipe_fd;

    size_t                fsync_period;
    size_t                fsync_count;

    struct logc_arraylist *levels;
    int                   syslog_facility;

    struct liblog_format  *format;
    rule_output_func      output;

    char                  record_name[LOG_MAXLEN_PATH + 1];
    char                  record_path[LOG_MAXLEN_PATH + 1];
    liblog_record_func    record_func;
};

struct liblog_rule *liblog_rule_new(char *line, struct logc_arraylist *levels, struct liblog_format *default_format, struct logc_arraylist *formats, unsigned int file_perms, size_t fsync_period, int *time_cache_count);

void liblog_rule_del(struct liblog_rule *a_rule);
void liblog_rule_profile(struct liblog_rule *a_rule, int flag);

int liblog_rule_is_wastebin(struct liblog_rule *a_rule);
int liblog_rule_match_category(struct liblog_rule *a_rule, char *category);
int liblog_rule_output(struct liblog_rule *a_rule, struct liblog_thread *a_thread);
int liblog_rule_set_record(struct liblog_rule *a_rule, struct logc_hashtable *records);

#endif
