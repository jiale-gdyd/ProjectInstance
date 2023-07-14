#include <errno.h>
#include <stdbool.h>
#include <mosquitto/mosquitto.h>

#include "config.h"
#include "memory_mosq.h"
#include "mosquitto_internal.h"

struct userdata__callback {
    const char *topic;
    int (*callback)(struct mosquitto *, void *, const struct mosquitto_message *);
    void       *userdata;
    int        qos;
};

struct userdata__simple {
    struct mosquitto_message *messages;
    int                      max_msg_count;
    int                      message_count;
    bool                     want_retained;
};

static void on_connect(struct mosquitto *mosq, void *obj, int rc)
{
    struct userdata__callback *userdata = (struct userdata__callback *)obj;
    MOSQ_UNUSED(rc);
    mosquitto_subscribe(mosq, NULL, userdata->topic, userdata->qos);
}

static void on_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
    int rc;
    struct userdata__callback *userdata = (struct userdata__callback *)obj;

    rc = userdata->callback(mosq, userdata->userdata, message);
    if (rc) {
        mosquitto_disconnect(mosq);
    }
}

static int on_message_simple(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
    int rc;
    struct userdata__simple *userdata = (struct userdata__simple *)obj;

    if (userdata->max_msg_count == 0) {
        return 0;
    }

    // 如果'want_retained'为false，请勿处理过时的保留消息
    if (!userdata->want_retained && message->retain) {
        return 0;
    }

    userdata->max_msg_count--;

    rc = mosquitto_message_copy(&userdata->messages[userdata->message_count], message);
    if (rc) {
        return rc;
    }

    userdata->message_count++;
    if (userdata->max_msg_count == 0) {
        mosquitto_disconnect(mosq);
    }

    return 0;
}

int mosquitto_subscribe_simple(struct mosquitto_message **messages, int msg_count, bool want_retained, const char *topic, int qos, const char *host, int port, const char *clientid, int keepalive, bool clean_session, const char *username, const char *password, const struct libmosquitto_will *will, const struct libmosquitto_tls *tls)
{
    int i;
    int rc;
    struct userdata__simple userdata;

    if (!topic || (msg_count < 1) || !messages) {
        return MOSQ_ERR_INVAL;
    }

    *messages = NULL;

    userdata.messages = (struct mosquitto_message *)calloc(sizeof(struct mosquitto_message), (size_t)msg_count);
    if (!userdata.messages) {
        return MOSQ_ERR_NOMEM;
    }

    userdata.message_count = 0;
    userdata.max_msg_count = msg_count;
    userdata.want_retained = want_retained;

    rc = mosquitto_subscribe_callback(on_message_simple, &userdata, topic, qos, host, port, clientid, keepalive, clean_session, username, password, will, tls);

    if (!rc && (userdata.max_msg_count == 0)) {
        *messages = userdata.messages;
        return MOSQ_ERR_SUCCESS;
    } else {
        for (i = 0; i < msg_count; i++) {
            mosquitto_message_free_contents(&userdata.messages[i]);
        }

        SAFE_FREE(userdata.messages);
        return rc;
    }
}

int mosquitto_subscribe_callback(int (*callback)(struct mosquitto *, void *, const struct mosquitto_message *), void *userdata, const char *topic, int qos, const char *host, int port, const char *clientid, int keepalive, bool clean_session, const char *username, const char *password, const struct libmosquitto_will *will, const struct libmosquitto_tls *tls)
{
    int rc;
    struct mosquitto *mosq;
    struct userdata__callback cb_userdata;

    if (!callback || !topic) {
        return MOSQ_ERR_INVAL;
    }

    cb_userdata.topic = topic;
    cb_userdata.qos = qos;
    cb_userdata.userdata = userdata;
    cb_userdata.callback = callback;

    mosq = mosquitto_new(clientid, clean_session, &cb_userdata);
    if (!mosq) {
        return MOSQ_ERR_NOMEM;
    }

    if (will) {
        rc = mosquitto_will_set(mosq, will->topic, will->payloadlen, will->payload, will->qos, will->retain);
        if (rc) {
            mosquitto_destroy(mosq);
            return rc;
        }
    }

    if (username) {
        rc = mosquitto_username_pw_set(mosq, username, password);
        if (rc) {
            mosquitto_destroy(mosq);
            return rc;
        }
    }

    if (tls) {
        rc = mosquitto_tls_set(mosq, tls->cafile, tls->capath, tls->certfile, tls->keyfile, tls->pw_callback);
        if (rc) {
            mosquitto_destroy(mosq);
            return rc;
        }

        rc = mosquitto_tls_opts_set(mosq, tls->cert_reqs, tls->tls_version, tls->ciphers);
        if (rc) {
            mosquitto_destroy(mosq);
            return rc;
        }
    }

    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_message_callback_set(mosq, on_message_callback);

    rc = mosquitto_connect(mosq, host, port, keepalive);
    if (rc) {
        mosquitto_destroy(mosq);
        return rc;
    }

    rc = mosquitto_loop_forever(mosq, -1, 1);
    mosquitto_destroy(mosq);

    return rc;
}
