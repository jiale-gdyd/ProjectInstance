#ifndef MOSQ_WILL_MOSQ_H
#define MOSQ_WILL_MOSQ_H

#include <mosquitto/mosquitto.h>
#include "mosquitto_internal.h"

int will__clear(struct mosquitto *mosq);
int will__set(struct mosquitto *mosq, const char *topic, int payloadlen, const void *payload, int qos, bool retain, mosquitto_property *properties);

#endif
