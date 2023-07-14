#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <mosquitto/mosquitto.h>

#include "config.h"
#include "tls_mosq.h"
#include "net_mosq.h"
#include "callbacks.h"
#include "util_mosq.h"
#include "socks_mosq.h"
#include "packet_mosq.h"
#include "mosquitto_internal.h"

int mosquitto_loop(struct mosquitto *mosq, int timeout, int max_packets)
{
    int rc;
    time_t now;
    int fdcount;
    char pairbuf;
    int maxfd = 0;
    time_t timeout_ms;
    fd_set readfds, writefds;
    struct timespec local_timeout;

    if (!mosq || (max_packets < 1)) {
        return MOSQ_ERR_INVAL;
    }

    if ((mosq->sock >= FD_SETSIZE) || (mosq->sockpairR >= FD_SETSIZE)) {
        return MOSQ_ERR_INVAL;
    }

    FD_ZERO(&readfds);
    FD_ZERO(&writefds);

    if (net__is_connected(mosq)) {
        maxfd = mosq->sock;
        FD_SET(mosq->sock, &readfds);

        pthread_mutex_lock(&mosq->out_packet_mutex);
        if (mosq->out_packet) {
            FD_SET(mosq->sock, &writefds);
        }
    
#ifdef WITH_TLS
        if (mosq->ssl)  {
            if (mosq->want_write) {
                FD_SET(mosq->sock, &writefds);
            }
        }
#endif
        pthread_mutex_unlock(&mosq->out_packet_mutex);
    } else {
        return MOSQ_ERR_NO_CONN;
    }

    if (mosq->sockpairR != INVALID_SOCKET) {
        // sockpairR用于在超时之前中断select()，调用publish()等
        FD_SET(mosq->sockpairR, &readfds);
        if ((int)mosq->sockpairR > maxfd) {
            maxfd = mosq->sockpairR;
        }
    }

    timeout_ms = timeout;
    if (timeout_ms < 0) {
        timeout_ms = 1000;
    }

    now = mosquitto_time();
    pthread_mutex_lock(&mosq->msgtime_mutex);
    if (mosq->next_msg_out && ((now + timeout_ms / 1000) > mosq->next_msg_out)) {
        timeout_ms = (mosq->next_msg_out - now) * 1000;
    }
    pthread_mutex_unlock(&mosq->msgtime_mutex);

    if (timeout_ms < 0) {
        // 某处存在延迟，这意味着应该已经发送了一条消息
        timeout_ms = 0;
    }

    local_timeout.tv_sec = timeout_ms / 1000;
    local_timeout.tv_nsec = (timeout_ms - local_timeout.tv_sec * 1000) * 1000000;

    fdcount = pselect(maxfd + 1, &readfds, &writefds, NULL, &local_timeout, NULL);
    if (fdcount == -1) {
        if (errno == EINTR) {
            return MOSQ_ERR_SUCCESS;
        } else {
            return MOSQ_ERR_ERRNO;
        }
    } else {
        if (net__is_connected(mosq)) {
            if (FD_ISSET(mosq->sock, &readfds)) {
                rc = mosquitto_loop_read(mosq, max_packets);
                if (rc || (mosq->sock == INVALID_SOCKET)) {
                    return rc;
                }
            }

            if ((mosq->sockpairR != INVALID_SOCKET) && FD_ISSET(mosq->sockpairR, &readfds)) {
                if (read(mosq->sockpairR, &pairbuf, 1) == 0) {

                }

                // 伪写是可能的，即使没有要求也可以刺激输出写，因为那时不存在publish或其他命令
                if (net__is_connected(mosq)) {
                    FD_SET(mosq->sock, &writefds);
                }
            }

            if (net__is_connected(mosq) && FD_ISSET(mosq->sock, &writefds)) {
                rc = mosquitto_loop_write(mosq, max_packets);
                if (rc || !net__is_connected(mosq)) {
                    return rc;
                }
            }
        }
    }

    return mosquitto_loop_misc(mosq);
}

static int interruptible_sleep(struct mosquitto *mosq, time_t reconnect_delay)
{
    int fdcount;
    char pairbuf;
    int maxfd = 0;
    fd_set readfds;
    struct timespec local_timeout;

    local_timeout.tv_sec = reconnect_delay;
    local_timeout.tv_nsec = 0;

    FD_ZERO(&readfds);
    maxfd = 0;

    while ((mosq->sockpairR != INVALID_SOCKET) && (read(mosq->sockpairR, &pairbuf, 1) > 0)) {
        ;
    }

    if (mosq->sockpairR != INVALID_SOCKET) {
        // sockpairR用于在调用mosquitto_loop_stop()之前在超时之前中断select()
        FD_SET(mosq->sockpairR, &readfds);
        maxfd = mosq->sockpairR;
    }

    fdcount = pselect(maxfd + 1, &readfds, NULL, NULL, &local_timeout, NULL);
    if (fdcount == -1) {
        if (errno == EINTR) {
            return MOSQ_ERR_SUCCESS;
        } else {
            return MOSQ_ERR_ERRNO;
        }
    } else if ((mosq->sockpairR != INVALID_SOCKET) && FD_ISSET(mosq->sockpairR, &readfds)) {
        if (read(mosq->sockpairR, &pairbuf, 1) == 0){

        }
    }

    return MOSQ_ERR_SUCCESS;
}

int mosquitto_loop_forever(struct mosquitto *mosq, int timeout, int max_packets)
{
    int run = 1;
    int rc = MOSQ_ERR_SUCCESS;
    unsigned long reconnect_delay;

    if (!mosq) {
        return MOSQ_ERR_INVAL;
    }
    mosq->reconnects = 0;

    while (run) {
        do {
            pthread_testcancel();
            rc = mosquitto_loop(mosq, timeout, max_packets);
        } while (run && (rc == MOSQ_ERR_SUCCESS));

        // 致命错误后退出
        switch (rc) {
            case MOSQ_ERR_NOMEM:
            case MOSQ_ERR_PROTOCOL:
            case MOSQ_ERR_INVAL:
            case MOSQ_ERR_NOT_FOUND:
            case MOSQ_ERR_TLS:
            case MOSQ_ERR_PAYLOAD_SIZE:
            case MOSQ_ERR_NOT_SUPPORTED:
            case MOSQ_ERR_AUTH:
            case MOSQ_ERR_ACL_DENIED:
            case MOSQ_ERR_UNKNOWN:
            case MOSQ_ERR_EAI:
            case MOSQ_ERR_PROXY:
                return rc;

            case MOSQ_ERR_ERRNO:
                break;
        }

        if (errno == EPROTO) {
            return rc;
        }

        do {
            pthread_testcancel();

            rc = MOSQ_ERR_SUCCESS;
            if (mosquitto__get_request_disconnect(mosq)) {
                run = 0;
            } else {
                if (mosq->reconnect_delay_max > mosq->reconnect_delay) {
                    if (mosq->reconnect_exponential_backoff) {
                        reconnect_delay = mosq->reconnect_delay * (mosq->reconnects + 1) * (mosq->reconnects + 1);
                    } else {
                        reconnect_delay = mosq->reconnect_delay * (mosq->reconnects + 1);
                    }
                } else {
                    reconnect_delay = mosq->reconnect_delay;
                }

                if (reconnect_delay > mosq->reconnect_delay_max) {
                    reconnect_delay = mosq->reconnect_delay_max;
                } else {
                    mosq->reconnects++;
                }

                rc = interruptible_sleep(mosq, (time_t)reconnect_delay);
                if (rc) {
                    return rc;
                }

                if (mosquitto__get_request_disconnect(mosq)) {
                    run = 0;
                } else {
                    rc = mosquitto_reconnect(mosq);
                }
            }
        } while (run && (rc != MOSQ_ERR_SUCCESS));
    }

    return rc;
}

int mosquitto_loop_misc(struct mosquitto *mosq)
{
    if (!mosq) {
        return MOSQ_ERR_INVAL;
    }

    if (!net__is_connected(mosq)) {
        return MOSQ_ERR_NO_CONN;
    }

    return mosquitto__check_keepalive(mosq);
}

static int mosquitto__loop_rc_handle(struct mosquitto *mosq, int rc)
{
    enum mosquitto_client_state state;

    if (rc) {
        net__socket_close(mosq);
        state = mosquitto__get_state(mosq);
        if ((state == mosq_cs_disconnecting) || (state == mosq_cs_disconnected)) {
            rc = MOSQ_ERR_SUCCESS;
        }

        callback__on_disconnect(mosq, rc, NULL);
    }

    return rc;
}

int mosquitto_loop_read(struct mosquitto *mosq, int max_packets)
{
    int i;
    int rc = MOSQ_ERR_SUCCESS;

    if (max_packets < 1) {
        return MOSQ_ERR_INVAL;
    }

#ifdef WITH_TLS
    if (mosq->want_connect){
        rc = net__socket_connect_tls(mosq);
        if (MOSQ_ERR_TLS == rc){
            rc = mosquitto__loop_rc_handle(mosq, rc);
        }

        return rc;
    }
#endif

    pthread_mutex_lock(&mosq->msgs_out.mutex);
    max_packets = mosq->msgs_out.queue_len;
    pthread_mutex_unlock(&mosq->msgs_out.mutex);

    pthread_mutex_lock(&mosq->msgs_in.mutex);
    max_packets += mosq->msgs_in.queue_len;
    pthread_mutex_unlock(&mosq->msgs_in.mutex);

    if (max_packets < 1) {
        max_packets = 1;
    }

    // 此处的len队列告诉有多少消息正在等待处理，并且QoS>0。应该尝试在此循环中处理那么多消息，以便跟上
    for (i = 0; (i < max_packets) || SSL_DATA_PENDING(mosq); i++) {
        if (mosq->socks5_host) {
            rc = socks5__read(mosq);
        } else {
            switch (mosq->transport) {
                case mosq_t_tcp:
                case mosq_t_ws:
                    rc = packet__read(mosq);
                    break;

                default:
                    return MOSQ_ERR_INVAL;
            }
        }

        if (rc || (errno == EAGAIN) || (errno == COMPAT_EWOULDBLOCK)) {
            return mosquitto__loop_rc_handle(mosq, rc);
        }
    }

    return rc;
}

int mosquitto_loop_write(struct mosquitto *mosq, int max_packets)
{
    int i;
    int rc = MOSQ_ERR_SUCCESS;

    if (max_packets < 1) {
        return MOSQ_ERR_INVAL;
    }

    for (i = 0; i < max_packets; i++) {
        rc = packet__write(mosq);
        if (rc || (errno == EAGAIN) || (errno == COMPAT_EWOULDBLOCK)) {
            return mosquitto__loop_rc_handle(mosq, rc);
        }
    }

    return rc;
}
