#ifndef MOSQ_UTIL_MOSQ_H
#define MOSQ_UTIL_MOSQ_H

#include <stdio.h>
#include <mosquitto/mosquitto.h>

#include "config.h"
#include "tls_mosq.h"
#include "mosquitto_internal.h"

int mosquitto__check_keepalive(struct mosquitto *mosq);
uint16_t mosquitto__mid_generate(struct mosquitto *mosq);

enum mosquitto_client_state mosquitto__get_state(struct mosquitto *mosq);
int mosquitto__set_state(struct mosquitto *mosq, enum mosquitto_client_state state);

void mosquitto__set_request_disconnect(struct mosquitto *mosq, bool request_disconnect);
bool mosquitto__get_request_disconnect(struct mosquitto *mosq);

#ifdef WITH_TLS
int mosquitto__hex2bin_sha1(const char *hex, unsigned char **bin);
int mosquitto__hex2bin(const char *hex, unsigned char *bin, int bin_max_len);
#endif

int util__random_bytes(void *bytes, int count);

void util__increment_send_quota(struct mosquitto *mosq);
void util__decrement_send_quota(struct mosquitto *mosq);

void util__increment_receive_quota(struct mosquitto *mosq);
void util__decrement_receive_quota(struct mosquitto *mosq);

#endif
