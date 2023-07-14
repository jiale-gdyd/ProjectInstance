#ifndef MOSQ_PACKET_MOSQ_H
#define MOSQ_PACKET_MOSQ_H

#include <mosquitto/mosquitto.h>
#include "mosquitto_internal.h"

int packet__alloc(struct mosquitto__packet **packet, uint8_t command, uint32_t remaining_length);

void packet__cleanup_all(struct mosquitto *mosq);
void packet__cleanup(struct mosquitto__packet_in *packet);
void packet__cleanup_all_no_locks(struct mosquitto *mosq);

int packet__queue(struct mosquitto *mosq, struct mosquitto__packet *packet);
int packet__check_oversize(struct mosquitto *mosq, uint32_t remaining_length);

int packet__read_byte(struct mosquitto__packet_in *packet, uint8_t *byte);
int packet__read_bytes(struct mosquitto__packet_in *packet, void *bytes, uint32_t count);

int packet__read_uint16(struct mosquitto__packet_in *packet, uint16_t *word);
int packet__read_uint32(struct mosquitto__packet_in *packet, uint32_t *word);
int packet__read_string(struct mosquitto__packet_in *packet, char **str, uint16_t *length);
int packet__read_varint(struct mosquitto__packet_in *packet, uint32_t *word, uint8_t *bytes);
int packet__read_binary(struct mosquitto__packet_in *packet, uint8_t **data, uint16_t *length);

void packet__write_byte(struct mosquitto__packet *packet, uint8_t byte);
void packet__write_bytes(struct mosquitto__packet *packet, const void *bytes, uint32_t count);

int packet__write_varint(struct mosquitto__packet *packet, uint32_t word);
void packet__write_uint16(struct mosquitto__packet *packet, uint16_t word);
void packet__write_uint32(struct mosquitto__packet *packet, uint32_t word);
void packet__write_string(struct mosquitto__packet *packet, const char *str, uint16_t length);

int packet__read(struct mosquitto *mosq);
int packet__write(struct mosquitto *mosq);

unsigned int packet__varint_bytes(uint32_t word);

#endif
