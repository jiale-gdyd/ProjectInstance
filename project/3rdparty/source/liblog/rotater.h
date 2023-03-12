#ifndef ROTATER_H
#define ROTATER_H

#include "lockfile.h"
#include "logc_defs.h"

struct liblog_rotater {
    pthread_mutex_t       lock_mutex;
    char                  *lock_file;
    int                   lock_fd;

    char                  *base_path;
    char                  *archive_path;
    char                  glob_path[LOG_MAXLEN_PATH + 1];
    size_t                num_start_len;
    size_t                num_end_len;
    int                   num_width;
    int                   mv_type;
    int                   max_count;
    struct logc_arraylist *files;
};

void liblog_rotater_del(struct liblog_rotater *a_rotater);
struct liblog_rotater *liblog_rotater_new(char *lock_file);

void liblog_rotater_profile(struct liblog_rotater *a_rotater, int flag);
int liblog_rotater_rotate(struct liblog_rotater *a_rotater, char *base_path, size_t msg_len, char *archive_path, long archive_max_size, int archive_max_count);

#endif
