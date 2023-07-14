#ifndef MOSQ_MOSQUITTO_H
#define MOSQ_MOSQUITTO_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __cplusplus
#include <stdbool.h>
#endif

#include <stddef.h>
#include <stdint.h>

#include "mqtt_protocol.h"

#define LIBMOSQUITTO_MAJOR              (2)
#define LIBMOSQUITTO_MINOR              (1)
#define LIBMOSQUITTO_REVISION           (0)

#define LIBMOSQUITTO_VERSION_NUMBER     (LIBMOSQUITTO_MAJOR*1000000+LIBMOSQUITTO_MINOR*1000+LIBMOSQUITTO_REVISION)

// 日志类型
#define MOSQ_LOG_NONE                   (0<<0)
#define MOSQ_LOG_INFO                   (1<<0)
#define MOSQ_LOG_NOTICE                 (1<<1)
#define MOSQ_LOG_WARNING                (1<<2)
#define MOSQ_LOG_ERR                    (1<<3)
#define MOSQ_LOG_DEBUG                  (1<<4)
#define MOSQ_LOG_SUBSCRIBE              (1<<5)
#define MOSQ_LOG_UNSUBSCRIBE            (1<<6)
#define MOSQ_LOG_WEBSOCKETS             (1<<7)
#define MOSQ_LOG_INTERNAL               (0x80000000U)
#define MOSQ_LOG_ALL                    (0xFFFFFFFFU)

// MQTT协议版本
#define MQTT_PROTOCOL_V31               (3)
#define MQTT_PROTOCOL_V311              (4)
#define MQTT_PROTOCOL_V5                (5)

// MQTT规范将客户端ID限制为最多23个字符
#define MOSQ_MQTT_ID_MAX_LENGTH         (23)

// 错误值
enum mosq_err_t {
    MOSQ_ERR_QUOTA_EXCEEDED              = -6,
    MOSQ_ERR_AUTH_DELAYED                = -5,
    MOSQ_ERR_AUTH_CONTINUE               = -4,
    MOSQ_ERR_NO_SUBSCRIBERS              = -3,
    MOSQ_ERR_SUB_EXISTS                  = -2,
    MOSQ_ERR_CONN_PENDING                = -1,
    MOSQ_ERR_SUCCESS                     = 0,
    MOSQ_ERR_NOMEM                       = 1,
    MOSQ_ERR_PROTOCOL                    = 2,
    MOSQ_ERR_INVAL                       = 3,
    MOSQ_ERR_NO_CONN                     = 4,
    MOSQ_ERR_CONN_REFUSED                = 5,
    MOSQ_ERR_NOT_FOUND                   = 6,
    MOSQ_ERR_CONN_LOST                   = 7,
    MOSQ_ERR_TLS                         = 8,
    MOSQ_ERR_PAYLOAD_SIZE                = 9,
    MOSQ_ERR_NOT_SUPPORTED               = 10,
    MOSQ_ERR_AUTH                        = 11,
    MOSQ_ERR_ACL_DENIED                  = 12,
    MOSQ_ERR_UNKNOWN                     = 13,
    MOSQ_ERR_ERRNO                       = 14,
    MOSQ_ERR_EAI                         = 15,
    MOSQ_ERR_PROXY                       = 16,
    MOSQ_ERR_PLUGIN_DEFER                = 17,
    MOSQ_ERR_MALFORMED_UTF8              = 18,
    MOSQ_ERR_KEEPALIVE                   = 19,
    MOSQ_ERR_LOOKUP                      = 20,
    MOSQ_ERR_MALFORMED_PACKET            = 21,
    MOSQ_ERR_DUPLICATE_PROPERTY          = 22,
    MOSQ_ERR_TLS_HANDSHAKE               = 23,
    MOSQ_ERR_QOS_NOT_SUPPORTED           = 24,
    MOSQ_ERR_OVERSIZE_PACKET             = 25,
    MOSQ_ERR_OCSP                        = 26,
    MOSQ_ERR_TIMEOUT                     = 27,
    MOSQ_ERR_ALREADY_EXISTS              = 31,
    MOSQ_ERR_PLUGIN_IGNORE               = 32,
    MOSQ_ERR_HTTP_BAD_ORIGIN             = 33,
    MOSQ_ERR_UNSPECIFIED                 = 128,
    MOSQ_ERR_IMPLEMENTATION_SPECIFIC     = 131,
    MOSQ_ERR_CLIENT_IDENTIFIER_NOT_VALID = 133,
    MOSQ_ERR_BAD_USERNAME_OR_PASSWORD    = 134,
    MOSQ_ERR_SERVER_UNAVAILABLE          = 136,
    MOSQ_ERR_SERVER_BUSY                 = 137,
    MOSQ_ERR_BANNED                      = 138,
    MOSQ_ERR_BAD_AUTHENTICATION_METHOD   = 140,
    MOSQ_ERR_SESSION_TAKEN_OVER          = 142,
    MOSQ_ERR_RECEIVE_MAXIMUM_EXCEEDED    = 147,
    MOSQ_ERR_TOPIC_ALIAS_INVALID         = 148,
    MOSQ_ERR_ADMINISTRATIVE_ACTION       = 152,
    MOSQ_ERR_RETAIN_NOT_SUPPORTED        = 154,
    MOSQ_ERR_CONNECTION_RATE_EXCEEDED    = 159,
};

// 选项值
enum mosq_opt_t {
    MOSQ_OPT_PROTOCOL_VERSION      = 1,
    MOSQ_OPT_SSL_CTX               = 2,
    MOSQ_OPT_SSL_CTX_WITH_DEFAULTS = 3,
    MOSQ_OPT_RECEIVE_MAXIMUM       = 4,
    MOSQ_OPT_SEND_MAXIMUM          = 5,
    MOSQ_OPT_TLS_KEYFORM           = 6,
    MOSQ_OPT_TLS_ENGINE            = 7,
    MOSQ_OPT_TLS_ENGINE_KPASS_SHA1 = 8,
    MOSQ_OPT_TLS_OCSP_REQUIRED     = 9,
    MOSQ_OPT_TLS_ALPN              = 10,
    MOSQ_OPT_TCP_NODELAY           = 11,
    MOSQ_OPT_BIND_ADDRESS          = 12,
    MOSQ_OPT_TLS_USE_OS_CERTS      = 13,
    MOSQ_OPT_DISABLE_SOCKETPAIR    = 14,
    MOSQ_OPT_TRANSPORT             = 15,
    MOSQ_OPT_HTTP_PATH             = 16,
    MOSQ_OPT_HTTP_HEADER_SIZE      = 17,
};

enum mosq_transport_t {
    MOSQ_T_TCP        = 1,
    MOSQ_T_WEBSOCKETS = 2,
};

struct mosquitto_message {
    int  mid;
    char *topic;
    void *payload;
    int  payloadlen;
    int  qos;
    bool retain;
};

struct libmosquitto_will {
    char *topic;
    void *payload;
    int  payloadlen;
    int  qos;
    bool retain;
};

struct libmosquitto_auth {
    char *username;
    char *password;
};

struct libmosquitto_tls {
    char *cafile;
    char *capath;
    char *certfile;
    char *keyfile;
    char *ciphers;
    char *tls_version;
    int  (*pw_callback)(char *buf, int size, int rwflag, void *userdata);
    int  cert_reqs;
};

struct mosquitto;
typedef struct mqtt5__property mosquitto_property;

struct mosquitto_message_v5{
    void               *payload;
    char               *topic;
    mosquitto_property *properties;
    uint32_t           payloadlen;
    uint8_t            qos;
    bool               retain;
    uint8_t            padding[2];
};

/**
 * 可用于获取mosquitto库的版本信息。这使应用程序可以将库版本与使用LIBMOSQUITTO_MAJOR，LIBMOSQUITTO_MINOR和LIBMOSQUITTO_REVISION定义的编译版本进行比较
 *
 * @param major     整数指针。如果不为NULL，则将在此变量中返回库的主版本
 * @param minor     整数指针。如果不为NULL，则将在此变量中返回库的次要版本
 * @param revision  整数指针。如果不为NULL，则将在此变量中返回库的修订版
 *
 * @return  int            LIBMOSQUITTO_VERSION_NUMBER - 这是一个基于主要，次要和修订版值的唯一编号
 */
int mosquitto_lib_version(int *major, int *minor, int *revision);

/**
 * 必须在其他任何mosquitto功能之前调用。该函数不是线程安全的
 *
 * @return
 *      MOSQ_ERR_SUCCESS - 成功
 */
int mosquitto_lib_init(void);

/**
 * 调用与该库关联的资源释放
 *
 * @return
 *      MOSQ_ERR_SUCCESS - 总是返回该值
 */
int mosquitto_lib_cleanup(void);

/**
 * 用于释放与mosquitto客户端实例关联的内存
 * 
 * @param mosq  指向释放的struct mosquitto指针
 */
void mosquitto_destroy(struct mosquitto *mosq);

/**
 * 创建一个新的mosquitto客户端实例
 * 
 * @param id                用作客户端ID的字符串。如果为NULL，将生成一个随机的客户端ID。如果id为NULL，则clean_session必须为true
 * @param clean_session     设置为true以指示代理在断开连接时清除所有消息和订阅，设置为false以指示其保留它们。请注意，客户端将永远不会在断开连接时丢弃自己的传出消息。
 *                          调用<mosquitto_connect>或<mosquitto_reconnect>将导致重新发送消息。使用<mosquitto_reinitialise>将客户端重置为其原始状态。如果id参数为NULL，
 *                          则必须将其设置为true
 * @param obj               用户指针，将作为参数传递给指定的任何回调
 * 
 * @return
 *      指向成功的结构mosquitto的指针。失败时为NULL。询问errno以确定失败的原因:
 *      ENOMEM - 内存不足
 *      EINVAL - 输入参数无效
 */
struct mosquitto *mosquitto_new(const char *id, bool clean_session, void *obj);

/**
 * 此功能允许重新使用现有的mosquitto客户端。调用mosquitto实例以关闭所有打开的网络连接，释放内存并使用新参数重新初始化客户端。最终结果与<mosquitto_new>的输出相同
 * 
 * @param mosq              有效的mosquitto实例
 * @param id                用作客户端ID的字符串。如果为NULL，将生成一个随机的客户端ID。如果id为NULL，则clean_session必须为true
 * @param clean_session     设置为true以指示代理在断开连接时清除所有消息和订阅，设置为false以指示其保留它们。如果id参数为NULL，则必须将其设置为true
 * @param obj               用户指针，将作为参数传递给指定的任何回调
 * 
 * @return
 *      MOSQ_ERR_SUCCESS - 成功
 *      MOSQ_ERR_INVAL   - 如果输入参数无效
 *      MOSQ_ERR_NOMEM   - 如果发生内存不足的情况
 */
int mosquitto_reinitialise(struct mosquitto *mosq, const char *id, bool clean_session, void *obj);

/**
 * 配置mosquitto实例的遗嘱信息。默认情况下，客户没有遗嘱。必须在调用<mosquitto_connect>之前调用它。对于使用所有MQTT协议版本的客户端，使用此功能是有效的。
 * 如果需要设置MQTT v5 Will属性，请改用<mosquitto_will_set_v5>
 * 
 * @param mosq          有效的mosquitto实例
 * @param topic         发布遗嘱的主题
 * @param payloadlen    有效负载的大小(字节)。有效值在0到268,435,455之间
 * @param payload       指向要发送的数据的指针。如果payloadlen>0，则它必须是有效的存储位置
 * @param qos           整数0、1或2，表示将用于遗嘱的服务质量
 * @param retain        设置为true以使遗嘱成为保留消息
 * 
 * @return
 *      MOSQ_ERR_SUCCESS        - 成功
 *      MOSQ_ERR_INVAL          - 如果输入参数无效
 *      MOSQ_ERR_NOMEM          - 如果发生内存不足的情况
 *      MOSQ_ERR_PAYLOAD_SIZE   - 如果payloadlen太大
 *      MOSQ_ERR_MALFORMED_UTF8 - 如果主题无效，则为UTF-8
 */
int mosquitto_will_set(struct mosquitto *mosq, const char *topic, int payloadlen, const void *payload, int qos, bool retain);

/**
 * 使用附加的属性配置mosquitto实例的遗嘱信息。默认情况下，客户没有遗嘱。必须在调用<mosquitto_connect>之前调用它。如果mosquitto实例`mosq`使用的是MQTT v5，则`properties`参数将应用于遗嘱。
 * 对于MQTT v3.1.1及以下版本，`properties`参数将被忽略。在创建客户端之后，将其设置为立即使用MQTT v5: mosquitto_int_option(mosq, MOSQ_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V5);
 * 
 * @param mosq          有效的mosquitto实例
 * @param topic         发布遗嘱的主题
 * @param payloadlen    有效负载的大小(字节)。有效值在0到268,435,455之间
 * @param payload       指向要发送的数据的指针。如果payloadlen>0，则它必须是有效的存储位置
 * @param qos           整数0、1或2，表示将用于遗嘱的服务质量
 * @param retain        设置为true以使遗嘱成为保留消息
 * @param properties    MQTT 5属性列表。可以为NULL。仅在成功时，属性列表将成为libmosquitto的属性，一旦调用此函数，该列表将被库释放。应用程序必须在出错时释放属性列表
 * 
 * @return
 *      MOSQ_ERR_SUCCESS            - 成功
 *      MOSQ_ERR_INVAL              - 如果输入参数无效
 *      MOSQ_ERR_NOMEM              - 如果发生内存不足的情况
 *      MOSQ_ERR_PAYLOAD_SIZE       - 如果payloadlen太大
 *      MOSQ_ERR_MALFORMED_UTF8     - 如果主题无效，则为UTF-8
 *      MOSQ_ERR_NOT_SUPPORTED      - 如果属性不为NULL并且客户端未使用MQTT v5
 *      MOSQ_ERR_PROTOCOL           - 如果财产对遗嘱无效
 *      MOSQ_ERR_DUPLICATE_PROPERTY - 如果属性在禁止的地方重复
 */
int mosquitto_will_set_v5(struct mosquitto *mosq, const char *topic, int payloadlen, const void *payload, int qos, bool retain, mosquitto_property *properties);

/**
 * 删除以前配置的遗嘱。必须在调用<mosquitto_connect>之前调用它
 * 
 * @param mosq  有效的mosquitto实例
 * 
 * @return
 *      MOSQ_ERR_SUCCESS - 成功
 *      MOSQ_ERR_INVAL   - 如果输入参数无效
 */
int mosquitto_will_clear(struct mosquitto *mosq);

/**
 * 配置mosquitto实例的用户名和密码。默认情况下，不会发送用户名或密码。对于v3.1和v3.1.1客户端，如果username为NULL，则password参数将被忽略。必须在调用<mosquitto_connect>之前调用它
 * 
 * @param mosq      有效的mosquitto实例
 * @param username  要以字符串形式发送的用户名，或者为NULL以禁用身份验证
 * @param password  以字符串形式发送的密码。当用户名有效时设置为NULL，以便仅发送用户名
 * 
 * @return
 *      MOSQ_ERR_SUCCESS - 成功
 *      MOSQ_ERR_INVAL   - 如果输入参数无效
 *      MOSQ_ERR_NOMEM   - 如果发生内存不足的情况
 */
int mosquitto_username_pw_set(struct mosquitto *mosq, const char *username, const char *password);

/**
 * 连接到MQTT代理。对于使用所有MQTT协议版本的客户端，使用此功能是有效的。如果需要设置MQTT v5 CONNECT属性，请改用<mosquitto_connect_bind_v5>
 * 
 * @param mosq          有效的mosquitto实例
 * @param host          要连接的代理的主机名或IP地址
 * @param port          要连接的网络端口。通常是1883
 * @param keepalive     如果代理服务器在该时间内没有其他消息交换过，则代理应该向客户端发送PING消息的秒数
 * 
 * @return 
 *      MOSQ_ERR_SUCCESS - 成功
 *      MOSQ_ERR_INVAL   - 如果输入参数无效，则可以是以下任何一个: mosq == NULL、host == NULL、port < 0、keepalive < 5
 *      MOSQ_ERR_ERRNO   - 如果系统调用返回错误。变量errno包含错误代码。在可用的地方使用strerror_r()
 */
int mosquitto_connect(struct mosquitto *mosq, const char *host, int port, int keepalive);

/**
 * 连接到MQTT代理。通过添加bind_address参数，这扩展了<mosquitto_connect>的功能。如果您需要限制通过特定接口的网络通信，请使用此功能
 * 
 * @param mosq          有效的mosquitto实例
 * @param host          要连接的代理的主机名或IP地址
 * @param port          要连接的网络端口。通常是1883
 * @param keepalive     如果代理服务器在该时间内没有其他消息交换过，则代理应该向客户端发送PING消息的秒数
 * @param bind_address  要绑定的本地网络接口的主机名或IP地址
 * 
 * @return
 *      MOSQ_ERR_SUCCESS - 成功
 *      MOSQ_ERR_INVAL   - 如果输入参数无效
 *      MOSQ_ERR_ERRNO   - 如果系统调用返回错误。变量errno包含错误代码。在可用的地方使用strerror_r()
 */
int mosquitto_connect_bind(struct mosquitto *mosq, const char *host, int port, int keepalive, const char *bind_address);

/**
 * 连接到MQTT代理。通过添加bind_address参数和MQTT v5属性，这扩展了<mosquitto_connect>的功能。如果您需要限制通过特定接口的网络通信，请使用此功能。
 * 使用例如<mosquitto_property_add_string>和类似的属性来创建属性列表，然后将它们附加到此发布中。需要使用<mosquitto_property_free_all>释放属性。
 * 如果mosquitto实例`mosq`使用的是MQTT v5，则`properties`参数将应用于CONNECT消息。对于MQTT v3.1.1及以下版本，`properties`参数将被忽略。在创建客户端之后，将其设置为立即使用MQTT v5:
 * mosquitto_int_option(mosq, MOSQ_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V5);
 * 
 * @param mosq          有效的mosquitto实例
 * @param host          要连接的代理的主机名或IP地址
 * @param port          要连接的网络端口。通常是1883
 * @param keepalive     如果代理服务器在该时间内没有其他消息交换过，则代理应该向客户端发送PING消息的秒数
 * @param bind_address  要绑定的本地网络接口的主机名或IP地址
 * @param properties    连接的MQTT 5属性(而不是Will)
 * 
 * @return
 *      MOSQ_ERR_SUCCESS            - 成功
 *      MOSQ_ERR_INVAL              - 如果输入参数无效，则可以是以下任何一个: mosq == NULL、host == NULL、port < 0、keepalive < 5
 *      MOSQ_ERR_ERRNO              - 如果系统调用返回错误。变量errno包含错误代码。在可用的地方使用strerror_r()
 *      MOSQ_ERR_DUPLICATE_PROPERTY - 如果属性在禁止的地方重复
 *      MOSQ_ERR_PROTOCOL           - 如果任何属性对于与CONNECT一起使用无效
 */
int mosquitto_connect_bind_v5(struct mosquitto *mosq, const char *host, int port, int keepalive, const char *bind_address, const mosquitto_property *properties);

/**
 * 连接到MQTT代理。这是一个非阻塞调用。如果使用<mosquitto_connect_async>，则客户端必须使用线程接口<mosquitto_loop_start>。如果需要使用<mosquitto_loop>，
 * 则必须使用<mosquitto_connect>连接客户端。可以在<mosquitto_loop_start>之前或之后调用
 * 
 * @param mosq          有效的mosquitto实例
 * @param host          要连接的代理的主机名或IP地址
 * @param port          要连接的网络端口。通常是1883
 * @param keepalive     如果代理服务器在该时间内没有其他消息交换过，则代理应该向客户端发送PING消息的秒数
 * 
 * @return
 *      MOSQ_ERR_SUCCESS - 成功
 *      MOSQ_ERR_INVAL   - 如果输入参数无效
 *      MOSQ_ERR_ERRNO   - 如果系统调用返回错误。变量errno包含错误代码。在可用的地方使用strerror_r()
 */
int mosquitto_connect_async(struct mosquitto *mosq, const char *host, int port, int keepalive);

/**
 * 连接到MQTT代理。这是一个非阻塞调用。如果使用<mosquitto_connect_bind_async>，则客户端必须使用线程接口<mosquitto_loop_start>。如果需要使用<mosquitto_loop>，
 * 则必须使用<mosquitto_connect>连接客户端。通过添加bind_address参数，这扩展了<mosquitto_connect_async>的功能。如果您需要限制通过特定接口的网络通信，请使用此功能。
 * 可以在<mosquitto_loop_start>之前或之后调用。
 * 
 * @param mosq          有效的mosquitto实例
 * @param host          要连接的代理的主机名或IP地址
 * @param port          要连接的网络端口。通常是1883
 * @param keepalive     如果代理服务器在该时间内没有其他消息交换过，则代理应该向客户端发送PING消息的秒数
 * @param bind_address
 * 
 * @return
 *      MOSQ_ERR_SUCCESS - 成功
 *      MOSQ_ERR_INVAL   - 如果输入参数无效，则可以是以下任何一个: mosq == NULL、host == NULL、port < 0、keepalive < 5
 *      MOSQ_ERR_ERRNO   - 如果系统调用返回错误。变量errno包含错误代码。在可用的地方使用strerror_r()
 */
int mosquitto_connect_bind_async(struct mosquitto *mosq, const char *host, int port, int keepalive, const char *bind_address);

/**
 * 连接到MQTT代理。如果将`host`设置为`example.com`，则此调用将尝试检索`_secure-mqtt._tcp.example.com`或`_mqtt._tcp.example.com`的DNS SRV记录，以发现实际的主机连接。
 * DNS SRV支持通常不编译到libmosquitto中，不建议使用此调用
 * 
 * @param mosq          有效的mosquitto实例
 * @param host          搜索SRV记录的主机名
 * @param keepalive     如果代理服务器在该时间内没有其他消息交换过，则代理应该向客户端发送PING消息的秒数
 * @param bind_address  要绑定的本地网络接口的主机名或IP地址
 * 
 * @return
 *      MOSQ_ERR_SUCCESS - 成功
 *      MOSQ_ERR_INVAL   - 如果输入参数无效，则可以是以下任何一个: mosq == NULL、host == NULL、port < 0、keepalive < 5
 *      MOSQ_ERR_ERRNO   - 如果系统调用返回错误。变量errno包含错误代码。在可用的地方使用strerror_r()
 */
int mosquitto_connect_srv(struct mosquitto *mosq, const char *host, int keepalive, const char *bind_address);

/**
 * 重新连接到代理。此功能提供了一种在连接丢失后重新连接到代理的简便方法。它使用<mosquitto_connect>调用中提供的值。不能在<mosquitto_connect>之前调用它
 * 
 * @param mosq  有效的mosquitto实例
 * 
 * @return:
 *      MOSQ_ERR_SUCCESS - 成功
 *      MOSQ_ERR_INVAL   - 如果输入参数无效
 *      MOSQ_ERR_NOMEM   - 如果发生内存不足的情况
 *      MOSQ_ERR_ERRNO   - 如果系统调用返回错误。变量errno包含错误代码。在可用的地方使用strerror_r()
 */
int mosquitto_reconnect(struct mosquitto *mosq);

/**
 * 重新连接到代理。<mosquitto_reconnect>的非阻塞版本。此功能提供了一种在连接丢失后重新连接到代理的简便方法。它使用<mosquitto_connect>或<mosquitto_connect_async>调用中提供的值。
 * 不能在<mosquitto_connect>之前调用它
 * 
 * @param mosq  有效的mosquitto实例
 * 
 * @return:
 *      MOSQ_ERR_SUCCESS - 成功
 *      MOSQ_ERR_INVAL   - 如果输入参数无效
 *      MOSQ_ERR_NOMEM   - 如果发生内存不足的情况
 *      MOSQ_ERR_ERRNO   - 如果系统调用返回错误。变量errno包含错误代码。在可用的地方使用strerror_r()
 */
int mosquitto_reconnect_async(struct mosquitto *mosq);

/**
 * 与代理断开连接。对于使用所有MQTT协议版本的客户端，使用此功能是有效的。如果需要设置MQTT v5 DISCONNECT属性，请改用<mosquitto_disconnect_v5>
 * 
 * @param mosq  有效的mosquitto实例
 * 
 * @return
 *      MOSQ_ERR_SUCCESS - 成功
 *      MOSQ_ERR_INVAL   - 如果输入参数无效
 *      MOSQ_ERR_NO_CONN - 如果客户端未连接到代理
 */
int mosquitto_disconnect(struct mosquitto *mosq);

/**
 * 通过附加的MQTT属性与代理断开连接。使用例如<mosquitto_property_add_string>和类似的属性来创建属性列表，然后将它们附加到此发布中。需要使用<mosquitto_property_free_all>释放属性。
 * 如果mosquitto实例`mosq`使用的是MQTT v5，则`properties`参数将应用于DISCONNECT消息。对于MQTT v3.1.1及以下版本，`properties`参数将被忽略。在创建客户端之后，将其设置为立即使用MQTT v5:
 * mosquitto_int_option(mosq, MOSQ_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V5);
 * 
 * @param mosq          有效的mosquitto实例
 * @param reason_code   断开连接原因代码
 * @param properties    有效的mosquitto_property列表，或NULL
 * 
 * @return
 *      MOSQ_ERR_SUCCESS            - 成功
 *      MOSQ_ERR_INVAL              - 如果输入参数无效
 *      MOSQ_ERR_NO_CONN            - 如果客户端未连接到代理
 *      MOSQ_ERR_DUPLICATE_PROPERTY - 如果属性在禁止的地方重复
 *      MOSQ_ERR_PROTOCOL           - 如果任何属性对于与DISCONNECT一起使用无效
 */
int mosquitto_disconnect_v5(struct mosquitto *mosq, int reason_code, const mosquitto_property *properties);

/**
 * 发布有关给定主题的消息。对于使用所有MQTT协议版本的客户端，使用此功能是有效的。如果需要设置MQTT v5 PUBLISH属性，请改用<mosquitto_publish_v5>
 * 
 * @param mosq          有效的mosquitto实例
 * @param mid           指向int的指针。如果不为NULL，则函数会将其设置为此特定消息的消息ID。然后可以将其与publish回调一起使用，以确定何时发送消息。
 *                      请注意，尽管MQTT协议不对QoS = 0的消息使用消息ID，但libmosquitto为其分配消息ID，以便可以使用此参数对其进行跟踪
 * @param topic         要发布的以NULL结尾的字符串主题
 * @param payloadlen    有效负载的大小(字节)。有效值在0到268,435,455之间
 * @param payload       指向要发送的数据的指针。如果payloadlen>0，则它必须是有效的存储位置
 * @param qos           整数值0、1或2，指示要用于消息的服务质量
 * @param retain        设置为true以保留消息
 * 
 * @return
 *      MOSQ_ERR_SUCCESS           - 成功
 *      MOSQ_ERR_INVAL             - 如果输入参数无效
 *      MOSQ_ERR_NOMEM             - 如果发生内存不足的情况
 *      MOSQ_ERR_NO_CONN           - 如果客户端未连接到代理
 *      MOSQ_ERR_PROTOCOL          - 与代理进行通信时是否存在协议错误
 *      MOSQ_ERR_PAYLOAD_SIZE      - 如果payloadlen太大
 *      MOSQ_ERR_MALFORMED_UTF8    - 如果主题无效，则为UTF-8
 *      MOSQ_ERR_QOS_NOT_SUPPORTED - 如果QoS大于代理支持的QoS
 *      MOSQ_ERR_OVERSIZE_PACKET   - 如果结果包大于代理支持的包
 */
int mosquitto_publish(struct mosquitto *mosq, int *mid, const char *topic, int payloadlen, const void *payload, int qos, bool retain);

/**
 * 使用附加的MQTT属性在给定主题上发布消息。使用例如 <mosquitto_property_add_string>和类似的属性来创建属性列表，然后将它们附加到此发布中。需要使用<mosquitto_property_free_all>释放属性。
 * 如果mosquitto实例`mosq`使用的是MQTT v5，则`properties`参数将应用于发布消息。对于MQTT v3.1.1及以下版本，`properties`参数将被忽略。在创建客户端之后，将其设置为立即使用MQTT v5:
 * mosquitto_int_option(mosq, MOSQ_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V5);
 * 
 * @param mosq          有效的mosquitto实例
 * @param mid           指向int的指针。如果不为NULL，则函数会将其设置为此特定消息的消息ID。然后可以将其与publish回调一起使用，以确定何时发送消息。
 *                      请注意，尽管MQTT协议不对QoS = 0的消息使用消息ID，但libmosquitto为其分配消息ID，以便可以使用此参数对其进行跟踪
 * @param topic         要发布的以NULL结尾的字符串主题
 * @param payloadlen    有效负载的大小(字节)。有效值在0到268,435,455之间
 * @param payload       指向要发送的数据的指针。如果payloadlen>0，则它必须是有效的存储位置
 * @param qos           整数值0、1或2，指示要用于消息的服务质量
 * @param retain        设置为true以保留消息
 * @param properties    有效的mosquitto_property列表，或NULL
 * 
 * @return
 *      MOSQ_ERR_SUCCESS            - 成功
 *      MOSQ_ERR_INVAL              - 如果输入参数无效
 *      MOSQ_ERR_NOMEM              - 如果发生内存不足的情况
 *      MOSQ_ERR_NO_CONN            - 如果客户端未连接到代理
 *      MOSQ_ERR_PROTOCOL           - 与代理进行通信时是否存在协议错误
 *      MOSQ_ERR_PAYLOAD_SIZE       - 如果payloadlen太大
 *      MOSQ_ERR_MALFORMED_UTF8     - 如果主题无效，则为UTF-8
 *      MOSQ_ERR_DUPLICATE_PROPERTY - 如果属性在禁止的地方重复
 *      MOSQ_ERR_PROTOCOL           - 如果任何属性对于与PUBLISH一起使用无效
 *      MOSQ_ERR_QOS_NOT_SUPPORTED  - 如果QoS大于代理支持的QoS
 *      MOSQ_ERR_OVERSIZE_PACKET    - 如果结果包大于代理支持的包
 */
int mosquitto_publish_v5(struct mosquitto *mosq, int *mid, const char *topic, int payloadlen, const void *payload, int qos, bool retain, const mosquitto_property *properties);

/**
 * 订阅主题。对于使用所有MQTT协议版本的客户端，使用此功能是有效的。如果需要设置MQTT v5 SUBSCRIBE属性，请改用<mosquitto_subscribe_v5>
 * 
 * @param mosq  有效的mosquitto实例
 * @param mid   指向int的指针。如果不为NULL，则函数会将其设置为此特定消息的消息ID。然后可以将其与订阅回调一起使用，以确定何时发送消息。
 * @param sub   订阅模式，即订阅主题 - 不得为NULL或空字符串
 * @param qos   此订阅请求的服务质量
 * 
 * @return
 *      MOSQ_ERR_SUCCESS         - 成功
 *      MOSQ_ERR_INVAL           - 如果输入参数无效
 *      MOSQ_ERR_NOMEM           - 如果发生内存不足的情况
 *      MOSQ_ERR_NO_CONN         - 如果客户端未连接到代理
 *      MOSQ_ERR_MALFORMED_UTF8  - 如果主题无效，则为UTF-8
 *      MOSQ_ERR_OVERSIZE_PACKET - 如果结果包大于代理支持的包
 */
int mosquitto_subscribe(struct mosquitto *mosq, int *mid, const char *sub, int qos);

/**
 * 订阅具有附加的MQTT属性的主题。使用例如 <mosquitto_property_add_string>和类似的属性来创建属性列表，然后将它们附加到此发布中。需要使用<mosquitto_property_free_all>释放属性。
 * 如果mosquitto实例`mosq`使用的是MQTT v5，则`properties`参数将应用于发布消息。对于MQTT v3.1.1及以下版本，`properties`参数将被忽略。在创建客户端之后，将其设置为立即使用MQTT v5:
 * mosquitto_int_option(mosq, MOSQ_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V5);
 * 
 * @param mosq          有效的mosquitto实例
 * @param mid           指向int的指针。如果不为NULL，则函数会将其设置为此特定消息的消息ID。然后可以将其与订阅回调一起使用，以确定何时发送消息
 * @param sub           订阅模式，即订阅主题 - 不得为NULL或空字符串
 * @param qos           此订阅请求的服务质量
 * @param options       适用于此订阅的选项，或在一起。设置为0以使用默认选项，否则从<mqtt5_sub_options>列表中选择
 * @param properties    有效的mosquitto_property列表，或NULL
 * 
 * @return
 *      MOSQ_ERR_SUCCESS            - 成功
 *      MOSQ_ERR_INVAL              - 如果输入参数无效
 *      MOSQ_ERR_NOMEM              - 如果发生内存不足的情况
 *      MOSQ_ERR_NO_CONN            - 如果客户端未连接到代理
 *      MOSQ_ERR_MALFORMED_UTF8     - 如果主题无效，则为UTF-8
 *      MOSQ_ERR_DUPLICATE_PROPERTY - 如果属性在禁止的地方重复
 *      MOSQ_ERR_PROTOCOL           - 如果任何属性对于与SUBSCRIBE一起使用无效
 *      MOSQ_ERR_OVERSIZE_PACKET    - 如果结果包大于代理支持的包
 */
int mosquitto_subscribe_v5(struct mosquitto *mosq, int *mid, const char *sub, int qos, int options, const mosquitto_property *properties);

/**
 * 订阅多个主题
 * 
 * @param mosq          有效的mosquitto实例
 * @param mid           指向int的指针。如果不为NULL，则函数会将其设置为此特定消息的消息ID。然后可以将其与订阅回调一起使用，以确定何时发送消息
 * @param sub_count     订阅数
 * @param sub           sub_count指针数组，每个指针都指向订阅字符串。"char *const *const"数据类型可确保指针数组和它们指向的字符串均不可更改。如果您不熟悉此功能，
 *                      可以将其视为更安全的"char **"，等效于简单字符串指针的"const char *"。每个字符串不得为NULL或空字符串
 * @param qos           每个订阅请求的服务质量
 * @param options       适用于此订阅的选项，或在一起。此参数不用于MQTT v3订阅。设置为0以使用默认选项，否则从<mqtt5_sub_options>列表中选择
 * @param properties    有效的mosquitto_property列表，或NULL。仅与MQTT v5客户端一起使用
 * 
 * @return
 *      MOSQ_ERR_SUCCESS         - 成功
 *      MOSQ_ERR_INVAL           - 如果输入参数无效
 *      MOSQ_ERR_NOMEM           - 如果发生内存不足的情况
 *      MOSQ_ERR_NO_CONN         - 如果客户端未连接到代理
 *      MOSQ_ERR_MALFORMED_UTF8  - 如果主题无效，则为UTF-8
 *      MOSQ_ERR_OVERSIZE_PACKET - 如果结果包大于代理支持的包
 */
int mosquitto_subscribe_multiple(struct mosquitto *mosq, int *mid, int sub_count, char *const *const sub, int qos, int options, const mosquitto_property *properties);

/**
 * 退订主题
 * 
 * @param mosq  有效的mosquitto实例
 * @param mid   指向int的指针。如果不为NULL，则函数会将其设置为此特定消息的消息ID。然后可以将其与取消订阅回调一起使用，以确定何时发送消息
 * @param sub   取消订阅模式，即取消订阅的主题 - 不得为NULL或空字符串
 * 
 * @return
 *      MOSQ_ERR_SUCCESS         - 成功
 *      MOSQ_ERR_INVAL           - 如果输入参数无效
 *      MOSQ_ERR_NOMEM           - 如果发生内存不足的情况
 *      MOSQ_ERR_NO_CONN         - 如果客户端未连接到代理
 *      MOSQ_ERR_MALFORMED_UTF8  - 如果主题无效，则为UTF-8
 *      MOSQ_ERR_OVERSIZE_PACKET - 如果结果包大于代理支持的包
 */
int mosquitto_unsubscribe(struct mosquitto *mosq, int *mid, const char *sub);

/**
 * 取消订阅带有附加的MQTT属性的主题。对于使用所有MQTT协议版本的客户端，使用此功能是有效的。如果需要设置MQTT v5 UNSUBSCRIBE属性，请改用<mosquitto_unsubscribe_v5>。
 * 使用例如<mosquitto_property_add_string>和类似的属性来创建属性列表，然后将它们附加到此发布中。需要使用<mosquitto_property_free_all>释放属性。如果mosquitto实例`mosq`使用的是MQTT v5，
 * 则`properties`参数将应用于发布消息。对于MQTT v3.1.1及以下版本，`properties`参数将被忽略。在创建客户端之后，将其设置为立即使用MQTT v5: mosquitto_int_option(mosq, MOSQ_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V5);
 * 
 * @param mosq          有效的mosquitto实例
 * @param mid           指向int的指针。如果不为NULL，则函数会将其设置为此特定消息的消息ID。然后可以将其与取消订阅回调一起使用，以确定何时发送消息
 * @param sub           取消订阅模式，即取消订阅的主题 - 不得为NULL或空字符串
 * @param properties    有效的mosquitto_property列表，或NULL。仅与MQTTv5客户端一起使用
 * 
 * @return
 *      MOSQ_ERR_SUCCESS            - 成功
 *      MOSQ_ERR_INVAL              - 如果输入参数无效
 *      MOSQ_ERR_NOMEM              - 如果发生内存不足的情况
 *      MOSQ_ERR_NO_CONN            - 如果客户端未连接到代理
 *      MOSQ_ERR_MALFORMED_UTF8     - 如果主题无效，则为UTF-8
 *      MOSQ_ERR_DUPLICATE_PROPERTY - 如果属性在禁止的地方重复
 *      MOSQ_ERR_PROTOCOL           - 如果任何属性对于与SUBSCRIBE一起使用无效
 *      MOSQ_ERR_OVERSIZE_PACKET    - 如果结果包大于代理支持的包
 */
int mosquitto_unsubscribe_v5(struct mosquitto *mosq, int *mid, const char *sub, const mosquitto_property *properties);

/**
 * 退订多个主题
 * 
 * @param mosq          有效的mosquitto实例
 * @param mid           指向int的指针。如果不为NULL，则函数会将其设置为此特定消息的消息ID。然后可以将其与取消订阅回调一起使用，以确定何时发送消息
 * @param sub_count     取消订阅的数量
 * @param sub           sub_count指针数组，每个指针都指向订阅字符串。"char *const *const"数据类型可确保指针数组和它们指向的字符串均不可更改。如果您不熟悉此功能，
 *                      可以将其视为更安全的"char **"，等效于简单字符串指针的"const char *"。每个sub不得为NULL或空字符串
 * @param properties    有效的mosquitto_property列表，或NULL。仅与MQTT v5客户端一起使用
 * 
 * @return
 *      MOSQ_ERR_SUCCESS         - 成功
 *      MOSQ_ERR_INVAL           - 如果输入参数无效
 *      MOSQ_ERR_NOMEM           - 如果发生内存不足的情况
 *      MOSQ_ERR_NO_CONN         - 如果客户端未连接到代理
 *      MOSQ_ERR_MALFORMED_UTF8  - 如果主题无效，则为UTF-8
 *      MOSQ_ERR_OVERSIZE_PACKET - 如果结果包大于代理支持的包
 */
int mosquitto_unsubscribe_multiple(struct mosquitto *mosq, int *mid, int sub_count, char *const *const sub, const mosquitto_property *properties);

/**
 * 将mosquitto消息的内容复制到另一条消息。对于保留on_message()回调中收到的消息很有用
 * 
 * @param dst   指向要复制到的有效mosquitto_message结构的指针
 * @param src   指向要复制的有效mosquitto_message结构的指针
 * 
 * @return
 *      MOSQ_ERR_SUCCESS - 成功
 *      MOSQ_ERR_INVAL   - 如果输入参数无效
 *      MOSQ_ERR_NOMEM   - 如果发生内存不足的情况
 */
int mosquitto_message_copy(struct mosquitto_message *dst, const struct mosquitto_message *src);

/**
 * 完全释放mosquitto_message结构
 * 
 * @param message   指向mosquitto_message指针的指针
 */
void mosquitto_message_free(struct mosquitto_message **message);

/**
 * 释放mosquitto_message结构内容，使该结构不受影响
 * 
 * @param message   指向mosquitto_message结构的指针以释放其内容
 */
void mosquitto_message_free_contents(struct mosquitto_message *message);

/**
 * 这是线程客户端接口的一部分。调用一次即可启动一个新线程来处理网络流量。这提供了自己重复调用<mosquitto_loop>的替代方法
 * 
 * @param mosq  有效的mosquitto实例
 * 
 * @return
 *      MOSQ_ERR_SUCCESS       - 成功
 *      MOSQ_ERR_INVAL         - 如果输入参数无效
 *      MOSQ_ERR_NOT_SUPPORTED - 如果没有线程支持
 */
int mosquitto_loop_start(struct mosquitto *mosq);

/**
 * 这是线程客户端接口的一部分。调用一次可停止先前使用<mosquitto_loop_start>创建的网络线程。该调用将一直阻塞，直到网络线程结束。为了使网络线程结束，
 * 您必须先前已调用<mosquitto_disconnect>或将force参数设置为true
 * 
 * @param mosq      有效的mosquitto实例
 * @param force     设置为true强制取消线程。如果为false，则必须已经调用<mosquitto_disconnect>
 * 
 * @return
 *      MOSQ_ERR_SUCCESS       - 成功
 *      MOSQ_ERR_INVAL         - 如果输入参数无效
 *      MOSQ_ERR_NOT_SUPPORTED - 如果没有线程支持
 */
int mosquitto_loop_stop(struct mosquitto *mosq, bool force);

/**
 * 此函数在无限阻塞循环中为您调用loop()。对于只想在程序中运行MQTT客户端循环的情况，这很有用。如果服务器连接丢失，它将处理连接。如果您在回调中调用mosquitto Disconnect()，它将返回
 * 
 * @param mosq          有效的mosquitto实例
 * @param timeout       超时之前，在select()调用中等待网络活动的最大毫秒数。设置为0即时返回。设置为负可使用默认值1000ms
 * @param max_packets   此参数当前未使用，应设置为1，以实现将来的兼容性
 * 
 * @return
 *      MOSQ_ERR_SUCCESS   - 成功
 *      MOSQ_ERR_INVAL     - 如果输入参数无效
 *      MOSQ_ERR_NOMEM     - 如果发生内存不足的情况
 *      MOSQ_ERR_NO_CONN   - 如果客户端未连接到代理
 *      MOSQ_ERR_CONN_LOST - 如果与代理的连接丢失
 *      MOSQ_ERR_PROTOCOL  - 与代理进行通信时是否存在协议错误
 *      MOSQ_ERR_ERRNO     - 如果系统调用返回错误。变量errno包含错误代码。在可用的地方使用strerror_r()
 */
int mosquitto_loop_forever(struct mosquitto *mosq, int timeout, int max_packets);

/**
 * 客户端的主网络环路。必须经常调用它，以保持客户端和代理之间的通信正常。这是通过<mosquitto_loop_forever>和<mosquitto_loop_start>执行的，这是处理网络环路的推荐方法。
 * 如果需要，您也可以使用此功能。一定不能在回调中调用它。如果存在传入数据，则将对其进行处理。传出的命令，例如 通常在调用它们的函数后立即发送<mosquitto_publish>，但这并不总是可能的。
 * <mosquitto_loop>还将尝试发送所有剩余的传出消息，其中还包括命令，这些命令是QoS>0消息流的一部分。这将调用select()来监视客户端网络套接字。如果要将mosquitto客户端操作与自己的select()
 * 调用集成在一起，请使用<mosquitto_socket>，<mosquitto_loop_read>，<mosquitto_loop_write>和<mosquitto_loop_misc>。
 * 
 * @param mosq          有效的mosquitto实例
 * @param timeout       超时之前，在select()调用中等待网络活动的最大毫秒数。设置为0即时返回。设置为负可使用默认值1000ms
 * @param max_packets   此参数当前未使用，应设置为1，以实现将来的兼容性
 * 
 * @return
 *      MOSQ_ERR_SUCCESS   - 成功
 *      MOSQ_ERR_INVAL     - 如果输入参数无效
 *      MOSQ_ERR_NOMEM     - 如果发生内存不足的情况
 *      MOSQ_ERR_NO_CONN   - 如果客户端未连接到代理
 *      MOSQ_ERR_CONN_LOST - 如果与代理的连接丢失
 *      MOSQ_ERR_PROTOCOL  - 与代理进行通信时是否存在协议错误
 *      MOSQ_ERR_ERRNO     - 如果系统调用返回错误。变量errno包含错误代码。在可用的地方使用strerror_r()
 */
int mosquitto_loop(struct mosquitto *mosq, int timeout, int max_packets);

/**
 * 进行网络读取操作。仅当您不使用mosquitto_loop()并自己监视客户端网络套接字的活动时，才应使用此方法
 * 
 * @param mosq          有效的mosquitto实例
 * @param max_packets   此参数当前未使用，应设置为1，以实现将来的兼容性
 * 
 * @return
 *      MOSQ_ERR_SUCCESS   - 成功
 *      MOSQ_ERR_INVAL     - 如果输入参数无效
 *      MOSQ_ERR_NOMEM     - 如果发生内存不足的情况
 *      MOSQ_ERR_NO_CONN   - 如果客户端未连接到代理
 *      MOSQ_ERR_CONN_LOST - 如果与代理的连接丢失
 *      MOSQ_ERR_PROTOCOL  - 与代理进行通信时是否存在协议错误
 *      MOSQ_ERR_ERRNO     - 如果系统调用返回错误。变量errno包含错误代码。在可用的地方使用strerror_r()
 */
int mosquitto_loop_read(struct mosquitto *mosq, int max_packets);

/**
 * 进行网络写入操作。仅当您不使用mosquitto_loop()并自己监视客户端网络套接字的活动时，才应使用此方法
 * 
 * @param mosq          有效的mosquitto实例
 * @param max_packets   此参数当前未使用，应设置为1，以实现将来的兼容性
 * 
 * @return
 *      MOSQ_ERR_SUCCESS   - 成功
 *      MOSQ_ERR_INVAL     - 如果输入参数无效
 *      MOSQ_ERR_NOMEM     - 如果发生内存不足的情况
 *      MOSQ_ERR_NO_CONN   - 如果客户端未连接到代理
 *      MOSQ_ERR_CONN_LOST - 如果与代理的连接丢失
 *      MOSQ_ERR_PROTOCOL  - 与代理进行通信时是否存在协议错误
 *      MOSQ_ERR_ERRNO     - 如果系统调用返回错误。变量errno包含错误代码。在可用的地方使用strerror_r()
 */
int mosquitto_loop_write(struct mosquitto *mosq, int max_packets);

/**
 * 执行网络循环中所需的其他操作。仅当您不使用mosquitto_loop()并自己监视客户端网络套接字的活动时，才应使用此方法。此函数处理PING并检查是否需要重试消息，因此应该相当频繁地调用它，
 * 大约每秒一次就足够了
 * 
 * @param mosq  有效的mosquitto实例
 * 
 * @return
 *      MOSQ_ERR_SUCCESS - 成功
 *      MOSQ_ERR_INVAL   - 如果输入参数无效
 *      MOSQ_ERR_NO_CONN - 如果客户端未连接到代理
 */
int mosquitto_loop_misc(struct mosquitto *mosq);

/**
 * 返回mosquitto实例的套接字句柄。如果要在自己的select()调用中包含mosquitto客户端，则很有用
 * 
 * @param mosq  有效的mosquitto实例
 * 
 * @return
 *      mosquitto客户端的套接字或失败时为-1
 */
int mosquitto_socket(struct mosquitto *mosq);

/**
 * 如果有准备写入套接字的数据，则返回true
 * 
 * @param mosq  有效的mosquitto实例
 */
bool mosquitto_want_write(struct mosquitto *mosq);

/**
 * 用于告诉库您的应用程序正在使用线程，而不使用<mosquitto_loop_start>。在非线程模式下，该库的操作略有不同，以简化其操作。如果您正在管理自己的线程并且不使用此功能，则将由于竞争条件而崩溃。
 * 使用<mosquitto_loop_start>时，此设置是自动设置的
 * 
 * @param mosq      有效的mosquitto实例
 * @param threaded  如果您的应用程序正在使用线程，则为true，否则为false
 */
int mosquitto_threaded_set(struct mosquitto *mosq, bool threaded);

/**
 * 用于设置客户端的选项。不建议使用此函数，而应使用替换<mosquitto_int_option>，<mosquitto_string_option>和<mosquitto_void_option>函数
 * 
 * @param mosq      有效的mosquitto实例
 * @param option    设置选项
 *                      MOSQ_OPT_PROTOCOL_VERSION      - 值必须为int，设置为MQTT_PROTOCOL_V31或MQTT_PROTOCOL_V311。必须在客户端连接之前进行设置。默认为MQTT_PROTOCOL_V31
 *                      MOSQ_OPT_SSL_CTX               - 传递一个opensl SSL_CTX以便在创建TLS连接时使用，而不是通过libmosquit来创建自己的连接。必须先调用此函数，然后才能生效。
 *                                                       如果使用此选项，则有责任确保使用安全设置。设置为NULL意味着，如果要使用TLS，libmosquitto将使用其自己的SSL_CTX。
 *                                                       此选项仅适用于openssl 1.1.0及更高版本
 *                      MOSQ_OPT_SSL_CTX_WITH_DEFAULTS - 值必须是设置为1或0的int。如果设置为1，则用户指定的使用MOSQ_OPT_SSL_CTX传入的SSL_CTX将应用默认选项。
 *                                                       这意味着您只需要更改与您相关的值。如果使用此选项，则必须正常配置TLS选项，即，应至少使用<mosquitto_tls_set>来配置cafile/capath。
 *                                                       此选项仅适用于openssl 1.1.0及更高版本
 * @param value     特定于选项的值
 */
int mosquitto_opts_set(struct mosquitto *mosq, enum mosq_opt_t option, void *value);

/**
 * 用于为客户端设置整数选项
 * 
 * @param mosq      一个有效的mosquitto实例
 * @param option    设置选项
 *                      MOSQ_OPT_TCP_NODELAY           - 设置为1可在客户端套接字上禁用Nagle的算法。这具有减少单个消息的等待时间的效果，但潜在的代价是增加了发送数据包的数量。默认值为0，表示Nagle保持启用状态
 *                      MOSQ_OPT_PROTOCOL_VERSION      - 值必须设置为MQTT_PROTOCOL_V31，MQTT_PROTOCOL_V311或MQTT_PROTOCOL_V5。必须在客户端连接之前进行设置。默认为MQTT_PROTOCOL_V311
 *                      MOSQ_OPT_RECEIVE_MAXIMUM       - 值可以设置在1到65535之间(含1和65535)，并表示此客户端希望立即处理的传入QoS 1和QoS 2消息的最大数目。默认值为20。此选项对MQTT v3.1或v3.1.1客户端无效。请注意，如果MQTT_PROP_RECEIVE_MAXIMUM属性在传递给mosquitto_connect_v5()的属性列表中，则该属性将覆盖此选项。但是，建议使用此选项
 *                      MOSQ_OPT_SEND_MAXIMUM          - 值可以设置在1到65535之间(含1和65535)，代表该客户端一次尝试“处于运行状态”的传出QoS 1和QoS 2消息的最大数量。默认值为20。此选项对MQTT v3.1或v3.1.1客户端无效。请注意，如果连接到的代理发送的MQTT_PROP_RECEIVE_MAXIMUM属性的值比该选项的值低，那么将使用代理提供的值
 *                      MOSQ_OPT_SSL_CTX_WITH_DEFAULTS - 如果将value设置为非零值，则使用MOSQ_OPT_SSL_CTX传递的用户指定的SSL_CTX将应用默认选项。这意味着您只需要更改与您相关的值。如果使用此选项，则必须正常配置TLS选项，即，应使用mosquitto_tls_set至少配置cafile/capath。此选项仅适用于openssl 1.1.0及更高版本
 *                      MOSQ_OPT_TLS_OCSP_REQUIRED     - 设置是否需要对TLS连接进行OCSP检查。设置为1启用检查，设置为0(默认值)不进行检查
 *                      MOSQ_OPT_TLS_USE_OS_CERTS      - 设置为1以指示客户端加载和信任OS提供的用于TLS连接的CA证书。设置为0(默认值)以仅使用手动指定的CA证书
 *                      MOSQ_OPT_DISABLE_SOCKETPAIR    - 默认情况下，每个连接的客户端将创建一对内部连接的套接字，以便在另一个线程调用< mosquito to_publish>或其他类似命令时通知并唤醒网络线程。如果使用外部循环操作，则没有必要这样做，并且每个客户端会额外消耗两个套接字。将此选项设置为1以禁用套接字对的使用
 *                      MOSQ_OPT_TRANSPORT             - 让客户端正常地通过TCP连接MQTT，或者通过WebSockets连接MQTT。设置为"MOSQ_T_TCP"或"MOSQ_T_WEBSOCKETS"
 *                      MOSQ_OPT_HTTP_HEADER_SIZE      - Size将要分配的缓冲区的大小。默认为4096。设置为低于100将导致MOSQ_ERR_INVAL的返回值。这应该在启动连接之前设置。如果您尝试在初始http请求进行时设置此参数，那么它将返回MOSQ_ERR_INVAL
 * @param value     选项的具体值
 * 
 * @return 
 *      MOSQ_ERR_SUCCESS       - 成功
 *      MOSQ_ERR_INVAL         - 输出参数无效
 *      MOSQ_ERR_NOMEM         - 内存不足
 *      MOSQ_ERR_NOT_SUPPORTED - 不支持
 */
int mosquitto_int_option(struct mosquitto *mosq, enum mosq_opt_t option, int value);

/**
 * 用于为客户端设置void *选项
 * 
 * @param mosq      一个有效的mosquitto实例
 * @param option    设置选项
 *                      MOSQ_OPT_SSL_CTX - 传递一个在创建TLS连接时使用的openssl SSL_CTX，而不是libmosquit来创建自己的连接。必须先调用此函数，然后才能生效。如果使用此选项，
 *                                         则有责任确保使用安全设置。设置为NULL意味着，如果要使用TLS，libmosquitto将使用其自己的SSL_CTX。此选项仅适用于openssl 1.1.0及更高版本
 * @param value     选项的具体值
 * 
 * @return
 *      MOSQ_ERR_SUCCESS       - 成功
 *      MOSQ_ERR_INVAL         - 输出参数无效
 *      MOSQ_ERR_NOMEM         - 内存不足
 *      MOSQ_ERR_NOT_SUPPORTED - 不支持
 */
int mosquitto_void_option(struct mosquitto *mosq, enum mosq_opt_t option, void *value);

/**
 * 用于为客户端设置const char *选项
 * 
 * @param mosq      一个有效的mosquitto实例
 * @param option    设置选项
 *                      MOSQ_OPT_TLS_ENGINE     - 为客户端配置TLS引擎支持。传递创建TLS连接时要使用的TLS引擎ID。必须在mosquitto_connect之前设置
 *                      MOSQ_OPT_TLS_KEYFORM    - 配置客户端以根据密钥文件的类型来区别对待密钥文件。必须在mosquitto_connect之前设置。设置为pem或engine，以确定从哪里获得TLS连接的私钥。
 *                                                默认为pem，一个普通的私钥文件
 *                      MOSQ_OPT_TLS_KPASS_SHA1 - 在TLS引擎要求使用密码进行访问的情况下，此选项允许将私钥密码的十六进制编码SHA1哈希直接传递给引擎。必须在mosquitto_connect之前设置
 *                      MOSQ_OPT_TLS_ALPN       - 如果要连接的代理在单个TLS端口上具有多个可用的服务，例如MQTT和WebSockets，请使用此选项为连接配置ALPN选项
 *                      MOSQ_OPT_BIND_ADDRESS   - 设置连接时要绑定到的本地网络接口的主机名或IP地址
 * @param value     选项的具体值
 * 
 * @return
 *      MOSQ_ERR_SUCCESS       - 成功
 *      MOSQ_ERR_INVAL         - 输出参数无效
 *      MOSQ_ERR_NOMEM         - 内存不足
 *      MOSQ_ERR_NOT_SUPPORTED - 不支持
 */
int mosquitto_string_option(struct mosquitto *mosq, enum mosq_opt_t option, const char *value);

/**
 * 当客户端在mosquitto_loop_forever中意外断开连接或在mosquitto_loop_start之后意外断开连接时，控制客户端的行为。如果不使用此功能，则默认行为是以1秒的延迟反复尝试重新连接，
 * 直到连接成功为止。使用reconnect_delay参数可更改两次连续尝试之间的延迟。您还可以通过将reconnect_exponential_backoff设置为true并使用reconnect_delay_max设置延迟的上限来启用重新连接
 * 之间的时间的指数补偿
 * 
 * @param mosq                              一个有效的mosquitto实例
 * @param reconnect_delay                   两次重新连接之间等待的秒数
 * @param reconnect_delay_max               两次重新连接之间等待的最大秒数
 * @param reconnect_exponential_backoff     在重新连接尝试之间使用指数退避。设置为true以启用指数补偿
 * 
 * @return
 *      MOSQ_ERR_SUCCESS - 成功
 *      MOSQ_ERR_INVAL   - 如果输入参数无效
 */
int mosquitto_reconnect_delay_set(struct mosquitto *mosq, unsigned int reconnect_delay, unsigned int reconnect_delay_max, bool reconnect_exponential_backoff);

/**
 * 此功能是不受欢迎的。请将mosquitto_int_option函数与MOSQ_OPT_SEND_MAXIMUM选项一起使用。设置一次可以“运行”的QoS 1和2消息的数量。传输中的消息是其传递流程的一部分。
 * 尝试使用mosquitto_publish发送更多消息将导致消息排队，直到传输中的消息数量减少为止。此处更大的数量会导致更大的消息吞吐量，但如果设置为高于代理上的最大飞行中消息，则可能导致延迟
 * 在被确认的消息中。最大设置为0
 * 
 * @param mosq                      一个有效的mosquitto实例
 * @param max_inflight_messages     传输上消息的最大数量。默认为20
 * 
 * @return
 *      MOSQ_ERR_SUCCESS - 成功
 *      MOSQ_ERR_INVAL   - 如果输入参数无效
 */
int mosquitto_max_inflight_messages_set(struct mosquitto *mosq, unsigned int max_inflight_messages);

void mosquitto_message_retry_set(struct mosquitto *mosq, unsigned int message_retry);

/**
 * 检索mosquitto客户端的userdata变量
 * 
 * @param mosq  一个有效的mosquitto实例
 * 
 * @return
 *      指向userdata成员变量的指针
 */
void *mosquitto_userdata(struct mosquitto *mosq);

/**
 * 调用mosquitto_new时，作为obj参数给出的指针将作为用户数据传递给回调。mosquitto_user_data_set函数允许该obj参数随时更新。该功能不会修改当前用户数据指针指向的内存。
 * 如果它是动态分配的内存，则必须自己释放它
 * 
 * @param mosq  一个有效的mosquitto实例
 * @param obj   用户指针，将作为参数传递给指定的任何回调
 */
void mosquitto_user_data_set(struct mosquitto *mosq, void *obj);

/**
 * 为客户端配置基于证书的SSL/TLS支持。必须在mosquitto_connect之前调用。不能与mosquitto_tls_psk_set一起使用。使用cafile定义要信任的证书颁发机构证书(即服务器证书必须使用以下证书之一签名)。
 * 如果要连接的服务器要求客户端 提供证书，并使用客户端证书和私钥定义certfile和keyfile。如果您的私钥已加密，请提供密码回调功能，否则您将必须在命令行中输入密码
 * 
 * @param mosq          一个有效的mosquitto实例
 * @param cafile        包含PEM编码的受信任CA证书文件的文件的路径。cafile或capath不能为NULL
 * @param capath        包含PEM编码的受信任CA证书文件的目录的路径。cafile或capath不能为NULL
 * @param certfile      包含此客户端的PEM编码的证书文件的文件的路径。如果为NULL，则密钥文件也必须为NULL，并且不会使用任何客户端证书
 * @param keyfile       包含此客户端的PEM编码的私钥的文件的路径。如果为NULL，则certfile也必须为NULL，并且不会使用任何客户端证书
 * @param pw_callback   如果密钥文件已加密，请设置pw_callback以允许您的客户端传递正确的密码进行解密。如果设置为NULL，则必须在命令行中输入密码。
 *                      您的回调必须将密码写入"buf"，即"size"个字节长。返回值必须是密码的长度。"userdata"将设置为正在调用的mosquitto实例。
 *                      可以使用mosquitto_userdata检索mosquitto userdata成员变量
 * 
 * @return
 *      MOSQ_ERR_SUCCESS - 成功
 *      MOSQ_ERR_INVAL   - 如果输入参数无效
 *      MOSQ_ERR_NOMEM   - 如果发生内存不足的情况
 */
int mosquitto_tls_set(struct mosquitto *mosq, const char *cafile, const char *capath, const char *certfile, const char *keyfile, int (*pw_callback)(char *buf, int size, int rwflag, void *userdata));

/**
 * 在服务器证书中配置对服务器主机名的验证。如果value设置为true，则无法保证所连接的主机没有模拟您的服务器。这在初始服务器测试中可能很有用，但是例如，
 * 恶意第三方可能会通过DNS欺骗来假冒您的服务器。请勿在实际系统中使用此功能。将值设置为true会使连接加密毫无意义。必须在mosquitto_connect之前调用
 * 
 * @param mosq      一个有效的mosquitto实例
 * @param value     如果设置为false，则执行默认的证书主机名检查。如果设置为true，则不执行主机名检查并且连接不安全
 * 
 * @return
 *      MOSQ_ERR_SUCCESS - 成功
 *      MOSQ_ERR_INVAL   - 如果输入参数无效
 */
int mosquitto_tls_insecure_set(struct mosquitto *mosq, bool value);

/**
 * 为客户端配置基于预共享密钥的TLS支持。必须在mosquitto_connect之前调用。不能与mosquitto_tls_set一起使用
 * 
 * @param mosq      一个有效的mosquitto实例
 * @param psk       十六进制格式的预共享密钥，没有前导"0x"
 * @param identity  该客户的身份。可以用作用户名，具体取决于服务器设置
 * @param ciphers   一个字符串，描述可使用的PSK密码。有关更多信息，请参见openssl密码工具。如果为NULL，则将使用默认密码
 *  
 * @return
 *      MOSQ_ERR_SUCCESS - 成功
 *      MOSQ_ERR_INVAL   - 如果输入参数无效
 *      MOSQ_ERR_NOMEM   - 如果发生内存不足的情况
 */
int mosquitto_tls_psk_set(struct mosquitto *mosq, const char *psk, const char *identity, const char *ciphers);

/**
 * 设置高级SSL/TLS选项。必须在mosquitto_connect之前调用
 * 
 * @param mosq          一个有效的mosquitto实例
 * @param cert_reqs     一个整数，它定义客户端将在服务器上强加的验证要求。可以是以下之一:
 *                      SSL_VERIFY_NONE(0): 不会以任何方式验证服务器
 *                      SSL_VERIFY_PEER(1): 如果验证失败，将验证服务器证书，并中止连接。推荐的默认值为SSL_VERIFY_PEER。使用SSL_VERIFY_NONE不提供安全性
 * @param tls_version   用作字符串的SSL/TLS协议的版本。如果为NULL，则使用默认值。缺省值和可用值取决于编译库所依据的openssl版本。对于openssl>=1.0.1，可用选项为tlsv1.2，tlsv1.1和tlsv1，
 *                      默认设置为tlv1.2。对于openssl<1.0.1，仅tlsv1可用
 * @param ciphers       描述可用密码的字符串。有关更多信息，请参见openssl密码工具。如果为NULL，则将使用默认密码
 *  
 * @return
 *      MOSQ_ERR_SUCCESS - 成功
 *      MOSQ_ERR_INVAL   - 如果输入参数无效
 *      MOSQ_ERR_NOMEM   - 如果发生内存不足的情况
 */
int mosquitto_tls_opts_set(struct mosquitto *mosq, int cert_reqs, const char *tls_version, const char *ciphers);

/**
 * 检索指向此客户端中用于TLS连接的SSL结构的指针。这可以用在例如 连接回调以执行其他验证步骤
 * 
 * @param mosq  一个有效的mosquitto实例
 * 
 * @return
 *      如果客户端使用TLS，返回指向openssl SSL结构的有效指针，否则返回NULL
 */
void *mosquitto_ssl_get(struct mosquitto *mosq);

/**
 * 设置连接回调。当代理发送CONNACK消息以响应连接时，将调用此方法
 * 
 * @param mosq          一个有效的mosquitto实例
 * @param on_connect    回调函数，格式如下: void callback(struct mosquitto *mosq, void *obj, int rc)
 * 
 * 回调参数:
 * @param mosq          进行回调的mosquitto实例
 * @param obj           mosquitto_new中提供的用户数据
 * @param rc            连接响应的返回码。这些值由使用的MQTT协议版本定义
 */
void mosquitto_connect_callback_set(struct mosquitto *mosq, void (*on_connect)(struct mosquitto *, void *, int));

/**
 * 设置连接回调。当代理发送CONNACK消息以响应连接时，将调用此方法
 * 
 * @param mosq          一个有效的mosquitto实例
 * @param on_connect    回调函数，格式如下: void callback(struct mosquitto *mosq, void *obj, int rc, int flags)
 * 
 * 回调参数:
 * @param mosq          进行回调的mosquitto实例
 * @param obj           mosquitto_new中提供的用户数据
 * @param rc            连接响应的返回码。这些值由使用的MQTT协议版本定义
 * @param flags         连接标志
 */
void mosquitto_connect_with_flags_callback_set(struct mosquitto *mosq, void (*on_connect)(struct mosquitto *, void *, int, int));

/**
 * 设置连接回调。当代理发送CONNACK消息以响应连接时调用此方法。对所有MQTT协议版本都设置此回调是有效的。如果它与使用MQTT v3.1.1或更早版本的MQTT客户端一起使用，则props参数将始终为NULL
 * 
 * @param mosq          一个有效的mosquitto实例
 * @param on_connect    回调函数，格式如下: void callback(struct mosquitto *mosq, void *obj, int rc, int flags, const mosquitto_property *props)
 * 
 * 回调参数:
 * @param mosq          进行回调的mosquitto实例
 * @param obj           mosquitto_new中提供的用户数据
 * @param rc            连接响应的返回码。这些值由使用的MQTT协议版本定义
 * @param flags         连接标志
 * @param props         MQTT v5属性的列表，或者为NULL
 */
void mosquitto_connect_v5_callback_set(struct mosquitto *mosq, void (*on_connect)(struct mosquitto *, void *, int, int, const mosquitto_property *props));

/**
 * 设置预连接回调。在尝试连接到代理之前，将调用预连接回调。如果使用的是<mosquitto_loop_start>或<mosquitto_loop_forever>，这可能会很有用，因为在客户端断开连接时，
 * 默认情况下，库将自动重新连接。使用预连接回调，可以设置用户名，密码和TLS相关参数
 * 
 * @param mosq              一个有效的mosquitto实例
 * @param on_pre_connect    具有以下形式的回调函数: void callback(struct mosquitto *mosq, void *obj)
 * 
 * 回调参数:
 * @param mosq          进行回调的mosquitto实例
 * @param obj           mosquitto_new中提供的用户数据
 */
void mosquitto_pre_connect_callback_set(struct mosquitto *mosq, void (*on_pre_connect)(struct mosquitto *, void *));

/**
 * 设置断开连接回调。当代理收到DISCONNECT命令并断开与客户端的连接时，将调用此方法
 * 
 * @param mosq              一个有效的mosquitto实例
 * @param on_disconnect     具有以下形式的回调函数: void callback(struct mosquitto *mosq, void *obj, int rc)
 * 
 * 回调参数:
 * @param mosq          进行回调的mosquitto实例
 * @param obj           mosquitto_new中提供的用户数据
 * @param rc            指示断开原因的整数值。值为0表示客户端已调用mosquitto_disconnect。其他任何值表示断开连接是意外的
 */
void mosquitto_disconnect_callback_set(struct mosquitto *mosq, void (*on_disconnect)(struct mosquitto *, void *, int));

/**
 * 设置断开连接回调。当代理接收到DISCONNECT命令并断开与客户端的连接时，将调用此方法。为所有MQTT协议版本设置此回调是有效的。如果它与使用MQTT v3.1.1或更早版本的MQTT客户端一起使用，
 * 则props参数将始终为NULL
 * 
 * @param mosq              一个有效的mosquitto实例
 * @param on_disconnect     具有以下形式的回调函数: void callback(struct mosquitto *mosq, void *obj, int rc, const mosquitto_property *props)
 * 
 * 回调参数:
 * @param mosq          进行回调的mosquitto实例
 * @param obj           mosquitto_new中提供的用户数据
 * @param rc            指示断开原因的整数值。值为0表示客户端已调用mosquitto_disconnect。其他任何值表示断开连接是意外的
 * @param props         MQTT v5属性的列表，或者为NULL
 */
void mosquitto_disconnect_v5_callback_set(struct mosquitto *mosq, void (*on_disconnect)(struct mosquitto *, void *, int, const mosquitto_property *props));

/**
 * 设置发布回调。当以mosquitto_publish发起的消息已成功发送到代理时，将调用此方法
 * 
 * @param mosq          一个有效的mosquitto实例
 * @param on_publish    回调函数，格式如下: void callback(struct mosquitto *mosq, void *obj, int mid)
 * 
 * 回调参数:
 * @param mosq          进行回调的mosquitto实例
 * @param obj           mosquitto_new中提供的用户数据
 * @param mid           已发送消息的消息ID
 */
void mosquitto_publish_callback_set(struct mosquitto *mosq, void (*on_publish)(struct mosquitto *, void *, int));

/**
 * 设置发布回调。当以mosquitto_publish发起的消息已发送到代理时，将调用此方法。如果消息发送成功，或者代理响应错误，都会调用此回调，这将反映在reason_code参数中。
 * 对于所有MQTT协议版本都必须设置此回调。如果它与使用MQTT v3.1.1或更早版本的MQTT客户端一起使用，则props参数将始终为NULL
 * 
 * @param mosq          一个有效的mosquitto实例
 * @param on_publish    回调函数，格式如下: void callback(struct mosquitto *mosq, void *obj, int mid, int reason_code, const mosquitto_property *props)
 * 
 * 回调参数:
 * @param mosq          进行回调的mosquitto实例
 * @param obj           mosquitto_new中提供的用户数据
 * @param mid           已发送消息的消息ID
 * @param reason_code   MQTT v5原因码
 * @param props         MQTT v5属性的列表，或者为NULL
 */
void mosquitto_publish_v5_callback_set(struct mosquitto *mosq, void (*on_publish)(struct mosquitto *, void *, int, int, const mosquitto_property *props));

/**
 * 设置消息回调。当从代理接收到消息时，将调用此方法
 * 
 * @param mosq          一个有效的mosquitto实例
 * @param on_message    回调函数，格式如下: void callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
 * 
 * 回调参数:
 * @param mosq          进行回调的mosquitto实例
 * @param obj           mosquitto_new中提供的用户数据
 * @param message       消息数据。回调完成后，库将释放此变量和关联的内存。客户应复制其所需的任何数据
 */
void mosquitto_message_callback_set(struct mosquitto *mosq, void (*on_message)(struct mosquitto *, void *, const struct mosquitto_message *));

/**
 * 设置消息回调。当从代理接收到消息时，将调用此方法。对所有MQTT协议版本都设置此回调是有效的。如果它与使用MQTT v3.1.1或更早版本的MQTT客户端一起使用，则props参数将始终为NULL
 * 
 * @param mosq          一个有效的mosquitto实例
 * @param on_message    回调函数，格式如下: void callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message, const mosquitto_property *props)
 * 
 * 回调参数:
 * @param mosq          进行回调的mosquitto实例
 * @param obj           mosquitto_new中提供的用户数据
 * @param message       消息数据。回调完成后，库将释放此变量和关联的内存。客户应复制其所需的任何数据
 * @param props         MQTT v5属性的列表，或者为NULL
 */
void mosquitto_message_v5_callback_set(struct mosquitto *mosq, void (*on_message)(struct mosquitto *, void *, const struct mosquitto_message *, const mosquitto_property *props));

/**
 * 设置订阅回调。当代理响应订阅请求时，将调用此方法
 * 
 * @param mosq          一个有效的mosquitto实例
 * @param on_subscribe  回调函数，格式如下: void callback(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos)
 * 
 * 回调参数:
 * @param mosq          进行回调的mosquitto实例
 * @param obj           mosquitto_new中提供的用户数据
 * @param mid           订阅消息的消息ID
 * @param qos_count     授予的订阅数(grant_qos的大小)
 * @param granted_qos   一个整数数组，指示每个订阅的已授予QoS
 */
void mosquitto_subscribe_callback_set(struct mosquitto *mosq, void (*on_subscribe)(struct mosquitto *, void *, int, int, const int *));

/**
 * 设置订阅回调。当代理响应订阅请求时，将调用此方法。为所有MQTT协议版本设置此回调是有效的。如果它与使用MQTT v3.1.1或更早版本的MQTT客户端一起使用，则props参数将始终为NULL
 * 
 * @param mosq          一个有效的mosquitto实例
 * @param on_subscribe  回调函数，格式如下: void callback(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos, const mosquitto_property *props)
 * 
 * 回调参数:
 * @param mosq          进行回调的mosquitto实例
 * @param obj           mosquitto_new中提供的用户数据
 * @param mid           订阅消息的消息ID
 * @param qos_count     授予的订阅数(grant_qos的大小)
 * @param granted_qos   一个整数数组，指示每个订阅的已授予QoS
 * @param props         MQTT v5属性的列表，或者为NULL
 */
void mosquitto_subscribe_v5_callback_set(struct mosquitto *mosq, void (*on_subscribe)(struct mosquitto *, void *, int, int, const int *, const mosquitto_property *props));

/**
 * 设置退订回调。当代理响应取消订阅请求时，将调用此方法
 * 
 * @param mosq              一个有效的mosquitto实例
 * @param on_unsubscribe    回调函数，格式如下: void callback(struct mosquitto *mosq, void *obj, int mid)
 * 
 * 回调参数:
 * @param mosq              进行回调的mosquitto实例
 * @param obj               mosquitto_new中提供的用户数据
 * @param mid               退订消息的消息ID
 */
void mosquitto_unsubscribe_callback_set(struct mosquitto *mosq, void (*on_unsubscribe)(struct mosquitto *, void *, int));

/**
 * 设置退订回调。当代理响应取消订阅请求时调用此方法。对所有MQTT协议版本都设置此回调是有效的。如果它与使用MQTT v3.1.1或更早版本的MQTT客户端一起使用，则props参数将始终为NULL
 * 
 * @param mosq              一个有效的mosquitto实例
 * @param on_unsubscribe    回调函数，格式如下: void callback(struct mosquitto *mosq, void *obj, int mid, const mosquitto_property *props)
 * 
 * 回调参数:
 * @param mosq              进行回调的mosquitto实例
 * @param obj               mosquitto_new中提供的用户数据
 * @param mid               退订消息的消息ID
 * @param props             MQTT v5属性的列表，或者为NULL
 */
void mosquitto_unsubscribe_v5_callback_set(struct mosquitto *mosq, void (*on_unsubscribe)(struct mosquitto *, void *, int, const mosquitto_property *props));
void mosquitto_unsubscribe2_v5_callback_set(struct mosquitto *mosq, void (*on_unsubscribe)(struct mosquitto *, void *, int, int, const int *, const mosquitto_property *props));

/**
 * 设置日志记录回调。如果要从客户端库获取事件日志信息，则应使用此方法
 * 
 * @param mosq      一个有效的mosquitto实例
 * @param on_log    回调函数，格式如下: void callback(struct mosquitto *mosq, void *obj, int level, const char *str)
 * 
 * 回调参数:
 * @param mosq      进行回调的mosquitto实例
 * @param obj       mosquitto_new中提供的用户数据
 * @param level     来自以下值的日志消息级别: MOSQ_LOG_INFO、MOSQ_LOG_NOTICE、MOSQ_LOG_WARNING、MOSQ_LOG_ERR、MOSQ_LOG_DEBUG
 * @param str       消息字符串
 */
void mosquitto_log_callback_set(struct mosquitto *mosq, void (*on_log)(struct mosquitto *, void *, int, const char *));

/**
 * 将客户端配置为在连接时使用SOCKS5代理。连接前必须先调用。支持无和用户名/密码身份验证
 * 
 * @param mosq          一个有效的mosquitto实例
 * @param host          SOCKS5代理以连接到的主机
 * @param port          要使用的SOCKS5代理端口
 * @param username      如果不为NULL，则在使用代理进行身份验证时使用此用户名
 * @param password      如果不为NULL并且用户名不为NULL，则在使用代理进行身份验证时使用此密码
 *  
 * @return
 *      MOSQ_ERR_SUCCESS - 成功
 *      MOSQ_ERR_INVAL   - 如果输入参数无效
 *      MOSQ_ERR_NOMEM   - 内存不足
 */
int mosquitto_socks5_set(struct mosquitto *mosq, const char *host, int port, const char *username, const char *password);

/**
 * 调用以获得mosquitto错误号的const字符串描述
 * 
 * @param mosq_errno    mosquitto错误号
 * 
 * @return
 *      描述错误的常量字符串
 */
const char *mosquitto_strerror(int mosq_errno);

/**
 * 调用以获取MQTT原因码的const字符串描述
 * 
 * @param reason_code   MQTT原因码
 * 
 * @return
 *      描述原因的常量字符串
 */
const char *mosquitto_reason_string(int reason_code);

/**
 * 调用以获得MQTT连接结果的const字符串描述
 * 
 * @param connack_code  MQTT连接结果
 * 
 * @return
 *      描述结果的常量字符串
 */
const char *mosquitto_connack_string(int connack_code);

/**
 * 接受代表MQTT命令的字符串输入，并将其转换为libmosquitto整数表示
 * 
 * @param str   要解析的字符串
 * @param cmd   指向int的指针，用于结果
 * 
 * @return
 *      MOSQ_ERR_SUCCESS - 成功
 *      MOSQ_ERR_INVAL   - 输入无效
 */
int mosquitto_string_to_command(const char *str, int *cmd);

/**
 * 在mosquitto_sub_topic_tokenise中分配的可用内存
 * 
 * @param topics    指向字符串数组的指针
 * @param count     字符串数组中的项目数
 * 
 * @return
 *      MOSQ_ERR_SUCCESS - 成功
 *      MOSQ_ERR_INVAL   - 如果输入参数无效
 */
int mosquitto_sub_topic_tokens_free(char ***topics, int count);

/**
 * 将主题或订阅字符串标记为代表主题层次结构的字符串数组。例如，subtopic: "a/deep/topic/hierarchy" --> topics[0] = "a" topics[1] = "deep" topics[2] = "topic" topics[3] = "hierarchy"；
 * subtopic: "/a/deep/topic/hierarchy/" --> topics[0] = NULL topics[1] = "a" topics[2] = "deep" topics[3] = "topic" topics[4] = "hierarchy"
 * 
 * @param subtopic  要标记化的订阅/主题
 * @param topics    存储字符串数组的指针
 * @param count     一个int指针，用于存储主题数组中的项目数
 * 
 * @return 
 *      MOSQ_ERR_SUCCESS        - 成功
 *      MOSQ_ERR_NOMEM          - 如果发生内存不足的情况
 *      MOSQ_ERR_MALFORMED_UTF8 - 如果主题无效，不为UTF-8
 */
int mosquitto_sub_topic_tokenise(const char *subtopic, char ***topics, int *count);

/**
 * 检查主题是否与订阅匹配。例如，foo/bar将匹配订阅foo/#或+/bar non/matching将不匹配订阅non/+/+
 * 
 * @param sub       用于检查主题的订阅字符串
 * @param topic     要检查的主题
 * @param result    bool保持结果的指针。如果主题与订阅匹配，则将设置为true
 * 
 * @return
 *      MOSQ_ERR_SUCCESS - 成功
 *      MOSQ_ERR_INVAL   - 如果输入参数无效
 */
int mosquitto_topic_matches_sub(const char *sub, const char *topic, bool *result);

/**
 * 检查主题是否与订阅匹配
 * 
 * @param sub       用于检查主题的订阅字符串
 * @param sublen    用于检查主题的订阅字符串长度
 * @param topic     要检查的主题
 * @param topiclen  要检查的主题长度
 * @param result    bool保持结果的指针。如果主题与订阅匹配，则将设置为true
 * 
 * @return
 *      MOSQ_ERR_SUCCESS - 成功
 *      MOSQ_ERR_INVAL   - 如果输入参数无效
 */
int mosquitto_topic_matches_sub2(const char *sub, size_t sublen, const char *topic, size_t topiclen, bool *result);

/**
 * 检查主题是否与订阅匹配，并使用客户端ID/用户名模式替换
 * 
 * @param sub       用于检查主题的订阅字符串
 * @param topic     要检查的主题
 * @param clientid  要在模式中替换的客户端id。如果为空，则任何%c模式都将不匹配
 * @param username  要在模式中替换的用户名。如果为空，则任何%u模式都将不匹配
 * @param result    bool保持结果的指针。如果主题与订阅匹配，则将设置为true
 * 
 * @return
 *      MOSQ_ERR_SUCCESS - 成功
 *      MOSQ_ERR_INVAL   - 如果输入参数无效
 *      MOSQ_ERR_NOMEM   - 如果发生内存不足的情况
 */
int mosquitto_topic_matches_sub_with_pattern(const char *sub, const char *topic, const char *clientid, const char *username, bool *result);

/**
 * 检查订阅是否与ACL主题过滤器匹配
 *
 * @param acl       主题过滤器字符串来检查子
 * @param sub       要检查的订阅主题
 * @param result    保存结果的bool指针。如果订阅与acl匹配，则将设置为true
 *
 * @return
 *      MOSQ_ERR_SUCCESS - 成功
 *      MOSQ_ERR_INVAL   - 如果输入参数无效
 */
int mosquitto_sub_matches_acl(const char *acl, const char *sub, bool *result);

/**
 * 检查订阅(带通配符的主题过滤器)是否与ACL(带通配符的主题过滤器)匹配，并使用客户端 ID/用户名模式替换。任何恰好是%c或%u的ACL层次结构实例将分别替换为客户端ID或用户名
 * 
 * @param acl       ACL主题过滤器字符串来检查子
 * @param sub       订阅检查主题
 * @param clientid  要替换为模式的客户端ID。如果为NULL，则任何%c模式都将不匹配
 * @param username  要在模式中替换的用户名。如果为NULL，则任何%u个模式都将不匹配
 * @param result    保存结果的bool指针。如果订阅与ACL匹配，则将设置为true
 * 
 * @return
 *      MOSQ_ERR_SUCCESS - 成功
 *      MOSQ_ERR_INVAL   - 如果输入参数无效
 */
int mosquitto_sub_matches_acl_with_pattern(const char *acl, const char *sub, const char *clientid, const char *username, bool *result);

/**
 * 检查要发布的主题是否有效。该主题在主题中搜索+或#并检查其长度。此检查已在mosquitto_publish和mosquitto_will_set中进行，因此无需在它们之前直接调用它。
 * 例如，如果您希望在建立连接之前检查主题的有效性，则可能会很有用
 * 
 * @param topic     要检查的主题
 * 
 * @return
 *      MOSQ_ERR_SUCCESS        - 一个有效的主题
 *      MOSQ_ERR_INVAL          - 如果主题包含无效位置的+或#，或者它太长
 *      MOSQ_ERR_MALFORMED_UTF8 - 如果主题不为UTF-8
 */
int mosquitto_pub_topic_check(const char *topic);

/**
 * 检查要发布的主题是否有效。该主题在主题中搜索+或#并检查其长度。此检查已在mosquitto_publish和mosquitto_will_set中进行，因此无需在它们之前直接调用它。
 * 例如，如果您希望在建立连接之前检查主题的有效性，则可能会很有用
 * 
 * @param topic     要检查的主题
 * @param topiclen  主题长度(以字节为单位)
 * 
 * @return
 *      MOSQ_ERR_SUCCESS        - 一个有效的主题
 *      MOSQ_ERR_INVAL          - 如果主题包含无效位置的+或#，或者它太长
 *      MOSQ_ERR_MALFORMED_UTF8 - 如果主题不为UTF-8
 */
int mosquitto_pub_topic_check2(const char *topic, size_t topiclen);

/**
 * 检查用于订阅的主题是否有效，这会在主题中搜索+或#并检查它们是否不在无效位置，例如foo/#/bar，foo/+bar或foo/bar#并检查其长度。此检查已在mosquitto_subscribe和mosquitto_unsubscribe中进行，
 * 因此无需在它们之前直接调用它。例如，如果您希望在建立连接之前检查主题的有效性，则可能会很有用
 * 
 * @param topic     要检查的主题
 * 
 * @return
 *      MOSQ_ERR_SUCCESS        - 一个有效的主题
 *      MOSQ_ERR_INVAL          - 如果主题包含无效位置的+或#，或者它太长
 *      MOSQ_ERR_MALFORMED_UTF8 - 如果主题不为UTF-8
 */
int mosquitto_sub_topic_check(const char *topic);

/**
 * 检查用于订阅的主题是否有效，这会在主题中搜索+或#并检查它们是否不在无效位置，例如foo/#/bar，foo/+bar或foo/bar#并检查其长度。此检查已在mosquitto_subscribe和mosquitto_unsubscribe中进行，
 * 因此无需在它们之前直接调用它。例如，如果您希望在建立连接之前检查主题的有效性，则可能会很有用
 * 
 * @param topic     要检查的主题
 * @param topiclen  主题的字节长度
 *  
 * @return
 *      MOSQ_ERR_SUCCESS        - 一个有效的主题
 *      MOSQ_ERR_INVAL          - 如果主题包含无效位置的+或#，或者它太长
 *      MOSQ_ERR_MALFORMED_UTF8 - 如果主题不为UTF-8
 */
int mosquitto_sub_topic_check2(const char *topic, size_t topiclen);

/**
 * 根据UTF-8规范和MQTT的补充，Helper函数可以验证UTF-8字符串是否有效
 * 
 * @param str   要检查的字符串
 * @param len   字符串的长度(以字节为单位)
 * 
 * @return
 *      MOSQ_ERR_SUCCESS        - 成功
 *      MOSQ_ERR_INVAL          - 如果str为NULL或len<0或len>65536
 *      MOSQ_ERR_MALFORMED_UTF8 - 如果str无效，不为UTF-8
 */
int mosquitto_validate_utf8(const char *str, int len);

/**
 * 帮助程序功能使订阅主题和检索某些消息非常简单。这将连接到代理，订阅主题，等待msg_count消息接收，然后在完全断开连接后返回
 * 
 * @param messages          指向"struct mosquitto_message *"的指针。收到的消息将在此处返回。出错时，将其设置为NULL。
 * @param msg_count         要检索的消息数
 * @param want_retained     如果设置为true，则关于msg_count的过期保留消息将被视为普通消息。如果设置为false，它们将被忽略
 * @param topic             要使用的订阅主题(允许使用通配符)
 * @param qos               用于订阅的QOS
 * @param host              要连接的代理主机
 * @param port              代理正在侦听的网络端口
 * @param clientid          要使用的客户端ID；如果应生成随机的客户端ID，则为NULL
 * @param keepalive         MQTT keepalive值
 * @param clean_session     MQTT清洁会话标志
 * @param username          用户名字符串，如果没有用户名认证，则为NULL
 * @param password          密码字符串，如果为空则为NULL
 * @param will              包含遗嘱信息的libmosquitto_will结构，无遗嘱为NULL
 * @param tls               包含与TLS相关的参数的libmosquitto_tls结构，或者为NULL(不使用TLS)
 * 
 * @return
 *      MOSQ_ERR_SUCCESS成功，其他失败
 */
int mosquitto_subscribe_simple(struct mosquitto_message **messages, int msg_count, bool want_retained, const char *topic, int qos, const char *host, int port, const char *clientid, int keepalive, bool clean_session, const char *username, const char *password, const struct libmosquitto_will *will, const struct libmosquitto_tls *tls);

/**
 * 帮助程序功能使订阅主题和处理某些消息非常简单，这将连接到代理，订阅主题，然后将接收到的消息传递给用户提供的回调。如果回调返回1，则干净断开连接并返回
 * 
 * @param callback          回调函数，格式如下:int callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)请注意，此方法与普通的on_message回调相同，
 *                          只是它返回一个int
 * @param userdata          用户提供的指针将传递给回调
 * @param topic             要使用的订阅主题(允许使用通配符)
 * @param qos               用于订阅的QOS
 * @param host              要连接的代理主机
 * @param port              代理正在侦听的网络端口
 * @param clientid          要使用的客户端ID；如果应生成随机的客户端ID，则为NULL
 * @param keepalive         MQTT keepalive值
 * @param clean_session     MQTT清洁会话标志
 * @param username          用户名字符串，如果没有用户名认证，则为NULL
 * @param password          密码字符串，如果为空则为NULL
 * @param will              包含遗嘱信息的libmosquitto_will结构，无遗嘱为NULL
 * @param tls               包含与TLS相关的参数的libmosquitto_tls结构，或者为NULL(不使用TLS)
 * 
 * @return
 *      MOSQ_ERR_SUCCESS成功，其他失败
 */
int mosquitto_subscribe_callback(int (*callback)(struct mosquitto *, void *, const struct mosquitto_message *), void *userdata, const char *topic, int qos, const char *host, int port, const char *clientid, int keepalive, bool clean_session, const char *username, const char *password, const struct libmosquitto_will *will, const struct libmosquitto_tls *tls);

/**
 * 在属性列表中添加一个新的byte属性，如果*proplist == NULL，则将创建一个新列表，否则将新属性附加到该列表中
 *
 * @param proplist      指向mosquitto_property指针的指针，属性列表
 * @param identifier    属性标识符(例如MQTT_PROP_MESSAGE_EXPIRY_INTERVAL)
 * @param value         新属性的整数值
 *
 * @return
 *      MOSQ_ERR_SUCCESS - 成功
 *      MOSQ_ERR_INVAL   - 如果标识符无效，或者proplist为NULL
 *      MOSQ_ERR_NOMEM   - 内存不足
 */
int mosquitto_property_add_byte(mosquitto_property **proplist, int identifier, uint8_t value);

/**
 * 在属性列表中添加一个新的int16属性，如果*proplist == NULL，则将创建一个新列表，否则将新属性附加到该列表中
 *
 * @param proplist      指向mosquitto_property指针的指针，属性列表
 * @param identifier    属性标识符(例如MQTT_PROP_MESSAGE_EXPIRY_INTERVAL)
 * @param value         新属性的整数值
 *
 * @return
 *      MOSQ_ERR_SUCCESS - 成功
 *      MOSQ_ERR_INVAL   - 如果标识符无效，或者proplist为NULL
 *      MOSQ_ERR_NOMEM   - 内存不足
 */
int mosquitto_property_add_int16(mosquitto_property **proplist, int identifier, uint16_t value);

/**
 * 在属性列表中添加一个新的int32属性，如果*proplist == NULL，则将创建一个新列表，否则将新属性附加到该列表中
 *
 * @param proplist      指向mosquitto_property指针的指针，属性列表
 * @param identifier    属性标识符(例如MQTT_PROP_MESSAGE_EXPIRY_INTERVAL)
 * @param value         新属性的整数值
 *
 * @return
 *      MOSQ_ERR_SUCCESS - 成功
 *      MOSQ_ERR_INVAL   - 如果标识符无效，或者proplist为NULL
 *      MOSQ_ERR_NOMEM   - 内存不足
 */
int mosquitto_property_add_int32(mosquitto_property **proplist, int identifier, uint32_t value);

/**
 * 在属性列表中添加一个新的varint属性，如果*proplist == NULL，则将创建一个新列表，否则将新属性添加到该列表中
 *
 * @param proplist      指向mosquitto_property指针的指针，属性列表
 * @param identifier    属性标识符(例如MQTT_PROP_SUBSCRIPTION_IDENTIFIER)
 * @param value         新属性的整数值
 *
 * @return
 *      MOSQ_ERR_SUCCESS - 成功
 *      MOSQ_ERR_INVAL   - 如果标识符无效，或者proplist为NULL
 *      MOSQ_ERR_NOMEM   - 内存不足
 */
int mosquitto_property_add_varint(mosquitto_property **proplist, int identifier, uint32_t value);

/**
 * 在属性列表中添加一个新的字符串属性，如果*proplist == NULL，则将创建一个新列表，否则将新属性添加到该列表中
 *
 * @param proplist      指向mosquitto_property指针的指针，属性列表
 * @param identifier    属性标识符(例如MQTT_PROP_CONTENT_TYPE)
 * @param value         新属性的字符串值，必须为UTF-8并且零终止
 *
 * @return
 *      MOSQ_ERR_SUCCESS        - 成功
 *      MOSQ_ERR_INVAL          - 如果标识符无效，如果值为NULL或proplist为NULL
 *      MOSQ_ERR_NOMEM          - 内存不足
 *      MOSQ_ERR_MALFORMED_UTF8 - 如果名称或值无效，不为UTF-8
 */
int mosquitto_property_add_string(mosquitto_property **proplist, int identifier, const char *value);

/**
 * 在属性列表中添加一个新的二进制属性，如果*proplist == NULL，将创建一个新列表，否则将新属性附加到该列表中
 *
 * @param proplist      指向mosquitto_property指针的指针，属性列表
 * @param identifier    属性标识符(例如MQTT_PROP_PAYLOAD_FORMAT_INDICATOR)
 * @param value         指向属性数据的指针
 * @param len           属性数据的长度(以字节为单位)
 *
 * @return
 *      MOSQ_ERR_SUCCESS - 成功
 *      MOSQ_ERR_INVAL   - 如果标识符无效，或者proplist为NULL
 *      MOSQ_ERR_NOMEM   - 内存不足
 */
int mosquitto_property_add_binary(mosquitto_property **proplist, int identifier, const void *value, uint16_t len);

/**
 * 在属性列表中添加一个新的字符串对属性，如果*proplist == NULL，则将创建一个新列表，否则将新属性附加到该列表中
 *
 * @param proplist      指向mosquitto_property指针的指针，属性列表
 * @param identifier    属性标识符(例如MQTT_PROP_USER_PROPERTY)
 * @param name          新属性的字符串名称，必须为UTF-8且零终止
 * @param value         新属性的字符串值，必须为UTF-8并且零终止
 *
 * @return
 *      MOSQ_ERR_SUCCESS        - 成功
 *      MOSQ_ERR_INVAL          - 如果标识符无效，名称或值为NULL或属性列表为NULL
 *      MOSQ_ERR_NOMEM          - 内存不足
 *      MOSQ_ERR_MALFORMED_UTF8 - 如果名称或值无效，不为UTF-8
 */
int mosquitto_property_add_string_pair(mosquitto_property **proplist, int identifier, const char *name, const char *value);

int mosquitto_property_remove(mosquitto_property **proplist, const mosquitto_property *property);

/**
 * 返回单个属性的属性标识符
 * 
 * @param property  指向有效mosquitto_property指针的指针
 * 
 * @return
 *      成功时有效的属性标识符，0错误
 */
int mosquitto_property_identifier(const mosquitto_property *property);

/**
 * 返回属性列表中的下一个属性。用于遍历属性列表
 * 
 * @param proplist  指向mosquitto_property指针的指针，属性列表
 * 
 * @return
 *      如果proplist为NULL，或者列表中没有其他项目，则指向列表NULL中的下一个项目
 */
mosquitto_property *mosquitto_property_next(const mosquitto_property *proplist);

/**
 * 尝试从属性列表或单个属性中读取与标识符匹配的字节属性。该函数可以使用返回值和skip_first搜索具有相同标识符的多个条目。但是请注意，禁止大多数属性重复。
 * 如果未找到该属性，则*value不会被修改，因此可以安全地传递一个具有默认值的变量，该变量有可能被覆盖
 * 
 * @param proplist      要读取的属性
 * @param identifier    属性标识符(例如MQTT_PROP_PAYLOAD_FORMAT_INDICATOR)
 * @param value         存储值的指针，如果不需要该值，则为NULL
 * @param skip_first    布尔值，指示是否应忽略列表中的第一项。通常应设置为false
 * 
 * @return
 *      如果找到的属性为NULL，找不到的属性或属性列表为NULL，则为有效的属性指针
 */
const mosquitto_property *mosquitto_property_read_byte(const mosquitto_property *proplist, int identifier, uint8_t *value, bool skip_first);

/**
 * 从属性读取字符串属性值。成功时，值必须由应用程序进行free()处理
 * 
 * @param proplist      要读取的属性
 * @param identifier    属性标识符(例如MQTT_PROP_PAYLOAD_FORMAT_INDICATOR)
 * @param value         指向char *的指针，用于存储属性数据；如果不需要该值，则为NULL
 * @param skip_first    布尔值，指示是否应忽略列表中的第一项。通常应设置为false
 * 
 * @return
 *      如果找到的属性为NULL，找不到的属性或属性列表为NULL或发生内存不足的情况，则为有效的属性指针
 */
const mosquitto_property *mosquitto_property_read_string(const mosquitto_property *proplist, int identifier, char **value, bool skip_first);

/**
 * 从属性读取int16属性值
 * 
 * @param proplist      要读取的属性
 * @param identifier    属性标识符(例如MQTT_PROP_PAYLOAD_FORMAT_INDICATOR)
 * @param value         存储值的指针，如果不需要该值，则为NULL
 * @param skip_first    布尔值，指示是否应忽略列表中的第一项。通常应设置为false
 * 
 * @return
 *      如果找到的属性为NULL，找不到的属性或属性列表为NULL，则为有效的属性指针
 */
const mosquitto_property *mosquitto_property_read_int16(const mosquitto_property *proplist, int identifier, uint16_t *value, bool skip_first);

/**
 * 从属性读取int32属性值
 * 
 * @param proplist      要读取的属性
 * @param identifier    属性标识符(例如MQTT_PROP_PAYLOAD_FORMAT_INDICATOR)
 * @param value         存储值的指针，如果不需要该值，则为NULL
 * @param skip_first    布尔值，指示是否应忽略列表中的第一项。通常应设置为false
 * 
 * @return
 *      如果找到的属性为NULL，找不到的属性或属性列表为NULL，则为有效的属性指针
 */
const mosquitto_property *mosquitto_property_read_int32(const mosquitto_property *proplist, int identifier, uint32_t *value, bool skip_first);

/**
 * 从属性读取varint属性值
 * 
 * @param proplist      要读取的属性
 * @param identifier    属性标识符(例如MQTT_PROP_PAYLOAD_FORMAT_INDICATOR)
 * @param value         存储值的指针，如果不需要该值，则为NULL
 * @param skip_first    布尔值，指示是否应忽略列表中的第一项。通常应设置为false
 * 
 * @return
 *      如果找到的属性为NULL，找不到的属性或属性列表为NULL，则为有效的属性指针
 */
const mosquitto_property *mosquitto_property_read_varint(const mosquitto_property *proplist, int identifier, uint32_t *value, bool skip_first);

/**
 * 从属性读取二进制属性值。成功后，名称和值必须由应用程序进行free()处理
 * 
 * @param proplist      要读取的属性
 * @param identifier    属性标识符(例如MQTT_PROP_PAYLOAD_FORMAT_INDICATOR)
 * @param value         存储值的指针，如果不需要该值，则为NULL
 * @param len           读取值的长度
 * @param skip_first    布尔值，指示是否应忽略列表中的第一项。通常应设置为false
 * 
 * @return
 *      如果找到的属性为NULL，找不到的属性或属性列表为NULL或发生内存不足的情况，则为有效的属性指针
 */
const mosquitto_property *mosquitto_property_read_binary(const mosquitto_property *proplist, int identifier, void **value, uint16_t *len, bool skip_first);

/**
 * 从属性读取字符串对属性值对。成功后，名称和值必须由应用程序进行free()处理
 * 
 * @param proplist      要读取的属性
 * @param identifier    属性标识符(例如MQTT_PROP_PAYLOAD_FORMAT_INDICATOR)
 * @param name          用于存储名称属性数据的char *指针；如果不需要名称，则为NULL
 * @param value         指向char *的指针，用于存储属性数据；如果不需要该值，则为NULL
 * @param skip_first    布尔值，指示是否应忽略列表中的第一项。通常应设置为false
 * 
 * @return
 *      如果找到的属性为NULL，找不到的属性或属性列表为NULL或发生内存不足的情况，则为有效的属性指针
 */
const mosquitto_property *mosquitto_property_read_string_pair(const mosquitto_property *proplist, int identifier, char **name, char **value, bool skip_first);

int mosquitto_property_type(const mosquitto_property *property);
uint8_t mosquitto_property_byte_value(const mosquitto_property *property);
uint16_t mosquitto_property_int16_value(const mosquitto_property *property);
uint32_t mosquitto_property_int32_value(const mosquitto_property *property);
uint32_t mosquitto_property_varint_value(const mosquitto_property *property);
const void *mosquitto_property_binary_value(const mosquitto_property *property);
uint16_t mosquitto_property_binary_value_length(const mosquitto_property *property);
const char *mosquitto_property_string_value(const mosquitto_property *property);
uint16_t mosquitto_property_string_value_length(const mosquitto_property *property);
const char *mosquitto_property_string_name(const mosquitto_property *property);
uint16_t mosquitto_property_string_name_length(const mosquitto_property *property);

/**
 * 从属性列表中释放所有属性。释放列表并将*properties设置为NULL
 * 
 * @param properties    可用属性列表
 */
void mosquitto_property_free_all(mosquitto_property **properties);

/**
 * mosquitto属性拷贝
 * 
 * @param dest  新属性列表的指针
 * @param src   源属性列表的指针
 *
 * @return
 *      MOSQ_ERR_SUCCESS - 成功
 *      MOSQ_ERR_INVAL   - 如果dest为NULL
 *      MOSQ_ERR_NOMEM   - 内存不足时(目标将设置为NULL)
 */
int mosquitto_property_copy_all(mosquitto_property **dest, const mosquitto_property *src);

/**
 * 检查属性标识符对于给定命令是否有效
 *
 * @param command     MQTT命令(例如CMD_CONNECT)
 * @param identifier  MQTT属性(例如MQTT_PROP_USER_PROPERTY)
 *
 * @return
 *      MOSQ_ERR_SUCCESS  - 如果标识符对于命令有效
 *      MOSQ_ERR_PROTOCOL - 如果标识符对于与命令一起使用无效
 */
int mosquitto_property_check_command(int command, int identifier);

/**
 * 检查属性列表对于特定命令是否有效，是否存在重复项以及在可能的情况下值是否有效。请注意，每当将属性传递给该函数时，该函数便会在库中内部使用，因此在基本用法中并不需要此函数，
 * 但是在使用属性点之前检查属性列表会有所帮助
 * 
 * @param command       MQTT命令(例如CMD_CONNECT)
 * @param properties    要检查的MQTT属性列表
 *  
 * @return
 *      MOSQ_ERR_SUCCESS            - 如果所有属性均有效
 *      MOSQ_ERR_DUPLICATE_PROPERTY - 如果属性在禁止的地方重复
 *      MOSQ_ERR_PROTOCOL           - 如果任何属性无效
 */
int mosquitto_property_check_all(int command, const mosquitto_property *properties);

/**
 * 返回属性名称作为属性标识符的字符串。属性名称是在MQTT规范中定义的，用-作为分隔符，例如：payload-format-indicator
 *
 * @param identifier  有效的MQTT属性标识符整数
 *
 * @return
 *      成功时属性名称的const字符串失败时为NULL
 */
const char *mosquitto_property_identifier_to_string(int identifier);

/**
 * 解析属性名称字符串，并将其转换为属性标识符和数据类型。属性名称是在MQTT规范中定义的，用-作为分隔符，例如：payload-format-indicator
 * 
 * @param propname      要解析的字符串
 * @param identifier    指向要接收属性标识符的int的指针
 * @param type          指向要接收属性类型的int的指针
 * 
 * @return
 *      MOSQ_ERR_SUCCESS - 成功
 *      MOSQ_ERR_INVAL   - 如果字符串与属性不匹配
 */
int mosquitto_string_to_property_info(const char *propname, int *identifier, int *type);

#ifdef __cplusplus
}
#endif

#endif
