#ifndef MOSQ_ALIAS_MOSQ_H
#define MOSQ_ALIAS_MOSQ_H

#include <stdint.h>
#include "mosquitto_internal.h"

void alias__free_all(struct mosquitto *mosq);
int alias__add_r2l(struct mosquitto *mosq, const char *topic, uint16_t alias);
int alias__add_l2r(struct mosquitto *mosq, const char *topic, uint16_t *alias);
int alias__find_by_alias(struct mosquitto *mosq, int direction, uint16_t alias, char **topic);
int alias__find_by_topic(struct mosquitto *mosq, int direction, const char *topic, uint16_t *alias);

#endif
