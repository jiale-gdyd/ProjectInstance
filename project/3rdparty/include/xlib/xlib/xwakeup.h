#ifndef __X_WAKEUP_H__
#define __X_WAKEUP_H__

#include "xpoll.h"

typedef struct _XWakeup XWakeup;

XWakeup *x_wakeup_new(void);
void x_wakeup_free(XWakeup *wakeup);

void x_wakeup_signal(XWakeup *wakeup);
void x_wakeup_acknowledge(XWakeup *wakeup);
void x_wakeup_get_pollfd(XWakeup *wakeup, XPollFD *poll_fd);

#endif
