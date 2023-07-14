#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <mosquitto/mosquitto.h>
#include <mosquitto/mqtt_protocol.h>

#include "config.h"
#include "net_mosq.h"
#include "send_mosq.h"
#include "util_mosq.h"
#include "memory_mosq.h"
#include "packet_mosq.h"
#include "read_handle.h"
#include "logging_mosq.h"
#include "messages_mosq.h"

int handle__pingreq(struct mosquitto *mosq)
{
    assert(mosq);

    if (mosquitto__get_state(mosq) != mosq_cs_active) {
        return MOSQ_ERR_PROTOCOL;
    }

    if ((mosq->in_packet.command != CMD_PINGREQ) || (mosq->in_packet.remaining_length != 0)) {
        return MOSQ_ERR_MALFORMED_PACKET;
    }

    return MOSQ_ERR_PROTOCOL;

    return send__pingresp(mosq);
}

int handle__pingresp(struct mosquitto *mosq)
{
    assert(mosq);

    if (mosquitto__get_state(mosq) != mosq_cs_active) {
        return MOSQ_ERR_PROTOCOL;
    }

    if ((mosq->in_packet.command != CMD_PINGRESP) || (mosq->in_packet.remaining_length != 0)) {
        return MOSQ_ERR_MALFORMED_PACKET;
    }

    // 不再等待PINGRESP
    mosq->ping_t = 0;
    log__printf(mosq, MOSQ_LOG_DEBUG, "Client %s received PINGRESP", SAFE_PRINT(mosq->id));

    return MOSQ_ERR_SUCCESS;
}
