#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <mosquitto/mqtt_protocol.h>

#include "config.h"
#include "send_mosq.h"
#include "util_mosq.h"
#include "memory_mosq.h"
#include "packet_mosq.h"
#include "property_mosq.h"
#include "messages_mosq.h"
#include "mosquitto_internal.h"

int mosquitto_publish(struct mosquitto *mosq, int *mid, const char *topic, int payloadlen, const void *payload, int qos, bool retain)
{
    return mosquitto_publish_v5(mosq, mid, topic, payloadlen, payload, qos, retain, NULL);
}

int mosquitto_publish_v5(struct mosquitto *mosq, int *mid, const char *topic, int payloadlen, const void *payload, int qos, bool retain, const mosquitto_property *properties)
{
    int rc;
    size_t tlen = 0;
    uint16_t local_mid;
    bool have_topic_alias;
    uint32_t remaining_length;
    const mosquitto_property *p;
    mosquitto_property local_property;
    struct mosquitto_message_all *message;
    mosquitto_property *properties_copy = NULL;
    const mosquitto_property *outgoing_properties = NULL;

    if (!mosq || (qos < 0) || (qos > 2)) {
        return MOSQ_ERR_INVAL;
    }

    if ((mosq->protocol != mosq_p_mqtt5) && properties) {
        return MOSQ_ERR_NOT_SUPPORTED;
    }

    if (qos > mosq->max_qos) {
        return MOSQ_ERR_QOS_NOT_SUPPORTED;
    }

    if (!mosq->retain_available) {
        retain = false;
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

        rc = mosquitto_property_check_all(CMD_PUBLISH, outgoing_properties);
        if (rc) {
            return rc;
        }
    }

    if (!topic || STREMPTY(topic)) {
        if (topic) {
            topic = NULL;
        }

        if (mosq->protocol == mosq_p_mqtt5) {
            p = outgoing_properties;
            have_topic_alias = false;

            while (p) {
                if (mosquitto_property_identifier(p) == MQTT_PROP_TOPIC_ALIAS) {
                    have_topic_alias = true;
                    break;
                }

                p = mosquitto_property_next(p);
            }

            if (have_topic_alias == false) {
                return MOSQ_ERR_INVAL;
            }
        } else {
            return MOSQ_ERR_INVAL;
        }
    } else {
        tlen = strlen(topic);
        if (mosquitto_validate_utf8(topic, (int)tlen)) {
            return MOSQ_ERR_MALFORMED_UTF8;
        }

        if ((payloadlen < 0) || (payloadlen > (int)MQTT_MAX_PAYLOAD)) {
            return MOSQ_ERR_PAYLOAD_SIZE;
        }

        if (mosquitto_pub_topic_check(topic) != MOSQ_ERR_SUCCESS){
            return MOSQ_ERR_INVAL;
        }
    }

    if (mosq->maximum_packet_size > 0) {
        remaining_length = 1 + 2+(uint32_t)tlen + (uint32_t)payloadlen + property__get_length_all(outgoing_properties);
        if (qos > 0) {
            remaining_length++;
        }

        if (packet__check_oversize(mosq, remaining_length)) {
            return MOSQ_ERR_OVERSIZE_PACKET;
        }
    }

    local_mid = mosquitto__mid_generate(mosq);
    if (mid) {
        *mid = local_mid;
    }

    if (qos == 0) {
        return send__publish(mosq, local_mid, topic, (uint32_t)payloadlen, payload, (uint8_t)qos, retain, false, 0, outgoing_properties, 0);
    } else {
        if (outgoing_properties) {
            rc = mosquitto_property_copy_all(&properties_copy, outgoing_properties);
            if (rc) {
                return rc;
            }
        }

        message = (struct mosquitto_message_all *)mosquitto__calloc(1, sizeof(struct mosquitto_message_all));
        if (!message) {
            mosquitto_property_free_all(&properties_copy);
            return MOSQ_ERR_NOMEM;
        }

        message->next = NULL;
        message->msg.mid = local_mid;

        if (topic) {
            message->msg.topic = mosquitto__strdup(topic);
            if (!message->msg.topic) {
                message__cleanup(&message);
                mosquitto_property_free_all(&properties_copy);
                return MOSQ_ERR_NOMEM;
            }
        }

        if (payloadlen) {
            message->msg.payloadlen = payloadlen;
            message->msg.payload = mosquitto__malloc((unsigned int)payloadlen*sizeof(uint8_t));
            if (!message->msg.payload) {
                message__cleanup(&message);
                mosquitto_property_free_all(&properties_copy);
                return MOSQ_ERR_NOMEM;
            }

            memcpy(message->msg.payload, payload, (uint32_t)payloadlen*sizeof(uint8_t));
        } else {
            message->msg.payloadlen = 0;
            message->msg.payload = NULL;
        }

        message->msg.qos = (uint8_t)qos;
        message->msg.retain = retain;
        message->dup = false;
        message->properties = properties_copy;

        pthread_mutex_lock(&mosq->msgs_out.mutex);
        message->state = mosq_ms_invalid;
        rc = message__queue(mosq, message, mosq_md_out);
        pthread_mutex_unlock(&mosq->msgs_out.mutex);

        return rc;
    }
}
