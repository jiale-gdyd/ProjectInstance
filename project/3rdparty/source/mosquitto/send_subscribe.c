#include <assert.h>
#include <string.h>
#include <mosquitto/mosquitto.h>
#include <mosquitto/mqtt_protocol.h>

#include "config.h"
#include "send_mosq.h"
#include "util_mosq.h"
#include "memory_mosq.h"
#include "packet_mosq.h"
#include "logging_mosq.h"
#include "property_mosq.h"
#include "mosquitto_internal.h"

int send__subscribe(struct mosquitto *mosq, int *mid, int topic_count, char *const *const topic, int topic_qos, const mosquitto_property *properties)
{
    int i;
    int rc;
    size_t tlen;
    uint32_t packetlen;
    uint16_t local_mid;
    struct mosquitto__packet *packet = NULL;

    assert(mosq);
    assert(topic);

    packetlen = 2;
    if (mosq->protocol == mosq_p_mqtt5) {
        packetlen += property__get_remaining_length(properties);
    }

    for (i = 0; i < topic_count; i++) {
        tlen = strlen(topic[i]);
        if (tlen > UINT16_MAX) {
            return MOSQ_ERR_INVAL;
        }

        packetlen += 2U + (uint16_t)tlen + 1U;
    }

    rc = packet__alloc(&packet, CMD_SUBSCRIBE | 2, packetlen);
    if (rc) {
        mosquitto__FREE(packet);
        return rc;
    }

    // 可变标题
    local_mid = mosquitto__mid_generate(mosq);
    if (mid) {
        *mid = (int)local_mid;
    }
    packet__write_uint16(packet, local_mid);

    if (mosq->protocol == mosq_p_mqtt5) {
        property__write_all(packet, properties, true);
    }

    // payload
    for (i = 0; i < topic_count; i++) {
        packet__write_string(packet, topic[i], (uint16_t)strlen(topic[i]));
        packet__write_byte(packet, (uint8_t)topic_qos);
    }

    for (i = 0; i < topic_count; i++) {
        log__printf(mosq, MOSQ_LOG_DEBUG, "Client %s sending SUBSCRIBE (Mid: %d, Topic: %s, QoS: %d, Options: 0x%02x)", SAFE_PRINT(mosq->id), local_mid, topic[i], topic_qos & 0x03, topic_qos & 0xFC);
    }

    return packet__queue(mosq, packet);
}
