#include <string.h>
#include <mosquitto/mosquitto.h>
#include <mosquitto/mqtt_protocol.h>

#include "config.h"
#include "net_mosq.h"
#include "send_mosq.h"
#include "callbacks.h"
#include "util_mosq.h"
#include "socks_mosq.h"
#include "memory_mosq.h"
#include "packet_mosq.h"
#include "logging_mosq.h"
#include "messages_mosq.h"
#include "mosquitto_internal.h"

static char alphanum[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

static int mosquitto__reconnect(struct mosquitto *mosq, bool blocking);
static int mosquitto__connect_init(struct mosquitto *mosq, const char *host, int port, int keepalive);

static int mosquitto__connect_init(struct mosquitto *mosq, const char *host, int port, int keepalive)
{
    int i, rc;

    if (!mosq) {
        return MOSQ_ERR_INVAL;
    }

    if (!host || (port < 0) || (port > UINT16_MAX)) {
        return MOSQ_ERR_INVAL;
    }

    if ((keepalive != 0) && ((keepalive < 5) || (keepalive > UINT16_MAX))) {
        return MOSQ_ERR_INVAL;
    }

    // 仅MQTT v3.1要求发送客户端ID
    if ((mosq->id == NULL) && (mosq->protocol == mosq_p_mqtt31)) {
        mosq->id = (char *)mosquitto__calloc(24, sizeof(char));
        if (!mosq->id) {
            return MOSQ_ERR_NOMEM;
        }

        mosq->id[0] = 'm';
        mosq->id[1] = 'o';
        mosq->id[2] = 's';
        mosq->id[3] = 'q';
        mosq->id[4] = '-';

        rc = util__random_bytes(&mosq->id[5], 18);
        if (rc) {
            return rc;
        }

        for (i = 5; i < 23; i++) {
            mosq->id[i] = alphanum[(mosq->id[i] & 0x7F) % (sizeof(alphanum) - 1)];
        }
    }

    mosquitto__FREE(mosq->host);
    mosq->host = mosquitto__strdup(host);
    if (!mosq->host) {
        return MOSQ_ERR_NOMEM;
    }
    mosq->port = (uint16_t)port;

    mosq->keepalive = (uint16_t)keepalive;
    mosq->msgs_in.inflight_quota = mosq->msgs_in.inflight_maximum;
    mosq->msgs_out.inflight_quota = mosq->msgs_out.inflight_maximum;
    mosq->retain_available = 1;
    mosquitto__set_request_disconnect(mosq, false);

    return MOSQ_ERR_SUCCESS;
}

int mosquitto_connect(struct mosquitto *mosq, const char *host, int port, int keepalive)
{
    return mosquitto_connect_bind(mosq, host, port, keepalive, NULL);
}

int mosquitto_connect_bind(struct mosquitto *mosq, const char *host, int port, int keepalive, const char *bind_address)
{
    return mosquitto_connect_bind_v5(mosq, host, port, keepalive, bind_address, NULL);
}

int mosquitto_connect_bind_v5(struct mosquitto *mosq, const char *host, int port, int keepalive, const char *bind_address, const mosquitto_property *properties)
{
    int rc;

    if (bind_address) {
        rc = mosquitto_string_option(mosq, MOSQ_OPT_BIND_ADDRESS, bind_address);
        if (rc) {
            return rc;
        }
    }

    mosquitto_property_free_all(&mosq->connect_properties);
    if (properties) {
        rc = mosquitto_property_check_all(CMD_CONNECT, properties);
        if (rc) {
            return rc;
        }

        rc = mosquitto_property_copy_all(&mosq->connect_properties, properties);
        if (rc) {
            return rc;
        }
        mosq->connect_properties->client_generated = true;
    }

    rc = mosquitto__connect_init(mosq, host, port, keepalive);
    if (rc) {
        return rc;
    }
    mosquitto__set_state(mosq, mosq_cs_new);

    return mosquitto__reconnect(mosq, true);
}

int mosquitto_connect_async(struct mosquitto *mosq, const char *host, int port, int keepalive)
{
    return mosquitto_connect_bind_async(mosq, host, port, keepalive, NULL);
}

int mosquitto_connect_bind_async(struct mosquitto *mosq, const char *host, int port, int keepalive, const char *bind_address)
{
    int rc;

    if (bind_address) {
        rc = mosquitto_string_option(mosq, MOSQ_OPT_BIND_ADDRESS, bind_address);
        if (rc) {
            return rc;
        }
    }

    rc = mosquitto__connect_init(mosq, host, port, keepalive);
    if (rc) {
        return rc;
    }

    return mosquitto__reconnect(mosq, false);
}

int mosquitto_reconnect_async(struct mosquitto *mosq)
{
    return mosquitto__reconnect(mosq, false);
}

int mosquitto_reconnect(struct mosquitto *mosq)
{
    return mosquitto__reconnect(mosq, true);
}

static int mosquitto__reconnect(struct mosquitto *mosq, bool blocking)
{
    int rc;
    mosquitto_property local_property;
    const mosquitto_property *outgoing_properties = NULL;

    if (!mosq) {
        return MOSQ_ERR_INVAL;
    }

    if (!mosq->host) {
        return MOSQ_ERR_INVAL;
    }

    if (mosq->connect_properties) {
        if (mosq->protocol != mosq_p_mqtt5) {
            return MOSQ_ERR_NOT_SUPPORTED;
        }

        if (mosq->connect_properties->client_generated) {
            outgoing_properties = mosq->connect_properties;
        } else {
            memcpy(&local_property, mosq->connect_properties, sizeof(mosquitto_property));
            local_property.client_generated = true;
            local_property.next = NULL;
            outgoing_properties = &local_property;
        }

        rc = mosquitto_property_check_all(CMD_CONNECT, outgoing_properties);
        if (rc) {
            return rc;
        }
    }

    pthread_mutex_lock(&mosq->msgtime_mutex);
    mosq->last_msg_in = mosquitto_time();
    mosq->next_msg_out = mosq->last_msg_in + mosq->keepalive;
    pthread_mutex_unlock(&mosq->msgtime_mutex);

    mosq->ping_t = 0;

    packet__cleanup(&mosq->in_packet);
    packet__cleanup_all(mosq);

    message__reconnect_reset(mosq, false);

    if (net__is_connected(mosq)) {
        net__socket_close(mosq);
    }

    callback__on_pre_connect(mosq);

    if (mosq->socks5_host) {
        rc = net__socket_connect(mosq, mosq->socks5_host, mosq->socks5_port, mosq->bind_address, blocking);
    } else {
        rc = net__socket_connect(mosq, mosq->host, mosq->port, mosq->bind_address, blocking);
    }

    if (rc > 0) {
        mosquitto__set_state(mosq, mosq_cs_connect_pending);
        return rc;
    }

    if (mosq->socks5_host) {
        mosquitto__set_state(mosq, mosq_cs_socks5_new);
        return socks5__send(mosq);
    } else {
        mosquitto__set_state(mosq, mosq_cs_connected);
        rc = send__connect(mosq, mosq->keepalive, mosq->clean_start, outgoing_properties);
        if (rc) {
            packet__cleanup_all(mosq);
            net__socket_close(mosq);
        }

        return rc;
    }
}

int mosquitto_disconnect(struct mosquitto *mosq)
{
    return mosquitto_disconnect_v5(mosq, 0, NULL);
}

int mosquitto_disconnect_v5(struct mosquitto *mosq, int reason_code, const mosquitto_property *properties)
{
    int rc;
    mosquitto_property local_property;
    const mosquitto_property *outgoing_properties = NULL;

    if (!mosq) {
        return MOSQ_ERR_INVAL;
    }

    if ((mosq->protocol != mosq_p_mqtt5) && properties) {
        return MOSQ_ERR_NOT_SUPPORTED;
    }

    if ((reason_code < 0) || (reason_code > UINT8_MAX)) {
        return MOSQ_ERR_INVAL;
    }

    if (properties) {
        if (properties->client_generated){
            outgoing_properties = properties;
        } else {
            memcpy(&local_property, properties, sizeof(mosquitto_property));
            local_property.client_generated = true;
            local_property.next = NULL;
            outgoing_properties = &local_property;
        }

        rc = mosquitto_property_check_all(CMD_DISCONNECT, outgoing_properties);
        if (rc) {
            return rc;
        }
    }

    mosquitto__set_state(mosq, mosq_cs_disconnected);
    mosquitto__set_request_disconnect(mosq, true);

    if (!net__is_connected(mosq)) {
        return MOSQ_ERR_NO_CONN;
    } else {
        return send__disconnect(mosq, (uint8_t)reason_code, outgoing_properties);
    }
}

void do_client_disconnect(struct mosquitto *mosq, int reason_code, const mosquitto_property *properties)
{
    mosquitto__set_state(mosq, mosq_cs_disconnected);
    net__socket_close(mosq);

    packet__cleanup_all(mosq);

    pthread_mutex_lock(&mosq->msgtime_mutex);
    mosq->next_msg_out = mosquitto_time() + mosq->keepalive;
    pthread_mutex_unlock(&mosq->msgtime_mutex);

    callback__on_disconnect(mosq, reason_code, properties);
}
