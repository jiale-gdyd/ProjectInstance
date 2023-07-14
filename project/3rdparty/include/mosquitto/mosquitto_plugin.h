#ifndef MOSQ_MOSQUITTO_PLUGIN_H
#define MOSQ_MOSQUITTO_PLUGIN_H

// 此标头包含在编写Mosquitto插件时使用的函数声明

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include "mosquitto.h"
#include "mosquitto_broker.h"

#define MOSQ_PLUGIN_VERSION             5       // 从版本5开始的通用插件接口
#define MOSQ_AUTH_PLUGIN_VERSION        4       // 在版本4处停止旧的仅认证接口

#define MOSQ_ACL_NONE                   0x00
#define MOSQ_ACL_READ                   0x01
#define MOSQ_ACL_WRITE                  0x02
#define MOSQ_ACL_SUBSCRIBE              0x04
#define MOSQ_ACL_UNSUBSCRIBE            0x08

struct mosquitto;

struct mosquitto_opt {
    char *key;
    char *value;
};

struct mosquitto_auth_opt {
    char *key;
    char *value;
};

struct mosquitto_acl_msg {
    const char *topic;
    const void *payload;
    long       payloadlen;
    int        qos;
    bool       retain;
};

int mosquitto_plugin_version(int supported_version_count, const int *supported_versions);

#define MOSQUITTO_PLUGIN_DECLARE_VERSION(A)                                                     \
    int mosquitto_plugin_version(int supported_version_count, const int *supported_versions)    \
    {                                                                                           \
        int i;                                                                                  \
        for (i = 0; i < supported_version_count; i++) {                                         \
            if (supported_versions[i] == (A)) {                                                 \
                return (A);                                                                     \
            }                                                                                   \
        }                                                                                       \
        return -1;                                                                              \
    }

int mosquitto_plugin_init(mosquitto_plugin_id_t *identifier, void **userdata, struct mosquitto_opt *options, int option_count);
int mosquitto_plugin_cleanup(void *userdata, struct mosquitto_opt *options, int option_count);

int mosquitto_auth_plugin_version(void);
int mosquitto_auth_plugin_init(void **user_data, struct mosquitto_opt *opts, int opt_count);
int mosquitto_auth_plugin_cleanup(void *user_data, struct mosquitto_opt *opts, int opt_count);

int mosquitto_auth_security_init(void *user_data, struct mosquitto_opt *opts, int opt_count, bool reload);
int mosquitto_auth_security_cleanup(void *user_data, struct mosquitto_opt *opts, int opt_count, bool reload);

int mosquitto_auth_acl_check(void *user_data, int access, struct mosquitto *client, const struct mosquitto_acl_msg *msg);
int mosquitto_auth_unpwd_check(void *user_data, struct mosquitto *client, const char *username, const char *password);

int mosquitto_auth_psk_key_get(void *user_data, struct mosquitto *client, const char *hint, const char *identity, char *key, int max_key_len);

int mosquitto_auth_start(void *user_data, struct mosquitto *client, const char *method, bool reauth, const void *data_in, uint16_t data_in_len, void **data_out, uint16_t *data_out_len);
int mosquitto_auth_continue(void *user_data, struct mosquitto *client, const char *method, const void *data_in, uint16_t data_in_len, void **data_out, uint16_t *data_out_len);

#ifdef __cplusplus
}
#endif

#endif
