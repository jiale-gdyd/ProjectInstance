#include <assert.h>
#include <string.h>
#include <mosquitto/mosquitto.h>
#include <mosquitto/mqtt_protocol.h>

#include "config.h"
#include "utlist.h"
#include "net_mosq.h"
#include "send_mosq.h"
#include "alias_mosq.h"
#include "memory_mosq.h"
#include "packet_mosq.h"
#include "logging_mosq.h"
#include "property_mosq.h"
#include "mosquitto_internal.h"

#define metrics__int_inc(stat, val)

int send__publish(struct mosquitto *mosq, uint16_t mid, const char *topic, uint32_t payloadlen, const void *payload, uint8_t qos, bool retain, bool dup, uint32_t subscription_identifier, const mosquitto_property *store_props, uint32_t expiry_interval)
{
    assert(mosq);

    if (!net__is_connected(mosq)) {
        return MOSQ_ERR_NO_CONN;
    }

    if (!mosq->retain_available) {
        retain = false;
    }

    log__printf(mosq, MOSQ_LOG_DEBUG, "Client %s sending PUBLISH (d%d, q%d, r%d, m%d, '%s', ... (%ld bytes))", SAFE_PRINT(mosq->id), dup, qos, retain, mid, topic, (long)payloadlen);

    return send__real_publish(mosq, mid, topic, payloadlen, payload, qos, retain, dup, subscription_identifier, store_props, expiry_interval);
}

int send__real_publish(struct mosquitto *mosq, uint16_t mid, const char *topic, uint32_t payloadlen, const void *payload, uint8_t qos, bool retain, bool dup, uint32_t subscription_identifier, const mosquitto_property *store_props, uint32_t expiry_interval)
{
    int rc;
    unsigned int packetlen;
    mosquitto_property expiry_prop;
    unsigned int proplen = 0, varbytes;
    struct mosquitto__packet *packet = NULL;

    assert(mosq);

    if (topic) {
        packetlen = 2 + (unsigned int)strlen(topic) + payloadlen;
    } else {
        packetlen = 2 + payloadlen;
    }

    if (qos > 0) {
        // 消息ID
        packetlen += 2;
    }

    if (mosq->protocol == mosq_p_mqtt5) {
        proplen = 0;
        proplen += property__get_length_all(store_props);

        if (expiry_interval > 0) {
            expiry_prop.next = NULL;
            expiry_prop.value.i32 = expiry_interval;
            expiry_prop.identifier = MQTT_PROP_MESSAGE_EXPIRY_INTERVAL;
            expiry_prop.property_type = MQTT_PROP_TYPE_INT32;
            expiry_prop.client_generated = false;

            proplen += property__get_length_all(&expiry_prop);
        }

        varbytes = packet__varint_bytes(proplen);
        if (varbytes > 4) {
            // FIXME - 属性太大，不要发布任何内容 - 应该先删除一些
            store_props = NULL;
            expiry_interval = 0;
        } else {
            packetlen += proplen + varbytes;
        }
    }

    if (packet__check_oversize(mosq, packetlen)) {
        log__printf(mosq, MOSQ_LOG_NOTICE, "Dropping too large outgoing PUBLISH (%d bytes)", packetlen);
        return MOSQ_ERR_OVERSIZE_PACKET;
    }

    rc = packet__alloc(&packet, (uint8_t)(CMD_PUBLISH | (uint8_t)((dup & 0x1) << 3) | (uint8_t)(qos << 1) | retain), packetlen);
    if (rc) {
        return rc;
    }

    packet->mid = mid;

    // 可变标题(主题字符串)
    if (topic) {
        packet__write_string(packet, topic, (uint16_t)strlen(topic));
    } else {
        packet__write_uint16(packet, 0);
    }

    if (qos > 0) {
        packet__write_uint16(packet, mid);
    }

    if (mosq->protocol == mosq_p_mqtt5) {
        packet__write_varint(packet, proplen);
        property__write_all(packet, store_props, false);
        if (expiry_interval > 0) {
            property__write_all(packet, &expiry_prop, false);
        }
    }

    // payload
    if (payloadlen && payload) {
        packet__write_bytes(packet, payload, payloadlen);
    }

    return packet__queue(mosq, packet);
}
