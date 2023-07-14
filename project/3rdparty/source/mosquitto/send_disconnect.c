#include <assert.h>
#include <mosquitto/mosquitto.h>
#include <mosquitto/mqtt_protocol.h>

#include "config.h"
#include "send_mosq.h"
#include "memory_mosq.h"
#include "packet_mosq.h"
#include "logging_mosq.h"
#include "property_mosq.h"
#include "mosquitto_internal.h"

int send__disconnect(struct mosquitto *mosq, uint8_t reason_code, const mosquitto_property *properties)
{
    int rc;
    uint32_t remaining_length;
    struct mosquitto__packet *packet = NULL;

    assert(mosq);
    log__printf(mosq, MOSQ_LOG_DEBUG, "Client %s sending DISCONNECT", SAFE_PRINT(mosq->id));
    assert(mosq);

    if ((mosq->protocol == mosq_p_mqtt5) && ((reason_code != 0) || properties)) {
        remaining_length = 1;
        if (properties) {
            remaining_length += property__get_remaining_length(properties);
        }
    } else {
        remaining_length = 0;
    }

    rc = packet__alloc(&packet, CMD_DISCONNECT, remaining_length);
    if (rc) {
        mosquitto__FREE(packet);
        return rc;
    }

    if ((mosq->protocol == mosq_p_mqtt5) && ((reason_code != 0) || properties)) {
        packet__write_byte(packet, reason_code);
        if (properties) {
            property__write_all(packet, properties, true);
        }
    }

    return packet__queue(mosq, packet);
}
