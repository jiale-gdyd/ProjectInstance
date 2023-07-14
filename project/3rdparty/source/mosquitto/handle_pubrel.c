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

int handle__pubrel(struct mosquitto *mosq)
{
    int rc;
    uint16_t mid;
    uint8_t reason_code;
    mosquitto_property *properties = NULL;
    struct mosquitto_message_all *message = NULL;

    assert(mosq);

    if (mosquitto__get_state(mosq) != mosq_cs_active) {
        return MOSQ_ERR_PROTOCOL;
    }

    if ((mosq->protocol != mosq_p_mqtt31) && (mosq->in_packet.command != (CMD_PUBREL | 2))) {
        return MOSQ_ERR_MALFORMED_PACKET;
    }

    if (mosq->protocol != mosq_p_mqtt31) {
        if ((mosq->in_packet.command & 0x0F) != 0x02) {
            return MOSQ_ERR_PROTOCOL;
        }
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

        if ((reason_code != MQTT_RC_SUCCESS) && (reason_code != MQTT_RC_PACKET_ID_NOT_FOUND)) {
            return MOSQ_ERR_PROTOCOL;
        }

        if (mosq->in_packet.remaining_length > 3) {
            rc = property__read_all(CMD_PUBREL, &mosq->in_packet, &properties);
            if (rc) {
                return rc;
            }

            mosquitto_property_free_all(&properties);
        }
    }

    if (mosq->in_packet.pos < mosq->in_packet.remaining_length) {
        return MOSQ_ERR_MALFORMED_PACKET;
    }

    log__printf(mosq, MOSQ_LOG_DEBUG, "Client %s received PUBREL (Mid: %d)", SAFE_PRINT(mosq->id), mid);

    rc = send__pubcomp(mosq, mid, NULL);
    if (rc) {
        message__remove(mosq, mid, mosq_md_in, &message, 2);
        return rc;
    }

    rc = message__remove(mosq, mid, mosq_md_in, &message, 2);
    if (rc == MOSQ_ERR_SUCCESS) {
        // 仅当我们从队列中删除该消息时才传递该消息 - 这样可以防止同一消息的多个回调
        callback__on_message(mosq, &message->msg, message->properties);
        message__cleanup(&message);
    } else if (rc == MOSQ_ERR_NOT_FOUND) {
        return MOSQ_ERR_SUCCESS;
    } else {
        return rc;
    }

    return MOSQ_ERR_SUCCESS;
}
