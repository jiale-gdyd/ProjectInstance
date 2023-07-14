#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <mosquitto/mosquitto.h>

#include "config.h"
#include "memory_mosq.h"
#include "logging_mosq.h"
#include "mosquitto_internal.h"

int log__printf(struct mosquitto *mosq, unsigned int priority, const char *fmt, ...)
{
    char *s;
    va_list va;
    size_t len;

    assert(mosq);
    assert(fmt);

    pthread_mutex_lock(&mosq->log_callback_mutex);
    if (mosq->on_log) {
        len = strlen(fmt) + 500;
        s = (char *)mosquitto__malloc(len * sizeof(char));
        if (!s) {
            pthread_mutex_unlock(&mosq->log_callback_mutex);
            return MOSQ_ERR_NOMEM;
        }

        va_start(va, fmt);
        vsnprintf(s, len, fmt, va);
        va_end(va);
        s[len - 1] = '\0';  // 确保字符串以NULL终止

        mosq->on_log(mosq, mosq->userdata, (int)priority, s);

        mosquitto__FREE(s);
    }
    pthread_mutex_unlock(&mosq->log_callback_mutex);

    return MOSQ_ERR_SUCCESS;
}
