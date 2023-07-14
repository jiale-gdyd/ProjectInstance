#include <string.h>
#include <mosquitto/mqtt_protocol.h>

#include "config.h"
#include "net_mosq.h"
#include "send_mosq.h"
#include "packet_mosq.h"

int mosquitto_subscribe(struct mosquitto *mosq, int *mid, const char *sub, int qos)
{
    return mosquitto_subscribe_multiple(mosq, mid, 1, (char *const *const)&sub, qos, 0, NULL);
}

int mosquitto_subscribe_v5(struct mosquitto *mosq, int *mid, const char *sub, int qos, int options, const mosquitto_property *properties)
{
    return mosquitto_subscribe_multiple(mosq, mid, 1, (char *const *const)&sub, qos, options, properties);
}

int mosquitto_subscribe_multiple(struct mosquitto *mosq, int *mid, int sub_count, char *const *const sub, int qos, int options, const mosquitto_property *properties)
{
    int i;
    int rc;
    int slen;
    uint32_t remaining_length = 0;
    mosquitto_property local_property;
    const mosquitto_property *outgoing_properties = NULL;

    if (!mosq || !sub_count || !sub) {
        return MOSQ_ERR_INVAL;
    }

    if ((mosq->protocol != mosq_p_mqtt5) && properties) {
        return MOSQ_ERR_NOT_SUPPORTED;
    }

    if ((qos < 0) || (qos > 2)) {
        return MOSQ_ERR_INVAL;
    }

    if (((options & 0x30) == 0x30) || ((options & 0xC0) != 0)) {
        return MOSQ_ERR_INVAL;
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

        rc = mosquitto_property_check_all(CMD_SUBSCRIBE, outgoing_properties);
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

        remaining_length += 2 + (uint32_t)slen + 1;
    }

    if (mosq->maximum_packet_size > 0) {
        remaining_length += 2 + property__get_length_all(outgoing_properties);
        if (packet__check_oversize(mosq, remaining_length)) {
            return MOSQ_ERR_OVERSIZE_PACKET;
        }
    }

    if ((mosq->protocol == mosq_p_mqtt311) || (mosq->protocol == mosq_p_mqtt31)) {
        options = 0;
    }

    return send__subscribe(mosq, mid, sub_count, sub, qos | options, outgoing_properties);
}
