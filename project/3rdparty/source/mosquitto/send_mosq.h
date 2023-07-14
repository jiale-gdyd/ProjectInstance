#ifndef MOSQ_SEND_MOSQ_H
#define MOSQ_SEND_MOSQ_H

#include <mosquitto/mosquitto.h>
#include "property_mosq.h"

int send__simple_command(struct mosquitto *mosq, uint8_t command);
int send__command_with_mid(struct mosquitto *mosq, uint8_t command, uint16_t mid, bool dup, uint8_t reason_code, const mosquitto_property *properties);

int send__real_publish(struct mosquitto *mosq, uint16_t mid, const char *topic, uint32_t payloadlen, const void *payload, uint8_t qos, bool retain, bool dup, uint32_t subscription_identifier, const mosquitto_property *store_props, uint32_t expiry_interval);

int send__disconnect(struct mosquitto *mosq, uint8_t reason_code, const mosquitto_property *properties);
int send__connect(struct mosquitto *mosq, uint16_t keepalive, bool clean_session, const mosquitto_property *properties);

int send__pingreq(struct mosquitto *mosq);
int send__pingresp(struct mosquitto *mosq);

int send__pubcomp(struct mosquitto *mosq, uint16_t mid, const mosquitto_property *properties);
int send__puback(struct mosquitto *mosq, uint16_t mid, uint8_t reason_code, const mosquitto_property *properties);

int send__pubrel(struct mosquitto *mosq, uint16_t mid, const mosquitto_property *properties);
int send__pubrec(struct mosquitto *mosq, uint16_t mid, uint8_t reason_code, const mosquitto_property *properties);

int send__publish(struct mosquitto *mosq, uint16_t mid, const char *topic, uint32_t payloadlen, const void *payload, uint8_t qos, bool retain, bool dup, uint32_t subscription_identifier, const mosquitto_property *store_props, uint32_t expiry_interval);

int send__unsubscribe(struct mosquitto *mosq, int *mid, int topic_count, char *const *const topic, const mosquitto_property *properties);
int send__subscribe(struct mosquitto *mosq, int *mid, int topic_count, char *const *const topic, int topic_qos, const mosquitto_property *properties);

#endif
