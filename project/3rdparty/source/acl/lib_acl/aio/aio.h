#ifndef ACL_LIBACL_AIO_AIO_H
#define ACL_LIBACL_AIO_AIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "acl/lib_acl/stdlib/acl_define.h"
#include "acl/lib_acl/stdlib/acl_vstream.h"
#include "acl/lib_acl/stdlib/acl_vstring.h"
#include "acl/lib_acl/stdlib/acl_array.h"
#include "acl/lib_acl/net/acl_dns.h"
#include "acl/lib_acl/aio/acl_aio.h"

struct ACL_AIO {
    ACL_EVENT *event;
    int   delay_sec;
    int   delay_usec;
    int   keep_read;
    int   rbuf_size;
    int   event_mode;
    ACL_ARRAY *dead_streams;
    ACL_DNS *dns;
};

typedef struct AIO_READ_HOOK {
    ACL_AIO_READ_FN callback;
    void *ctx;
    char  disable;
} AIO_READ_HOOK;

typedef struct AIO_WRITE_HOOK {
    ACL_AIO_WRITE_FN callback;
    void *ctx;
    char  disable;
} AIO_WRITE_HOOK;

typedef struct AIO_CLOSE_HOOK {
    ACL_AIO_CLOSE_FN callback;
    void *ctx;
    char  disable;
} AIO_CLOSE_HOOK;

typedef struct AIO_TIMEO_HOOK {
    ACL_AIO_TIMEO_FN callback;
    void *ctx;
    char  disable;
} AIO_TIMEO_HOOK;

typedef struct AIO_CONNECT_HOOK {
    ACL_AIO_CONNECT_FN callback;
    void *ctx;
    char  disable;
} AIO_CONNECT_HOOK;

#define __AIO_NESTED_MAX            10

#define __default_line_length       4096

int aio_timeout_callback(ACL_ASTREAM *astream);
void aio_close_callback(ACL_ASTREAM *astream);

void aio_delay_check(ACL_AIO *aio);

#ifdef __cplusplus
}
#endif

#endif
