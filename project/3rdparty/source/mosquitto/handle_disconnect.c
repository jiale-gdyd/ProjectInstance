#include <stdio.h>
#include <string.h>
#include <mosquitto/mqtt_protocol.h>

#include "config.h"
#include "net_mosq.h"
#include "send_mosq.h"
#include "util_mosq.h"
#include "memory_mosq.h"
#include "packet_mosq.h"
#include "read_handle.h"
#include "logging_mosq.h"
#include "property_mosq.h"

int handle__disconnect(struct mosquitto *mosq)
{
    int rc;
    uint8_t reason_code;
    mosquitto_property *properties = NULL;

    if (!mosq) {
        return MOSQ_ERR_INVAL;
    }

    if (mosq->protocol != mosq_p_mqtt5) {
        return MOSQ_ERR_PROTOCOL;
    }

    if (mosq->in_packet.command != CMD_DISCONNECT) {
        return MOSQ_ERR_MALFORMED_PACKET;
    }

    rc = packet__read_byte(&mosq->in_packet, &reason_code);
    if (rc) {
        return rc;
    }

    if (mosq->in_packet.remaining_length > 2) {
        rc = property__read_all(CMD_DISCONNECT, &mosq->in_packet, &properties);
        if (rc) {
            return rc;
        }

        mosquitto_property_free_all(&properties);
    }

    log__printf(mosq, MOSQ_LOG_DEBUG, "Received DISCONNECT (%d)", reason_code);

    do_client_disconnect(mosq, reason_code, properties);
    mosquitto_property_free_all(&properties);

    return MOSQ_ERR_SUCCESS;
}
