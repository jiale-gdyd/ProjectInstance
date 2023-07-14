#include <assert.h>
#include <stdio.h>
#include <string.h>
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

int handle__pubrec(struct mosquitto *mosq)
{
    int rc;
    uint16_t mid;
    uint8_t reason_code = 0;
    mosquitto_property *properties = NULL;

    assert(mosq);

    if (mosquitto__get_state(mosq) != mosq_cs_active) {
        return MOSQ_ERR_PROTOCOL;
    }

    if (mosq->in_packet.command != CMD_PUBREC) {
        return MOSQ_ERR_MALFORMED_PACKET;
    }

    rc = packet__read_uint16(&mosq->in_packet, &mid);
    if (rc) {
        return rc;
    }

    if (mid == 0) {
        return MOSQ_ERR_PROTOCOL;
    }

    if ((mosq->protocol == mosq_p_mqtt5) && (mosq->in_packet.remaining_length > 2)) {
        rc = packet__read_byte(&mosq->in_packet, &reason_code);
        if (rc) {
            return rc;
        }

        if ((reason_code != MQTT_RC_SUCCESS)
            && (reason_code != MQTT_RC_NO_MATCHING_SUBSCRIBERS)
            && (reason_code != MQTT_RC_UNSPECIFIED)
            && (reason_code != MQTT_RC_IMPLEMENTATION_SPECIFIC)
            && (reason_code != MQTT_RC_NOT_AUTHORIZED)
            && (reason_code != MQTT_RC_TOPIC_NAME_INVALID)
            && (reason_code != MQTT_RC_PACKET_ID_IN_USE)
            && (reason_code != MQTT_RC_QUOTA_EXCEEDED))
        {
            return MOSQ_ERR_PROTOCOL;
        }

        if (mosq->in_packet.remaining_length > 3) {
            rc = property__read_all(CMD_PUBREC, &mosq->in_packet, &properties);
            if (rc) {
                return rc;
            }

            // 立即释放，暂时不使用Reason String或User Property做任何事情
            mosquitto_property_free_all(&properties);
        }
    }

    if (mosq->in_packet.pos < mosq->in_packet.remaining_length) {
        return MOSQ_ERR_MALFORMED_PACKET;
    }

    log__printf(mosq, MOSQ_LOG_DEBUG, "Client %s received PUBREC (Mid: %d)", mosq->id, mid);

    if ((reason_code < 0x80) || (mosq->protocol != mosq_p_mqtt5)) {
        rc = message__out_update(mosq, mid, mosq_ms_wait_for_pubcomp, 2);
    } else {
        if (!message__delete(mosq, mid, mosq_md_out, 2)) {
            // 仅通知客户端消息已发送一次
            callback__on_publish(mosq, mid, reason_code, properties);
        }

        util__increment_send_quota(mosq);

        pthread_mutex_lock(&mosq->msgs_out.mutex);
        message__release_to_inflight(mosq, mosq_md_out);
        pthread_mutex_unlock(&mosq->msgs_out.mutex);

        return MOSQ_ERR_SUCCESS;
    }

    if (rc == MOSQ_ERR_NOT_FOUND) {
        log__printf(mosq, MOSQ_LOG_WARNING, "Warning: Received PUBREC from %s for an unknown packet identifier %d.", SAFE_PRINT(mosq->id), mid);
    } else if (rc != MOSQ_ERR_SUCCESS) {
        return rc;
    }

    rc = send__pubrel(mosq, mid, NULL);
    if (rc) {
        return rc;
    }

    return MOSQ_ERR_SUCCESS;
}
