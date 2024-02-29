#include "config.h"

#ifdef WITH_TLS
#include <string.h>
#include <strings.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <openssl/ssl.h>
#include <openssl/conf.h>
#include <openssl/x509v3.h>

#include "tls_mosq.h"
#include "logging_mosq.h"
#include "mosquitto_internal.h"

extern int tls_ex_index_mosq;

int mosquitto__server_certificate_verify(int preverify_ok, X509_STORE_CTX *ctx)
{
    // Preverify应该已经检查了到期，撤销。我们需要验证主机名
    SSL *ssl;
    X509 *cert;
    struct mosquitto *mosq;

    // 如果preverify_ok失败，请始终拒绝
    if (!preverify_ok) {
        return 0;
    }

    ssl = (SSL *)X509_STORE_CTX_get_ex_data(ctx, SSL_get_ex_data_X509_STORE_CTX_idx());
    mosq = (struct mosquitto *)SSL_get_ex_data(ssl, tls_ex_index_mosq);
    if (!mosq) {
        return 0;
    }

    if ((mosq->tls_insecure == false) && (mosq->port != 0)) {
        if (X509_STORE_CTX_get_error_depth(ctx) == 0) {
            // FIXME - 使用X509_check_host()等获得足够新的openssl(>=1.1.x)
            cert = X509_STORE_CTX_get_current_cert(ctx);

            // 这是对等证书，所有其他证书都在链中
            preverify_ok = mosquitto__verify_certificate_hostname(cert, mosq->host);
            if (preverify_ok != 1) {
                log__printf(mosq, MOSQ_LOG_ERR, "Error: host name verification failed.");
            }

            return preverify_ok;
        } else {
            return preverify_ok;
        }
    } else {
        return preverify_ok;
    }
}

static int mosquitto__cmp_hostname_wildcard(char *certname, const char *hostname)
{
    size_t i;
    size_t len;

    if (!certname || !hostname) {
        return 1;
    }

    if (certname[0] == '*') {
        if (certname[1] != '.') {
            return 1;
        }

        certname += 2;
        len = strlen(hostname);

        for (i = 0; i < (len - 1); i++) {
            if (hostname[i] == '.') {
                hostname += i + 1;
                break;
            }
        }

        len = strlen(hostname);
        int dotcount = 0;
        for (i = 0; i < len - 1; i++) {
            if (hostname[i] == '.') {
                dotcount++;
            }
        }

        if (dotcount < 1) {
            return 1;
        }

        return strcasecmp(certname, hostname);
    } else {
        return strcasecmp(certname, hostname);
    }
}

int mosquitto__verify_certificate_hostname(X509 *cert, const char *hostname)
{
    int i;
    int ipv6_ok;
    int ipv4_ok;
    char name[256];
    X509_NAME *subj;
    const GENERAL_NAME *nval;
    bool have_san_dns = false;
    const unsigned char *data;
    unsigned char ipv4_addr[4];
    unsigned char ipv6_addr[16];
    STACK_OF(GENERAL_NAME) *san;

    ipv6_ok = inet_pton(AF_INET6, hostname, &ipv6_addr);
    ipv4_ok = inet_pton(AF_INET, hostname, &ipv4_addr);

    san = (STACK_OF(GENERAL_NAME) *)X509_get_ext_d2i(cert, NID_subject_alt_name, NULL, NULL);
    if (san) {
        for (i = 0; i < sk_GENERAL_NAME_num(san); i++) {
            nval = sk_GENERAL_NAME_value(san, i);
            if (nval->type == GEN_DNS) {
                data = ASN1_STRING_get0_data(nval->d.dNSName);
                if (data && !mosquitto__cmp_hostname_wildcard((char *)data, hostname)) {
                    sk_GENERAL_NAME_pop_free(san, GENERAL_NAME_free);
                    return 1;
                }

                have_san_dns = true;
            } else if (nval->type == GEN_IPADD) {
                data = ASN1_STRING_get0_data(nval->d.iPAddress);
                if ((nval->d.iPAddress->length == 4) && ipv4_ok) {
                    if (!memcmp(ipv4_addr, data, 4)) {
                        sk_GENERAL_NAME_pop_free(san, GENERAL_NAME_free);
                        return 1;
                    }
                } else if ((nval->d.iPAddress->length == 16) && ipv6_ok) {
                    if (!memcmp(ipv6_addr, data, 16)) {
                        sk_GENERAL_NAME_pop_free(san, GENERAL_NAME_free);
                        return 1;
                    }
                }
            }
        }
        sk_GENERAL_NAME_pop_free(san, GENERAL_NAME_free);

        if (have_san_dns) {
            // 如果subjectAltName DNS条目不存在，则仅检查CN
            return 0;
        }
    }

    subj = X509_get_subject_name(cert);
    if (X509_NAME_get_text_by_NID(subj, NID_commonName, name, sizeof(name)) > 0) {
        name[sizeof(name) - 1] = '\0';
        if (!mosquitto__cmp_hostname_wildcard(name, hostname)) {
            return 1;
        }
    }

    return 0;
}
#endif
