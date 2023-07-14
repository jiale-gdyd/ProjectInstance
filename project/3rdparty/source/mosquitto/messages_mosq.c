#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <mosquitto/mosquitto.h>

#include "config.h"
#include "utlist.h"
#include "send_mosq.h"
#include "time_mosq.h"
#include "util_mosq.h"
#include "memory_mosq.h"
#include "messages_mosq.h"
#include "mosquitto_internal.h"

void message__cleanup(struct mosquitto_message_all **message)
{
    struct mosquitto_message_all *msg;

    if (!message || !*message) {
        return;
    }

    msg = *message;

    mosquitto__FREE(msg->msg.topic);
    mosquitto__FREE(msg->msg.payload);
    mosquitto_property_free_all(&msg->properties);
    mosquitto__FREE(msg);
}

void message__cleanup_all(struct mosquitto *mosq)
{
    struct mosquitto_message_all *tail, *tmp;

    assert(mosq);

    DL_FOREACH_SAFE(mosq->msgs_in.inflight, tail, tmp) {
        DL_DELETE(mosq->msgs_in.inflight, tail);
        message__cleanup(&tail);
    }

    DL_FOREACH_SAFE(mosq->msgs_out.inflight, tail, tmp) {
        DL_DELETE(mosq->msgs_out.inflight, tail);
        message__cleanup(&tail);
    }
}

int mosquitto_message_copy(struct mosquitto_message *dst, const struct mosquitto_message *src)
{
    if (!dst || !src) {
        return MOSQ_ERR_INVAL;
    }

    dst->mid = src->mid;
    dst->topic = mosquitto__strdup(src->topic);
    if (!dst->topic) {
        return MOSQ_ERR_NOMEM;
    }

    dst->qos = src->qos;
    dst->retain = src->retain;
    if (src->payloadlen) {
        dst->payload = mosquitto__calloc((unsigned int)src->payloadlen+1, sizeof(uint8_t));
        if (!dst->payload) {
            mosquitto__FREE(dst->topic);
            return MOSQ_ERR_NOMEM;
        }

        memcpy(dst->payload, src->payload, (unsigned int)src->payloadlen);
        dst->payloadlen = src->payloadlen;
    } else {
        dst->payloadlen = 0;
        dst->payload = NULL;
    }

    return MOSQ_ERR_SUCCESS;
}

int message__delete(struct mosquitto *mosq, uint16_t mid, enum mosquitto_msg_direction dir, int qos)
{
    int rc;
    struct mosquitto_message_all *message;

    assert(mosq);

    rc = message__remove(mosq, mid, dir, &message, qos);
    if (rc == MOSQ_ERR_SUCCESS) {
        message__cleanup(&message);
    }

    return rc;
}

void mosquitto_message_free(struct mosquitto_message **message)
{
    struct mosquitto_message *msg;

    if (!message || !*message) {
        return;
    }

    msg = *message;

    mosquitto__FREE(msg->topic);
    mosquitto__FREE(msg->payload);
    mosquitto__FREE(msg);
}

void mosquitto_message_free_contents(struct mosquitto_message *message)
{
    if (!message) {
        return;
    }

    mosquitto__FREE(message->topic);
    mosquitto__FREE(message->payload);
}

int message__queue(struct mosquitto *mosq, struct mosquitto_message_all *message, enum mosquitto_msg_direction dir)
{
    // 进入此功能之前，请先锁定mosq->*_message_mutex
    assert(mosq);
    assert(message);
    assert(message->msg.qos != 0);

    if (dir == mosq_md_out) {
        DL_APPEND(mosq->msgs_out.inflight, message);
        mosq->msgs_out.queue_len++;
    } else {
        DL_APPEND(mosq->msgs_in.inflight, message);
        mosq->msgs_in.queue_len++;
    }

    return message__release_to_inflight(mosq, dir);
}

void message__reconnect_reset(struct mosquitto *mosq, bool update_quota_only)
{
    struct mosquitto_message_all *message, *tmp;

    assert(mosq);

    pthread_mutex_lock(&mosq->msgs_in.mutex);
    mosq->msgs_in.inflight_quota = mosq->msgs_in.inflight_maximum;
    mosq->msgs_in.queue_len = 0;
    DL_FOREACH_SAFE(mosq->msgs_in.inflight, message, tmp) {
        mosq->msgs_in.queue_len++;
        if (message->msg.qos != 2) {
            DL_DELETE(mosq->msgs_in.inflight, message);
            message__cleanup(&message);
        } else {
            // 消息状态可以在这里保留，因为它应该与客户端获得的任何内容匹配
            util__decrement_receive_quota(mosq);
        }
    }
    pthread_mutex_unlock(&mosq->msgs_in.mutex);

    pthread_mutex_lock(&mosq->msgs_out.mutex);
    mosq->msgs_out.inflight_quota = mosq->msgs_out.inflight_maximum;
    mosq->msgs_out.queue_len = 0;
    DL_FOREACH_SAFE(mosq->msgs_out.inflight, message, tmp) {
        mosq->msgs_out.queue_len++;

        if (mosq->msgs_out.inflight_quota != 0) {
            util__decrement_send_quota(mosq);
            if (update_quota_only == false) {
                if (message->msg.qos == 1) {
                    message->state = mosq_ms_publish_qos1;
                } else if (message->msg.qos == 2) {
                    if (message->state == mosq_ms_wait_for_pubrec) {
                        message->state = mosq_ms_publish_qos2;
                    } else if (message->state == mosq_ms_wait_for_pubcomp) {
                        message->state = mosq_ms_resend_pubrel;
                    }

                    // 应该能够保存状态
                }
            }
        } else {
            message->state = mosq_ms_invalid;
        }
    }
    pthread_mutex_unlock(&mosq->msgs_out.mutex);
}

int message__release_to_inflight(struct mosquitto *mosq, enum mosquitto_msg_direction dir)
{
    // 进入此功能之前，请先锁定mosq->*_message_mutex
    int rc = MOSQ_ERR_SUCCESS;
    struct mosquitto_message_all *cur, *tmp;

    if (dir == mosq_md_out) {
        DL_FOREACH_SAFE(mosq->msgs_out.inflight, cur, tmp) {
            if (mosq->msgs_out.inflight_quota > 0) {
                if ((cur->msg.qos > 0) && (cur->state == mosq_ms_invalid)) {
                    if (cur->msg.qos == 1) {
                        cur->state = mosq_ms_wait_for_puback;
                    } else if (cur->msg.qos == 2) {
                        cur->state = mosq_ms_wait_for_pubrec;
                    }

                    rc = send__publish(mosq, (uint16_t)cur->msg.mid, cur->msg.topic, (uint32_t)cur->msg.payloadlen, cur->msg.payload, (uint8_t)cur->msg.qos, cur->msg.retain, cur->dup, 0, cur->properties, 0);
                    if (rc) {
                        return rc;
                    }

                    util__decrement_send_quota(mosq);
                }
            } else {
                return MOSQ_ERR_SUCCESS;
            }
        }
    }

    return rc;
}

int message__remove(struct mosquitto *mosq, uint16_t mid, enum mosquitto_msg_direction dir, struct mosquitto_message_all **message, int qos)
{
    bool found = false;
    struct mosquitto_message_all *cur, *tmp;

    assert(mosq);
    assert(message);

    if (dir == mosq_md_out) {
        pthread_mutex_lock(&mosq->msgs_out.mutex);

        DL_FOREACH_SAFE(mosq->msgs_out.inflight, cur, tmp) {
            if ((found == false) && (cur->msg.mid == mid)) {
                if (cur->msg.qos != qos) {
                    pthread_mutex_unlock(&mosq->msgs_out.mutex);
                    return MOSQ_ERR_PROTOCOL;
                }
                DL_DELETE(mosq->msgs_out.inflight, cur);

                *message = cur;
                mosq->msgs_out.queue_len--;
                found = true;
                break;
            }
        }
        pthread_mutex_unlock(&mosq->msgs_out.mutex);

        if (found) {
            return MOSQ_ERR_SUCCESS;
        } else {
            return MOSQ_ERR_NOT_FOUND;
        }
    } else {
        pthread_mutex_lock(&mosq->msgs_in.mutex);
        DL_FOREACH_SAFE(mosq->msgs_in.inflight, cur, tmp) {
            if (cur->msg.mid == mid) {
                if (cur->msg.qos != qos) {
                    pthread_mutex_unlock(&mosq->msgs_in.mutex);
                    return MOSQ_ERR_PROTOCOL;
                }

                DL_DELETE(mosq->msgs_in.inflight, cur);
                *message = cur;
                mosq->msgs_in.queue_len--;
                found = true;
                break;
            }
        }

        pthread_mutex_unlock(&mosq->msgs_in.mutex);
        if (found) {
            return MOSQ_ERR_SUCCESS;
        } else {
            return MOSQ_ERR_NOT_FOUND;
        }
    }
}

void message__retry_check(struct mosquitto *mosq)
{
    struct mosquitto_message_all *msg;

    assert(mosq);

    pthread_mutex_lock(&mosq->msgs_out.mutex);
    DL_FOREACH(mosq->msgs_out.inflight, msg) {
        switch (msg->state) {
            case mosq_ms_publish_qos1:
            case mosq_ms_publish_qos2:
                msg->dup = true;
                send__publish(mosq, (uint16_t)msg->msg.mid, msg->msg.topic, (uint32_t)msg->msg.payloadlen, msg->msg.payload, (uint8_t)msg->msg.qos, msg->msg.retain, msg->dup, 0, msg->properties, 0);
                break;

            case mosq_ms_wait_for_pubrel:
                msg->dup = true;
                send__pubrec(mosq, (uint16_t)msg->msg.mid, 0, NULL);
                break;

            case mosq_ms_resend_pubrel:
            case mosq_ms_wait_for_pubcomp:
                msg->dup = true;
                send__pubrel(mosq, (uint16_t)msg->msg.mid, NULL);
                break;

            default:
                break;
        }
    }
    pthread_mutex_unlock(&mosq->msgs_out.mutex);
}

void mosquitto_message_retry_set(struct mosquitto *mosq, unsigned int message_retry)
{
    MOSQ_UNUSED(mosq);
    MOSQ_UNUSED(message_retry);
}

int message__out_update(struct mosquitto *mosq, uint16_t mid, enum mosquitto_msg_state state, int qos)
{
    struct mosquitto_message_all *message, *tmp;

    assert(mosq);

    pthread_mutex_lock(&mosq->msgs_out.mutex);
    DL_FOREACH_SAFE(mosq->msgs_out.inflight, message, tmp) {
        if (message->msg.mid == mid) {
            if (message->msg.qos != qos) {
                pthread_mutex_unlock(&mosq->msgs_out.mutex);
                return MOSQ_ERR_PROTOCOL;
            }

            message->state = state;
            pthread_mutex_unlock(&mosq->msgs_out.mutex);
            return MOSQ_ERR_SUCCESS;
        }
    }
    pthread_mutex_unlock(&mosq->msgs_out.mutex);

    return MOSQ_ERR_NOT_FOUND;
}

int mosquitto_max_inflight_messages_set(struct mosquitto *mosq, unsigned int max_inflight_messages)
{
    return mosquitto_int_option(mosq, MOSQ_OPT_SEND_MAXIMUM, (int)max_inflight_messages);
}
