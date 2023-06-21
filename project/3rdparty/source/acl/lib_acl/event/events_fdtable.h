#ifndef ACL_LIBACL_EVENT_EVENTS_FDTABLE_H
#define ACL_LIBACL_EVENT_EVENTS_FDTABLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "acl/lib_acl/event/acl_events.h"

extern ACL_EVENT_FDTABLE *event_fdtable_alloc(void);
extern void event_fdtable_free(ACL_EVENT_FDTABLE *fdp);
extern void event_fdtable_reset(ACL_EVENT_FDTABLE *fdp);

#ifdef __cplusplus
}
#endif

#endif
