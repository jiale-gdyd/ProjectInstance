#ifndef ACL_LIB_ACL_MASTER_TEMPLATE_IFMONITOR_H
#define ACL_LIB_ACL_MASTER_TEMPLATE_IFMONITOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "acl/lib_acl/event/acl_events.h"

#ifdef ACL_LINUX

typedef void (*monitor_callback)(void *);

void netlink_monitor(ACL_EVENT *event, monitor_callback callback, void *ctx);
#endif

#ifdef __cplusplus
}
#endif

#endif
