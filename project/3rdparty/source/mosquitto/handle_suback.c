#include <assert.h>
#include <mosquitto/mosquitto.h>
#include <mosquitto/mqtt_protocol.h>

#include "config.h"
#include "callbacks.h"
#include "util_mosq.h"
#include "packet_mosq.h"
#include "memory_mosq.h"
#include "read_handle.h"
#include "logging_mosq.h"
#include "property_mosq.h"
#include "mosquitto_internal.h"

int handle__suback(struct mosquitto *mosq)
{
    int rc;
    int i = 0;
    uint8_t qos;
    uint16_t mid;
    int qos_count;
    int *granted_qos;
    mosquitto_property *properties = NULL;

    assert(mosq);

    if (mosquitto__get_state(mosq) != mosq_cs_active) {
        return MOSQ_ERR_PROTOCOL;
    }

    if (mosq->in_packet.command != CMD_SUBACK) {
        return MOSQ_ERR_MALFORMED_PACKET;
    }

    log__printf(mosq, MOSQ_LOG_DEBUG, "Client %s received SUBACK", SAFE_PRINT(mosq->id));

    rc = packet__read_uint16(&mosq->in_packet, &mid);
    if (rc) {
        return rc;
    }

    if (mid == 0) {
        return MOSQ_ERR_PROTOCOL;
    }

    if (mosq->protocol == mosq_p_mqtt5) {
        rc = property__read_all(CMD_SUBACK, &mosq->in_packet, &properties);
        if (rc) {
            return rc;
        }
    }

    qos_count = (int)(mosq->in_packet.remaining_length - mosq->in_packet.pos);
    granted_qos = (int *)mosquitto__malloc((size_t)qos_count * sizeof(int));
    if (!granted_qos) {
        mosquitto_property_free_all(&properties);
        return MOSQ_ERR_NOMEM;
    }

    while (mosq->in_packet.pos < mosq->in_packet.remaining_length) {
        rc = packet__read_byte(&mosq->in_packet, &qos);
        if (rc) {
            mosquitto__FREE(granted_qos);
            mosquitto_property_free_all(&properties);
            return rc;
        }

        granted_qos[i] = (int)qos;
        i++;
    }

    callback__on_subscribe(mosq, mid, qos_count, granted_qos, properties);
    mosquitto_property_free_all(&properties);
    mosquitto__FREE(granted_qos);

    return MOSQ_ERR_SUCCESS;
}
