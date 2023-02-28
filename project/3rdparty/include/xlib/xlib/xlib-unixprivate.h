#ifndef __X_UNIXPRIVATE_H__
#define __X_UNIXPRIVATE_H__

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifndef __USE_GNU
#define __USE_GNU
#endif

#include "config.h"

#include "xmacros.h"
#include "xtypes.h"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

X_BEGIN_DECLS

extern int pipe2(int pipefd[2], int flags);

static inline xboolean x_unix_open_pipe_internal(int *fds, xboolean close_on_exec)
{
#ifdef HAVE_PIPE2
    do {
        int ecode;

        ecode = pipe2(fds, close_on_exec ? O_CLOEXEC : 0);
        if ((ecode == -1) && (errno != ENOSYS)) {
            return FALSE;
        } else if (ecode == 0) {
            return TRUE;
        }
    } while (FALSE);
#endif

    if (pipe(fds) == -1) {
        return FALSE;
    }

    if (!close_on_exec) {
        return TRUE;
    }

    if ((fcntl(fds[0], F_SETFD, FD_CLOEXEC) == -1) || (fcntl(fds[1], F_SETFD, FD_CLOEXEC) == -1)) {
        int saved_errno = errno;

        close(fds[0]);
        close(fds[1]);
        fds[0] = -1;
        fds[1] = -1;

        errno = saved_errno;
        return FALSE;
    }

    return TRUE;
}

X_END_DECLS

#endif