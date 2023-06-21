#ifndef ACL_LIBACL_EVENT_EVENTS_WMSG_H
#define ACL_LIBACL_EVENT_EVENTS_WMSG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "events_define.h"

#ifdef ACL_EVENTS_STYLE_WMSG
#define EVENT_NAME_WMSG         "windows message"

ACL_API ACL_EVENT *event_new_wmsg(UINT nMsg);
#endif 

#ifdef __cplusplus
}
#endif

#endif
