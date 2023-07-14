#ifndef MOSQ_TLS_MOSQ_H
#define MOSQ_TLS_MOSQ_H

#include "config.h"

#ifdef WITH_TLS
#define SSL_DATA_PENDING(A)     ((A)->ssl && SSL_pending((A)->ssl))
#else
#define SSL_DATA_PENDING(A)     (0)
#endif

#ifdef WITH_TLS
#include <openssl/ssl.h>
#include <openssl/engine.h>

int mosquitto__verify_certificate_hostname(X509 *cert, const char *hostname);
int mosquitto__server_certificate_verify(int preverify_ok, X509_STORE_CTX *ctx);
#endif

#endif
