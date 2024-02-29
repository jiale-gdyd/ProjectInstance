#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <netdb.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <mosquitto/mqtt_protocol.h>

#include "config.h"

#ifdef WITH_TLS
#include <openssl/conf.h>
#include <openssl/engine.h>
#include <openssl/err.h>
#include <openssl/ui.h>
#include "tls_mosq.h"
#endif

#include "net_mosq.h"
#include "time_mosq.h"
#include "util_mosq.h"
#include "read_handle.h"
#include "memory_mosq.h"
#include "logging_mosq.h"

#ifdef WITH_TLS
int tls_ex_index_mosq = -1;
UI_METHOD *_ui_method = NULL;

static bool is_tls_initialized = false;

// 来自OpenSSL s_server/s_client的功能
static int ui_open(UI *ui)
{
    return UI_method_get_opener(UI_OpenSSL())(ui);
}

static int ui_read(UI *ui, UI_STRING *uis)
{
    return UI_method_get_reader(UI_OpenSSL())(ui, uis);
}

static int ui_write(UI *ui, UI_STRING *uis)
{
    return UI_method_get_writer(UI_OpenSSL())(ui, uis);
}

static int ui_close(UI *ui)
{
    return UI_method_get_closer(UI_OpenSSL())(ui);
}

static void setup_ui_method(void)
{
    _ui_method = UI_create_method("OpenSSL application user interface");
    UI_method_set_opener(_ui_method, ui_open);
    UI_method_set_reader(_ui_method, ui_read);
    UI_method_set_writer(_ui_method, ui_write);
    UI_method_set_closer(_ui_method, ui_close);
}

static void cleanup_ui_method(void)
{
    if (_ui_method) {
        UI_destroy_method(_ui_method);
        _ui_method = NULL;
    }
}

UI_METHOD *net__get_ui_method(void)
{
    return _ui_method;
}
#endif

int net__init(void)
{
    return MOSQ_ERR_SUCCESS;
}

void net__cleanup(void)
{
#ifdef WITH_TLS
#if !defined(OPENSSL_NO_ENGINE) && OPENSSL_API_LEVEL < 30000
    ENGINE_cleanup();
#endif
    is_tls_initialized = false;
    cleanup_ui_method();
#endif
}

#ifdef WITH_TLS
void net__init_tls(void)
{
    if (is_tls_initialized) {
        return;
    }

    OPENSSL_init_crypto(OPENSSL_INIT_ADD_ALL_CIPHERS | OPENSSL_INIT_ADD_ALL_DIGESTS | OPENSSL_INIT_LOAD_CONFIG, NULL);

#if !defined(OPENSSL_NO_ENGINE) && OPENSSL_API_LEVEL < 30000
    ENGINE_load_builtin_engines();
#endif

    setup_ui_method();
    if (tls_ex_index_mosq == -1) {
        tls_ex_index_mosq = SSL_get_ex_new_index(0, (void *)"client context", NULL, NULL, NULL);
    }

    is_tls_initialized = true;
}
#endif

bool net__is_connected(struct mosquitto *mosq)
{
    return (mosq->sock != INVALID_SOCKET);
}

int net__socket_close(struct mosquitto *mosq)
{
    int rc = 0;

    assert(mosq);

#ifdef WITH_TLS
    if (mosq->ssl) {
        if (!SSL_in_init(mosq->ssl)) {
            SSL_shutdown(mosq->ssl);
        }

        SSL_free(mosq->ssl);
        mosq->ssl = NULL;
    }
#endif

    if (net__is_connected(mosq)) {
        rc = COMPAT_CLOSE(mosq->sock);
        mosq->sock = INVALID_SOCKET;
    }

    return rc;
}

int net__socket_shutdown(struct mosquitto *mosq)
{
    int rc = 0;

    assert(mosq);

    if (net__is_connected(mosq)){
        rc = COMPAT_SHUTDOWN(mosq->sock);
    }

    return rc;
}

#ifdef FINAL_WITH_TLS_PSK
static unsigned int psk_client_callback(SSL *ssl, const char *hint, char *identity, unsigned int max_identity_len, unsigned char *psk, unsigned int max_psk_len)
{
    int len;
    struct mosquitto *mosq;

    MOSQ_UNUSED(hint);

    mosq = (struct mosquitto *)SSL_get_ex_data(ssl, tls_ex_index_mosq);
    if (!mosq) {
        return 0;
    }

    snprintf(identity, max_identity_len, "%s", mosq->tls_psk_identity);

    len = mosquitto__hex2bin(mosq->tls_psk, psk, (int)max_psk_len);
    if (len < 0) {
        return 0;
    }

    return (unsigned int)len;
}
#endif

static int net__try_connect_tcp(const char *host, uint16_t port, mosq_sock_t *sock, const char *bind_address, bool blocking)
{
    int s;
    struct addrinfo hints;
    int rc = MOSQ_ERR_SUCCESS;
    struct addrinfo *ainfo, *rp;
    struct addrinfo *ainfo_bind, *rp_bind;

    ainfo_bind = NULL;

    *sock = INVALID_SOCKET;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    s = getaddrinfo(host, NULL, &hints, &ainfo);
    if (s) {
        errno = s;
        return MOSQ_ERR_EAI;
    }

    if (bind_address) {
        s = getaddrinfo(bind_address, NULL, &hints, &ainfo_bind);
        if (s) {
            freeaddrinfo(ainfo);
            errno = s;
            return MOSQ_ERR_EAI;
        }
    }

    for (rp = ainfo; rp != NULL; rp = rp->ai_next) {
        *sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (*sock == INVALID_SOCKET) {
            continue;
        }

        if (rp->ai_family == AF_INET) {
            ((struct sockaddr_in *)rp->ai_addr)->sin_port = htons(port);
        } else if (rp->ai_family == AF_INET6) {
            ((struct sockaddr_in6 *)rp->ai_addr)->sin6_port = htons(port);
        } else {
            COMPAT_CLOSE(*sock);
            *sock = INVALID_SOCKET;
            continue;
        }

        if (bind_address) {
            for (rp_bind = ainfo_bind; rp_bind != NULL; rp_bind = rp_bind->ai_next) {
                if (bind(*sock, rp_bind->ai_addr, rp_bind->ai_addrlen) == 0) {
                    break;
                }
            }

            if (!rp_bind) {
                COMPAT_CLOSE(*sock);
                *sock = INVALID_SOCKET;
                continue;
            }
        }

        if (!blocking) {
            // 设置非阻塞
            if (net__socket_nonblock(sock)) {
                continue;
            }
        }

        rc = connect(*sock, rp->ai_addr, rp->ai_addrlen);
        if ((rc == 0) || (errno == EINPROGRESS) || (errno == COMPAT_EWOULDBLOCK)) {
            if ((rc < 0) && ((errno == EINPROGRESS) || (errno == COMPAT_EWOULDBLOCK))) {
                rc = MOSQ_ERR_CONN_PENDING;
            }

            if (blocking) {
                // 设置非阻塞
                if (net__socket_nonblock(sock)) {
                    continue;
                }
            }
            break;
        }

        COMPAT_CLOSE(*sock);
        *sock = INVALID_SOCKET;
    }
    freeaddrinfo(ainfo);

    if (bind_address) {
        freeaddrinfo(ainfo_bind);
    }

    if (!rp) {
        return MOSQ_ERR_ERRNO;
    }

    return rc;
}

static int net__try_connect_unix(const char *host, mosq_sock_t *sock)
{
    int s, rc;
    struct sockaddr_un addr;

    if ((host == NULL) || (strlen(host) == 0) || (strlen(host) > (sizeof(addr.sun_path) - 1))) {
        return MOSQ_ERR_INVAL;
    }

    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, host, sizeof(addr.sun_path)-1);

    s = socket(AF_UNIX, SOCK_STREAM, 0);
    if (s < 0) {
        return MOSQ_ERR_ERRNO;
    }

    rc = net__socket_nonblock(&s);
    if (rc) {
        return rc;
    }

    rc = connect(s, (struct sockaddr *)&addr, sizeof(struct sockaddr_un));
    if (rc < 0) {
        close(s);
        return MOSQ_ERR_ERRNO;
    }
    *sock = s;

    return 0;
}

int net__try_connect(const char *host, uint16_t port, mosq_sock_t *sock, const char *bind_address, bool blocking)
{
    if (port == 0) {
        return net__try_connect_unix(host, sock);
    } else {
        return net__try_connect_tcp(host, port, sock, bind_address, blocking);
    }
}

#ifdef WITH_TLS
void net__print_ssl_error(struct mosquitto *mosq)
{
    int num = 0;
    char ebuf[256];
    unsigned long e;

    e = ERR_get_error();
    while (e) {
        log__printf(mosq, MOSQ_LOG_ERR, "OpenSSL Error[%d]: %s", num, ERR_error_string(e, ebuf));
        e = ERR_get_error();
        num++;
    }
}

int net__socket_connect_tls(struct mosquitto *mosq)
{
    long res;

    ERR_clear_error();
    if (mosq->tls_ocsp_required) {
        // OCSP在所有当前受支持的OpenSSL版本中均可用
        if ((res = SSL_set_tlsext_status_type(mosq->ssl, TLSEXT_STATUSTYPE_ocsp)) != 1) {
            log__printf(mosq, MOSQ_LOG_ERR, "Could not activate OCSP (error: %ld)", res);
            return MOSQ_ERR_OCSP;
        }

        if ((res = SSL_CTX_set_tlsext_status_cb(mosq->ssl_ctx, mosquitto__verify_ocsp_status_cb)) != 1) {
            log__printf(mosq, MOSQ_LOG_ERR, "Could not activate OCSP (error: %ld)", res);
            return MOSQ_ERR_OCSP;
        }

        if ((res = SSL_CTX_set_tlsext_status_arg(mosq->ssl_ctx, mosq)) != 1) {
            log__printf(mosq, MOSQ_LOG_ERR, "Could not activate OCSP (error: %ld)", res);
            return MOSQ_ERR_OCSP;
        }
    }

    SSL_set_connect_state(mosq->ssl);

    return MOSQ_ERR_SUCCESS;
}
#endif

#ifdef WITH_TLS
static int net__tls_load_ca(struct mosquitto *mosq)
{
    int ret;

    if (mosq->tls_use_os_certs) {
        SSL_CTX_set_default_verify_paths(mosq->ssl_ctx);
    }

    if (mosq->tls_cafile) {
        ret = 0;//SSL_CTX_load_verify_file(mosq->ssl_ctx, mosq->tls_cafile);
        if (ret == 0) {
            log__printf(mosq, MOSQ_LOG_ERR, "Error: Unable to load CA certificates, check cafile \"%s\".", mosq->tls_cafile);
            return MOSQ_ERR_TLS;
        }
    }

    if (mosq->tls_capath) {
        ret = 0;//SSL_CTX_load_verify_dir(mosq->ssl_ctx, mosq->tls_capath);
        if (ret == 0) {
            log__printf(mosq, MOSQ_LOG_ERR, "Error: Unable to load CA certificates, check capath \"%s\".", mosq->tls_capath);
            return MOSQ_ERR_TLS;
        }
    }

    return MOSQ_ERR_SUCCESS;
}

static int net__init_ssl_ctx(struct mosquitto *mosq)
{
    int ret;
#if !defined(OPENSSL_NO_ENGINE) && OPENSSL_API_LEVEL < 30000
    EVP_PKEY *pkey;
    ENGINE *engine = NULL;
#endif
    uint8_t tls_alpn_len;
    uint8_t tls_alpn_wire[256];

    if (mosq->user_ssl_ctx) {
        mosq->ssl_ctx = mosq->user_ssl_ctx;

        if (!mosq->ssl_ctx_defaults) {
            return MOSQ_ERR_SUCCESS;
        } else if (!mosq->tls_cafile && !mosq->tls_capath && !mosq->tls_psk) {
            log__printf(mosq, MOSQ_LOG_ERR, "Error: If you use MOSQ_OPT_SSL_CTX then MOSQ_OPT_SSL_CTX_WITH_DEFAULTS must be true, or at least one of cafile, capath or psk must be specified.");
            return MOSQ_ERR_INVAL;
        }
    }

    // 应用默认的SSL_CTX设置。仅在未设置MOSQ_OPT_SSL_CTX或同时设置MOSQ_OPT_SSL_CTX和MOSQ_OPT_SSL_CTX_WITH_DEFAULTS的情况下才使用此选项
    if (mosq->tls_cafile || mosq->tls_capath || mosq->tls_psk || mosq->tls_use_os_certs) {
        if (!mosq->ssl_ctx) {
            net__init_tls();

            mosq->ssl_ctx = SSL_CTX_new(TLS_client_method());
            if (!mosq->ssl_ctx) {
                log__printf(mosq, MOSQ_LOG_ERR, "Error: Unable to create TLS context.");
                net__print_ssl_error(mosq);
                return MOSQ_ERR_TLS;
            }
        }

#ifdef SSL_OP_NO_TLSv1_3
        if (mosq->tls_psk) {
            SSL_CTX_set_options(mosq->ssl_ctx, SSL_OP_NO_TLSv1_3);
        }
#endif
        if (!mosq->tls_version) {
            SSL_CTX_set_options(mosq->ssl_ctx, SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1);
#ifdef SSL_OP_NO_TLSv1_3
        } else if (!strcmp(mosq->tls_version, "tlsv1.3")) {
            SSL_CTX_set_options(mosq->ssl_ctx, SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1 | SSL_OP_NO_TLSv1_2);
#endif
        } else if (!strcmp(mosq->tls_version, "tlsv1.2")) {
            SSL_CTX_set_options(mosq->ssl_ctx, SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1);
        } else if (!strcmp(mosq->tls_version, "tlsv1.1")) {
            SSL_CTX_set_options(mosq->ssl_ctx, SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1);
        } else {
            log__printf(mosq, MOSQ_LOG_ERR, "Error: Protocol %s not supported.", mosq->tls_version);
            return MOSQ_ERR_INVAL;
        }

        // 允许使用DHE密码
        SSL_CTX_set_dh_auto(mosq->ssl_ctx, 1);

        // 禁用压缩
        SSL_CTX_set_options(mosq->ssl_ctx, SSL_OP_NO_COMPRESSION);

        // 设置ALPN
        if (mosq->tls_alpn) {
            tls_alpn_len = (uint8_t) strnlen(mosq->tls_alpn, 254);
            tls_alpn_wire[0] = tls_alpn_len;    // 第一个字节是字符串的长度
            memcpy(tls_alpn_wire + 1, mosq->tls_alpn, tls_alpn_len);
            SSL_CTX_set_alpn_protos(mosq->ssl_ctx, tls_alpn_wire, tls_alpn_len + 1U);
        }

#ifdef SSL_MODE_RELEASE_BUFFERS
        // 每个SSL连接使用更少的内存
        SSL_CTX_set_mode(mosq->ssl_ctx, SSL_MODE_RELEASE_BUFFERS);
#endif

#if !defined(OPENSSL_NO_ENGINE) && OPENSSL_API_LEVEL < 30000
        if (mosq->tls_engine) {
            engine = ENGINE_by_id(mosq->tls_engine);
            if (!engine) {
                log__printf(mosq, MOSQ_LOG_ERR, "Error loading %s engine\n", mosq->tls_engine);
                return MOSQ_ERR_TLS;
            }

            if (!ENGINE_init(engine)) {
                log__printf(mosq, MOSQ_LOG_ERR, "Failed engine initialisation\n");
                ENGINE_free(engine);
                return MOSQ_ERR_TLS;
            }

            ENGINE_set_default(engine, ENGINE_METHOD_ALL);
            ENGINE_free(engine);
        }
#endif
        if (mosq->tls_ciphers) {
            ret = SSL_CTX_set_cipher_list(mosq->ssl_ctx, mosq->tls_ciphers);
            if (ret == 0) {
                log__printf(mosq, MOSQ_LOG_ERR, "Error: Unable to set TLS ciphers. Check cipher list \"%s\".", mosq->tls_ciphers);
#if !defined(OPENSSL_NO_ENGINE) && OPENSSL_API_LEVEL < 30000
                ENGINE_FINISH(engine);
#endif
                net__print_ssl_error(mosq);
                return MOSQ_ERR_TLS;
            }
        }

        if (mosq->tls_13_ciphers) {
            ret = SSL_CTX_set_ciphersuites(mosq->ssl_ctx, mosq->tls_13_ciphers);
            if (ret == 0) {
                log__printf(mosq, MOSQ_LOG_ERR, "Error: Unable to set TLS 1.3 ciphersuites. Check cipher_tls13 list \"%s\".", mosq->tls_13_ciphers);
                return MOSQ_ERR_TLS;
            }
        }

        if (mosq->tls_cafile || mosq->tls_capath || mosq->tls_use_os_certs) {
            ret = net__tls_load_ca(mosq);
            if (ret != MOSQ_ERR_SUCCESS) {
#if !defined(OPENSSL_NO_ENGINE) && OPENSSL_API_LEVEL < 30000
                ENGINE_FINISH(engine);
#endif
                net__print_ssl_error(mosq);
                return MOSQ_ERR_TLS;
            }

            if (mosq->tls_cert_reqs == 0) {
                SSL_CTX_set_verify(mosq->ssl_ctx, SSL_VERIFY_NONE, NULL);
            } else {
                SSL_CTX_set_verify(mosq->ssl_ctx, SSL_VERIFY_PEER, mosquitto__server_certificate_verify);
            }

            if (mosq->tls_pw_callback) {
                SSL_CTX_set_default_passwd_cb(mosq->ssl_ctx, mosq->tls_pw_callback);
                SSL_CTX_set_default_passwd_cb_userdata(mosq->ssl_ctx, mosq);
            }

            if (mosq->tls_certfile) {
                ret = SSL_CTX_use_certificate_chain_file(mosq->ssl_ctx, mosq->tls_certfile);
                if (ret != 1) {
                    log__printf(mosq, MOSQ_LOG_ERR, "Error: Unable to load client certificate \"%s\".", mosq->tls_certfile);

#if !defined(OPENSSL_NO_ENGINE) && OPENSSL_API_LEVEL < 30000
                    ENGINE_FINISH(engine);
#endif
                    net__print_ssl_error(mosq);
                    return MOSQ_ERR_TLS;
                }
            }

            if (mosq->tls_keyfile) {
                if (mosq->tls_keyform == mosq_k_engine) {
#if !defined(OPENSSL_NO_ENGINE) && OPENSSL_API_LEVEL < 30000
                    UI_METHOD *ui_method = net__get_ui_method();
                    if (mosq->tls_engine_kpass_sha1) {
                        if (!ENGINE_ctrl_cmd(engine, ENGINE_SECRET_MODE, ENGINE_SECRET_MODE_SHA, NULL, NULL, 0)) {
                            log__printf(mosq, MOSQ_LOG_ERR, "Error: Unable to set engine secret mode sha1");
                            ENGINE_FINISH(engine);
                            net__print_ssl_error(mosq);
                            return MOSQ_ERR_TLS;
                        }

                        if (!ENGINE_ctrl_cmd(engine, ENGINE_PIN, 0, mosq->tls_engine_kpass_sha1, NULL, 0)) {
                            log__printf(mosq, MOSQ_LOG_ERR, "Error: Unable to set engine pin");
                            ENGINE_FINISH(engine);
                            net__print_ssl_error(mosq);
                            return MOSQ_ERR_TLS;
                        }
                        ui_method = NULL;
                    }

                    pkey = ENGINE_load_private_key(engine, mosq->tls_keyfile, ui_method, NULL);
                    if (!pkey) {
                        log__printf(mosq, MOSQ_LOG_ERR, "Error: Unable to load engine private key file \"%s\".", mosq->tls_keyfile);
                        ENGINE_FINISH(engine);
                        net__print_ssl_error(mosq);
                        return MOSQ_ERR_TLS;
                    }

                    if (SSL_CTX_use_PrivateKey(mosq->ssl_ctx, pkey) <= 0) {
                        log__printf(mosq, MOSQ_LOG_ERR, "Error: Unable to use engine private key file \"%s\".", mosq->tls_keyfile);
                        ENGINE_FINISH(engine);
                        net__print_ssl_error(mosq);
                        return MOSQ_ERR_TLS;
                    }
#endif
                } else {
                    ret = SSL_CTX_use_PrivateKey_file(mosq->ssl_ctx, mosq->tls_keyfile, SSL_FILETYPE_PEM);
                    if (ret != 1) {
                        log__printf(mosq, MOSQ_LOG_ERR, "Error: Unable to load client key file \"%s\".", mosq->tls_keyfile);

#if !defined(OPENSSL_NO_ENGINE) && OPENSSL_API_LEVEL < 30000
                        ENGINE_FINISH(engine);
#endif
                        net__print_ssl_error(mosq);
                        return MOSQ_ERR_TLS;
                    }
                }

                ret = SSL_CTX_check_private_key(mosq->ssl_ctx);
                if (ret != 1) {
                    log__printf(mosq, MOSQ_LOG_ERR, "Error: Client certificate/key are inconsistent.");
#if !defined(OPENSSL_NO_ENGINE) && OPENSSL_API_LEVEL < 30000
                    ENGINE_FINISH(engine);
#endif
                    net__print_ssl_error(mosq);
                    return MOSQ_ERR_TLS;
                }
            }
#ifdef FINAL_WITH_TLS_PSK
        } else if (mosq->tls_psk) {
            SSL_CTX_set_psk_client_callback(mosq->ssl_ctx, psk_client_callback);
            if (mosq->tls_ciphers == NULL) {
                SSL_CTX_set_cipher_list(mosq->ssl_ctx, "PSK");
            }
#endif
        }
    }

    return MOSQ_ERR_SUCCESS;
}
#endif

int net__socket_connect_step3(struct mosquitto *mosq, const char *host)
{
#ifdef WITH_TLS
    BIO *bio;

    int rc = net__init_ssl_ctx(mosq);
    if (rc) {
        net__socket_close(mosq);
        return rc;
    }

    if (mosq->ssl_ctx) {
        if (mosq->ssl) {
            SSL_free(mosq->ssl);
        }

        mosq->ssl = SSL_new(mosq->ssl_ctx);
        if (!mosq->ssl) {
            net__socket_close(mosq);
            net__print_ssl_error(mosq);
            return MOSQ_ERR_TLS;
        }

        SSL_set_ex_data(mosq->ssl, tls_ex_index_mosq, mosq);
        bio = BIO_new_socket(mosq->sock, BIO_NOCLOSE);
        if (!bio) {
            net__socket_close(mosq);
            net__print_ssl_error(mosq);
            return MOSQ_ERR_TLS;
        }
        SSL_set_bio(mosq->ssl, bio, bio);

        // SNI解析所需
        if (SSL_set_tlsext_host_name(mosq->ssl, host) != 1) {
            net__socket_close(mosq);
            return MOSQ_ERR_TLS;
        }

        if (net__socket_connect_tls(mosq)) {
            net__socket_close(mosq);
            return MOSQ_ERR_TLS;
        }
    }
#else
    MOSQ_UNUSED(mosq);
    MOSQ_UNUSED(host);
#endif

    return MOSQ_ERR_SUCCESS;
}

int net__socket_connect(struct mosquitto *mosq, const char *host, uint16_t port, const char *bind_address, bool blocking)
{
    int rc, rc2;

    if (!mosq || !host) {
        return MOSQ_ERR_INVAL;
    }

    rc = net__try_connect(host, port, &mosq->sock, bind_address, blocking);
    if (rc > 0) {
        return rc;
    }

    if (mosq->tcp_nodelay && port) {
        int flag = 1;
        if (setsockopt(mosq->sock, IPPROTO_TCP, TCP_NODELAY, (const void *)&flag, sizeof(int)) != 0) {
            log__printf(mosq, MOSQ_LOG_WARNING, "Warning: Unable to set TCP_NODELAY.");
        }
    }

    if (!mosq->socks5_host) {
        rc2 = net__socket_connect_step3(mosq, host);
        if (rc2) {
            return rc2;
        }
    }

    return rc;
}

#ifdef WITH_TLS
static int net__handle_ssl(struct mosquitto *mosq, int ret)
{
    int err;

    err = SSL_get_error(mosq->ssl, ret);
    if (err == SSL_ERROR_WANT_READ) {
        ret = -1;
        errno = EAGAIN;
    } else if (err == SSL_ERROR_WANT_WRITE) {
        ret = -1;
        mosq->want_write = true;
        errno = EAGAIN;
    } else {
        net__print_ssl_error(mosq);
        errno = EPROTO;
    }

    ERR_clear_error();
    return ret;
}
#endif

ssize_t net__read(struct mosquitto *mosq, void *buf, size_t count)
{
#ifdef WITH_TLS
    int ret;
#endif

    assert(mosq);
    errno = 0;

#ifdef WITH_TLS
    if (mosq->ssl) {
        ret = SSL_read(mosq->ssl, buf, (int)count);
        if (ret <= 0) {
            ret = net__handle_ssl(mosq, ret);
        }

        return (ssize_t)ret;
    } else {
#endif
        return read(mosq->sock, buf, count);
#ifdef WITH_TLS
    }
#endif
}

ssize_t net__write(struct mosquitto *mosq, const void *buf, size_t count)
{
#ifdef WITH_TLS
    int ret;
#endif

    assert(mosq);
    errno = 0;

#ifdef WITH_TLS
    if (mosq->ssl) {
        mosq->want_write = false;
        ret = SSL_write(mosq->ssl, buf, (int)count);
        if (ret < 0) {
            ret = net__handle_ssl(mosq, ret);
        }

        return (ssize_t)ret;
    } else {
#endif
        return write(mosq->sock, buf, count);
#ifdef WITH_TLS
    }
#endif
}

int net__socket_nonblock(mosq_sock_t *sock)
{
    int opt;

    // 设置非阻塞
    opt = fcntl(*sock, F_GETFL, 0);
    if (opt == -1) {
        COMPAT_CLOSE(*sock);
        *sock = INVALID_SOCKET;

        return MOSQ_ERR_ERRNO;
    }

    if (fcntl(*sock, F_SETFL, opt | O_NONBLOCK) == -1) {
        // 如果任一fcntl失败，则不要允许该客户端连接
        COMPAT_CLOSE(*sock);
        *sock = INVALID_SOCKET;

        return MOSQ_ERR_ERRNO;
    }

    return MOSQ_ERR_SUCCESS;
}

int net__socketpair(mosq_sock_t *pairR, mosq_sock_t *pairW)
{
    int sv[2];

    *pairR = INVALID_SOCKET;
    *pairW = INVALID_SOCKET;

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == -1) {
        return MOSQ_ERR_ERRNO;
    }

    if (net__socket_nonblock(&sv[0])) {
        COMPAT_CLOSE(sv[1]);
        return MOSQ_ERR_ERRNO;
    }

    if (net__socket_nonblock(&sv[1])) {
        COMPAT_CLOSE(sv[0]);
        return MOSQ_ERR_ERRNO;
    }

    *pairR = sv[0];
    *pairW = sv[1];

    return MOSQ_ERR_SUCCESS;
}

void *mosquitto_ssl_get(struct mosquitto *mosq)
{
#ifdef WITH_TLS
    return mosq->ssl;
#else
    MOSQ_UNUSED(mosq);
    return NULL;
#endif
}
