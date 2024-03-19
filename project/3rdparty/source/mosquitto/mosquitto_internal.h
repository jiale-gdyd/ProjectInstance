#ifndef MOSQ_MOSQUITTO_INTERNAL_H
#define MOSQ_MOSQUITTO_INTERNAL_H

#include "config.h"

#ifdef WITH_TLS
#include <openssl/ssl.h>
#else
#include <time.h>
#endif

#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <mosquitto/mosquitto.h>

#include "time_mosq.h"

// 别名方向 - 本地<->远程
#define ALIAS_DIR_L2R               (1)
#define ALIAS_DIR_R2L               (2)

#define STREMPTY(str)               (str[0] == '\0')

#define WS_PACKET_OFFSET            16

#define SAFE_PRINT(A)               (A) ? (A) : "null"

#define MSG_EXPIRY_INFINITE         UINT32_MAX

typedef int mosq_sock_t;

enum mosquitto_msg_direction {
    mosq_md_in  = 0,
    mosq_md_out = 1
};

enum mosquitto_msg_state {
    mosq_ms_invalid          = 0,
    mosq_ms_publish_qos0     = 1,
    mosq_ms_publish_qos1     = 2,
    mosq_ms_wait_for_puback  = 3,
    mosq_ms_publish_qos2     = 4,
    mosq_ms_wait_for_pubrec  = 5,
    mosq_ms_resend_pubrel    = 6,
    mosq_ms_wait_for_pubrel  = 7,
    mosq_ms_resend_pubcomp   = 8,
    mosq_ms_wait_for_pubcomp = 9,
    mosq_ms_send_pubrec      = 10,
    mosq_ms_queued           = 11
};

enum mosquitto_client_state {
    mosq_cs_new                   = 0,
    mosq_cs_connected             = 1,
    mosq_cs_disconnecting         = 2,
    mosq_cs_active                = 3,
    mosq_cs_connect_pending       = 4,
    mosq_cs_connect_srv           = 5,
    mosq_cs_disconnect_ws         = 6,
    mosq_cs_disconnected          = 7,
    mosq_cs_socks5_new            = 8,
    mosq_cs_socks5_start          = 9,
    mosq_cs_socks5_request        = 10,
    mosq_cs_socks5_reply          = 11,
    mosq_cs_socks5_auth_ok        = 12,
    mosq_cs_socks5_userpass_reply = 13,
    mosq_cs_socks5_send_userpass  = 14,
    mosq_cs_expiring              = 15,
    mosq_cs_duplicate             = 17,     // 已被另一个具有相同ID的客户接管的客户端
    mosq_cs_disconnect_with_will  = 18,
    mosq_cs_disused               = 19,     // 已添加到废弃列表中的客户端将被释放
    mosq_cs_authenticating        = 20,     // 客户端已发送CONNECT，但仍在进行扩展身份验证
    mosq_cs_reauthenticating      = 21,     // 客户端正在重新认证，在完成之前不应该做任何其他事情
    mosq_cs_delayed_auth          = 22,     // 客户端正在等待插件的身份验证结果
};

enum mosquitto__protocol {
    mosq_p_invalid = 0,
    mosq_p_mqtts   = 1,
    mosq_p_mqtt31  = 3,
    mosq_p_mqtt311 = 4,
    mosq_p_mqtt5   = 5,
};

enum mosquitto__threaded_state {
    mosq_ts_none,                           // 没有使用线程
    mosq_ts_self,                           // libmosquitto启动的线程
    mosq_ts_external                        // 由外部代码启动的线程
};

enum mosquitto__transport {
    mosq_t_invalid = 0,
    mosq_t_tcp     = 1,
    mosq_t_ws      = 2,
    mosq_t_sctp    = 3,
    mosq_t_http    = 4,
};

struct mosquitto__alias {
    char     *topic;
    uint16_t alias;
};

struct session_expiry_list {
    struct mosquitto           *context;
    struct session_expiry_list *prev;
    struct session_expiry_list *next;
};

struct mosquitto__packet {
    struct mosquitto__packet *next;
    uint32_t                 remaining_length;
    uint32_t                 packet_length;
    uint32_t                 to_process;
    uint32_t                 pos;
    uint16_t                 mid;
    uint8_t                  command;
    int8_t                   remaining_count;
    uint8_t                  payload[];
};

struct mosquitto__packet_in {
    uint8_t  *payload;
    uint32_t remaining_mult;
    uint32_t remaining_length;
    uint32_t packet_length;
    uint32_t to_process;
    uint32_t pos;
    uint8_t  command;
    int8_t   remaining_count;
};

struct mosquitto_message_all {
    struct mosquitto_message_all *next;
    struct mosquitto_message_all *prev;
    mosquitto_property           *properties;
    enum mosquitto_msg_state     state;
    bool                         dup;
    struct mosquitto_message     msg;
    uint32_t                     expiry_interval;
};

#ifdef WITH_TLS
enum mosquitto__keyform {
    mosq_k_pem    = 0,
    mosq_k_engine = 1,
};
#endif

struct will_delay_list {
    struct mosquitto       *context;
    struct will_delay_list *prev;
    struct will_delay_list *next;
};

struct mosquitto_msg_data {
    struct mosquitto_message_all *inflight;
    int                          queue_len;
    pthread_mutex_t              mutex;
    int                          inflight_quota;
    uint16_t                     inflight_maximum;
};

#define WS_CONTINUATION     0x00
#define WS_TEXT             0x01
#define WS_BINARY           0x02
#define WS_CLOSE            0x08
#define WS_PING             0x09
#define WS_PONG             0x0A

struct client_stats {
    uint64_t messages_received;
    uint64_t messages_sent;
    uint64_t messages_dropped;
};

struct mosquitto {
    mosq_sock_t                     sock;
    mosq_sock_t                     sockpairR;
    mosq_sock_t                     sockpairW;
    uint32_t                        maximum_packet_size;
    uint64_t                        last_cmsg_id;
    enum mosquitto__protocol        protocol;
    char                            *address;
    char                            *id;
    char                            *username;
    char                            *password;
    uint16_t                        keepalive;
    uint16_t                        last_mid;
    enum mosquitto_client_state     state;
    uint8_t                         transport;
    time_t                          last_msg_in;
    time_t                          next_msg_out;
    time_t                          ping_t;
    struct mosquitto__packet_in     in_packet;
    struct mosquitto__packet        *out_packet;
    struct mosquitto_message_all    *will;
    struct mosquitto__alias         *aliases_l2r;
    struct mosquitto__alias         *aliases_r2l;
    struct will_delay_list          *will_delay_entry;
    uint16_t                        alias_count_l2r;
    uint16_t                        alias_count_r2l;
    uint16_t                        alias_max_l2r;
    uint32_t                        will_delay_interval;
    int                             out_packet_count;
    int64_t                         out_packet_bytes;
    time_t                          will_delay_time;
#ifdef WITH_TLS
    SSL                             *ssl;
    SSL_CTX                         *ssl_ctx;
    SSL_CTX                         *user_ssl_ctx;
    char                            *tls_cafile;
    char                            *tls_capath;
    char                            *tls_certfile;
    char                            *tls_keyfile;
    int (*tls_pw_callback)(char *buf, int size, int rwflag, void *userdata);
    char                            *tls_version;
    char                            *tls_ciphers;
    char                            *tls_13_ciphers;
    char                            *tls_psk;
    char                            *tls_psk_identity;
    char                            *tls_engine;
    char                            *tls_engine_kpass_sha1;
    char                            *tls_alpn;
    int                             tls_cert_reqs;
    bool                            tls_insecure;
    bool                            ssl_ctx_defaults;
    bool                            tls_ocsp_required;
    bool                            tls_use_os_certs;
    enum mosquitto__keyform         tls_keyform;
#endif
    bool                            want_write;
    bool                            want_connect;

    pthread_mutex_t                 callback_mutex;
    pthread_mutex_t                 log_callback_mutex;
    pthread_mutex_t                 msgtime_mutex;
    pthread_mutex_t                 out_packet_mutex;
    pthread_mutex_t                 state_mutex;
    pthread_mutex_t                 mid_mutex;
    pthread_t                       thread_id;

    bool                            clean_start;
    time_t                          session_expiry_time;
    uint32_t                        session_expiry_interval;

    char                            *socks5_host;
    uint16_t                        socks5_port;
    char                            *socks5_username;
    char                            *socks5_password;

    void                            *userdata;
    struct mosquitto_msg_data       msgs_in;
    struct mosquitto_msg_data       msgs_out;
    void (*on_pre_connect)(struct mosquitto *, void *userdata);
    void (*on_connect)(struct mosquitto *, void *userdata, int rc);
    void (*on_connect_with_flags)(struct mosquitto *, void *userdata, int rc, int flags);
    void (*on_connect_v5)(struct mosquitto *, void *userdata, int rc, int flags, const mosquitto_property *props);
    void (*on_disconnect)(struct mosquitto *, void *userdata, int rc);
    void (*on_disconnect_v5)(struct mosquitto *, void *userdata, int rc, const mosquitto_property *props);
    void (*on_publish)(struct mosquitto *, void *userdata, int mid);
    void (*on_publish_v5)(struct mosquitto *, void *userdata, int mid, int reason_code, const mosquitto_property *props);
    void (*on_message)(struct mosquitto *, void *userdata, const struct mosquitto_message *message);
    void (*on_message_v5)(struct mosquitto *, void *userdata, const struct mosquitto_message *message, const mosquitto_property *props);
    void (*on_subscribe)(struct mosquitto *, void *userdata, int mid, int qos_count, const int *granted_qos);
    void (*on_subscribe_v5)(struct mosquitto *, void *userdata, int mid, int qos_count, const int *granted_qos, const mosquitto_property *props);
    void (*on_unsubscribe)(struct mosquitto *, void *userdata, int mid);
    void (*on_unsubscribe_v5)(struct mosquitto *, void *userdata, int mid, const mosquitto_property *props);
    void (*on_unsubscribe2_v5)(struct mosquitto *, void *userdata, int mid, int reason_code_count, const int *reason_codes, const mosquitto_property *props);
    void (*on_log)(struct mosquitto *, void *userdata, int level, const char *str);

    char                            *host;
    char                            *bind_address;
    unsigned int                    reconnects;
    unsigned int                    reconnect_delay;
    unsigned int                    reconnect_delay_max;
    int                             callback_depth;
    uint16_t                        port;
    bool                            reconnect_exponential_backoff;
    bool                            request_disconnect;
    bool                            disable_socketpair;
    char                            threaded;
    struct mosquitto__packet        *out_packet_last;
    mosquitto_property              *connect_properties;

    uint8_t                         max_qos;
    uint8_t                         retain_available;
    bool                            tcp_nodelay;

    uint32_t                        events;
};

void do_client_disconnect(struct mosquitto *mosq, int reason_code, const mosquitto_property *properties);

#endif
