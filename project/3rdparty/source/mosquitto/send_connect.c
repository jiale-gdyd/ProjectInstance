#include <assert.h>
#include <string.h>
#include <mosquitto/mosquitto.h>
#include <mosquitto/mqtt_protocol.h>

#include "config.h"
#include "send_mosq.h"
#include "packet_mosq.h"
#include "memory_mosq.h"
#include "logging_mosq.h"
#include "property_mosq.h"
#include "mosquitto_internal.h"

int send__connect(struct mosquitto *mosq, uint16_t keepalive, bool clean_session, const mosquitto_property *properties)
{
    int rc;
    uint8_t byte;
    uint8_t version;
    uint8_t will = 0;
    uint32_t headerlen;
    uint32_t payloadlen;
    uint16_t receive_maximum;
    uint32_t proplen = 0, varbytes;
    char *clientid, *username, *password;
    mosquitto_property *local_props = NULL;
    struct mosquitto__packet *packet = NULL;

    assert(mosq);

    if ((mosq->protocol == mosq_p_mqtt31) && !mosq->id) {
        return MOSQ_ERR_PROTOCOL;
    }

    clientid = mosq->id;
    username = mosq->username;
    password = mosq->password;

    if (mosq->protocol == mosq_p_mqtt5) {
        // 从选项生成属性
        if (!mosquitto_property_read_int16(properties, MQTT_PROP_RECEIVE_MAXIMUM, &receive_maximum, false)) {
            rc = mosquitto_property_add_int16(&local_props, MQTT_PROP_RECEIVE_MAXIMUM, mosq->msgs_in.inflight_maximum);
            if (rc) {
                return rc;
            }
        } else {
            mosq->msgs_in.inflight_maximum = receive_maximum;
            mosq->msgs_in.inflight_quota = receive_maximum;
        }

        version = MQTT_PROTOCOL_V5;
        headerlen = 10;
        proplen = 0;
        proplen += property__get_length_all(properties);
        proplen += property__get_length_all(local_props);
        varbytes = packet__varint_bytes(proplen);
        headerlen += proplen + varbytes;
    } else if (mosq->protocol == mosq_p_mqtt311) {
        version = MQTT_PROTOCOL_V311;
        headerlen = 10;
    } else if (mosq->protocol == mosq_p_mqtt31) {
        version = MQTT_PROTOCOL_V31;
        headerlen = 12;
    } else {
        return MOSQ_ERR_INVAL;
    }

    if (clientid) {
        payloadlen = (uint32_t)(2U + strlen(clientid));
    } else {
        payloadlen = 2U;
    }

    if (mosq->will) {
        will = 1;
        assert(mosq->will->msg.topic);

        payloadlen += (uint32_t)(2 + strlen(mosq->will->msg.topic) + 2 + (uint32_t)mosq->will->msg.payloadlen);
        if (mosq->protocol == mosq_p_mqtt5) {
            payloadlen += property__get_remaining_length(mosq->will->properties);
        }
    }

    // 进行此检查后，我们可以确保用户名和密码始终对当前协议有效，因此在检查密码之前无需检查用户名
    if ((mosq->protocol == mosq_p_mqtt31) || (mosq->protocol == mosq_p_mqtt311)) {
        if ((password != NULL) && (username == NULL)) {
            return MOSQ_ERR_INVAL;
        }
    }

    if (username) {
        payloadlen += (uint32_t)(2 + strlen(username));
    }

    if (password) {
        payloadlen += (uint32_t)(2 + strlen(password));
    }

    rc = packet__alloc(&packet, CMD_CONNECT, headerlen + payloadlen);
    if (rc) {
        mosquitto__FREE(packet);
        return rc;
    }

    // 可变标题
    if (version == MQTT_PROTOCOL_V31) {
        packet__write_string(packet, PROTOCOL_NAME_v31, (uint16_t)strlen(PROTOCOL_NAME_v31));
    } else {
        packet__write_string(packet, PROTOCOL_NAME, (uint16_t)strlen(PROTOCOL_NAME));
    }

    packet__write_byte(packet, version);
    byte = (uint8_t)((clean_session & 0x1) << 1);
    if (will) {
        byte = byte | (uint8_t)(((mosq->will->msg.qos & 0x3) << 3) | ((will & 0x1) << 2));
        if (mosq->retain_available) {
            byte |= (uint8_t)((mosq->will->msg.retain & 0x1) << 5);
        }
    }

    if (username) {
        byte = byte | 0x1 << 7;
    }

    if (mosq->password) {
        byte = byte | 0x1 << 6;
    }

    packet__write_byte(packet, byte);
    packet__write_uint16(packet, keepalive);

    if (mosq->protocol == mosq_p_mqtt5) {
        // 写属性
        packet__write_varint(packet, proplen);
        property__write_all(packet, properties, false);
        property__write_all(packet, local_props, false);
    }
    mosquitto_property_free_all(&local_props);

    // payload
    if (clientid) {
        packet__write_string(packet, clientid, (uint16_t)strlen(clientid));
    } else {
        packet__write_uint16(packet, 0);
    }

    if (will) {
        if (mosq->protocol == mosq_p_mqtt5) {
            // 写遗嘱属性
            property__write_all(packet, mosq->will->properties, true);
        }

        packet__write_string(packet, mosq->will->msg.topic, (uint16_t)strlen(mosq->will->msg.topic));
        packet__write_string(packet, (const char *)mosq->will->msg.payload, (uint16_t)mosq->will->msg.payloadlen);
    }

    if (username) {
        packet__write_string(packet, username, (uint16_t)strlen(username));
    }

    if (password) {
        packet__write_string(packet, password, (uint16_t)strlen(password));
    }

    mosq->keepalive = keepalive;
    log__printf(mosq, MOSQ_LOG_DEBUG, "Client %s sending CONNECT", SAFE_PRINT(clientid));

    return packet__queue(mosq, packet);
}
