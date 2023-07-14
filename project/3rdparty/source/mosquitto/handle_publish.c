#include <assert.h>
#include <string.h>
#include <mosquitto/mosquitto.h>
#include <mosquitto/mqtt_protocol.h>

#include "config.h"
#include "callbacks.h"
#include "send_mosq.h"
#include "time_mosq.h"
#include "util_mosq.h"
#include "alias_mosq.h"
#include "memory_mosq.h"
#include "packet_mosq.h"
#include "read_handle.h"
#include "logging_mosq.h"
#include "messages_mosq.h"
#include "property_mosq.h"
#include "mosquitto_internal.h"

int handle__publish(struct mosquitto *mosq)
{
    int rc = 0;
    uint16_t slen;
    uint8_t header;
    uint16_t mid = 0;
    uint16_t topic_alias = 0;
    struct mosquitto_message_all *message;
    mosquitto_property *properties = NULL;

    assert(mosq);

    if (mosquitto__get_state(mosq) != mosq_cs_active) {
        return MOSQ_ERR_PROTOCOL;
    }

    message = (struct mosquitto_message_all *)mosquitto__calloc(1, sizeof(struct mosquitto_message_all));
    if (!message) {
        return MOSQ_ERR_NOMEM;
    }
    header = mosq->in_packet.command;

    message->dup = (header & 0x08) >> 3;
    message->msg.qos = (header & 0x06) >> 1;
    message->msg.retain = (header & 0x01);

    rc = packet__read_string(&mosq->in_packet, &message->msg.topic, &slen);
    if (rc) {
        message__cleanup(&message);
        return rc;
    }

    if ((mosq->protocol != mosq_p_mqtt5) && (slen == 0)) {
        message__cleanup(&message);
        return MOSQ_ERR_PROTOCOL;
    }

    if (message->msg.qos > 0) {
        if (mosq->protocol == mosq_p_mqtt5) {
            if (mosq->msgs_in.inflight_quota == 0) {
                message__cleanup(&message);
                // FIXME - 应该在此处发送DISCONNECT
                return MOSQ_ERR_PROTOCOL;
            }
        }

        rc = packet__read_uint16(&mosq->in_packet, &mid);
        if (rc) {
            message__cleanup(&message);
            return rc;
        }

        if (mid == 0) {
            message__cleanup(&message);
            return MOSQ_ERR_PROTOCOL;
        }

        message->msg.mid = (int)mid;
    }

    if (mosq->protocol == mosq_p_mqtt5) {
        rc = property__read_all(CMD_PUBLISH, &mosq->in_packet, &properties);
        if (rc) {
            message__cleanup(&message);
            return rc;
        }

        mosquitto_property_read_int16(properties, MQTT_PROP_TOPIC_ALIAS, &topic_alias, false);
        if (topic_alias != 0) {
            if (message->msg.topic) {
                // 设置新的主题别名
                if (alias__add_r2l(mosq, message->msg.topic, topic_alias)) {
                    message__cleanup(&message);
                    mosquitto_property_free_all(&properties);
                    return MOSQ_ERR_NOMEM;
                }
            } else {
                // 检索现有的主题别名
                mosquitto__FREE(message->msg.topic);
                if (alias__find_by_alias(mosq, ALIAS_DIR_R2L, topic_alias, &message->msg.topic)) {
                    message__cleanup(&message);
                    mosquitto_property_free_all(&properties);
                    return MOSQ_ERR_PROTOCOL;
                }
            }
        }
    }

    // 如果现在还没有主题，那就是协议错误
    if ((topic_alias == 0) && (message->msg.topic == NULL)) {
        message__cleanup(&message);
        mosquitto_property_free_all(&properties);

        return MOSQ_ERR_PROTOCOL;
    }

    message->msg.payloadlen = (int)(mosq->in_packet.remaining_length - mosq->in_packet.pos);
    if (message->msg.payloadlen) {
        message->msg.payload = mosquitto__calloc((size_t)message->msg.payloadlen+1, sizeof(uint8_t));
        if (!message->msg.payload) {
            message__cleanup(&message);
            mosquitto_property_free_all(&properties);
            return MOSQ_ERR_NOMEM;
        }

        rc = packet__read_bytes(&mosq->in_packet, message->msg.payload, (uint32_t)message->msg.payloadlen);
        if (rc) {
            message__cleanup(&message);
            mosquitto_property_free_all(&properties);
            return rc;
        }
    }

    log__printf(mosq, MOSQ_LOG_DEBUG, "Client %s received PUBLISH (d%d, q%d, r%d, m%d, '%s', ... (%ld bytes))", SAFE_PRINT(mosq->id), message->dup, message->msg.qos, message->msg.retain, message->msg.mid, message->msg.topic, (long)message->msg.payloadlen);

    switch (message->msg.qos) {
        case 0:
            callback__on_message(mosq, &message->msg, properties);
            message__cleanup(&message);
            mosquitto_property_free_all(&properties);
            return MOSQ_ERR_SUCCESS;

        case 1:
            util__decrement_receive_quota(mosq);
            rc = send__puback(mosq, mid, 0, NULL);
            callback__on_message(mosq, &message->msg, properties);
            message__cleanup(&message);
            mosquitto_property_free_all(&properties);
            return rc;

        case 2:
            message->properties = properties;
            util__decrement_receive_quota(mosq);
            rc = send__pubrec(mosq, mid, 0, NULL);
            pthread_mutex_lock(&mosq->msgs_in.mutex);
            message->state = mosq_ms_wait_for_pubrel;
            message__queue(mosq, message, mosq_md_in);
            pthread_mutex_unlock(&mosq->msgs_in.mutex);
            return rc;
            
        default:
            message__cleanup(&message);
            mosquitto_property_free_all(&properties);
            return MOSQ_ERR_PROTOCOL;
    }
}
