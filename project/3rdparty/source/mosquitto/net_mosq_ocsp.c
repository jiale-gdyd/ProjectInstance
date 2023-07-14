#include "config.h"

#ifdef WITH_TLS
#include "net_mosq.h"
#include "logging_mosq.h"
#include "mosquitto_internal.h"

#include <openssl/ssl.h>
#include <openssl/tls1.h>
#include <openssl/ocsp.h>
#include <openssl/safestack.h>

int mosquitto__verify_ocsp_status_cb(SSL *ssl, void *arg)
{
    long len;
    unsigned char *p;
    X509_STORE *st = NULL;
    const unsigned char *cp;
    OCSP_RESPONSE *rsp = NULL;
    OCSP_BASICRESP *br = NULL;
    STACK_OF(X509) *ch = NULL;
    int ocsp_status, result2, i;
    struct mosquitto *mosq = (struct mosquitto *)arg;

    MOSQ_UNUSED(ssl);

    len = SSL_get_tlsext_status_ocsp_resp(mosq->ssl, &p);
    log__printf(mosq, MOSQ_LOG_DEBUG, "OCSP: SSL_get_tlsext_status_ocsp_resp returned %ld bytes", len);

    // 以下函数期望使用const指针
    cp = (const unsigned char *)p;
    if (!cp || (len <= 0)) {
        log__printf(mosq, MOSQ_LOG_DEBUG, "OCSP: no response");
        goto end;
    }

    rsp = d2i_OCSP_RESPONSE(NULL, &cp, len);
    if (rsp == NULL) {
        log__printf(mosq, MOSQ_LOG_DEBUG, "OCSP: invalid response");
        goto end;
    }

    ocsp_status = OCSP_response_status(rsp);
    if (ocsp_status != OCSP_RESPONSE_STATUS_SUCCESSFUL) {
        log__printf(mosq, MOSQ_LOG_DEBUG, "OCSP: invalid status: %s (%d)", OCSP_response_status_str(ocsp_status), ocsp_status);
        goto end;
    }

    br = OCSP_response_get1_basic(rsp);
    if (!br) {
        log__printf(mosq, MOSQ_LOG_DEBUG, "OCSP: invalid response");
        goto end;
    }

    ch = SSL_get_peer_cert_chain(mosq->ssl);
    if (sk_X509_num(ch) <= 0) {
        log__printf(mosq, MOSQ_LOG_ERR, "OCSP: we did not receive certificates of the server (num: %d)", sk_X509_num(ch));
        goto end;
    }

    st = SSL_CTX_get_cert_store(mosq->ssl_ctx);

    // 其他检查程序通常会在1.0.2a之前的OpenSSL中解决问题(例如libcurl)。对于当前所有受支持的OpenSSL项目版本，都不再需要此功能
    if ((result2 = OCSP_basic_verify(br, ch, st, 0)) <= 0) {
        log__printf(mosq, MOSQ_LOG_DEBUG, "OCSP: response verification failed (error: %d)", result2);
        goto end;
    }

    for (i = 0; i < OCSP_resp_count(br); i++) {
        int cert_status, crl_reason;
        OCSP_SINGLERESP *single = NULL;
        ASN1_GENERALIZEDTIME *rev, *thisupd, *nextupd;

        single = OCSP_resp_get0(br, i);
        if (!single) {
            continue;
        }

        cert_status = OCSP_single_get0_status(single, &crl_reason, &rev, &thisupd, &nextupd);

        log__printf(mosq, MOSQ_LOG_DEBUG, "OCSP: SSL certificate status: %s (%d)", OCSP_cert_status_str(cert_status), cert_status);

        switch (cert_status) {
            case V_OCSP_CERTSTATUS_GOOD:
                // OCSP装订结果将在过期后的5分钟内被接受
                if (!OCSP_check_validity(thisupd, nextupd, 5 * 60L, -1L)) {
                    log__printf(mosq, MOSQ_LOG_DEBUG, "OCSP: OCSP response has expired");
                    goto end;
                }
                break;

            case V_OCSP_CERTSTATUS_REVOKED:
                log__printf(mosq, MOSQ_LOG_DEBUG, "OCSP: SSL certificate revocation reason: %s (%d)", OCSP_crl_reason_str(crl_reason), crl_reason);
                goto end;

            case V_OCSP_CERTSTATUS_UNKNOWN:
                goto end;

            default:
                log__printf(mosq, MOSQ_LOG_DEBUG, "OCSP: SSL certificate revocation status unknown");
                goto end;
        }
    }

    if (br != NULL) {
        OCSP_BASICRESP_free(br);
    }

    if (rsp != NULL) {
        OCSP_RESPONSE_free(rsp);
    }

    // OK
    return 1;

end:
    if (br != NULL) {
        OCSP_BASICRESP_free(br);
    }

    if (rsp != NULL) {
        OCSP_RESPONSE_free(rsp);
    }

    // Not OK
    return 0;
}
#endif
