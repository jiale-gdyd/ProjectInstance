#ifndef MOSQ_MESSAGES_MOSQ_H
#define MOSQ_MESSAGES_MOSQ_H

#include <mosquitto/mosquitto.h>
#include "mosquitto_internal.h"

void message__cleanup_all(struct mosquitto *mosq);
void message__cleanup(struct mosquitto_message_all **message);

int message__delete(struct mosquitto *mosq, uint16_t mid, enum mosquitto_msg_direction dir, int qos);
int message__queue(struct mosquitto *mosq, struct mosquitto_message_all *message, enum mosquitto_msg_direction dir);

void message__reconnect_reset(struct mosquitto *mosq, bool update_quota_only);
int message__release_to_inflight(struct mosquitto *mosq, enum mosquitto_msg_direction dir);

int message__remove(struct mosquitto *mosq, uint16_t mid, enum mosquitto_msg_direction dir, struct mosquitto_message_all **message, int qos);

void message__retry_check(struct mosquitto *mosq);

int message__out_update(struct mosquitto *mosq, uint16_t mid, enum mosquitto_msg_state state, int qos);

#endif
