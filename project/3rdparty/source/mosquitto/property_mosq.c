#include <errno.h>
#include <string.h>
#include <assert.h>
#include <strings.h>
#include <mosquitto/mosquitto.h>
#include <mosquitto/mqtt_protocol.h>

#include "config.h"
#include "memory_mosq.h"
#include "packet_mosq.h"
#include "property_mosq.h"

static int property__read(struct mosquitto__packet_in *packet, uint32_t *len, mosquitto_property *property)
{
    int rc;
    uint8_t byte;
    uint16_t uint16;
    uint32_t uint32;
    uint32_t varint;
    char *str1, *str2;
    uint8_t byte_count;
    uint16_t slen1, slen2;
    uint32_t property_identifier;

    if (!property) {
        return MOSQ_ERR_INVAL;
    }

    rc = packet__read_varint(packet, &property_identifier, NULL);
    if (rc) {
        return rc;
    }
    *len -= 1;

    memset(property, 0x00, sizeof(mosquitto_property));

    property->identifier = (int32_t)property_identifier;
    switch (property_identifier) {
        case MQTT_PROP_PAYLOAD_FORMAT_INDICATOR:
        case MQTT_PROP_REQUEST_PROBLEM_INFORMATION:
        case MQTT_PROP_REQUEST_RESPONSE_INFORMATION:
        case MQTT_PROP_MAXIMUM_QOS:
        case MQTT_PROP_RETAIN_AVAILABLE:
        case MQTT_PROP_WILDCARD_SUB_AVAILABLE:
        case MQTT_PROP_SUBSCRIPTION_ID_AVAILABLE:
        case MQTT_PROP_SHARED_SUB_AVAILABLE:
            rc = packet__read_byte(packet, &byte);
            if (rc) {
                return rc;
            }
            *len -= 1;
            property->value.i8 = byte;
            property->property_type = MQTT_PROP_TYPE_BYTE;
            break;

        case MQTT_PROP_SERVER_KEEP_ALIVE:
        case MQTT_PROP_RECEIVE_MAXIMUM:
        case MQTT_PROP_TOPIC_ALIAS_MAXIMUM:
        case MQTT_PROP_TOPIC_ALIAS:
            rc = packet__read_uint16(packet, &uint16);
            if (rc) {
                return rc;
            }
            *len -= 2;
            property->value.i16 = uint16;
            property->property_type = MQTT_PROP_TYPE_INT16;
            break;

        case MQTT_PROP_MESSAGE_EXPIRY_INTERVAL:
        case MQTT_PROP_SESSION_EXPIRY_INTERVAL:
        case MQTT_PROP_WILL_DELAY_INTERVAL:
        case MQTT_PROP_MAXIMUM_PACKET_SIZE:
            rc = packet__read_uint32(packet, &uint32);
            if (rc) {
                return rc;
            }
            *len -= 4;
            property->value.i32 = uint32;
            property->property_type = MQTT_PROP_TYPE_INT32;
            break;

        case MQTT_PROP_SUBSCRIPTION_IDENTIFIER:
            rc = packet__read_varint(packet, &varint, &byte_count);
            if (rc) {
                return rc;
            }
            *len -= byte_count;
            property->value.varint = varint;
            property->property_type = MQTT_PROP_TYPE_VARINT;
            break;

        case MQTT_PROP_CONTENT_TYPE:
        case MQTT_PROP_RESPONSE_TOPIC:
        case MQTT_PROP_ASSIGNED_CLIENT_IDENTIFIER:
        case MQTT_PROP_AUTHENTICATION_METHOD:
        case MQTT_PROP_RESPONSE_INFORMATION:
        case MQTT_PROP_SERVER_REFERENCE:
        case MQTT_PROP_REASON_STRING:
            rc = packet__read_string(packet, &str1, &slen1);
            if (rc) {
                return rc;
            }
            *len = (*len) - 2 - slen1;
            property->value.s.v = str1;
            property->value.s.len = slen1;
            property->property_type = MQTT_PROP_TYPE_STRING;
            break;

        case MQTT_PROP_AUTHENTICATION_DATA:
        case MQTT_PROP_CORRELATION_DATA:
            rc = packet__read_binary(packet, (uint8_t **)&str1, &slen1);
            if (rc) {
                return rc;
            }
            *len = (*len) - 2 - slen1;
            property->value.bin.v = str1;
            property->value.bin.len = slen1;
            property->property_type = MQTT_PROP_TYPE_BINARY;
            break;

        case MQTT_PROP_USER_PROPERTY:
            rc = packet__read_string(packet, &str1, &slen1);
            if (rc) {
                return rc;
            }
            *len = (*len) - 2 - slen1;

            rc = packet__read_string(packet, &str2, &slen2);
            if (rc) {
                mosquitto__FREE(str1);
                return rc;
            }
            *len = (*len) - 2 - slen2;

            property->name.v = str1;
            property->name.len = slen1;
            property->value.s.v = str2;
            property->value.s.len = slen2;
            property->property_type = MQTT_PROP_TYPE_STRING_PAIR;
            break;

        default:
            return MOSQ_ERR_MALFORMED_PACKET;
    }

    return MOSQ_ERR_SUCCESS;
}

int property__read_all(int command, struct mosquitto__packet_in *packet, mosquitto_property **properties)
{
    int rc;
    uint32_t proplen;
    mosquitto_property *p, *tail = NULL;

    rc = packet__read_varint(packet, &proplen, NULL);
    if (rc) {
        return rc;
    }

    *properties = NULL;

    // 对于某些类型，必须保留属性的顺序，因此对于所有类型，请保持相同的顺序
    while (proplen > 0) {
        p = (mosquitto_property *)mosquitto__calloc(1, sizeof(mosquitto_property));
        if (!p) {
            mosquitto_property_free_all(properties);
            return MOSQ_ERR_NOMEM;
        }

        rc = property__read(packet, &proplen, p); 
        if (rc) {
            mosquitto__FREE(p);
            mosquitto_property_free_all(properties);
            return rc;
        }

        if (!(*properties)) {
            *properties = p;
        } else {
            tail->next = p;
        }
        tail = p;
    }

    rc = mosquitto_property_check_all(command, *properties);
    if (rc) {
        mosquitto_property_free_all(properties);
        return rc;
    }

    return MOSQ_ERR_SUCCESS;
}

void property__free(mosquitto_property **property)
{
    if (!property || !(*property)) {
        return;
    }

    switch ((*property)->property_type) {
        case MQTT_PROP_TYPE_STRING:
            mosquitto__FREE((*property)->value.s.v);
            break;

        case MQTT_PROP_TYPE_BINARY:
            mosquitto__FREE((*property)->value.bin.v);
            break;

        case MQTT_PROP_TYPE_STRING_PAIR:
            mosquitto__FREE((*property)->name.v);
            mosquitto__FREE((*property)->value.s.v);
            break;

        case MQTT_PROP_TYPE_BYTE:
        case MQTT_PROP_TYPE_INT16:
        case MQTT_PROP_TYPE_INT32:
        case MQTT_PROP_TYPE_VARINT:
            break;
    }

    SAFE_FREE(*property);
}

void mosquitto_property_free_all(mosquitto_property **property)
{
    mosquitto_property *p, *next;

    if (!property) {
        return;
    }

    p = *property;
    while (p) {
        next = p->next;
        property__free(&p);
        p = next;
    }
    *property = NULL;
}

unsigned int property__get_length(const mosquitto_property *property)
{
    if (!property) {
        return 0;
    }

    switch (property->property_type) {
        case MQTT_PROP_TYPE_BYTE:
            return 2;                                                   // 1字节(识别码) + 1字节

        case MQTT_PROP_TYPE_INT16:
            return 3;                                                   // 1字节(识别码) + 2字节

        case MQTT_PROP_TYPE_INT32:
            return 5;                                                   // 1字节(识别码) + 4字节

        case MQTT_PROP_TYPE_VARINT:
            if (property->value.varint < 0x80) {
                return 2;
            } else if (property->value.varint < 0x4000) {
                return 3;
            } else if (property->value.varint < 0x200000) {
                return 4;
            } else if (property->value.varint < 0x10000000) {
                return 5;
            } else {
                return 0;
            }

        case MQTT_PROP_TYPE_BINARY:
            return (3U + property->value.bin.len);                      //  1字节(识别码) + 2字节(长度) + n字节(payload)

        case MQTT_PROP_TYPE_STRING:
            return (3U + property->value.s.len);                        // 1字节(识别码) + 2字节(长度) + n字节(string)

        case MQTT_PROP_TYPE_STRING_PAIR:
            return (5U + property->value.s.len + property->name.len);   // 1字节(识别码) + 2 * (2字节(长度) + n字节(string))

        default:
            return 0;
    }

    return 0;
}

unsigned int property__get_length_all(const mosquitto_property *property)
{
    unsigned int len = 0;
    const mosquitto_property *p;

    p = property;
    while (p) {
        len += property__get_length(p);
        p = p->next;
    }

    return len;
}

unsigned int property__get_remaining_length(const mosquitto_property *props)
{
    unsigned int proplen, varbytes;

    proplen = property__get_length_all(props);
    varbytes = packet__varint_bytes(proplen);

    return proplen + varbytes;
}

static int property__write(struct mosquitto__packet *packet, const mosquitto_property *property)
{
    int rc;

    rc = packet__write_varint(packet, (uint32_t)mosquitto_property_identifier(property));
    if (rc) {
        return rc;
    }

    switch (property->property_type) {
        case MQTT_PROP_TYPE_BYTE:
            packet__write_byte(packet, property->value.i8);
            break;

        case MQTT_PROP_TYPE_INT16:
            packet__write_uint16(packet, property->value.i16);
            break;

        case MQTT_PROP_TYPE_INT32:
            packet__write_uint32(packet, property->value.i32);
            break;

        case MQTT_PROP_TYPE_VARINT:
            return packet__write_varint(packet, property->value.varint);

        case MQTT_PROP_TYPE_STRING:
            packet__write_string(packet, property->value.s.v, property->value.s.len);
            break;

        case MQTT_PROP_TYPE_BINARY:
            packet__write_uint16(packet, property->value.bin.len);
            packet__write_bytes(packet, property->value.bin.v, property->value.bin.len);
            break;

        case MQTT_PROP_TYPE_STRING_PAIR:
            packet__write_string(packet, property->name.v, property->name.len);
            packet__write_string(packet, property->value.s.v, property->value.s.len);
            break;

        default:
            return MOSQ_ERR_INVAL;
    }

    return MOSQ_ERR_SUCCESS;
}

int property__write_all(struct mosquitto__packet *packet, const mosquitto_property *properties, bool write_len)
{
    int rc;
    const mosquitto_property *p;

    if (write_len) {
        rc = packet__write_varint(packet, property__get_length_all(properties));
        if (rc) {
            return rc;
        }
    }

    p = properties;
    while (p) {
        rc = property__write(packet, p);
        if (rc) {
            return rc;
        }
        p = p->next;
    }

    return MOSQ_ERR_SUCCESS;
}

int mosquitto_property_check_command(int command, int identifier)
{
    switch (identifier) {
        case MQTT_PROP_PAYLOAD_FORMAT_INDICATOR:
        case MQTT_PROP_MESSAGE_EXPIRY_INTERVAL:
        case MQTT_PROP_CONTENT_TYPE:
        case MQTT_PROP_RESPONSE_TOPIC:
        case MQTT_PROP_CORRELATION_DATA:
            if ((command != CMD_PUBLISH) && (command != CMD_WILL)) {
                return MOSQ_ERR_PROTOCOL;
            }
            break;

        case MQTT_PROP_SUBSCRIPTION_IDENTIFIER:
            if ((command != CMD_PUBLISH) && (command != CMD_SUBSCRIBE)) {
                return MOSQ_ERR_PROTOCOL;
            }
            break;

        case MQTT_PROP_SESSION_EXPIRY_INTERVAL:
            if ((command != CMD_CONNECT) && (command != CMD_CONNACK) && (command != CMD_DISCONNECT)) {
                return MOSQ_ERR_PROTOCOL;
            }
            break;

        case MQTT_PROP_AUTHENTICATION_METHOD:
        case MQTT_PROP_AUTHENTICATION_DATA:
            if ((command != CMD_CONNECT) && (command != CMD_CONNACK) && (command != CMD_AUTH)) {
                return MOSQ_ERR_PROTOCOL;
            }
            break;

        case MQTT_PROP_ASSIGNED_CLIENT_IDENTIFIER:
        case MQTT_PROP_SERVER_KEEP_ALIVE:
        case MQTT_PROP_RESPONSE_INFORMATION:
        case MQTT_PROP_MAXIMUM_QOS:
        case MQTT_PROP_RETAIN_AVAILABLE:
        case MQTT_PROP_WILDCARD_SUB_AVAILABLE:
        case MQTT_PROP_SUBSCRIPTION_ID_AVAILABLE:
        case MQTT_PROP_SHARED_SUB_AVAILABLE:
            if (command != CMD_CONNACK) {
                return MOSQ_ERR_PROTOCOL;
            }
            break;

        case MQTT_PROP_WILL_DELAY_INTERVAL:
            if (command != CMD_WILL) {
                return MOSQ_ERR_PROTOCOL;
            }
            break;

        case MQTT_PROP_REQUEST_PROBLEM_INFORMATION:
        case MQTT_PROP_REQUEST_RESPONSE_INFORMATION:
            if (command != CMD_CONNECT) {
                return MOSQ_ERR_PROTOCOL;
            }
            break;

        case MQTT_PROP_SERVER_REFERENCE:
            if ((command != CMD_CONNACK) && (command != CMD_DISCONNECT)) {
                return MOSQ_ERR_PROTOCOL;
            }
            break;

        case MQTT_PROP_REASON_STRING:
            if ((command == CMD_CONNECT) || (command == CMD_PUBLISH) || (command == CMD_SUBSCRIBE) || (command == CMD_UNSUBSCRIBE)) {
                return MOSQ_ERR_PROTOCOL;
            }
            break;

        case MQTT_PROP_RECEIVE_MAXIMUM:
        case MQTT_PROP_TOPIC_ALIAS_MAXIMUM:
        case MQTT_PROP_MAXIMUM_PACKET_SIZE:
            if ((command != CMD_CONNECT) && (command != CMD_CONNACK)) {
                return MOSQ_ERR_PROTOCOL;
            }
            break;

        case MQTT_PROP_TOPIC_ALIAS:
            if (command != CMD_PUBLISH) {
                return MOSQ_ERR_PROTOCOL;
            }
            break;

        case MQTT_PROP_USER_PROPERTY:
            break;

        default:
            return MOSQ_ERR_PROTOCOL;
    }

    return MOSQ_ERR_SUCCESS;
}

const char *mosquitto_property_identifier_to_string(int identifier)
{
    switch (identifier) {
        case MQTT_PROP_PAYLOAD_FORMAT_INDICATOR:
            return "payload-format-indicator";

        case MQTT_PROP_MESSAGE_EXPIRY_INTERVAL:
            return "message-expiry-interval";

        case MQTT_PROP_CONTENT_TYPE:
            return "content-type";

        case MQTT_PROP_RESPONSE_TOPIC:
            return "response-topic";

        case MQTT_PROP_CORRELATION_DATA:
            return "correlation-data";

        case MQTT_PROP_SUBSCRIPTION_IDENTIFIER:
            return "subscription-identifier";

        case MQTT_PROP_SESSION_EXPIRY_INTERVAL:
            return "session-expiry-interval";

        case MQTT_PROP_ASSIGNED_CLIENT_IDENTIFIER:
            return "assigned-client-identifier";

        case MQTT_PROP_SERVER_KEEP_ALIVE:
            return "server-keep-alive";

        case MQTT_PROP_AUTHENTICATION_METHOD:
            return "authentication-method";

        case MQTT_PROP_AUTHENTICATION_DATA:
            return "authentication-data";

        case MQTT_PROP_REQUEST_PROBLEM_INFORMATION:
            return "request-problem-information";

        case MQTT_PROP_WILL_DELAY_INTERVAL:
            return "will-delay-interval";

        case MQTT_PROP_REQUEST_RESPONSE_INFORMATION:
            return "request-response-information";

        case MQTT_PROP_RESPONSE_INFORMATION:
            return "response-information";

        case MQTT_PROP_SERVER_REFERENCE:
            return "server-reference";

        case MQTT_PROP_REASON_STRING:
            return "reason-string";

        case MQTT_PROP_RECEIVE_MAXIMUM:
            return "receive-maximum";

        case MQTT_PROP_TOPIC_ALIAS_MAXIMUM:
            return "topic-alias-maximum";

        case MQTT_PROP_TOPIC_ALIAS:
            return "topic-alias";

        case MQTT_PROP_MAXIMUM_QOS:
            return "maximum-qos";

        case MQTT_PROP_RETAIN_AVAILABLE:
            return "retain-available";

        case MQTT_PROP_USER_PROPERTY:
            return "user-property";

        case MQTT_PROP_MAXIMUM_PACKET_SIZE:
            return "maximum-packet-size";

        case MQTT_PROP_WILDCARD_SUB_AVAILABLE:
            return "wildcard-subscription-available";

        case MQTT_PROP_SUBSCRIPTION_ID_AVAILABLE:
            return "subscription-identifier-available";

        case MQTT_PROP_SHARED_SUB_AVAILABLE:
            return "shared-subscription-available";

        default:
            return NULL;
    }
}

int mosquitto_string_to_property_info(const char *propname, int *identifier, int *type)
{
    if (!propname) {
        return MOSQ_ERR_INVAL;
    }

    if (!strcasecmp(propname, "payload-format-indicator")) {
        *identifier = MQTT_PROP_PAYLOAD_FORMAT_INDICATOR;
        *type = MQTT_PROP_TYPE_BYTE;
    } else if (!strcasecmp(propname, "message-expiry-interval")) {
        *identifier = MQTT_PROP_MESSAGE_EXPIRY_INTERVAL;
        *type = MQTT_PROP_TYPE_INT32;
    } else if (!strcasecmp(propname, "content-type")) {
        *identifier = MQTT_PROP_CONTENT_TYPE;
        *type = MQTT_PROP_TYPE_STRING;
    } else if (!strcasecmp(propname, "response-topic")) {
        *identifier = MQTT_PROP_RESPONSE_TOPIC;
        *type = MQTT_PROP_TYPE_STRING;
    } else if (!strcasecmp(propname, "correlation-data")) {
        *identifier = MQTT_PROP_CORRELATION_DATA;
        *type = MQTT_PROP_TYPE_BINARY;
    } else if (!strcasecmp(propname, "subscription-identifier")) {
        *identifier = MQTT_PROP_SUBSCRIPTION_IDENTIFIER;
        *type = MQTT_PROP_TYPE_VARINT;
    } else if (!strcasecmp(propname, "session-expiry-interval")) {
        *identifier = MQTT_PROP_SESSION_EXPIRY_INTERVAL;
        *type = MQTT_PROP_TYPE_INT32;
    } else if (!strcasecmp(propname, "assigned-client-identifier")) {
        *identifier = MQTT_PROP_ASSIGNED_CLIENT_IDENTIFIER;
        *type = MQTT_PROP_TYPE_STRING;
    } else if (!strcasecmp(propname, "server-keep-alive")) {
        *identifier = MQTT_PROP_SERVER_KEEP_ALIVE;
        *type = MQTT_PROP_TYPE_INT16;
    } else if (!strcasecmp(propname, "authentication-method")) {
        *identifier = MQTT_PROP_AUTHENTICATION_METHOD;
        *type = MQTT_PROP_TYPE_STRING;
    } else if (!strcasecmp(propname, "authentication-data")) {
        *identifier = MQTT_PROP_AUTHENTICATION_DATA;
        *type = MQTT_PROP_TYPE_BINARY;
    } else if (!strcasecmp(propname, "request-problem-information")) {
        *identifier = MQTT_PROP_REQUEST_PROBLEM_INFORMATION;
        *type = MQTT_PROP_TYPE_BYTE;
    } else if (!strcasecmp(propname, "will-delay-interval")) {
        *identifier = MQTT_PROP_WILL_DELAY_INTERVAL;
        *type = MQTT_PROP_TYPE_INT32;
    } else if (!strcasecmp(propname, "request-response-information")) {
        *identifier = MQTT_PROP_REQUEST_RESPONSE_INFORMATION;
        *type = MQTT_PROP_TYPE_BYTE;
    } else if (!strcasecmp(propname, "response-information")) {
        *identifier = MQTT_PROP_RESPONSE_INFORMATION;
        *type = MQTT_PROP_TYPE_STRING;
    } else if (!strcasecmp(propname, "server-reference")) {
        *identifier = MQTT_PROP_SERVER_REFERENCE;
        *type = MQTT_PROP_TYPE_STRING;
    } else if (!strcasecmp(propname, "reason-string")) {
        *identifier = MQTT_PROP_REASON_STRING;
        *type = MQTT_PROP_TYPE_STRING;
    } else if (!strcasecmp(propname, "receive-maximum")) {
        *identifier = MQTT_PROP_RECEIVE_MAXIMUM;
        *type = MQTT_PROP_TYPE_INT16;
    } else if (!strcasecmp(propname, "topic-alias-maximum")) {
        *identifier = MQTT_PROP_TOPIC_ALIAS_MAXIMUM;
        *type = MQTT_PROP_TYPE_INT16;
    } else if (!strcasecmp(propname, "topic-alias")) {
        *identifier = MQTT_PROP_TOPIC_ALIAS;
        *type = MQTT_PROP_TYPE_INT16;
    } else if (!strcasecmp(propname, "maximum-qos")) {
        *identifier = MQTT_PROP_MAXIMUM_QOS;
        *type = MQTT_PROP_TYPE_BYTE;
    } else if (!strcasecmp(propname, "retain-available")) {
        *identifier = MQTT_PROP_RETAIN_AVAILABLE;
        *type = MQTT_PROP_TYPE_BYTE;
    } else if (!strcasecmp(propname, "user-property")) {
        *identifier = MQTT_PROP_USER_PROPERTY;
        *type = MQTT_PROP_TYPE_STRING_PAIR;
    } else if (!strcasecmp(propname, "maximum-packet-size")) {
        *identifier = MQTT_PROP_MAXIMUM_PACKET_SIZE;
        *type = MQTT_PROP_TYPE_INT32;
    } else if (!strcasecmp(propname, "wildcard-subscription-available")) {
        *identifier = MQTT_PROP_WILDCARD_SUB_AVAILABLE;
        *type = MQTT_PROP_TYPE_BYTE;
    } else if (!strcasecmp(propname, "subscription-identifier-available")) {
        *identifier = MQTT_PROP_SUBSCRIPTION_ID_AVAILABLE;
        *type = MQTT_PROP_TYPE_BYTE;
    } else if (!strcasecmp(propname, "shared-subscription-available")) {
        *identifier = MQTT_PROP_SHARED_SUB_AVAILABLE;
        *type = MQTT_PROP_TYPE_BYTE;
    } else {
        return MOSQ_ERR_INVAL;
    }

    return MOSQ_ERR_SUCCESS;
}

static void property__add(mosquitto_property **proplist, struct mqtt5__property *prop)
{
    mosquitto_property *p;

    if (!(*proplist)) {
        *proplist = prop;
    }

    p = *proplist;
    while (p->next) {
        p = p->next;
    }

    p->next = prop;
    prop->next = NULL;
}

int mosquitto_property_add_byte(mosquitto_property **proplist, int identifier, uint8_t value)
{
    mosquitto_property *prop;

    if (!proplist) {
        return MOSQ_ERR_INVAL;
    }
    if ((identifier != MQTT_PROP_PAYLOAD_FORMAT_INDICATOR)
        && (identifier != MQTT_PROP_REQUEST_PROBLEM_INFORMATION)
        && (identifier != MQTT_PROP_REQUEST_RESPONSE_INFORMATION)
        && (identifier != MQTT_PROP_MAXIMUM_QOS)
        && (identifier != MQTT_PROP_RETAIN_AVAILABLE)
        && (identifier != MQTT_PROP_WILDCARD_SUB_AVAILABLE)
        && (identifier != MQTT_PROP_SUBSCRIPTION_ID_AVAILABLE)
        && (identifier != MQTT_PROP_SHARED_SUB_AVAILABLE))
    {
        return MOSQ_ERR_INVAL;
    }

    prop = (mosquitto_property *)mosquitto__calloc(1, sizeof(mosquitto_property));
    if (!prop) {
        return MOSQ_ERR_NOMEM;
    }

    prop->client_generated = true;
    prop->identifier = identifier;
    prop->value.i8 = value;
    prop->property_type = MQTT_PROP_TYPE_BYTE;

    property__add(proplist, prop);

    return MOSQ_ERR_SUCCESS;
}

int mosquitto_property_add_int16(mosquitto_property **proplist, int identifier, uint16_t value)
{
    mosquitto_property *prop;

    if (!proplist) {
        return MOSQ_ERR_INVAL;
    }

    if ((identifier != MQTT_PROP_SERVER_KEEP_ALIVE)
        && (identifier != MQTT_PROP_RECEIVE_MAXIMUM)
        && (identifier != MQTT_PROP_TOPIC_ALIAS_MAXIMUM)
        && (identifier != MQTT_PROP_TOPIC_ALIAS))
    {
        return MOSQ_ERR_INVAL;
    }

    prop = (mosquitto_property *)mosquitto__calloc(1, sizeof(mosquitto_property));
    if (!prop) {
        return MOSQ_ERR_NOMEM;
    }

    prop->client_generated = true;
    prop->identifier = identifier;
    prop->value.i16 = value;
    prop->property_type = MQTT_PROP_TYPE_INT16;

    property__add(proplist, prop);

    return MOSQ_ERR_SUCCESS;
}

int mosquitto_property_add_int32(mosquitto_property **proplist, int identifier, uint32_t value)
{
    mosquitto_property *prop;

    if (!proplist) {
        return MOSQ_ERR_INVAL;
    }

    if ((identifier != MQTT_PROP_MESSAGE_EXPIRY_INTERVAL)
        && (identifier != MQTT_PROP_SESSION_EXPIRY_INTERVAL)
        && (identifier != MQTT_PROP_WILL_DELAY_INTERVAL)
        && (identifier != MQTT_PROP_MAXIMUM_PACKET_SIZE))
    {
        return MOSQ_ERR_INVAL;
    }

    prop = (mosquitto_property *)mosquitto__calloc(1, sizeof(mosquitto_property));
    if (!prop) {
        return MOSQ_ERR_NOMEM;
    }

    prop->client_generated = true;
    prop->identifier = identifier;
    prop->value.i32 = value;
    prop->property_type = MQTT_PROP_TYPE_INT32;

    property__add(proplist, prop);

    return MOSQ_ERR_SUCCESS;
}

int mosquitto_property_add_varint(mosquitto_property **proplist, int identifier, uint32_t value)
{
    mosquitto_property *prop;

    if (!proplist || (value > 0x0FFFFFFF)) {
        return MOSQ_ERR_INVAL;
    }
    
    if (identifier != MQTT_PROP_SUBSCRIPTION_IDENTIFIER) {
        return MOSQ_ERR_INVAL;
    }

    prop = (mosquitto_property *)mosquitto__calloc(1, sizeof(mosquitto_property));
    if (!prop) {
        return MOSQ_ERR_NOMEM;
    }

    prop->client_generated = true;
    prop->identifier = identifier;
    prop->value.varint = value;
    prop->property_type = MQTT_PROP_TYPE_VARINT;

    property__add(proplist, prop);

    return MOSQ_ERR_SUCCESS;
}

int mosquitto_property_add_binary(mosquitto_property **proplist, int identifier, const void *value, uint16_t len)
{
    mosquitto_property *prop;

    if (!proplist) {
        return MOSQ_ERR_INVAL;
    }

    if ((identifier != MQTT_PROP_CORRELATION_DATA) && (identifier != MQTT_PROP_AUTHENTICATION_DATA)) {
        return MOSQ_ERR_INVAL;
    }

    prop = (mosquitto_property *)mosquitto__calloc(1, sizeof(mosquitto_property));
    if (!prop) {
        return MOSQ_ERR_NOMEM;
    }

    prop->client_generated = true;
    prop->identifier = identifier;
    prop->property_type = MQTT_PROP_TYPE_BINARY;

    if (len) {
        prop->value.bin.v = (char *)mosquitto__malloc(len);
        if (!prop->value.bin.v) {
            mosquitto__FREE(prop);
            return MOSQ_ERR_NOMEM;
        }

        memcpy(prop->value.bin.v, value, len);
        prop->value.bin.len = len;
    }

    property__add(proplist, prop);

    return MOSQ_ERR_SUCCESS;
}

int mosquitto_property_add_string(mosquitto_property **proplist, int identifier, const char *value)
{
    size_t slen = 0;
    mosquitto_property *prop;

    if (!proplist) {
        return MOSQ_ERR_INVAL;
    }

    if (value) {
        slen = strlen(value);
        if (mosquitto_validate_utf8(value, (int)slen)) {
            return MOSQ_ERR_MALFORMED_UTF8;
        }
    }

    if ((identifier != MQTT_PROP_CONTENT_TYPE)
        && (identifier != MQTT_PROP_RESPONSE_TOPIC)
        && (identifier != MQTT_PROP_ASSIGNED_CLIENT_IDENTIFIER)
        && (identifier != MQTT_PROP_AUTHENTICATION_METHOD)
        && (identifier != MQTT_PROP_RESPONSE_INFORMATION)
        && (identifier != MQTT_PROP_SERVER_REFERENCE)
        && (identifier != MQTT_PROP_REASON_STRING))
    {
        return MOSQ_ERR_INVAL;
    }

    prop = (mosquitto_property *)mosquitto__calloc(1, sizeof(mosquitto_property));
    if (!prop) {
        return MOSQ_ERR_NOMEM;
    }

    prop->client_generated = true;
    prop->identifier = identifier;
    prop->property_type = MQTT_PROP_TYPE_STRING;

    if (value && (slen > 0)) {
        prop->value.s.v = mosquitto__strdup(value);
        if (!prop->value.s.v) {
            mosquitto__FREE(prop);
            return MOSQ_ERR_NOMEM;
        }

        prop->value.s.len = (uint16_t)slen;
    }

    property__add(proplist, prop);

    return MOSQ_ERR_SUCCESS;
}

int mosquitto_property_add_string_pair(mosquitto_property **proplist, int identifier, const char *name, const char *value)
{
    mosquitto_property *prop;
    size_t slen_name = 0, slen_value = 0;

    if (!proplist) {
        return MOSQ_ERR_INVAL;
    }

    if (identifier != MQTT_PROP_USER_PROPERTY) {
        return MOSQ_ERR_INVAL;
    }

    if (name) {
        slen_name = strlen(name);
        if (mosquitto_validate_utf8(name, (int)slen_name)) {
            return MOSQ_ERR_MALFORMED_UTF8;
        }
    }

    if (value) {
        if (mosquitto_validate_utf8(value, (int)slen_value)) {
            return MOSQ_ERR_MALFORMED_UTF8;
        }
    }

    prop = (mosquitto_property *)mosquitto__calloc(1, sizeof(mosquitto_property));
    if (!prop) {
        return MOSQ_ERR_NOMEM;
    }

    prop->client_generated = true;
    prop->identifier = identifier;
    prop->property_type = MQTT_PROP_TYPE_STRING_PAIR;

    if (name) {
        prop->name.v = mosquitto__strdup(name);
        if (!prop->name.v) {
            mosquitto__FREE(prop);
            return MOSQ_ERR_NOMEM;
        }

        prop->name.len = (uint16_t)strlen(name);
    }

    if (value) {
        prop->value.s.v = mosquitto__strdup(value);
        if (!prop->value.s.v) {
            mosquitto__FREE(prop->name.v);
            mosquitto__FREE(prop);

            return MOSQ_ERR_NOMEM;
        }

        prop->value.s.len = (uint16_t)strlen(value);
    }

    property__add(proplist, prop);

    return MOSQ_ERR_SUCCESS;
}

int mosquitto_property_check_all(int command, const mosquitto_property *properties)
{
    int rc;
    const mosquitto_property *p, *tail;

    p = properties;
    while (p) {
        // 有效性检查
        if ((p->identifier == MQTT_PROP_REQUEST_PROBLEM_INFORMATION)
            || (p->identifier == MQTT_PROP_PAYLOAD_FORMAT_INDICATOR)
            || (p->identifier == MQTT_PROP_REQUEST_RESPONSE_INFORMATION)
            || (p->identifier == MQTT_PROP_MAXIMUM_QOS)
            || (p->identifier == MQTT_PROP_RETAIN_AVAILABLE)
            || (p->identifier == MQTT_PROP_WILDCARD_SUB_AVAILABLE)
            || (p->identifier == MQTT_PROP_SUBSCRIPTION_ID_AVAILABLE)
            || (p->identifier == MQTT_PROP_SHARED_SUB_AVAILABLE))
        {
            if (p->value.i8 > 1) {
                return MOSQ_ERR_PROTOCOL;
            }
        } else if (p->identifier == MQTT_PROP_MAXIMUM_PACKET_SIZE) {
            if (p->value.i32 == 0) {
                return MOSQ_ERR_PROTOCOL;
            }
        } else if ((p->identifier == MQTT_PROP_RECEIVE_MAXIMUM) || (p->identifier == MQTT_PROP_TOPIC_ALIAS)) {
            if (p->value.i16 == 0) {
                return MOSQ_ERR_PROTOCOL;
            }
        }

        // 检查错误命令的属性
        rc = mosquitto_property_check_command(command, p->identifier);
        if (rc) {
            return rc;
        }

        // 检查重复项
        if (p->identifier != MQTT_PROP_USER_PROPERTY) {
            tail = p->next;
            while (tail) {
                if (p->identifier == tail->identifier) {
                    return MOSQ_ERR_DUPLICATE_PROPERTY;
                }

                tail = tail->next;
            }
        }

        p = p->next;
    }

    return MOSQ_ERR_SUCCESS;
}

static const mosquitto_property *property__get_property(const mosquitto_property *proplist, int identifier, bool skip_first)
{
    bool is_first = true;
    const mosquitto_property *p;

    p = proplist;

    while (p) {
        if (p->identifier == identifier){
            if (!is_first || !skip_first) {
                return p;
            }

            is_first = false;
        }
        p = p->next;
    }

    return NULL;
}

int mosquitto_property_identifier(const mosquitto_property *property)
{
    if (property == NULL) {
        return 0;
    }

    return property->identifier;
}

int mosquitto_property_type(const mosquitto_property *property)
{
    if (property == NULL) {
        return 0;
    }

    return property->property_type;
}

mosquitto_property *mosquitto_property_next(const mosquitto_property *proplist)
{
    if (proplist == NULL) {
        return NULL;
    }

    return proplist->next;
}

const mosquitto_property *mosquitto_property_read_byte(const mosquitto_property *proplist, int identifier, uint8_t *value, bool skip_first)
{
    const mosquitto_property *p;

    if (!proplist) {
        return NULL;
    }

    p = property__get_property(proplist, identifier, skip_first);
    if (!p) {
        return NULL;
    }

    if ((p->identifier != MQTT_PROP_PAYLOAD_FORMAT_INDICATOR)
        && (p->identifier != MQTT_PROP_REQUEST_PROBLEM_INFORMATION)
        && (p->identifier != MQTT_PROP_REQUEST_RESPONSE_INFORMATION)
        && (p->identifier != MQTT_PROP_MAXIMUM_QOS)
        && (p->identifier != MQTT_PROP_RETAIN_AVAILABLE)
        && (p->identifier != MQTT_PROP_WILDCARD_SUB_AVAILABLE)
        && (p->identifier != MQTT_PROP_SUBSCRIPTION_ID_AVAILABLE)
        && (p->identifier != MQTT_PROP_SHARED_SUB_AVAILABLE))
    {
        return NULL;
    }

    if (value) {
        *value = p->value.i8;
    }

    return p;
}

const mosquitto_property *mosquitto_property_read_int16(const mosquitto_property *proplist, int identifier, uint16_t *value, bool skip_first)
{
    const mosquitto_property *p;
    if (!proplist) {
        return NULL;
    }

    p = property__get_property(proplist, identifier, skip_first);
    if (!p) {
        return NULL;
    }

    if ((p->identifier != MQTT_PROP_SERVER_KEEP_ALIVE)
        && (p->identifier != MQTT_PROP_RECEIVE_MAXIMUM)
        && (p->identifier != MQTT_PROP_TOPIC_ALIAS_MAXIMUM)
        && (p->identifier != MQTT_PROP_TOPIC_ALIAS))
    {
        return NULL;
    }

    if (value) {
        *value = p->value.i16;
    }

    return p;
}

const mosquitto_property *mosquitto_property_read_int32(const mosquitto_property *proplist, int identifier, uint32_t *value, bool skip_first)
{
    const mosquitto_property *p;
    if (!proplist) {
        return NULL;
    }

    p = property__get_property(proplist, identifier, skip_first);
    if (!p) {
        return NULL;
    }

    if ((p->identifier != MQTT_PROP_MESSAGE_EXPIRY_INTERVAL)
        && (p->identifier != MQTT_PROP_SESSION_EXPIRY_INTERVAL)
        && (p->identifier != MQTT_PROP_WILL_DELAY_INTERVAL)
        && (p->identifier != MQTT_PROP_MAXIMUM_PACKET_SIZE))
    {
        return NULL;
    }

    if (value) {
        *value = p->value.i32;
    }

    return p;
}

const mosquitto_property *mosquitto_property_read_varint(const mosquitto_property *proplist, int identifier, uint32_t *value, bool skip_first)
{
    const mosquitto_property *p;

    if (!proplist) {
        return NULL;
    }

    p = property__get_property(proplist, identifier, skip_first);
    if (!p) {
        return NULL;
    }

    if (p->identifier != MQTT_PROP_SUBSCRIPTION_IDENTIFIER) {
        return NULL;
    }

    if (value) {
        *value = p->value.varint;
    }

    return p;
}

const mosquitto_property *mosquitto_property_read_binary(const mosquitto_property *proplist, int identifier, void **value, uint16_t *len, bool skip_first)
{
    const mosquitto_property *p;

    if (!proplist || (value && !len) || (!value && len)) {
        return NULL;
    }

    if (value) {
        *value = NULL;
    }

    p = property__get_property(proplist, identifier, skip_first);
    if (!p) {
        return NULL;
    }

    if ((p->identifier != MQTT_PROP_CORRELATION_DATA) && (p->identifier != MQTT_PROP_AUTHENTICATION_DATA)) {
        return NULL;
    }

    if (value) {
        *len = p->value.bin.len;
        if (p->value.bin.len) {
            *value = calloc(1, *len + 1U);
            if (!(*value)) {
                return NULL;
            }

            memcpy(*value, p->value.bin.v, *len);
        } else {
            *value = NULL;
        }
    }

    return p;
}

const mosquitto_property *mosquitto_property_read_string(const mosquitto_property *proplist, int identifier, char **value, bool skip_first)
{
    const mosquitto_property *p;

    if (!proplist) {
        return NULL;
    }

    p = property__get_property(proplist, identifier, skip_first);
    if (!p) {
        return NULL;
    }

    if ((p->identifier != MQTT_PROP_CONTENT_TYPE)
        && (p->identifier != MQTT_PROP_RESPONSE_TOPIC)
        && (p->identifier != MQTT_PROP_ASSIGNED_CLIENT_IDENTIFIER)
        && (p->identifier != MQTT_PROP_AUTHENTICATION_METHOD)
        && (p->identifier != MQTT_PROP_RESPONSE_INFORMATION)
        && (p->identifier != MQTT_PROP_SERVER_REFERENCE)
        && (p->identifier != MQTT_PROP_REASON_STRING))
    {
        return NULL;
    }

    if (value) {
        if (p->value.s.len) {
            *value = calloc(1, (size_t)p->value.s.len+1);
            if (!(*value)) {
                return NULL;
            }

            memcpy(*value, p->value.s.v, p->value.s.len);
        } else {
            *value = NULL;
        }
    }

    return p;
}

const mosquitto_property *mosquitto_property_read_string_pair(const mosquitto_property *proplist, int identifier, char **name, char **value, bool skip_first)
{
    const mosquitto_property *p;
    if (!proplist) {
        return NULL;
    }

    if (name) {
        *name = NULL;
    }

    if (value) {
        *value = NULL;
    }

    p = property__get_property(proplist, identifier, skip_first);
    if (!p) {
        return NULL;
    }

    if (p->identifier != MQTT_PROP_USER_PROPERTY) {
        return NULL;
    }

    if (name) {
        if (p->name.len) {
            *name = calloc(1, (size_t)p->name.len+1);
            if (!(*name)) {
                return NULL;
            }

            memcpy(*name, p->name.v, p->name.len);
        } else {
            *name = NULL;
        }
    }

    if (value) {
        if (p->value.s.len) {
            *value = calloc(1, (size_t)p->value.s.len+1);
            if (!(*value)) {
                if (name) {
                    SAFE_FREE(*name);
                }

                return NULL;
            }

            memcpy(*value, p->value.s.v, p->value.s.len);
        } else {
            *value = NULL;
        }
    }

    return p;
}

int mosquitto_property_remove(mosquitto_property **proplist, const mosquitto_property *property)
{
    mosquitto_property *item, *item_prev = NULL;

    if ((proplist == NULL) || (property == NULL)) {
        return MOSQ_ERR_INVAL;
    }

    item = *proplist;
    while (item) {
        if (item == property) {
            if (item_prev == NULL)  {
                *proplist = (*proplist)->next;
            } else{
                item_prev->next = item->next;
            }

            item->next = NULL;
            return MOSQ_ERR_SUCCESS;
        }

        item_prev = item;
        item = item->next;
    }

    return MOSQ_ERR_NOT_FOUND;
}

int mosquitto_property_copy_all(mosquitto_property **dest, const mosquitto_property *src)
{
    mosquitto_property *pnew, *plast = NULL;

    if (!src) {
        return MOSQ_ERR_SUCCESS;
    }

    if (!dest) {
        return MOSQ_ERR_INVAL;
    }

    *dest = NULL;

    while (src) {
        pnew = (mosquitto_property *)calloc(1, sizeof(mosquitto_property));
        if (!pnew) {
            mosquitto_property_free_all(dest);
            return MOSQ_ERR_NOMEM;
        }

        if (plast) {
            plast->next = pnew;
        } else {
            *dest = pnew;
        }
        plast = pnew;

        pnew->client_generated = src->client_generated;
        pnew->identifier = src->identifier;
        pnew->property_type = src->property_type;
        switch (pnew->property_type) {
            case MQTT_PROP_TYPE_BYTE:
                pnew->value.i8 = src->value.i8;
                break;

            case MQTT_PROP_TYPE_INT16:
                pnew->value.i16 = src->value.i16;
                break;

            case MQTT_PROP_TYPE_INT32:
                pnew->value.i32 = src->value.i32;
                break;

            case MQTT_PROP_TYPE_VARINT:
                pnew->value.varint = src->value.varint;
                break;

            case MQTT_PROP_TYPE_STRING:
                pnew->value.s.len = src->value.s.len;
                if (src->value.s.v) {
                    pnew->value.s.v = strdup(src->value.s.v);
                    if (!pnew->value.s.v) {
                        mosquitto_property_free_all(dest);
                        return MOSQ_ERR_NOMEM;
                    }
                }
                break;

            case MQTT_PROP_TYPE_BINARY:
                pnew->value.bin.len = src->value.bin.len;
                if (src->value.bin.len) {
                    pnew->value.bin.v = malloc(pnew->value.bin.len);
                    if (!pnew->value.bin.v) {
                        mosquitto_property_free_all(dest);
                        return MOSQ_ERR_NOMEM;
                    }

                    memcpy(pnew->value.bin.v, src->value.bin.v, pnew->value.bin.len);
                }
                break;

            case MQTT_PROP_TYPE_STRING_PAIR:
                pnew->value.s.len = src->value.s.len;
                if (src->value.s.v) {
                    pnew->value.s.v = strdup(src->value.s.v);
                    if (!pnew->value.s.v) {
                        mosquitto_property_free_all(dest);
                        return MOSQ_ERR_NOMEM;
                    }
                }

                pnew->name.len = src->name.len;
                if (src->name.v) {
                    pnew->name.v = strdup(src->name.v);
                    if (!pnew->name.v) {
                        mosquitto_property_free_all(dest);
                        return MOSQ_ERR_NOMEM;
                    }
                }
                break;

            default:
                mosquitto_property_free_all(dest);
                return MOSQ_ERR_INVAL;
        }

        src = mosquitto_property_next(src);
    }

    return MOSQ_ERR_SUCCESS;
}

uint8_t mosquitto_property_byte_value(const mosquitto_property *property)
{
    if (property && (property->property_type == MQTT_PROP_TYPE_BYTE)) {
        return property->value.i8;
    } else {
        return 0;
    }
}

uint16_t mosquitto_property_int16_value(const mosquitto_property *property)
{
    if (property && (property->property_type == MQTT_PROP_TYPE_INT16)) {
        return property->value.i16;
    } else {
        return 0;
    }
}

uint32_t mosquitto_property_int32_value(const mosquitto_property *property)
{
    if (property && (property->property_type == MQTT_PROP_TYPE_INT32)) {
        return property->value.i32;
    } else {
        return 0;
    }
}

uint32_t mosquitto_property_varint_value(const mosquitto_property *property)
{
    if (property && (property->property_type == MQTT_PROP_TYPE_VARINT)) {
        return property->value.varint;
    } else {
        return 0;
    }
}

const void *mosquitto_property_binary_value(const mosquitto_property *property)
{
    if (property && (property->property_type == MQTT_PROP_TYPE_BINARY)) {
        return property->value.bin.v;
    } else {
        return NULL;
    }
}

uint16_t mosquitto_property_binary_value_length(const mosquitto_property *property)
{
    if (property && (property->property_type == MQTT_PROP_TYPE_BINARY)) {
        return property->value.bin.len;
    } else {
        return 0;
    }
}

const char *mosquitto_property_string_value(const mosquitto_property *property)
{
    if (property && ((property->property_type == MQTT_PROP_TYPE_STRING) || (property->property_type == MQTT_PROP_TYPE_STRING_PAIR))) {
        return property->value.s.v;
    } else {
        return NULL;
    }
}

uint16_t mosquitto_property_string_value_length(const mosquitto_property *property)
{
    if (property && ((property->property_type == MQTT_PROP_TYPE_STRING) || (property->property_type == MQTT_PROP_TYPE_STRING_PAIR))) {
        return property->value.s.len;
    } else {
        return 0;
    }
}

const char *mosquitto_property_string_name(const mosquitto_property *property)
{
    if (property && (property->property_type == MQTT_PROP_TYPE_STRING_PAIR)) {
        return property->name.v;
    } else {
        return NULL;
    }
}

uint16_t mosquitto_property_string_name_length(const mosquitto_property *property)
{
    if (property && (property->property_type == MQTT_PROP_TYPE_STRING_PAIR)) {
        return property->name.len;
    } else {
        return 0;
    }
}