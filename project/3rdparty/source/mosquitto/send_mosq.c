#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <mosquitto/mosquitto.h>
#include <mosquitto/mqtt_protocol.h>

#include "config.h"
#include "net_mosq.h"
#include "send_mosq.h"
#include "time_mosq.h"
#include "util_mosq.h"
#include "memory_mosq.h"
#include "packet_mosq.h"
#include "logging_mosq.h"
#include "property_mosq.h"
#include "mosquitto_internal.h"

int send__pingreq(struct mosquitto *mosq)
{
    int rc;

    assert(mosq);
    log__printf(mosq, MOSQ_LOG_DEBUG, "Client %s sending PINGREQ", SAFE_PRINT(mosq->id));

    rc = send__simple_command(mosq, CMD_PINGREQ);
    if  (rc == MOSQ_ERR_SUCCESS){
        mosq->ping_t = mosquitto_time();
    }

    return rc;
}

int send__pingresp(struct mosquitto *mosq)
{
    log__printf(mosq, MOSQ_LOG_DEBUG, "Client %s sending PINGRESP", SAFE_PRINT(mosq->id));
    return send__simple_command(mosq, CMD_PINGRESP);
}

int send__puback(struct mosquitto *mosq, uint16_t mid, uint8_t reason_code, const mosquitto_property *properties)
{
    log__printf(mosq, MOSQ_LOG_DEBUG, "Client %s sending PUBACK (m%d, rc%d)", SAFE_PRINT(mosq->id), mid, reason_code);

    util__increment_receive_quota(mosq);

    // 我们尚未使用原因字符串或用户属性
    return send__command_with_mid(mosq, CMD_PUBACK, mid, false, reason_code, properties);
}

int send__pubcomp(struct mosquitto *mosq, uint16_t mid, const mosquitto_property *properties)
{
    log__printf(mosq, MOSQ_LOG_DEBUG, "Client %s sending PUBCOMP (m%d)", SAFE_PRINT(mosq->id), mid);

    util__increment_receive_quota(mosq);

    // 我们尚未使用原因字符串或用户属性
    return send__command_with_mid(mosq, CMD_PUBCOMP, mid, false, 0, properties);
}

int send__pubrec(struct mosquitto *mosq, uint16_t mid, uint8_t reason_code, const mosquitto_property *properties)
{
    log__printf(mosq, MOSQ_LOG_DEBUG, "Client %s sending PUBREC (m%d, rc%d)", SAFE_PRINT(mosq->id), mid, reason_code);

    if ((reason_code >= 0x80) && (mosq->protocol == mosq_p_mqtt5)) {
        util__increment_receive_quota(mosq);
    }

    // 我们尚未使用原因字符串或用户属性
    return send__command_with_mid(mosq, CMD_PUBREC, mid, false, reason_code, properties);
}

int send__pubrel(struct mosquitto *mosq, uint16_t mid, const mosquitto_property *properties)
{
    log__printf(mosq, MOSQ_LOG_DEBUG, "Client %s sending PUBREL (m%d)", SAFE_PRINT(mosq->id), mid);

    // 我们尚未使用原因字符串或用户属性
    return send__command_with_mid(mosq, CMD_PUBREL | 2, mid, false, 0, properties);
}

int send__command_with_mid(struct mosquitto *mosq, uint8_t command, uint16_t mid, bool dup, uint8_t reason_code, const mosquitto_property *properties)
{
    int rc;
    uint32_t remaining_length;
    struct mosquitto__packet *packet = NULL;

    assert(mosq);

    if (dup) {
        command |= 8;
    }
    remaining_length = 2;

    if (mosq->protocol == mosq_p_mqtt5) {
        if ((reason_code != 0) || properties) {
            remaining_length += 1;
        }

        if (properties) {
            remaining_length += property__get_remaining_length(properties);
        }
    }

    rc = packet__alloc(&packet, command, remaining_length);
    if (rc) {
        mosquitto__FREE(packet);
        return rc;
    }

    packet__write_uint16(packet, mid);

    if (mosq->protocol == mosq_p_mqtt5) {
        if ((reason_code != 0) || properties) {
            packet__write_byte(packet, reason_code);
        }

        if (properties) {
            property__write_all(packet, properties, true);
        }
    }

    return packet__queue(mosq, packet);
}

int send__simple_command(struct mosquitto *mosq, uint8_t command)
{
    int rc;
    struct mosquitto__packet *packet = NULL;

    assert(mosq);

    rc = packet__alloc(&packet, command, 0);
    if (rc) {
        return rc;
    }

    return packet__queue(mosq, packet);
}
