#ifndef THREAD_H
#define THREAD_H

#include "buf.h"
#include "mdc.h"
#include "event.h"
#include "logc_defs.h"

struct liblog_thread {
    int                 init_version;
    struct liblog_mdc   *mdc;
    struct liblog_event *event;

    struct liblog_buf   *pre_path_buf;
    struct liblog_buf   *path_buf;
    struct liblog_buf   *archive_path_buf;
    struct liblog_buf   *pre_msg_buf;
    struct liblog_buf   *msg_buf;
};

void liblog_thread_del(struct liblog_thread *a_thread);
void liblog_thread_profile(struct liblog_thread *a_thread, int flag);
struct liblog_thread *liblog_thread_new(int init_version, size_t buf_size_min, size_t buf_size_max, int time_cache_count);

int liblog_thread_rebuild_event(struct liblog_thread *a_thread, int time_cache_count);
int liblog_thread_rebuild_msg_buf(struct liblog_thread *a_thread, size_t buf_size_min, size_t buf_size_max);

#endif
