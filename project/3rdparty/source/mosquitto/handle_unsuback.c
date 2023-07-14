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
#include "property_mosq.h"

int handle__unsuback(struct mosquitto *mosq)
{
    int rc;
    uint16_t mid;
    int *reason_codes = NULL;
    int reason_code_count = 0;
    mosquitto_property *properties = NULL;

    assert(mosq);

    if (mosquitto__get_state(mosq) != mosq_cs_active) {
        return MOSQ_ERR_PROTOCOL;
    }

    if (mosq->in_packet.command != CMD_UNSUBACK) {
        return MOSQ_ERR_MALFORMED_PACKET;
    }

    log__printf(mosq, MOSQ_LOG_DEBUG, "Client %s received UNSUBACK", SAFE_PRINT(mosq->id));

    rc = packet__read_uint16(&mosq->in_packet, &mid);
    if (rc) {
        return rc;
    }

    if (mid == 0) {
        return MOSQ_ERR_PROTOCOL;
    }

    if (mosq->protocol == mosq_p_mqtt5) {
        rc = property__read_all(CMD_UNSUBACK, &mosq->in_packet, &properties);
        if (rc) {
            return rc;
        }

        uint8_t byte;
        reason_code_count = (int)(mosq->in_packet.remaining_length - mosq->in_packet.pos);
        reason_codes = mosquitto__malloc((size_t)reason_code_count*sizeof(int));
        if (!reason_codes) {
            mosquitto_property_free_all(&properties);
            return MOSQ_ERR_NOMEM;
        }

        for (int i = 0; i < reason_code_count; i++) {
            rc = packet__read_byte(&mosq->in_packet, &byte);
            if (rc) {
                mosquitto__FREE(reason_codes);
                mosquitto_property_free_all(&properties);
                return rc;
            }

            reason_codes[i] = (int)byte;
        }
    }

    callback__on_unsubscribe(mosq, mid, reason_code_count, reason_codes, properties);

    mosquitto_property_free_all(&properties);
    mosquitto__FREE(reason_codes);

    return MOSQ_ERR_SUCCESS;
}
