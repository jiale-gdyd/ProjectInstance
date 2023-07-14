#ifndef MOSQ_PROPERTY_MOSQ_H
#define MOSQ_PROPERTY_MOSQ_H

#include <mosquitto/mosquitto.h>
#include "mosquitto_internal.h"

struct mqtt__string {
    char     *v;
    uint16_t len;
};

struct mqtt5__property {
    struct mqtt5__property  *next;
    union {
        uint8_t             i8;
        uint16_t            i16;
        uint32_t            i32;
        uint32_t            varint;
        struct mqtt__string bin;
        struct mqtt__string s;
    } value;
    struct mqtt__string     name;
    int32_t                 identifier;
    uint8_t                 property_type;
    bool                    client_generated;
};

void property__free(mosquitto_property **property);

int property__read_all(int command, struct mosquitto__packet_in *packet, mosquitto_property **property);
int property__write_all(struct mosquitto__packet *packet, const mosquitto_property *property, bool write_len);

unsigned int property__get_length(const mosquitto_property *property);
unsigned int property__get_length_all(const mosquitto_property *property);
unsigned int property__get_remaining_length(const mosquitto_property *props);

#endif
