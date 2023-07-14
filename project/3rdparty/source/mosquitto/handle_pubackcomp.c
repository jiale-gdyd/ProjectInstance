#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <mosquitto/mosquitto.h>
#include <mosquitto/mqtt_protocol.h>

#include "config.h"
#include "net_mosq.h"
#include "callbacks.h"
#include "send_mosq.h"
#include "util_mosq.h"
#include "memory_mosq.h"
#include "packet_mosq.h"
#include "read_handle.h"
#include "logging_mosq.h"
#include "messages_mosq.h"

int handle__pubackcomp(struct mosquitto *mosq, const char *type)
{
    int rc;
    int qos;
    uint16_t mid;
    uint8_t reason_code = 0;
    mosquitto_property *properties = NULL;

    assert(mosq);

    if (mosquitto__get_state(mosq) != mosq_cs_active) {
        return MOSQ_ERR_PROTOCOL;
    }

    if (mosq->protocol != mosq_p_mqtt31) {
        if ((mosq->in_packet.command & 0x0F) != 0x00) {
            return MOSQ_ERR_MALFORMED_PACKET;
        }
    }

    pthread_mutex_lock(&mosq->msgs_out.mutex);
    util__increment_send_quota(mosq);
    pthread_mutex_unlock(&mosq->msgs_out.mutex);

    rc = packet__read_uint16(&mosq->in_packet, &mid);
    if (rc) {
        return rc;
    }

    if (type[3] == 'A') {   // pubAck或pubComp
        if (mosq->in_packet.command != CMD_PUBACK) {
            return MOSQ_ERR_MALFORMED_PACKET;
        }

        qos = 1;
    } else {
        if (mosq->in_packet.command != CMD_PUBCOMP) {
            return MOSQ_ERR_MALFORMED_PACKET;
        }

        qos = 2;
    }

    if (mid == 0) {
        return MOSQ_ERR_PROTOCOL;
    }

    if ((mosq->protocol == mosq_p_mqtt5) && (mosq->in_packet.remaining_length > 2)) {
        rc = packet__read_byte(&mosq->in_packet, &reason_code);
        if (rc) {
            return rc;
        }

        if (mosq->in_packet.remaining_length > 3) {
            rc = property__read_all(CMD_PUBACK, &mosq->in_packet, &properties);
            if (rc) {
                return rc;
            }
        }

        if (type[3] == 'A') {
            if ((reason_code != MQTT_RC_SUCCESS)
                && (reason_code != MQTT_RC_NO_MATCHING_SUBSCRIBERS)
                && (reason_code != MQTT_RC_UNSPECIFIED)
                && (reason_code != MQTT_RC_IMPLEMENTATION_SPECIFIC)
                && (reason_code != MQTT_RC_NOT_AUTHORIZED)
                && (reason_code != MQTT_RC_TOPIC_NAME_INVALID)
                && (reason_code != MQTT_RC_PACKET_ID_IN_USE)
                && (reason_code != MQTT_RC_QUOTA_EXCEEDED)
                && (reason_code != MQTT_RC_PAYLOAD_FORMAT_INVALID))
            {
                mosquitto_property_free_all(&properties);
                return MOSQ_ERR_PROTOCOL;
            }
        } else {
            if ((reason_code != MQTT_RC_SUCCESS) && (reason_code != MQTT_RC_PACKET_ID_NOT_FOUND)) {
                mosquitto_property_free_all(&properties);
                return MOSQ_ERR_PROTOCOL;
            }
        }
    }

    if (mosq->in_packet.pos < mosq->in_packet.remaining_length) {
        mosquitto_property_free_all(&properties);
        return MOSQ_ERR_MALFORMED_PACKET;
    }

    log__printf(mosq, MOSQ_LOG_DEBUG, "Client %s received %s (Mid: %d, RC:%d)", SAFE_PRINT(mosq->id), type, mid, reason_code);

    rc = message__delete(mosq, mid, mosq_md_out, qos);
    if (rc == MOSQ_ERR_SUCCESS) {
        // 仅通知客户端消息已发送一次
        callback__on_publish(mosq, mid, reason_code, properties);
        mosquitto_property_free_all(&properties);
    } else {
        mosquitto_property_free_all(&properties);
        if (rc != MOSQ_ERR_NOT_FOUND) {
            return rc;
        }
    }

    pthread_mutex_lock(&mosq->msgs_out.mutex);
    message__release_to_inflight(mosq, mosq_md_out);
    pthread_mutex_unlock(&mosq->msgs_out.mutex);

    return MOSQ_ERR_SUCCESS;
}
