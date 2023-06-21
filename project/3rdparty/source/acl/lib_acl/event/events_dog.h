#ifndef ACL_LIBACL_EVENT_EVENTS_DOG_H
#define ACL_LIBACL_EVENT_EVENTS_DOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "acl/lib_acl/stdlib/acl_define.h"
#include "acl/lib_acl/event/acl_events.h"

typedef struct EVENT_DOG EVENT_DOG;

EVENT_DOG *event_dog_create(ACL_EVENT *eventp, int thread_mode);
void event_dog_notify(EVENT_DOG *evdog);
void event_dog_wait(EVENT_DOG *evdog);
ACL_VSTREAM *event_dog_client(EVENT_DOG *evdog);
void event_dog_free(EVENT_DOG *evdog);

#ifdef __cplusplus
}
#endif

#endif
