#include <fcntl.h>
#include <stdint.h>
#include <sys/eventfd.h>
#include <xlib/xlib/config.h>
#include <xlib/xlib/xpoll.h>
#include <xlib/xlib/xtypes.h>
#include <xlib/xlib/xwakeup.h>
#include <xlib/xlib/xlib-unix.h>

struct _XWakeup {
    xint fds[2];
};

XWakeup *x_wakeup_new(void)
{
    XWakeup *wakeup;
    XError *error = NULL;

    wakeup = x_slice_new(XWakeup);
    wakeup->fds[0] = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);

    if (wakeup->fds[0] != -1) {
        wakeup->fds[1] = -1;
        return wakeup;
    }

    if (!x_unix_open_pipe(wakeup->fds, O_CLOEXEC | O_NONBLOCK, &error)) {
        x_error("Creating pipes for XWakeup: %s", error->message);
    }

    if (!x_unix_set_fd_nonblocking(wakeup->fds[0], TRUE, &error) || !x_unix_set_fd_nonblocking(wakeup->fds[1], TRUE, &error)) {
        x_error("Set pipes non-blocking for XWakeup: %s", error->message);
    }

    return wakeup;
}

void x_wakeup_get_pollfd(XWakeup *wakeup, XPollFD *poll_fd)
{
    poll_fd->fd = wakeup->fds[0];
    poll_fd->events = X_IO_IN;
}

void x_wakeup_acknowledge(XWakeup *wakeup)
{
    int res;

    if (wakeup->fds[1] == -1) {
        uint64_t value;

        do {
            res = read(wakeup->fds[0], &value, sizeof(value));
        } while (X_UNLIKELY(res == -1 && errno == EINTR));
    } else {
        uint8_t value;

        do {
            res = read(wakeup->fds[0], &value, sizeof(value));
        } while (res == sizeof(value) || X_UNLIKELY(res == -1 && errno == EINTR));
    }
}

void x_wakeup_signal(XWakeup *wakeup)
{
    int res;

    if (wakeup->fds[1] == -1) {
        uint64_t one = 1;

        do {
            res = write(wakeup->fds[0], &one, sizeof one);
        } while (X_UNLIKELY(res == -1 && errno == EINTR));
    } else {
        uint8_t one = 1;

        do {
            res = write(wakeup->fds[1], &one, sizeof one);
        } while (X_UNLIKELY(res == -1 && errno == EINTR));
    }
}

void x_wakeup_free(XWakeup *wakeup)
{
    close(wakeup->fds[0]);
    if (wakeup->fds[1] != -1) {
        close(wakeup->fds[1]);
    }

    x_slice_free(XWakeup, wakeup);
}
