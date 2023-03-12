#include <errno.h>
#include <pthread.h>

#include "buf.h"
#include "mdc.h"
#include "event.h"
#include "thread.h"
#include "logc_defs.h"

void liblog_thread_profile(struct liblog_thread *a_thread, int flag)
{
    logc_assert(a_thread, );

    logc_profile(flag, "--thread[%p][%p][%p][%p,%p,%p,%p,%p]--",
        a_thread, a_thread->mdc, a_thread->event, a_thread->pre_path_buf, a_thread->path_buf,
        a_thread->archive_path_buf, a_thread->pre_msg_buf, a_thread->msg_buf);

    liblog_mdc_profile(a_thread->mdc, flag);
    liblog_event_profile(a_thread->event, flag);
    liblog_buf_profile(a_thread->pre_path_buf, flag);
    liblog_buf_profile(a_thread->path_buf, flag);
    liblog_buf_profile(a_thread->archive_path_buf, flag);
    liblog_buf_profile(a_thread->pre_msg_buf, flag);
    liblog_buf_profile(a_thread->msg_buf, flag);
}

void liblog_thread_del(struct liblog_thread *a_thread)
{
    logc_assert(a_thread, );

    if (a_thread->mdc) {
        liblog_mdc_del(a_thread->mdc);
    }

    if (a_thread->event) {
        liblog_event_del(a_thread->event);
    }

    if (a_thread->pre_path_buf) {
        liblog_buf_del(a_thread->pre_path_buf);
    }

    if (a_thread->path_buf) {
        liblog_buf_del(a_thread->path_buf);
    }

    if (a_thread->archive_path_buf) {
        liblog_buf_del(a_thread->archive_path_buf);
    }

    if (a_thread->pre_msg_buf) {
        liblog_buf_del(a_thread->pre_msg_buf);
    }

    if (a_thread->msg_buf) {
        liblog_buf_del(a_thread->msg_buf);
    }

    logc_debug("liblof_thread_del[%p]", a_thread);
    free(a_thread);
}

struct liblog_thread *liblog_thread_new(int init_version, size_t buf_size_min, size_t buf_size_max, int time_cache_count)
{
    struct liblog_thread *a_thread;

    a_thread = (struct liblog_thread *)calloc(1, sizeof(struct liblog_thread));
    if (!a_thread) {
        logc_error("calloc fail, errno[%d]", errno);
        return NULL;
    }

    a_thread->init_version = init_version;

    a_thread->mdc = liblog_mdc_new();
    if (!a_thread->mdc) {
        logc_error("liblog_mdc_new fail");
        goto err;
    }

    a_thread->event = liblog_event_new(time_cache_count);
    if (!a_thread->event) {
        logc_error("liblog_event_new fail");
        goto err;
    }

    a_thread->pre_path_buf = liblog_buf_new(LOG_MAXLEN_PATH + 1, LOG_MAXLEN_PATH + 1, NULL);
    if (!a_thread->pre_path_buf) {
        logc_error("liblog_buf_new fail");
        goto err;
    }

    a_thread->path_buf = liblog_buf_new(LOG_MAXLEN_PATH + 1, LOG_MAXLEN_PATH + 1, NULL);
    if (!a_thread->path_buf) {
        logc_error("liblog_buf_new fail");
        goto err;
    }

    a_thread->archive_path_buf = liblog_buf_new(LOG_MAXLEN_PATH + 1, LOG_MAXLEN_PATH + 1, NULL);
    if (!a_thread->archive_path_buf) {
        logc_error("liblog_buf_new fail");
        goto err;
    }

    a_thread->pre_msg_buf = liblog_buf_new(buf_size_min, buf_size_max, "..." LOG_FILE_NEWLINE);
    if (!a_thread->pre_msg_buf) {
        logc_error("liblog_buf_new fail");
        goto err;
    }

    a_thread->msg_buf = liblog_buf_new(buf_size_min, buf_size_max, "..." LOG_FILE_NEWLINE);
    if (!a_thread->msg_buf) {
        logc_error("liblog_buf_new fail");
        goto err;
    }

    return a_thread;

err:
    liblog_thread_del(a_thread);
    return NULL;
}

int liblog_thread_rebuild_msg_buf(struct liblog_thread *a_thread, size_t buf_size_min, size_t buf_size_max)
{
    struct liblog_buf *msg_buf_new = NULL;
    struct liblog_buf *pre_msg_buf_new = NULL;

    logc_assert(a_thread, -1);

    if ((a_thread->msg_buf->size_min == buf_size_min) && (a_thread->msg_buf->size_max == buf_size_max)) {
        logc_debug("buf size not changed, no need rebuild");
        return 0;
    }

    pre_msg_buf_new = liblog_buf_new(buf_size_min, buf_size_max, "..." LOG_FILE_NEWLINE);
    if (!pre_msg_buf_new) {
        logc_error("liblog_buf_new fail");
        goto err;
    }

    msg_buf_new = liblog_buf_new(buf_size_min, buf_size_max, "..." LOG_FILE_NEWLINE);
    if (!msg_buf_new) {
        logc_error("liblog_buf_new fail");
        goto err;
    }

    liblog_buf_del(a_thread->pre_msg_buf);
    a_thread->pre_msg_buf = pre_msg_buf_new;

    liblog_buf_del(a_thread->msg_buf);
    a_thread->msg_buf = msg_buf_new;

    return 0;

err:
    if (pre_msg_buf_new) {
        liblog_buf_del(pre_msg_buf_new);
    }

    if (msg_buf_new) {
        liblog_buf_del(msg_buf_new);
    }

    return -1;
}

int liblog_thread_rebuild_event(struct liblog_thread *a_thread, int time_cache_count)
{
    struct liblog_event *event_new = NULL;

    logc_assert(a_thread, -1);

    event_new = liblog_event_new(time_cache_count);
    if (!event_new) {
        logc_error("liblog_event_new fail");
        goto err;
    }

    liblog_event_del(a_thread->event);
    a_thread->event = event_new;

    return 0;

err:
    if (event_new) {
        liblog_event_del(event_new);
    }

    return -1;
}
