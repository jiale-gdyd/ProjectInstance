#ifndef __X_POLL_H__
#define __X_POLL_H__

#include "xlibconfig.h"
#include "xtypes.h"

X_BEGIN_DECLS

typedef struct _XPollFD XPollFD;

typedef xint (*XPollFunc)(XPollFD *ufds, xuint nfsd, xint timeout_);

struct _XPollFD {
    xint    fd;
    xushort events;
    xushort revents;
};

XLIB_AVAILABLE_IN_ALL
xint x_poll(XPollFD *fds, xuint nfds, xint timeout);

X_END_DECLS

#endif
