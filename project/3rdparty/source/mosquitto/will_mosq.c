#include <stdio.h>
#include <string.h>
#include <mosquitto/mosquitto.h>
#include <mosquitto/mqtt_protocol.h>

#include "config.h"
#include "net_mosq.h"
#include "send_mosq.h"
#include "util_mosq.h"
#include "will_mosq.h"
#include "memory_mosq.h"
#include "read_handle.h"
#include "logging_mosq.h"
#include "messages_mosq.h"
#include "mosquitto_internal.h"

int will__set(struct mosquitto *mosq, const char *topic, int payloadlen, const void *payload, int qos, bool retain, mosquitto_property *properties)
{
    mosquitto_property *p;
    int rc = MOSQ_ERR_SUCCESS;

    if (!mosq || !topic) {
        return MOSQ_ERR_INVAL;
    }

    if ((payloadlen < 0) || (payloadlen > (int)MQTT_MAX_PAYLOAD)) {
        return MOSQ_ERR_PAYLOAD_SIZE;
    }

    if ((payloadlen > 0) && !payload) {
        return MOSQ_ERR_INVAL;
    }

    if (mosquitto_pub_topic_check(topic)) {
        return MOSQ_ERR_INVAL;
    }

    if (mosquitto_validate_utf8(topic, (uint16_t)strlen(topic))) {
        return MOSQ_ERR_MALFORMED_UTF8;
    }

    if (properties) {
        if (mosq->protocol != mosq_p_mqtt5) {
            return MOSQ_ERR_NOT_SUPPORTED;
        }

        p = properties;
        while (p) {
            rc = mosquitto_property_check_command(CMD_WILL, mosquitto_property_identifier(p));
            if (rc) {
                return rc;
            }
            p = mosquitto_property_next(p);
        }
    }

    if (mosq->will) {
        mosquitto__FREE(mosq->will->msg.topic);
        mosquitto__FREE(mosq->will->msg.payload);
        mosquitto_property_free_all(&mosq->will->properties);
        mosquitto__FREE(mosq->will);
    }

    mosq->will = (struct mosquitto_message_all *)mosquitto__calloc(1, sizeof(struct mosquitto_message_all));
    if (!mosq->will) {
        return MOSQ_ERR_NOMEM;
    }

    mosq->will->msg.topic = mosquitto__strdup(topic);
    if (!mosq->will->msg.topic) {
        rc = MOSQ_ERR_NOMEM;
        goto cleanup;
    }

    mosq->will->msg.payloadlen = payloadlen;
    if (mosq->will->msg.payloadlen > 0) {
        if (!payload) {
            rc = MOSQ_ERR_INVAL;
            goto cleanup;
        }

        mosq->will->msg.payload = mosquitto__malloc(sizeof(char)*(unsigned int)mosq->will->msg.payloadlen);
        if (!mosq->will->msg.payload) {
            rc = MOSQ_ERR_NOMEM;
            goto cleanup;
        }

        memcpy(mosq->will->msg.payload, payload, (unsigned int)payloadlen);
    }

    mosq->will->msg.qos = qos;
    mosq->will->msg.retain = retain;
    mosq->will->properties = properties;

    return MOSQ_ERR_SUCCESS;

cleanup:
    if (mosq->will) {
        mosquitto__FREE(mosq->will->msg.topic);
        mosquitto__FREE(mosq->will->msg.payload);
        mosquitto__FREE(mosq->will);
    }

    return rc;
}

int will__clear(struct mosquitto *mosq)
{
    if (!mosq->will) {
        return MOSQ_ERR_SUCCESS;
    }

    mosquitto__FREE(mosq->will->msg.topic);
    mosquitto__FREE(mosq->will->msg.payload);
    mosquitto_property_free_all(&mosq->will->properties);
    mosquitto__FREE(mosq->will);

    mosq->will_delay_interval = 0;

    return MOSQ_ERR_SUCCESS;
}
