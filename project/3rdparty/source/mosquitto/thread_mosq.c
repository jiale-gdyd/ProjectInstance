#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <time.h>
#include <pthread.h>

#include "config.h"
#include "net_mosq.h"
#include "util_mosq.h"
#include "mosquitto_internal.h"

void *mosquitto__thread_main(void *obj);

int mosquitto_loop_start(struct mosquitto *mosq)
{
    if (!mosq || (mosq->threaded != mosq_ts_none)) {
        return MOSQ_ERR_INVAL;
    }

    mosq->threaded = mosq_ts_self;
    if (!pthread_create(&mosq->thread_id, NULL, mosquitto__thread_main, mosq)) {
        pthread_setname_np(mosq->thread_id, "mosquitto loop");
        return MOSQ_ERR_SUCCESS;
    }

    return MOSQ_ERR_ERRNO;
}

int mosquitto_loop_stop(struct mosquitto *mosq, bool force)
{
    char sockpair_data = 0;

    if (!mosq || (mosq->threaded != mosq_ts_self)) {
        return MOSQ_ERR_INVAL;
    }

    // 如果在线程模式下，将一个字节写入sockpairW(连接到sockpairR)以脱离select()
    if (mosq->sockpairW != INVALID_SOCKET) {
        if (write(mosq->sockpairW, &sockpair_data, 1)) {

        }
    }

    if (force) {
        pthread_cancel(mosq->thread_id);
    }

    pthread_join(mosq->thread_id, NULL);
    mosq->thread_id = pthread_self();
    mosq->threaded = mosq_ts_none;

    return MOSQ_ERR_SUCCESS;
}

void *mosquitto__thread_main(void *obj)
{
    struct timespec ts;
    struct mosquitto *mosq = (struct mosquitto *)obj;

    ts.tv_sec = 0;
    ts.tv_nsec = 10000000;

    if (!mosq) {
        return NULL;
    }

    do {
        if (mosquitto__get_state(mosq) == mosq_cs_new) {
            nanosleep(&ts, NULL);
        } else {
            break;
        }
    } while (1);

    if (!mosq->keepalive) {
        // 如果Keepalive禁用，请休眠一天
        mosquitto_loop_forever(mosq, 1000 * 60 * 60 * 24, 1);
    } else {
        // 为我们的keepalive价值而休眠。publish()等将唤醒我们
        mosquitto_loop_forever(mosq, mosq->keepalive * 1000, 1);
    }

    if (mosq->threaded == mosq_ts_self) {
        mosq->threaded = mosq_ts_none;
    }

    return obj;
}

int mosquitto_threaded_set(struct mosquitto *mosq, bool threaded)
{
    if (!mosq) {
        return MOSQ_ERR_INVAL;
    }

    if (threaded) {
        mosq->threaded = mosq_ts_external;
    } else {
        mosq->threaded = mosq_ts_none;
    }

    return MOSQ_ERR_SUCCESS;
}
