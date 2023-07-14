#ifndef MOSQ_NET_MOSQ_H
#define MOSQ_NET_MOSQ_H

#include <unistd.h>
#include <mosquitto/mosquitto.h>

#include "mosquitto_internal.h"

#define COMPAT_CLOSE(a)             close(a)
#define COMPAT_ECONNRESET           ECONNRESET
#define COMPAT_EINTR                EINTR
#define COMPAT_EWOULDBLOCK          EWOULDBLOCK

#ifndef INVALID_SOCKET
#define INVALID_SOCKET              (-1)
#endif

#define MOSQ_MSB(A)                 (uint8_t)((A & 0xFF00) >> 8)
#define MOSQ_LSB(A)                 (uint8_t)(A & 0x00FF)

int net__init(void);
void net__cleanup(void);

#ifdef WITH_TLS
void net__init_tls(void);
#endif

int net__socket_connect(struct mosquitto *mosq, const char *host, uint16_t port, const char *bind_address, bool blocking);
int net__socket_close(struct mosquitto *mosq);
int net__try_connect(const char *host, uint16_t port, mosq_sock_t *sock, const char *bind_address, bool blocking);
int net__try_connect_step1(struct mosquitto *mosq, const char *host);
int net__try_connect_step2(struct mosquitto *mosq, uint16_t port, mosq_sock_t *sock);
int net__socket_connect_step3(struct mosquitto *mosq, const char *host);
int net__socket_nonblock(mosq_sock_t *sock);
int net__socketpair(mosq_sock_t *sp1, mosq_sock_t *sp2);
bool net__is_connected(struct mosquitto *mosq);

ssize_t net__read(struct mosquitto *mosq, void *buf, size_t count);
ssize_t net__read_ws(struct mosquitto *mosq, void *buf, size_t count);
ssize_t net__write(struct mosquitto *mosq, const void *buf, size_t count);

#ifdef WITH_TLS
void net__print_ssl_error(struct mosquitto *mosq);
int net__socket_apply_tls(struct mosquitto *mosq);
int net__socket_connect_tls(struct mosquitto *mosq);
int mosquitto__verify_ocsp_status_cb(SSL *ssl, void *arg);
UI_METHOD *net__get_ui_method(void);

#define ENGINE_FINISH(e)        if (e) ENGINE_finish(e)
#define ENGINE_SECRET_MODE      "SECRET_MODE"
#define ENGINE_SECRET_MODE_SHA  0x1000
#define ENGINE_PIN              "PIN"
#endif

#endif
