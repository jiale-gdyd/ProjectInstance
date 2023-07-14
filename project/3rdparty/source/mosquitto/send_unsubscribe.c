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

int send__unsubscribe(struct mosquitto *mosq, int *mid, int topic_count, char *const *const topic, const mosquitto_property *properties)
{
    int i, rc;
    size_t tlen;
    uint32_t packetlen;
    uint16_t local_mid;
    struct mosquitto__packet *packet = NULL;

    assert(mosq);
    assert(topic);

    packetlen = 2;
    for (i = 0; i < topic_count; i++) {
        tlen = strlen(topic[i]);
        if (tlen > UINT16_MAX) {
            return MOSQ_ERR_INVAL;
        }
        packetlen += 2U + (uint16_t)tlen;
    }

    if (mosq->protocol == mosq_p_mqtt5) {
        packetlen += property__get_remaining_length(properties);
    }

    rc = packet__alloc(&packet, CMD_UNSUBSCRIBE | 2, packetlen);
    if (rc) {
        return rc;
    }

    // 可变标题
    local_mid = mosquitto__mid_generate(mosq);
    if (mid) {
        *mid = (int)local_mid;
    }
    packet__write_uint16(packet, local_mid);

    if (mosq->protocol == mosq_p_mqtt5) {
        // 尚未使用用户属性
        property__write_all(packet, properties, true);
    }

    // payload
    for (i = 0; i < topic_count; i++) {
        packet__write_string(packet, topic[i], (uint16_t)strlen(topic[i]));
    }

    for (i = 0; i < topic_count; i++) {
        log__printf(mosq, MOSQ_LOG_DEBUG, "Client %s sending UNSUBSCRIBE (Mid: %d, Topic: %s)", SAFE_PRINT(mosq->id), local_mid, topic[i]);
    }

    return packet__queue(mosq, packet);
}
