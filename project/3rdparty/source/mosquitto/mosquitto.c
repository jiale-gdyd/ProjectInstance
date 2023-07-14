#include <errno.h>
#include <signal.h>
#include <string.h>
#include <strings.h>
#include <sys/time.h>
#include <mosquitto/mosquitto.h>
#include <mosquitto/mqtt_protocol.h>

#include "config.h"
#include "net_mosq.h"
#include "will_mosq.h"
#include "packet_mosq.h"
#include "memory_mosq.h"
#include "logging_mosq.h"
#include "messages_mosq.h"
#include "mosquitto_internal.h"

static unsigned int init_refcount = 0;

void mosquitto__destroy(struct mosquitto *mosq);

int mosquitto_lib_version(int *major, int *minor, int *revision)
{
    if (major) {
        *major = LIBMOSQUITTO_MAJOR;
    }

    if (minor) {
        *minor = LIBMOSQUITTO_MINOR;
    }

    if (revision) {
        *revision = LIBMOSQUITTO_REVISION;
    }

    return LIBMOSQUITTO_VERSION_NUMBER;
}

int mosquitto_lib_init(void)
{
    int rc;

    if (init_refcount == 0) {
        struct timeval tv;

        gettimeofday(&tv, NULL);
        srand(tv.tv_sec * 1000 + tv.tv_usec / 1000);

        rc = net__init();
        if (rc != MOSQ_ERR_SUCCESS) {
            return rc;
        }
    }
    init_refcount++;

    return MOSQ_ERR_SUCCESS;
}

int mosquitto_lib_cleanup(void)
{
    if (init_refcount == 1) {
        net__cleanup();
    }

    if (init_refcount > 0) {
        --init_refcount;
    }

    return MOSQ_ERR_SUCCESS;
}

struct mosquitto *mosquitto_new(const char *id, bool clean_start, void *userdata)
{
    int rc;
    struct mosquitto *mosq = NULL;

    if ((clean_start == false) && (id == NULL)) {
        errno = EINVAL;
        return NULL;
    }

    signal(SIGPIPE, SIG_IGN);

    mosq = (struct mosquitto *)mosquitto__calloc(1, sizeof(struct mosquitto));
    if (mosq) {
        mosq->sock = INVALID_SOCKET;
        mosq->thread_id = pthread_self();
        mosq->sockpairR = INVALID_SOCKET;
        mosq->sockpairW = INVALID_SOCKET;

        rc = mosquitto_reinitialise(mosq, id, clean_start, userdata);
        if (rc) {
            mosquitto_destroy(mosq);
            if (rc == MOSQ_ERR_INVAL) {
                errno = EINVAL;
            } else if (rc == MOSQ_ERR_NOMEM) {
                errno = ENOMEM;
            }

            return NULL;
        }
    } else {
        errno = ENOMEM;
    }

    return mosq;
}

int mosquitto_reinitialise(struct mosquitto *mosq, const char *id, bool clean_start, void *userdata)
{
    if (!mosq) {
        return MOSQ_ERR_INVAL;
    }

    if ((clean_start == false) && (id == NULL)) {
        return MOSQ_ERR_INVAL;
    }

    mosquitto__destroy(mosq);
    memset(mosq, 0x00, sizeof(struct mosquitto));

    if (userdata) {
        mosq->userdata = userdata;
    } else {
        mosq->userdata = mosq;
    }

    mosq->protocol = mosq_p_mqtt311;
    mosq->sock = INVALID_SOCKET;
    mosq->sockpairR = INVALID_SOCKET;
    mosq->sockpairW = INVALID_SOCKET;
    mosq->keepalive = 60;
    mosq->clean_start = clean_start;

    if (id) {
        if (STREMPTY(id)) {
            return MOSQ_ERR_INVAL;
        }

        if (mosquitto_validate_utf8(id, (int)strlen(id))) {
            return MOSQ_ERR_MALFORMED_UTF8;
        }

        mosq->id = mosquitto__strdup(id);
    }

    packet__cleanup(&mosq->in_packet);
    mosq->out_packet = NULL;
    mosq->out_packet_count = 0;
    mosq->out_packet_bytes = 0;
    mosq->last_msg_in = mosquitto_time();
    mosq->next_msg_out = mosquitto_time() + mosq->keepalive;
    mosq->ping_t = 0;
    mosq->last_mid = 0;
    mosq->state = mosq_cs_new;
    mosq->max_qos = 2;
    mosq->msgs_in.inflight_maximum = 20;
    mosq->msgs_out.inflight_maximum = 20;
    mosq->msgs_in.inflight_quota = 20;
    mosq->msgs_out.inflight_quota = 20;
    mosq->will = NULL;
    mosq->on_connect = NULL;
    mosq->on_publish = NULL;
    mosq->on_message = NULL;
    mosq->on_subscribe = NULL;
    mosq->on_unsubscribe = NULL;
    mosq->host = NULL;
    mosq->port = 1883;
    mosq->reconnect_delay = 1;
    mosq->reconnect_delay_max = 1;
    mosq->reconnect_exponential_backoff = false;
    mosq->threaded = mosq_ts_none;

#ifdef WITH_TLS
    mosq->ssl = NULL;
    mosq->ssl_ctx = NULL;
    mosq->ssl_ctx_defaults = true;
    mosq->tls_cert_reqs = SSL_VERIFY_PEER;
    mosq->tls_insecure = false;
    mosq->want_write = false;
    mosq->tls_ocsp_required = false;
#endif

    pthread_mutex_init(&mosq->callback_mutex, NULL);
    pthread_mutex_init(&mosq->log_callback_mutex, NULL);
    pthread_mutex_init(&mosq->state_mutex, NULL);
    pthread_mutex_init(&mosq->out_packet_mutex, NULL);
    pthread_mutex_init(&mosq->msgtime_mutex, NULL);
    pthread_mutex_init(&mosq->msgs_in.mutex, NULL);
    pthread_mutex_init(&mosq->msgs_out.mutex, NULL);
    pthread_mutex_init(&mosq->mid_mutex, NULL);

    mosq->thread_id = pthread_self();

    if (mosq->disable_socketpair == false) {
        if (net__socketpair(&mosq->sockpairR, &mosq->sockpairW)) {
            log__printf(mosq, MOSQ_LOG_WARNING, "Warning: Unable to open socket pair, outgoing publish commands may be delayed.");
        }
    }

    return MOSQ_ERR_SUCCESS;
}

void mosquitto__destroy(struct mosquitto *mosq)
{
    if (!mosq) {
        return;
    }

    if ((mosq->threaded == mosq_ts_self) && !pthread_equal(mosq->thread_id, pthread_self())) {
        pthread_cancel(mosq->thread_id);
        pthread_join(mosq->thread_id, NULL);
        mosq->threaded = mosq_ts_none;
    }

    // 如果mosq->id不为NULL，则客户端已经初始化，因此互斥体需要销毁。如果mosq->id为NULL，则互斥体尚未初始化
    if (mosq->id) {
        pthread_mutex_destroy(&mosq->callback_mutex);
        pthread_mutex_destroy(&mosq->log_callback_mutex);
        pthread_mutex_destroy(&mosq->state_mutex);
        pthread_mutex_destroy(&mosq->out_packet_mutex);
        pthread_mutex_destroy(&mosq->msgtime_mutex);
        pthread_mutex_destroy(&mosq->msgs_in.mutex);
        pthread_mutex_destroy(&mosq->msgs_out.mutex);
        pthread_mutex_destroy(&mosq->mid_mutex);
    }

    if (net__is_connected(mosq)) {
        net__socket_close(mosq);
    }

    message__cleanup_all(mosq);
    will__clear(mosq);

#ifdef WITH_TLS
    if (mosq->ssl) {
        SSL_free(mosq->ssl);
    }

    if (mosq->ssl_ctx) {
        SSL_CTX_free(mosq->ssl_ctx);
    }

    mosquitto__FREE(mosq->tls_cafile);
    mosquitto__FREE(mosq->tls_capath);
    mosquitto__FREE(mosq->tls_certfile);
    mosquitto__FREE(mosq->tls_keyfile);
    if (mosq->tls_pw_callback) {
        mosq->tls_pw_callback = NULL;
    }
    mosquitto__FREE(mosq->tls_version);
    mosquitto__FREE(mosq->tls_ciphers);
    mosquitto__FREE(mosq->tls_psk);
    mosquitto__FREE(mosq->tls_psk_identity);
    mosquitto__FREE(mosq->tls_alpn);
#endif

    mosquitto__FREE(mosq->address);
    mosquitto__FREE(mosq->id);
    mosquitto__FREE(mosq->username);
    mosquitto__FREE(mosq->password);
    mosquitto__FREE(mosq->host);
    mosquitto__FREE(mosq->bind_address);

    mosquitto_property_free_all(&mosq->connect_properties);

    packet__cleanup_all_no_locks(mosq);

    packet__cleanup(&mosq->in_packet);
    if (mosq->sockpairR != INVALID_SOCKET) {
        COMPAT_CLOSE(mosq->sockpairR);
        mosq->sockpairR = INVALID_SOCKET;
    }

    if (mosq->sockpairW != INVALID_SOCKET) {
        COMPAT_CLOSE(mosq->sockpairW);
        mosq->sockpairW = INVALID_SOCKET;
    }
}

void mosquitto_destroy(struct mosquitto *mosq)
{
    if (!mosq) {
        return;
    }

    mosquitto__destroy(mosq);
    mosquitto__FREE(mosq);
}

int mosquitto_socket(struct mosquitto *mosq)
{
    if (!mosq) {
        return INVALID_SOCKET;
    }

    return mosq->sock;
}

bool mosquitto_want_write(struct mosquitto *mosq)
{
    bool result = false;
    if (mosq->out_packet) {
        result = true;
    }

#ifdef WITH_TLS
    if (mosq->ssl) {
        if (mosq->want_write) {
            result = true;
        }
    }
#endif

    return result;
}

int mosquitto_sub_topic_tokenise(const char *subtopic, char ***topics, int *count)
{
    size_t len;
    size_t hier;
    size_t tlen;
    size_t i, j;
    size_t start, stop;
    size_t hier_count = 1;

    if (!subtopic || !topics || !count) {
        return MOSQ_ERR_INVAL;
    }

    len = strlen(subtopic);
    for (i = 0; i < len; i++) {
        if (subtopic[i] == '/') {
            if (i > (len - 1)) {
                // 行尾的分隔符
            } else {
                hier_count++;
            }
        }
    }

    (*topics) = (char **)mosquitto__calloc(hier_count, sizeof(char *));
    if (!(*topics)) {
        return MOSQ_ERR_NOMEM;
    }

    start = 0;
    hier = 0;

    for (i = 0; i < (len + 1); i++) {
        if ((subtopic[i] == '/') || (subtopic[i] == '\0')) {
            stop = i;
            if (start != stop) {
                tlen = stop - start + 1;
                (*topics)[hier] = (char *)mosquitto__calloc(tlen, sizeof(char));
                if (!(*topics)[hier]) {
                    for (j = 0; j < hier; j++) {
                        mosquitto__FREE((*topics)[j]);
                    }

                    mosquitto__FREE((*topics));
                    return MOSQ_ERR_NOMEM;
                }

                for (j = start; j < stop; j++) {
                    (*topics)[hier][j - start] = subtopic[j];
                }
            }

            start = i + 1;
            hier++;
        }
    }
    *count = (int)hier_count;

    return MOSQ_ERR_SUCCESS;
}

int mosquitto_sub_topic_tokens_free(char ***topics, int count)
{
    int i;

    if (!topics || !(*topics) || (count < 1)) {
        return MOSQ_ERR_INVAL;
    }

    for (i = 0; i < count; i++){
        mosquitto__FREE((*topics)[i]);
    }
    mosquitto__FREE(*topics);

    return MOSQ_ERR_SUCCESS;
}
