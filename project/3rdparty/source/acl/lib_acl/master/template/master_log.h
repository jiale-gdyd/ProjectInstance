#ifndef ACL_LIB_ACL_MASTER_TEMPLATE_MASTER_LOG_H
#define ACL_LIB_ACL_MASTER_TEMPLATE_MASTER_LOG_H

#include "acl/lib_acl/stdlib/acl_define.h"

#ifndef ACL_CLIENT_ONLY

#ifdef __cplusplus
extern "C" {
#endif

void master_log_open(const char *procname);
void master_log_close(void);

#ifdef __cplusplus
}
#endif

#endif

#endif
