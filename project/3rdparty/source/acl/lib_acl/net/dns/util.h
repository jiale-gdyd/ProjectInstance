#ifndef ACL_LIB_ACL_NET_DNS_UTIL_H
#define ACL_LIB_ACL_NET_DNS_UTIL_H

#include "acl/lib_acl/lib_acl.h"

#ifdef __cplusplus
extern "C" {
#endif

ACL_ARGV *res_a_create(const ACL_RFC1035_RR *answer, int n);

#ifdef __cplusplus
}
#endif

#endif
