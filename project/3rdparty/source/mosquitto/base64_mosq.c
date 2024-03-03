#include "config.h"
#ifdef WITH_TLS
#include <openssl/opensslv.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/buffer.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <mosquitto/mosquitto.h>
#include <mosquitto/mosquitto_broker.h>

#include "base64_mosq.h"
#include "memory_mosq.h"

#ifdef WITH_TLS
int base64__encode(const unsigned char *in, size_t in_len, char **encoded)
{
    int rc = 1;
    BIO *bmem, *b64;
    BUF_MEM *bptr = NULL;

    b64 = BIO_new(BIO_f_base64());
    if (b64 == NULL) {
        return 1;
    }

    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bmem = BIO_new(BIO_s_mem());
    if (bmem) {
        b64 = BIO_push(b64, bmem);
        BIO_write(b64, in, (int)in_len);

        if (BIO_flush(b64) == 1) {
            BIO_get_mem_ptr(b64, &bptr);
            *encoded = malloc(bptr->length+1);
            if (*encoded) {
                memcpy(*encoded, bptr->data, bptr->length);
                (*encoded)[bptr->length] = '\0';
                rc = 0;
            }
        }
    }

    BIO_free_all(b64);
    return rc;
}

int base64__decode(const char *in, unsigned char **decoded, unsigned int *decoded_len)
{
    int len;
    int rc = 1;
    size_t slen;
    BIO *bmem, *b64;

    slen = strlen(in);

    b64 = BIO_new(BIO_f_base64());
    if (!b64) {
        return 1;
    }

    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bmem = BIO_new(BIO_s_mem());
    if (bmem) {
        b64 = BIO_push(b64, bmem);
        BIO_write(bmem, in, (int)slen);

        if (BIO_flush(bmem) == 1) {
            *decoded = calloc(slen, 1);

            if (*decoded) {
                len = BIO_read(b64, *decoded, (int)slen);
                if (len > 0) {
                    *decoded_len = (unsigned int)len;
                    rc = 0;
                } else {
                    SAFE_FREE(*decoded);
                }
            }
        }
    }

    BIO_free_all(b64);
    return rc;
}
#endif