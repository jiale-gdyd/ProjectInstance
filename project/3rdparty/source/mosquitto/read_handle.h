#ifndef MOSQ_READ_HANDLE_H
#define MOSQ_READ_HANDLE_H

#include <mosquitto/mosquitto.h>

struct mosquitto_db;

int handle__pingreq(struct mosquitto *mosq);
int handle__pingresp(struct mosquitto *mosq);

int handle__auth(struct mosquitto *mosq);
int handle__packet(struct mosquitto *mosq);

int handle__connack(struct mosquitto *mosq);
int handle__disconnect(struct mosquitto *mosq);

int handle__publish(struct mosquitto *mosq);
int handle__pubackcomp(struct mosquitto *mosq, const char *type);

int handle__pubrec(struct mosquitto *mosq);
int handle__pubrel(struct mosquitto *mosq);
int handle__suback(struct mosquitto *mosq);
int handle__unsuback(struct mosquitto *mosq);

#endif
