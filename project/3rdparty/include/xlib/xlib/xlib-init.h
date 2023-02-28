#ifndef __XLIB_INIT_H__
#define __XLIB_INIT_H__

#include "xmessages.h"

extern XLogLevelFlags x_log_msg_prefix;
extern XLogLevelFlags x_log_always_fatal;

void xlib_init(void);
void x_quark_init(void);
void x_error_init(void);

#endif
