#ifndef MOSQ_CALLBACKS_H
#define MOSQ_CALLBACKS_H

#include <mosquitto/mosquitto.h>

void callback__on_pre_connect(struct mosquitto *mosq);
void callback__on_disconnect(struct mosquitto *mosq, int rc, const mosquitto_property *props);
void callback__on_connect(struct mosquitto *mosq, uint8_t reason_code, uint8_t connect_flags, const mosquitto_property *properties);

void callback__on_publish(struct mosquitto *mosq, int mid, int reason_code, const mosquitto_property *properties);
void callback__on_message(struct mosquitto *mosq, const struct mosquitto_message *message, const mosquitto_property *properties);

void callback__on_unsubscribe(struct mosquitto *mosq, int mid, int reason_code_count, const int *reason_codes, const mosquitto_property *props);
void callback__on_subscribe(struct mosquitto *mosq, int mid, int qos_count, const int *granted_qos, const mosquitto_property *props);

#endif
