#include <errno.h>
#include <string.h>
#include <assert.h>
#include <mosquitto/mqtt_protocol.h>

#include "config.h"
#include "net_mosq.h"
#include "callbacks.h"
#include "util_mosq.h"
#include "packet_mosq.h"
#include "read_handle.h"
#include "memory_mosq.h"

#define metrics__int_inc(stat, val)
#define metrics__int_dec(stat, val)

int packet__alloc(struct mosquitto__packet **packet, uint8_t command, uint32_t remaining_length)
{
    int i;
    int8_t remaining_count;
    uint32_t packet_length;
    uint32_t remaining_length_stored;
    uint8_t remaining_bytes[5], byte;

    assert(packet);

    remaining_length_stored = remaining_length;
    remaining_count = 0;

    do {
        byte = remaining_length % 0x80;
        remaining_length = remaining_length / 0x80;

        // 如果还有更多数字要编码，请设置该数字的高位
        if (remaining_length > 0) {
            byte = byte | 0x80;
        }

        remaining_bytes[remaining_count] = byte;
        remaining_count++;
    } while ((remaining_length > 0) && (remaining_count < 5));

    if (remaining_count == 5) {
        return MOSQ_ERR_PAYLOAD_SIZE;
    }

    packet_length = remaining_length_stored + 1 + (uint8_t)remaining_count;
    (*packet) = (struct mosquitto__packet *)mosquitto__malloc(sizeof(struct mosquitto__packet) + packet_length + WS_PACKET_OFFSET);
    if ((*packet) == NULL) {
        return MOSQ_ERR_NOMEM;
    }

    memset((*packet), 0x00, sizeof(struct mosquitto__packet));
    (*packet)->command = command;
    (*packet)->remaining_length = remaining_length_stored;
    (*packet)->remaining_count = remaining_count;
    (*packet)->packet_length = packet_length + WS_PACKET_OFFSET;

    (*packet)->payload[WS_PACKET_OFFSET] = (*packet)->command;
    for (i = 0; i < (*packet)->remaining_count; i++) {
        (*packet)->payload[WS_PACKET_OFFSET + i + 1] = remaining_bytes[i];
    }
    (*packet)->pos = WS_PACKET_OFFSET + 1U + (uint8_t)(*packet)->remaining_count;

    return MOSQ_ERR_SUCCESS;
}

void packet__cleanup(struct mosquitto__packet_in *packet)
{
    if (!packet) {
        return;
    }

    // 释放数据和重置值
    packet->command = 0;
    packet->remaining_count = 0;
    packet->remaining_mult = 1;
    packet->remaining_length = 0;
    mosquitto__FREE(packet->payload);
    packet->to_process = 0;
    packet->pos = 0;
}

void packet__cleanup_all_no_locks(struct mosquitto *mosq)
{
    struct mosquitto__packet *packet;

    while (mosq->out_packet) {
        packet = mosq->out_packet;
        mosq->out_packet = mosq->out_packet->next;

        mosquitto__FREE(packet);
    }

    metrics__int_dec(mosq_gauge_out_packets, mosq->out_packet_count);
    metrics__int_dec(mosq_gauge_out_packet_bytes, mosq->out_packet_bytes);

    mosq->out_packet_count = 0;
    mosq->out_packet_bytes = 0;
    mosq->out_packet_last = NULL;

    packet__cleanup(&mosq->in_packet);
}

void packet__cleanup_all(struct mosquitto *mosq)
{
    pthread_mutex_lock(&mosq->out_packet_mutex);
    packet__cleanup_all_no_locks(mosq);
    pthread_mutex_unlock(&mosq->out_packet_mutex);
}

static void packet__queue_append(struct mosquitto *mosq, struct mosquitto__packet *packet)
{
    pthread_mutex_lock(&mosq->out_packet_mutex);
    if (mosq->out_packet) {
        mosq->out_packet_last->next = packet;
    } else {
        mosq->out_packet = packet;
    }

    mosq->out_packet_last = packet;
    mosq->out_packet_count++;
    mosq->out_packet_bytes += packet->packet_length;

    metrics__int_inc(mosq_gauge_out_packets, 1);
    metrics__int_inc(mosq_gauge_out_packet_bytes, packet->packet_length);
    pthread_mutex_unlock(&mosq->out_packet_mutex);
}

int packet__queue(struct mosquitto *mosq, struct mosquitto__packet *packet)
{
    char sockpair_data = 0;

    assert(mosq);
    assert(packet);

    {
        packet->next = NULL;
        packet->pos = WS_PACKET_OFFSET;
        packet->to_process = packet->packet_length - WS_PACKET_OFFSET;
    }

    packet__queue_append(mosq, packet);

    // 如果在线程模式下，将一个字节写入sockpairW(连接到sockpairR)以脱离select()
    if (mosq->sockpairW != INVALID_SOCKET) {
        if (write(mosq->sockpairW, &sockpair_data, 1)) {

        }
    }

    if ((mosq->callback_depth == 0) && (mosq->threaded == mosq_ts_none)) {
        return packet__write(mosq);
    } else {
        return MOSQ_ERR_SUCCESS;
    }
}

int packet__check_oversize(struct mosquitto *mosq, uint32_t remaining_length)
{
    uint32_t len;

    if (mosq->maximum_packet_size == 0) {
        return MOSQ_ERR_SUCCESS;
    }

    len = remaining_length + packet__varint_bytes(remaining_length);
    if (len > mosq->maximum_packet_size) {
        return MOSQ_ERR_OVERSIZE_PACKET;
    } else {
        return MOSQ_ERR_SUCCESS;
    }
}

static struct mosquitto__packet *packet__get_next_out(struct mosquitto *mosq)
{
    struct mosquitto__packet *packet = NULL;

    pthread_mutex_lock(&mosq->out_packet_mutex);
    if (mosq->out_packet) {
        mosq->out_packet_count--;
        mosq->out_packet_bytes -= mosq->out_packet->packet_length;

        metrics__int_dec(mosq_gauge_out_packets, 1);
        metrics__int_dec(mosq_gauge_out_packet_bytes, mosq->out_packet->packet_length);

        mosq->out_packet = mosq->out_packet->next;
        if (!mosq->out_packet) {
            mosq->out_packet_last = NULL;
        }

        packet = mosq->out_packet;
    }
    pthread_mutex_unlock(&mosq->out_packet_mutex);

    return packet;
}

int packet__write(struct mosquitto *mosq)
{
    ssize_t write_length;
    enum mosquitto_client_state state;
    struct mosquitto__packet *packet, *next_packet;

    if (!mosq) {
        return MOSQ_ERR_INVAL;
    }

    if (!net__is_connected(mosq)) {
        return MOSQ_ERR_NO_CONN;
    }

    pthread_mutex_lock(&mosq->out_packet_mutex);
    packet = mosq->out_packet;
    pthread_mutex_unlock(&mosq->out_packet_mutex);

    if (packet == NULL) {
        return MOSQ_ERR_SUCCESS;
    }

    state = mosquitto__get_state(mosq);
    if (state == mosq_cs_connect_pending) {
        return MOSQ_ERR_SUCCESS;
    }

    while (packet) {
        while (packet->to_process > 0) {
            write_length = net__write(mosq, &(packet->payload[packet->pos]), packet->to_process);
            if (write_length > 0) {
                metrics__int_inc(mosq_counter_bytes_sent, write_length);
                packet->to_process -= (uint32_t)write_length;
                packet->pos += (uint32_t)write_length;
            } else {
                if ((errno == EAGAIN) || (errno == COMPAT_EWOULDBLOCK)) {
                    return MOSQ_ERR_SUCCESS;
                } else {
                    switch (errno) {
                        case COMPAT_ECONNRESET:
                            return MOSQ_ERR_CONN_LOST;

                        case COMPAT_EINTR:
                            return MOSQ_ERR_SUCCESS;

                        default:
                        return MOSQ_ERR_ERRNO;
                    }
                }
            }
        }

        metrics__int_inc(mosq_counter_messages_sent, 1);

        if (((packet->command) & 0xF6) == CMD_PUBLISH) {
            callback__on_publish(mosq, packet->mid, 0, NULL);
        } else if (((packet->command) & 0xF0) == CMD_DISCONNECT) {
            do_client_disconnect(mosq, MOSQ_ERR_SUCCESS, NULL);
            return MOSQ_ERR_SUCCESS;
        }

        next_packet = packet__get_next_out(mosq);

        mosquitto__FREE(packet);
        packet = next_packet;

        pthread_mutex_lock(&mosq->msgtime_mutex);
        mosq->next_msg_out = mosquitto_time() + mosq->keepalive;
        pthread_mutex_unlock(&mosq->msgtime_mutex);
    }

    return MOSQ_ERR_SUCCESS;
}

int packet__read(struct mosquitto *mosq)
{
    int rc = 0;
    uint8_t byte;
    ssize_t read_length;
    enum mosquitto_client_state state;
    ssize_t (*local__read)(struct mosquitto *, void *, size_t);

    if (!mosq) {
        return MOSQ_ERR_INVAL;
    }

    if (!net__is_connected(mosq)) {
        return MOSQ_ERR_NO_CONN;
    }

    state = mosquitto__get_state(mosq);
    if (state == mosq_cs_connect_pending) {
        return MOSQ_ERR_SUCCESS;
    }

    {
        local__read = net__read;
    }

    if (!mosq->in_packet.command) {
        read_length = local__read(mosq, &byte, 1);
        if (read_length == 1) {
            mosq->in_packet.command = byte;
        } else {
            if (read_length == 0) {
                return MOSQ_ERR_CONN_LOST;
            }

            if ((errno == EAGAIN) || (errno == COMPAT_EWOULDBLOCK)) {
                return MOSQ_ERR_SUCCESS;
            } else {
                switch (errno) {
                    case COMPAT_ECONNRESET:
                        return MOSQ_ERR_CONN_LOST;

                    case COMPAT_EINTR:
                        return MOSQ_ERR_SUCCESS;

                    default:
                        return MOSQ_ERR_ERRNO;
                }
            }
        }
    }

    if (mosq->in_packet.remaining_count <= 0) {
        do {
            read_length = local__read(mosq, &byte, 1);
            if (read_length == 1) {
                mosq->in_packet.remaining_count--;

                // 协议定义的剩余长度最大为4个字节的长度。更有可能表示客户已损坏/恶意
                if (mosq->in_packet.remaining_count < -4) {
                    return MOSQ_ERR_MALFORMED_PACKET;
                }

                metrics__int_inc(mosq_counter_bytes_received, 1);
                mosq->in_packet.remaining_length += (byte & 0x7F) * mosq->in_packet.remaining_mult;
                mosq->in_packet.remaining_mult *= 0x80;
            } else {
                if (read_length == 0) {
                    // EOF
                    return MOSQ_ERR_CONN_LOST;
                }

                if ((errno == EAGAIN) || (errno == COMPAT_EWOULDBLOCK)) {
                    return MOSQ_ERR_SUCCESS;
                } else {
                    switch (errno) {
                        case COMPAT_ECONNRESET:
                            return MOSQ_ERR_CONN_LOST;

                        case COMPAT_EINTR:
                            return MOSQ_ERR_SUCCESS;

                        default:
                            return MOSQ_ERR_ERRNO;
                    }
                }
            }
        } while ((byte & 0x80) != 0);

        // 已经完成了剩余的长度的读取，因此使剩余的计数为正
        mosq->in_packet.remaining_count = (int8_t)(mosq->in_packet.remaining_count * -1);

        // FIXME - 从代理收到的传入消息的客户案例太大

        if (mosq->in_packet.remaining_length > 0) {
            mosq->in_packet.payload = (uint8_t *)mosquitto__malloc(mosq->in_packet.remaining_length * sizeof(uint8_t));
            if (!mosq->in_packet.payload) {
                return MOSQ_ERR_NOMEM;
            }

            mosq->in_packet.to_process = mosq->in_packet.remaining_length;
        }
    }

    while (mosq->in_packet.to_process > 0) {
        read_length = local__read(mosq, &(mosq->in_packet.payload[mosq->in_packet.pos]), mosq->in_packet.to_process);
        if (read_length > 0) {
            metrics__int_inc(mosq_counter_bytes_received, read_length);
            mosq->in_packet.to_process -= (uint32_t)read_length;
            mosq->in_packet.pos += (uint32_t)read_length;
        } else {
            if ((errno == EAGAIN) || (errno == COMPAT_EWOULDBLOCK)) {
                if (mosq->in_packet.to_process > 1000) {
                    pthread_mutex_lock(&mosq->msgtime_mutex);
                    mosq->last_msg_in = mosquitto_time();
                    pthread_mutex_unlock(&mosq->msgtime_mutex);
                }

                return MOSQ_ERR_SUCCESS;
            } else {
                switch (errno) {
                    case COMPAT_ECONNRESET:
                        return MOSQ_ERR_CONN_LOST;

                    case COMPAT_EINTR:
                        return MOSQ_ERR_SUCCESS;

                    default:
                        return MOSQ_ERR_ERRNO;
                }
            }
        }
    }

    // 读取该数据包的所有数据
    mosq->in_packet.pos = 0;

    rc = handle__packet(mosq);

    // 释放数据和重置值
    packet__cleanup(&mosq->in_packet);

    pthread_mutex_lock(&mosq->msgtime_mutex);
    mosq->last_msg_in = mosquitto_time();
    pthread_mutex_unlock(&mosq->msgtime_mutex);

    return rc;
}
