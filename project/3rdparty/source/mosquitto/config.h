#ifndef MOSQ_CONFIG_H
#define MOSQ_CONFIG_H

#include <linux/kconfig.h>

#define VERSION                         "2.0.14"

#define uthash_malloc(sz)               mosquitto_malloc(sz)
#define uthash_free(ptr, sz)            mosquitto_free(ptr)

#define MOSQ_UNUSED(A)                  (void)(A)

#if defined(CONFIG_MOSQUIITO_TLS)
#define WITH_EC
#define WITH_TLS
#define WITH_TLS_PSK
#endif

#define OPENSSL_LOAD_CONF

#ifdef WITH_TLS
#include <openssl/opensslconf.h>
#if defined(WITH_TLS_PSK) && !defined(OPENSSL_NO_PSK)
#define FINAL_WITH_TLS_PSK
#endif
#endif

#define WS_IS_LWS                       1
#define WS_IS_WSLAY                     2

#endif
