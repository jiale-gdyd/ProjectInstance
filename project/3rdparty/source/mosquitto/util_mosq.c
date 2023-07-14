#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <mosquitto/mosquitto.h>

#include "config.h"

#if !defined(WITH_TLS)
#if __GLIBC_PREREQ(2, 25)
#include <sys/random.h>
#define HAVE_GETRANDOM  1
#endif
#endif

#ifdef WITH_TLS
#include <openssl/bn.h>
#include <openssl/rand.h>
#endif

#include "net_mosq.h"
#include "tls_mosq.h"
#include "callbacks.h"
#include "send_mosq.h"
#include "time_mosq.h"
#include "util_mosq.h"
#include "memory_mosq.h"

int mosquitto__check_keepalive(struct mosquitto *mosq)
{
    int rc;
    time_t now;
    time_t last_msg_in;
    time_t next_msg_out;
    enum mosquitto_client_state state;

    assert(mosq);

    now = mosquitto_time();

    pthread_mutex_lock(&mosq->msgtime_mutex);
    next_msg_out = mosq->next_msg_out;
    last_msg_in = mosq->last_msg_in;
    pthread_mutex_unlock(&mosq->msgtime_mutex);

    if (mosq->keepalive && net__is_connected(mosq) && ((now >= next_msg_out) || ((now - last_msg_in) >= mosq->keepalive))) {
        state = mosquitto__get_state(mosq);
        if ((state == mosq_cs_active) && (mosq->ping_t == 0)) {
            send__pingreq(mosq);

            // 重置最后一次msg时间，以使服务器有时间发送pingresp
            pthread_mutex_lock(&mosq->msgtime_mutex);
            mosq->last_msg_in = now;
            mosq->next_msg_out = now + mosq->keepalive;
            pthread_mutex_unlock(&mosq->msgtime_mutex);
        } else {
            net__socket_close(mosq);
            state = mosquitto__get_state(mosq);
            if (state == mosq_cs_disconnecting) {
                rc = MOSQ_ERR_SUCCESS;
            } else {
                rc = MOSQ_ERR_KEEPALIVE;
            }
            callback__on_disconnect(mosq, rc, NULL);

            return rc;
        }
    }

    return MOSQ_ERR_SUCCESS;
}

uint16_t mosquitto__mid_generate(struct mosquitto *mosq)
{
    assert(mosq);

    uint16_t mid;

    pthread_mutex_lock(&mosq->mid_mutex);
    mosq->last_mid++;
    if (mosq->last_mid == 0) {
        mosq->last_mid++;
    }
    mid = mosq->last_mid;
    pthread_mutex_unlock(&mosq->mid_mutex);

    return mid;
}

#ifdef WITH_TLS
int mosquitto__hex2bin_sha1(const char *hex, unsigned char **bin)
{
    unsigned char *sha, tmp[SHA_DIGEST_LENGTH];

    if (mosquitto__hex2bin(hex, tmp, SHA_DIGEST_LENGTH) != SHA_DIGEST_LENGTH) {
        return MOSQ_ERR_INVAL;
    }

    sha = (unsigned char *)mosquitto__malloc(SHA_DIGEST_LENGTH);
    if (!sha) {
        return MOSQ_ERR_NOMEM;
    }
    memcpy(sha, tmp, SHA_DIGEST_LENGTH);
    *bin = sha;

    return MOSQ_ERR_SUCCESS;
}

int mosquitto__hex2bin(const char *hex, unsigned char *bin, int bin_max_len)
{
    int len;
    size_t i = 0;
    BIGNUM *bn = NULL;
    int leading_zero = 0;

    // 计算前导零的数量
    for (i = 0; i < strlen(hex); i = i + 2) {
        if (strncmp(hex + i, "00", 2) == 0) {
            if (leading_zero >= bin_max_len) {
                return 0;
            }

            bin[leading_zero] = 0;
            leading_zero++;
        } else {
            break;
        }
    }

    if (BN_hex2bn(&bn, hex) == 0) {
        if (bn) {
            BN_free(bn);
        }

        return 0;
    }

    if ((BN_num_bytes(bn) + leading_zero) > bin_max_len) {
        BN_free(bn);
        return 0;
    }

    len = BN_bn2bin(bn, bin + leading_zero);
    BN_free(bn);

    return (len + leading_zero);
}
#endif

void util__increment_receive_quota(struct mosquitto *mosq)
{
    if (mosq->msgs_in.inflight_quota < mosq->msgs_in.inflight_maximum) {
        mosq->msgs_in.inflight_quota++;
    }
}

void util__increment_send_quota(struct mosquitto *mosq)
{
    if (mosq->msgs_out.inflight_quota < mosq->msgs_out.inflight_maximum) {
        mosq->msgs_out.inflight_quota++;
    }
}

void util__decrement_receive_quota(struct mosquitto *mosq)
{
    if (mosq->msgs_in.inflight_quota > 0) {
        mosq->msgs_in.inflight_quota--;
    }
}

void util__decrement_send_quota(struct mosquitto *mosq)
{
    if (mosq->msgs_out.inflight_quota > 0) {
        mosq->msgs_out.inflight_quota--;
    }
}

int util__random_bytes(void *bytes, int count)
{
    int rc = MOSQ_ERR_UNKNOWN;

#ifdef WITH_TLS
    if (RAND_bytes((unsigned char *)bytes, count) == 1) {
        rc = MOSQ_ERR_SUCCESS;
    }
#elif defined(HAVE_GETRANDOM)
    if (getrandom(bytes, (size_t)count, 0) == count) {
        rc = MOSQ_ERR_SUCCESS;
    }
#else
    int i;

    for (i = 0; i < count; i++) {
        ((uint8_t *)bytes)[i] = (uint8_t )(random() & 0xFF);
    }
    rc = MOSQ_ERR_SUCCESS;
#endif

    return rc;
}

int mosquitto__set_state(struct mosquitto *mosq, enum mosquitto_client_state state)
{
    pthread_mutex_lock(&mosq->state_mutex);
    mosq->state = state;
    pthread_mutex_unlock(&mosq->state_mutex);

    return MOSQ_ERR_SUCCESS;
}

enum mosquitto_client_state mosquitto__get_state(struct mosquitto *mosq)
{
    enum mosquitto_client_state state;

    pthread_mutex_lock(&mosq->state_mutex);
    state = mosq->state;
    pthread_mutex_unlock(&mosq->state_mutex);

    return state;
}

void mosquitto__set_request_disconnect(struct mosquitto *mosq, bool request_disconnect)
{
    pthread_mutex_lock(&mosq->state_mutex);
    mosq->request_disconnect = request_disconnect;
    pthread_mutex_unlock(&mosq->state_mutex);
}

bool mosquitto__get_request_disconnect(struct mosquitto *mosq)
{
    bool request_disconnect;

    pthread_mutex_lock(&mosq->state_mutex);
    request_disconnect = mosq->request_disconnect;
    pthread_mutex_unlock(&mosq->state_mutex);

    return request_disconnect;
}
