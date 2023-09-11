#ifndef MOSQ_MOSQUITTO_BROKER_H
#define MOSQ_MOSQUITTO_BROKER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "mosquitto.h"
#include "mqtt_protocol.h"

enum mosquitto_protocol {
    mp_mqtt,
    mp_mqttsn,
    mp_websockets
};

enum mosquitto_broker_msg_direction {
    mosq_bmd_in  = 0,
    mosq_bmd_out = 1,
    mosq_bmd_all = 2
};

struct mosquitto_client {
    char                        *clientid;
    char                        *username;
    char                        *auth_method;
    struct mosquitto_message_v5 *will;
    time_t                      will_delay_time;
    time_t                      session_expiry_time;
    uint32_t                    will_delay_interval;
    uint32_t                    session_expiry_interval;
    uint32_t                    max_packet_size;
    uint16_t                    listener_port;
    uint8_t                     max_qos;
    bool                        retain_available;
    void                        *future2[8];
};

struct mosquitto_subscription {
    char               *clientid;
    char               *topic_filter;
    mosquitto_property *properties;
    uint32_t           identifier;
    uint8_t            options;
    uint8_t            padding[3];
    void               *future2[8];
};

struct mosquitto_base_msg {
    uint64_t           store_id;
    int64_t            expiry_time;
    char               *topic;
    void               *payload;
    char               *source_id;
    char               *source_username;
    mosquitto_property *properties;
    uint32_t           payloadlen;
    uint16_t           source_mid;
    uint16_t           source_port;
    uint8_t            qos;
    bool               retain;
    uint8_t            padding[6];
    void               *future2[8];
};

struct mosquitto_client_msg {
    const char *clientid;
    uint64_t   cmsg_id;
    uint64_t   store_id;
    uint32_t   subscription_identifier;
    uint16_t   mid;
    uint8_t    qos;
    bool       retain;
    uint8_t    dup;
    uint8_t    direction;
    uint8_t    state;
    uint8_t    padding[5];
    void       *future2[8];
};

// 回调事件
enum mosquitto_plugin_event {
    MOSQ_EVT_RELOAD                      = 1,
    MOSQ_EVT_ACL_CHECK                   = 2,
    MOSQ_EVT_BASIC_AUTH                  = 3,
    MOSQ_EVT_EXT_AUTH_START              = 4,
    MOSQ_EVT_EXT_AUTH_CONTINUE           = 5,
    MOSQ_EVT_CONTROL                     = 6,
    MOSQ_EVT_MESSAGE                     = 7,
    MOSQ_EVT_MESSAGE_IN                  = 7,
    MOSQ_EVT_PSK_KEY                     = 8,
    MOSQ_EVT_TICK                        = 9,
    MOSQ_EVT_DISCONNECT                  = 10,
    MOSQ_EVT_CONNECT                     = 11,
    MOSQ_EVT_SUBSCRIBE                   = 12,
    MOSQ_EVT_UNSUBSCRIBE                 = 13,
    MOSQ_EVT_PERSIST_RESTORE             = 14,
    MOSQ_EVT_PERSIST_BASE_MSG_ADD        = 15,
    MOSQ_EVT_PERSIST_BASE_MSG_DELETE     = 16,
    MOSQ_EVT_PERSIST_RETAIN_MSG_SET      = 17,
    MOSQ_EVT_PERSIST_RETAIN_MSG_DELETE   = 18,
    MOSQ_EVT_PERSIST_CLIENT_ADD          = 19,
    MOSQ_EVT_PERSIST_CLIENT_DELETE       = 20,
    MOSQ_EVT_PERSIST_CLIENT_UPDATE       = 21,
    MOSQ_EVT_PERSIST_SUBSCRIPTION_ADD    = 22,
    MOSQ_EVT_PERSIST_SUBSCRIPTION_DELETE = 23,
    MOSQ_EVT_PERSIST_CLIENT_MSG_ADD      = 24,
    MOSQ_EVT_PERSIST_CLIENT_MSG_DELETE   = 25,
    MOSQ_EVT_PERSIST_CLIENT_MSG_UPDATE   = 26,
    MOSQ_EVT_MESSAGE_OUT                 = 27,
    MOSQ_EVT_CLIENT_OFFLINE              = 28,
};

// MOSQ_EVT_RELOAD事件的数据
struct mosquitto_evt_reload {
    void                 *future;
    struct mosquitto_opt *options;
    int                  option_count;
    void                 *future2[4];
};

// MOSQ_EVT_ACL_CHECK事件的数据
struct mosquitto_evt_acl_check {
    void               *future;
    struct mosquitto   *client;
    const char         *topic;
    const void         *payload;
    mosquitto_property *properties;
    int                access;
    uint32_t           payloadlen;
    uint8_t            qos;
    bool               retain;
    void               *future2[4];
};

// MOSQ_EVT_BASIC_AUTH事件的数据
struct mosquitto_evt_basic_auth {
    void             *future;
    struct mosquitto *client;
    char             *username;
    char             *password;
    void             *future2[4];
};

// MOSQ_EVT_PSK_KEY事件的数据
struct mosquitto_evt_psk_key {
    void             *future;
    struct mosquitto *client;
    const char       *hint;
    const char       *identity;
    char             *key;
    int              max_key_len;
    void             *future2[4];
};

// MOSQ_EVT_EXTENDED_AUTH事件的数据
struct mosquitto_evt_extended_auth {
    void             *future;
    struct mosquitto *client;
    const void       *data_in;
    void             *data_out;
    uint16_t         data_in_len;
    uint16_t         data_out_len;
    const char       *auth_method;
    void             *future2[3];
};

// MOSQ_EVT_CONTROL事件的数据
struct mosquitto_evt_control {
    void                     *future;
    struct mosquitto         *client;
    const char               *topic;
    const void               *payload;
    const mosquitto_property *properties;
    char                     *reason_string;
    uint32_t                 payloadlen;
    uint8_t                  qos;
    uint8_t                  reason_code;
    bool                     retain;
    uint8_t                  padding;
    void                     *future2[4];
};

// MOSQ_EVT_MESSAGE事件的数据
struct mosquitto_evt_message {
    void               *future;
    struct mosquitto   *client;
    char               *topic;
    void               *payload;
    mosquitto_property *properties;
    char               *reason_string;
    uint32_t           payloadlen;
    uint8_t            qos;
    uint8_t            reason_code;
    bool               retain;
    uint8_t            padding;
    void               *future2[4];
};

// MOSQ_EVT_TICK事件的数据
struct mosquitto_evt_tick {
    void   *future;
    long   now_ns;
    long   next_ms;
    time_t now_s;
    time_t next_s;
    void   *future2[4];
};

// MOSQ_EVT_CONNECT事件的数据
struct mosquitto_evt_connect {
    void             *future;
    struct mosquitto *client;
    void             *future2[4];
};

// MOSQ_EVT_DISCONNECT事件的数据
struct mosquitto_evt_disconnect {
    void             *future;
    struct mosquitto *client;
    int              reason;
    void             *future2[4];
};

struct mosquitto_evt_client_offline {
    void             *future;
    struct mosquitto *client;
    int              reason;
    void             *future2[4];
};

// MOSQ_EVT_SUBSCRIBE事件的数据
struct mosquitto_evt_subscribe {
    void                          *future;
    struct mosquitto              *client;
    struct mosquitto_subscription data;
    void                          *future2[8];
};

// MOSQ_EVT_UNSUBSCRIBE事件的数据
struct mosquitto_evt_unsubscribe {
    void                          *future;
    struct mosquitto              *client;
    struct mosquitto_subscription data;
    void                          *future2[8];
};

struct mosquitto_evt_persist_restore {
    void *future[8];
};

struct mosquitto_evt_persist_client {
    void                              *future;
    const char                        *clientid;
    const char                        *username;
    const char                        *auth_method;
    const struct mosquitto_message_v5 *will;
    char                              *plugin_client_id;
    char                              *plugin_username;
    char                              *plugin_auth_method;
    struct mosquitto_message_v5       *plugin_will;
    time_t                            will_delay_time;
    time_t                            session_expiry_time;
    uint32_t                          will_delay_interval;
    uint32_t                          session_expiry_interval;
    uint32_t                          max_packet_size;
    uint16_t                          listener_port;
    uint8_t                           max_qos;
    bool                              retain_available;
    uint8_t                           padding[6];
    void                              *future2[8];
};

struct mosquitto_evt_persist_subscription {
    void                          *future;
    struct mosquitto_subscription data;
    void                          *future2[8];
};

struct mosquitto_evt_persist_client_msg {
    void                        *future;
    struct mosquitto_client_msg data;
    void                        *future2[8];
};

struct mosquitto_evt_persist_retain_msg {
    void       *future;
    const char *topic;
    uint64_t   store_id;
    void       *future2[8];
};

struct mosquitto_evt_persist_base_msg {
    void                      *future;
    struct mosquitto_base_msg data;
    void                      *future2[8];
};

// 回调定义
typedef int (*MOSQ_FUNC_generic_callback)(int, void *, void *);

typedef struct mosquitto_plugin_id_t mosquitto_plugin_id_t;

int mosquitto_plugin_set_info(mosquitto_plugin_id_t *identifier, const char *plugin_name, const char *plugin_version);

int mosquitto_callback_register(mosquitto_plugin_id_t *identifier, int event, MOSQ_FUNC_generic_callback cb_func, const void *event_data, void *userdata);
int mosquitto_callback_unregister(mosquitto_plugin_id_t *identifier, int event, MOSQ_FUNC_generic_callback cb_func, const void *event_data);

void mosquitto_free(void *mem);
void *mosquitto_malloc(size_t size);
void *mosquitto_realloc(void *ptr, size_t size);
void *mosquitto_calloc(size_t nmemb, size_t size);

char *mosquitto_strdup(const char *s);
void mosquitto_log_printf(int level, const char *fmt, ...);

struct mosquitto *mosquitto_client(const char *clientid);

const char *mosquitto_client_id(const struct mosquitto *client);
const char *mosquitto_client_address(const struct mosquitto *client);

int mosquitto_client_port(const struct mosquitto *client);

int mosquitto_client_protocol(const struct mosquitto *client);
int mosquitto_client_keepalive(const struct mosquitto *client);
int mosquitto_client_sub_count(const struct mosquitto *client);
void *mosquitto_client_certificate(const struct mosquitto *client);
bool mosquitto_client_clean_session(const struct mosquitto *client);
int mosquitto_client_protocol_version(const struct mosquitto *client);

const char *mosquitto_client_username(const struct mosquitto *client);
int mosquitto_set_username(struct mosquitto *client, const char *username);

int mosquitto_set_clientid(struct mosquitto *client, const char *clientid);

int mosquitto_kick_client_by_clientid(const char *clientid, bool with_will);
int mosquitto_kick_client_by_username(const char *username, bool with_will);

int mosquitto_apply_on_all_clients(int (*FUNC_client_functor)(const struct mosquitto *, void *), void *functor_context);

int mosquitto_broker_publish(const char *clientid, const char *topic, int payloadlen, void *payload, int qos, bool retain, mosquitto_property *properties);
int mosquitto_broker_publish_copy(const char *clientid, const char *topic, int payloadlen, const void *payload, int qos, bool retain, mosquitto_property *properties);

void mosquitto_complete_basic_auth(const char *clientid, int result);

int mosquitto_broker_node_id_set(uint16_t id);

int mosquitto_persist_client_add(struct mosquitto_evt_persist_client *client);
int mosquitto_persist_client_update(struct mosquitto_evt_persist_client *client);

int mosquitto_persist_client_delete(const char *clientid);

int mosquitto_persist_client_msg_add(struct mosquitto_evt_persist_client_msg *client_msg);
int mosquitto_persist_client_msg_delete(struct mosquitto_evt_persist_client_msg *client_msg);
int mosquitto_persist_client_msg_update(struct mosquitto_evt_persist_client_msg *client_msg);
int mosquitto_persist_client_msg_clear(struct mosquitto_evt_persist_client_msg *client_msg);

int mosquitto_persist_base_msg_delete(uint64_t store_id);
int mosquitto_persist_base_msg_add(struct mosquitto_base_msg *msg);

int mosquitto_subscription_add(const struct mosquitto_subscription *sub);

int mosquitto_subscription_delete(const char *clientid, const char *topic_filter);

int mosquitto_persist_retain_msg_set(const char *topic, uint64_t store_id);
int mosquitto_persist_retain_msg_delete(const char *topic);

const char *mosquitto_persistence_location(void);

#ifdef __cplusplus
}
#endif

#endif
