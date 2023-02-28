#include <time.h>
#include <poll.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>

#include <xlib/xlib/config.h>
#include <xlib/xlib/xlibconfig.h>
#include <xlib/xlib/xpoll.h>

xint x_poll(XPollFD *fds, xuint nfds, xint timeout)
{
    return poll((struct pollfd *)fds, nfds, timeout);
}
