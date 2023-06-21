#ifndef ACL_LIBACL_STDLIB_ACL_SANE_SOCKETPAIR_H
#define ACL_LIBACL_STDLIB_ACL_SANE_SOCKETPAIR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "acl_define.h"

int acl_sane_socketpair(int domain, int type, int protocol, ACL_SOCKET result[2]);

#ifdef __cplusplus
}
#endif

#endif
