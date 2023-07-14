#include <errno.h>
#include <string.h>
#include <assert.h>
#include <arpa/inet.h>
#include <mosquitto/mqtt_protocol.h>

#include "config.h"
#include "net_mosq.h"
#include "memory_mosq.h"
#include "packet_mosq.h"
#include "read_handle.h"

int packet__read_byte(struct mosquitto__packet_in *packet, uint8_t *byte)
{
    assert(packet);

    if ((packet->pos + 1) > packet->remaining_length) {
        return MOSQ_ERR_MALFORMED_PACKET;
    }

    *byte = packet->payload[packet->pos];
    packet->pos++;

    return MOSQ_ERR_SUCCESS;
}

void packet__write_byte(struct mosquitto__packet *packet, uint8_t byte)
{
    assert(packet);
    assert((packet->pos + 1) <= packet->packet_length);

    packet->payload[packet->pos] = byte;
    packet->pos++;
}

int packet__read_bytes(struct mosquitto__packet_in *packet, void *bytes, uint32_t count)
{
    assert(packet);

    if ((packet->pos + count) > packet->remaining_length) {
        return MOSQ_ERR_MALFORMED_PACKET;
    }

    memcpy(bytes, &(packet->payload[packet->pos]), count);
    packet->pos += count;

    return MOSQ_ERR_SUCCESS;
}

void packet__write_bytes(struct mosquitto__packet *packet, const void *bytes, uint32_t count)
{
    assert(packet);
    assert((packet->pos + count) <= packet->packet_length);

    memcpy(&(packet->payload[packet->pos]), bytes, count);
    packet->pos += count;
}

int packet__read_binary(struct mosquitto__packet_in *packet, uint8_t **data, uint16_t *length)
{
    int rc;
    uint16_t slen;

    assert(packet);

    rc = packet__read_uint16(packet, &slen);
    if (rc) {
        return rc;
    }

    if (slen == 0) {
        *data = NULL;
        *length = 0;

            return MOSQ_ERR_SUCCESS;
    }

    if ((packet->pos + slen) > packet->remaining_length) {
        return MOSQ_ERR_MALFORMED_PACKET;
    }

    *data = (uint8_t *)mosquitto__malloc(slen + 1U);
    if (*data) {
        memcpy(*data, &(packet->payload[packet->pos]), slen);
        ((uint8_t *)(*data))[slen] = '\0';
        packet->pos += slen;
    } else {
        return MOSQ_ERR_NOMEM;
    }
    *length = slen;

    return MOSQ_ERR_SUCCESS;
}

int packet__read_string(struct mosquitto__packet_in *packet, char **str, uint16_t *length)
{
    int rc;

    rc = packet__read_binary(packet, (uint8_t **)str, length);
    if (rc) {
        return rc;
    }

    if (*length == 0) {
        return MOSQ_ERR_SUCCESS;
    }

    if (mosquitto_validate_utf8(*str, *length)) {
        mosquitto__FREE(*str);
        *length = 0;

        return MOSQ_ERR_MALFORMED_UTF8;
    }

    return MOSQ_ERR_SUCCESS;
}

void packet__write_string(struct mosquitto__packet *packet, const char *str, uint16_t length)
{
    assert(packet);

    packet__write_uint16(packet, length);
    packet__write_bytes(packet, str, length);
}

int packet__read_uint16(struct mosquitto__packet_in *packet, uint16_t *word)
{
    uint16_t val;

    assert(packet);

    if ((packet->pos + 2) > packet->remaining_length) {
        return MOSQ_ERR_MALFORMED_PACKET;
    }

    memcpy(&val, &packet->payload[packet->pos], sizeof(uint16_t));
    packet->pos += sizeof(uint16_t);

    *word = ntohs(val);

    return MOSQ_ERR_SUCCESS;
}

void packet__write_uint16(struct mosquitto__packet *packet, uint16_t word)
{
    uint16_t bigendian = htons(word);

    assert(packet);
    assert((packet->pos + 2) <= packet->packet_length);

    memcpy(&packet->payload[packet->pos], &bigendian, 2);
    packet->pos += 2;
}

int packet__read_uint32(struct mosquitto__packet_in *packet, uint32_t *word)
{
    uint32_t val = 0;

    assert(packet);

    if ((packet->pos + 4) > packet->remaining_length) {
        return MOSQ_ERR_MALFORMED_PACKET;
    }

    memcpy(&val, &packet->payload[packet->pos], sizeof(uint32_t));
    packet->pos += sizeof(uint32_t);
    *word = ntohl(val);

    return MOSQ_ERR_SUCCESS;
}

void packet__write_uint32(struct mosquitto__packet *packet, uint32_t word)
{
    uint32_t bigendian = htonl(word);

    assert(packet);
    assert((packet->pos + 4) <= packet->packet_length);

    memcpy(&packet->payload[packet->pos], &bigendian, 4);
    packet->pos += 4;
}

int packet__read_varint(struct mosquitto__packet_in *packet, uint32_t *word, uint8_t *bytes)
{
    int i;
    uint8_t byte;
    uint32_t lword = 0;
    uint8_t lbytes = 0;
    unsigned int remaining_mult = 1;

    for (i = 0; i < 4; i++) {
        if (packet->pos < packet->remaining_length) {
            lbytes++;
            byte = packet->payload[packet->pos];
            lword += (byte & 0x7F) * remaining_mult;
            remaining_mult *= 0x80;
            packet->pos++;

            if ((byte & 0x80) == 0) {
                if ((lbytes > 1) && (byte == 0)) {
                    // 捕获超长编码
                    return MOSQ_ERR_MALFORMED_PACKET;
                } else {
                    *word = lword;
                    if (bytes) {
                        (*bytes) = lbytes;
                    }

                    return MOSQ_ERR_SUCCESS;
                }
            }
        } else {
            return MOSQ_ERR_MALFORMED_PACKET;
        }
    }

    return MOSQ_ERR_MALFORMED_PACKET;
}

int packet__write_varint(struct mosquitto__packet *packet, uint32_t word)
{
    uint8_t byte;
    int count = 0;

    do {
        byte = (uint8_t)(word % 0x80);
        word = word / 0x80;

        // 如果还有更多数字要编码，请设置该数字的高位
        if (word > 0) {
            byte = byte | 0x80;
        }

        packet__write_byte(packet, byte);
        count++;
    } while ((word > 0) && (count < 5));

    if (count == 5) {
        return MOSQ_ERR_MALFORMED_PACKET;
    }

    return MOSQ_ERR_SUCCESS;
}

unsigned int packet__varint_bytes(uint32_t word)
{
    if (word < 0x80) {
        return 1;
    } else if (word < 0x4000) {
        return 2;
    } else if (word < 0x200000) {
        return 3;
    } else if (word < 0x10000000) {
        return 4;
    } else {
        return 5;
    }
}
