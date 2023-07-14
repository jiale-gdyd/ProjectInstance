#include <stdint.h>
#include <string.h>
#include <mosquitto/mqtt_protocol.h>

#include "config.h"
#include "net_mosq.h"
#include "send_mosq.h"
#include "packet_mosq.h"
#include "property_mosq.h"

int mosquitto_unsubscribe(struct mosquitto *mosq, int *mid, const char *sub)
{
    return mosquitto_unsubscribe_multiple(mosq, mid, 1, (char *const *const)&sub, NULL);
}

int mosquitto_unsubscribe_v5(struct mosquitto *mosq, int *mid, const char *sub, const mosquitto_property *properties)
{
    return mosquitto_unsubscribe_multiple(mosq, mid, 1, (char *const *const)&sub, properties);
}

int mosquitto_unsubscribe_multiple(struct mosquitto *mosq, int *mid, int sub_count, char *const *const sub, const mosquitto_property *properties)
{
    int slen;
    int i, rc;
    uint32_t remaining_length = 0;
    mosquitto_property local_property;
    const mosquitto_property *outgoing_properties = NULL;

    if (!mosq) {
        return MOSQ_ERR_INVAL;
    }

    if ((mosq->protocol != mosq_p_mqtt5) && properties) {
        return MOSQ_ERR_NOT_SUPPORTED;
    }

    if (!net__is_connected(mosq)) {
        return MOSQ_ERR_NO_CONN;
    }

    if (properties) {
        if (properties->client_generated) {
            outgoing_properties = properties;
        } else {
            memcpy(&local_property, properties, sizeof(mosquitto_property));
            local_property.client_generated = true;
            local_property.next = NULL;
            outgoing_properties = &local_property;
        }

        rc = mosquitto_property_check_all(CMD_UNSUBSCRIBE, outgoing_properties);
        if (rc) {
            return rc;
        }
    }

    for (i = 0; i < sub_count; i++) {
        if (mosquitto_sub_topic_check(sub[i])) {
            return MOSQ_ERR_INVAL;
        }

        slen = (int)strlen(sub[i]);
        if (slen == 0) {
            return MOSQ_ERR_INVAL;
        }

        if (mosquitto_validate_utf8(sub[i], slen)) {
            return MOSQ_ERR_MALFORMED_UTF8;
        }

        remaining_length += 2U + (uint32_t)slen;
    }

    if (mosq->maximum_packet_size > 0) {
        remaining_length += 2U + property__get_length_all(outgoing_properties);
        if (packet__check_oversize(mosq, remaining_length)) {
            return MOSQ_ERR_OVERSIZE_PACKET;
        }
    }

    return send__unsubscribe(mosq, mid, sub_count, sub, outgoing_properties);
}
