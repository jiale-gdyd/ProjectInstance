#include "acl/lib_acl/StdAfx.h"

#ifndef ACL_PREPARE_COMPILE

#include "acl/lib_acl/stdlib/acl_define.h"

#include <string.h>  /* in SUNOS, FD_ZERO need memset() */
#include <errno.h>

#endif

#ifdef    ACL_UNIX
# include <fcntl.h>
# include <poll.h>
# include <unistd.h>
#endif

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "acl/lib_acl/stdlib/acl_msg.h"
#include "acl/lib_acl/stdlib/acl_mymalloc.h"
#include "acl/lib_acl/stdlib/acl_iostuff.h"
#include "../../init/init.h"

#if defined(ACL_LINUX) && !defined(MINGW)

#include "acl/lib_acl/init/acl_init.h"
#include "acl/lib_acl/thread/acl_pthread.h"
#include <sys/epoll.h>

typedef struct EPOLL_CTX
{
    acl_pthread_t tid;
    int  epfd;
} EPOLL_CTX;

#ifndef HAVE_NO_ATEXIT
static EPOLL_CTX *main_epoll_ctx = NULL;

static void main_epoll_end(void)
{
    if (main_epoll_ctx != NULL) {
        acl_msg_info("%s(%d), %s: close epoll_fd: %d, tid: %lu, %lu",
            __FILE__, __LINE__, __FUNCTION__, main_epoll_ctx->epfd,
            (unsigned long) main_epoll_ctx->tid,
            (unsigned long) acl_pthread_self());

        close(main_epoll_ctx->epfd);
        acl_myfree(main_epoll_ctx);
        main_epoll_ctx = NULL;
    }
}
#endif

static acl_pthread_key_t epoll_key;
static acl_pthread_once_t epoll_once = ACL_PTHREAD_ONCE_INIT;

static void thread_epoll_end(void *ctx)
{
    EPOLL_CTX *epoll_ctx = (EPOLL_CTX*) ctx;

    acl_msg_info("%s(%d), %s: close epoll_fd: %d, tid: %lu, %lu",
        __FILE__, __LINE__, __FUNCTION__, epoll_ctx->epfd,
        (unsigned long) epoll_ctx->tid,
        (unsigned long) acl_pthread_self());

    close(epoll_ctx->epfd);
    acl_myfree(epoll_ctx);
}

static void thread_epoll_once(void)
{
    if (acl_pthread_key_create(&epoll_key, thread_epoll_end) != 0) {
        abort();
    }
}

static EPOLL_CTX *thread_epoll_init(void)
{
    EPOLL_CTX *epoll_ctx = (EPOLL_CTX*) acl_mymalloc(sizeof(EPOLL_CTX));

    if (acl_pthread_setspecific(epoll_key, epoll_ctx) != 0)
        abort();

    epoll_ctx->tid = acl_pthread_self();
    epoll_ctx->epfd = epoll_create(1);
    if (epoll_ctx == NULL) {
        acl_msg_error("%s(%d): epoll_create error: %s",
            __FUNCTION__, __LINE__, acl_last_serror());
        return NULL;
    }

    if ((unsigned long) acl_pthread_self() == (unsigned long) acl_main_thread_self()) {
#ifndef HAVE_NO_ATEXIT
        main_epoll_ctx = epoll_ctx;
        atexit(main_epoll_end);
#endif
        acl_msg_info("%s(%d): %s, create epoll_fd: %d, tid: %lu, %lu",
            __FILE__, __LINE__, __FUNCTION__, epoll_ctx->epfd,
            (unsigned long) epoll_ctx->tid,
            (unsigned long) acl_pthread_self());
    } else {
        acl_msg_info("%s(%d): %s, create epoll_fd: %d, tid: %lu, %lu",
            __FILE__, __LINE__, __FUNCTION__, epoll_ctx->epfd,
            (unsigned long) epoll_ctx->tid,
            (unsigned long) acl_pthread_self());
    }

    return epoll_ctx;
}

static int thread_epoll_reopen(EPOLL_CTX *epoll_ctx)
{
    close(epoll_ctx->epfd);
    epoll_ctx->epfd = epoll_create(1);
    if (epoll_ctx->epfd == -1) {
        acl_msg_error("%s(%d): epoll_create error: %s",
            __FUNCTION__, __LINE__, acl_last_serror());
        return -1;
    }
    return 0;
}

int acl_read_epoll_wait(ACL_SOCKET fd, int delay)
{
    int   ret, retried = 0;
    EPOLL_CTX *epoll_ctx;
    struct epoll_event ee, events[1];
    time_t begin;

    if (acl_pthread_once(&epoll_once, thread_epoll_once) != 0) {
        abort();
    }
    epoll_ctx = (EPOLL_CTX*) acl_pthread_getspecific(epoll_key);
    if (epoll_ctx == NULL) {
        epoll_ctx = thread_epoll_init();
        if (epoll_ctx == NULL) {
            acl_msg_error("%s(%d): thread_epoll_init error",
                __FUNCTION__, __LINE__);
            return -1;
        }
    }

    ee.events   = EPOLLIN;
    ee.data.u64 = 0;
    ee.data.fd  = fd;

    while (1) {
        if (epoll_ctl(epoll_ctx->epfd, EPOLL_CTL_ADD, fd, &ee) == 0) {
            break;
        }

        ret = acl_last_error();

        if (ret == EEXIST) {
            break;
        }

        if (ret == EBADF || ret == EINVAL) {
            if (retried) {
                return -1;
            }
            if (thread_epoll_reopen(epoll_ctx) == -1) {
                return -1;
            }
            retried = 1;
            continue;
        }

        acl_msg_error("%s(%d): epoll_ctl error: %s, fd: %d, "
            "epfd: %d, tid: %lu, %lu", __FUNCTION__, __LINE__,
            acl_last_serror(), fd, epoll_ctx->epfd,
            (unsigned long) epoll_ctx->tid,
            (unsigned long) acl_pthread_self());

        return -1;
    }

    retried = 0;

    for (;;) {
        time(&begin);

        ret = epoll_wait(epoll_ctx->epfd, events, 1, delay);
        if (ret == -1) {
            ret = acl_last_error();

            if (ret == ACL_EINTR) {
                acl_msg_warn(">>>>catch EINTR, try again<<<");
                continue;
            } else if (ret == EBADF || ret == EINTR) {
                acl_msg_error("%s(%d): fd: %d, epfd: %d,"
                    " error: %s", __FUNCTION__, __LINE__, fd,
                    epoll_ctx->epfd, acl_last_serror());

                if (retried) {
                    ret = -1;
                    break;
                }
                if (thread_epoll_reopen(epoll_ctx) == -1) {
                    ret = -1;
                    break;
                }

                retried = 1;
                continue;
            }

            acl_msg_error("%s(%d): epoll_wait error: %s, fd: %d,"
                " epfd: %d, tid: %lu, %lu", __FUNCTION__, __LINE__,
                acl_last_serror(), fd, epoll_ctx->epfd,
                (unsigned long) epoll_ctx->tid,
                (unsigned long) acl_pthread_self());
            ret = -1;
            break;
        } else if (ret == 0) {
            /*
            acl_msg_warn("%s(%d), %s: poll timeout: %s, fd: %d, "
                "delay: %d, spent: %ld", __FILE__, __LINE__,
                __FUNCTION__, acl_last_serror(), fd, delay,
                (long) (time(NULL) - begin));
            */
            if (delay == 0) {
                acl_set_error(ACL_EAGAIN);
            } else {
                acl_set_error(ACL_ETIMEDOUT);
            }
            ret = -1;
            break;
        } else if ((events[0].events & EPOLLIN) != 0) {
            ret = 0;
        } else if ((events[0].events & (EPOLLERR | EPOLLHUP)) != 0) {
            ret = 0;
        } else {
            acl_msg_warn("%s(%d), %s: poll error: %s, fd: %d, "
                "delay: %d, spent: %ld", __FILE__, __LINE__,
                __FUNCTION__, acl_last_serror(), fd, delay,
                (long) (time(NULL) - begin));
            ret = -1;
        }
        break;
    }

    ee.events = 0;
    ee.data.u64 = 0;
    ee.data.fd = fd;
    if (epoll_ctl(epoll_ctx->epfd, EPOLL_CTL_DEL, fd, &ee) == -1) {
        acl_msg_error("%s(%d): epoll_ctl error: %s, fd: %d, epfd: %d,"
            " tid: %lu, %lu", __FUNCTION__, __LINE__, acl_last_serror(),
            fd, epoll_ctx->epfd, (unsigned long) epoll_ctx->tid,
            (unsigned long) acl_pthread_self());
        return -1;
    }

    return ret;
}

#endif

static int __poll_delay_slice = 100;

void acl_set_delay_slice(int n) {
    if (n > 0) {
        __poll_delay_slice = n;
    }
}

#if defined(ACL_HAS_POLL)

# if defined(ACL_WINDOWS)
static acl_poll_fn __sys_poll = WSAPoll;
# else
static acl_poll_fn __sys_poll = poll;
# endif

/* xxx: defined in acl_write_wait.c */
extern void set_poll4write(acl_poll_fn fn);

static void set_poll4read(acl_poll_fn fn)
{
    __sys_poll = fn;
}

void acl_set_poll(acl_poll_fn fn)
{
    set_poll4read(fn);
}

int acl_read_poll_wait(ACL_SOCKET fd, int delay)
{
    struct pollfd fds;
    time_t begin;
    int left = delay;

    for (;;) {
        fds.fd = fd;
#ifdef ACL_WINDOWS
        /* bugfix, POLLHUP, pollerr can't be set for windows */
        fds.events = POLLIN /* | POLLHUP | POLLERR */;
#else
        fds.events = POLLIN | POLLHUP | POLLERR | POLLPRI;
#endif
        fds.revents = 0;

        acl_set_error(0);

        time(&begin);

#ifdef __APPLE__
        /* �����߳�����ͨ�� poll �����ȴ�ʱ���������߳��첽������ close ���̣�
         * poll Ӧ���������ش���Ŷԣ����� MacOS/iOS �ϵ� poll �и����⣬��
         * һֱ�ȴ���ʱ�ŷ��أ����Դ˴��� 100 ms Ϊ��λ��ʱ�������Ƭ���Ծ�����
         * ��ظ�֪�� close ��Ϊ��
         */
        if (left > __poll_delay_slice) {
            delay = __poll_delay_slice;
        } else {
            delay = left;
        }
#else
        delay = left;
#endif
        left -= delay;

        switch (__sys_poll(&fds, 1, delay)) {
#ifdef ACL_WINDOWS
        case SOCKET_ERROR:
#else
        case -1:
#endif
            if (acl_last_error() == ACL_EINTR) {
                break;
            }
            /*
            acl_msg_error("%s(%d), %s: poll error(%s), fd: %d",
                __FILE__, __LINE__, __FUNCTION__,
                acl_last_serror(), (int) fd);
            */
            return -1;
        case 0:
            /*
            acl_msg_warn("%s(%d), %s: poll timeout: %s, fd: %d, "
                "delay: %d, spent: %ld", __FILE__, __LINE__,
                __FUNCTION__, acl_last_serror(), fd, delay,
                (long) (time(NULL) - begin));
            */
            if (left > 0) {
                break;
            }

            if (delay == 0) {
                acl_set_error(ACL_EAGAIN);
            } else {
                acl_set_error(ACL_ETIMEDOUT);
            }
            return -1;
        default:
            if ((fds.revents & POLLIN)) {
                return 0;
            }
            if (fds.revents & (POLLHUP | POLLERR | POLLNVAL)) {
                acl_msg_warn("%s(%d), %s: poll error: %s, "
                    "fd: %d, delay: %d, spent: %ld",
                    __FILE__, __LINE__, __FUNCTION__ ,
                    acl_last_serror(), fd, delay,
                    (long) (time(NULL) - begin));
                return 0;
            }

            acl_msg_warn("%s(%d), %s: poll error: %s, fd: %d, "
                "delay: %d, spent: %ld", __FILE__, __LINE__,
                __FUNCTION__, acl_last_serror(), fd, delay,
                (long) (time(NULL) - begin));
            return -1;
        }
    }
}

#endif

#if defined(WIN32_XX)

static HANDLE __handle = NULL;

int acl_read_iocp_wait(ACL_SOCKET fd, int timeout)
{
    OVERLAPPED *overlapped, *lpOverlapped;
    DWORD recvBytes;
    BOOL isSuccess;
    ACL_SOCKET fd2;
    DWORD bytesTransferred = 0;
    DWORD delay = timeout;
    HANDLE h2;

    if (__handle == NULL) {
        __handle = CreateIoCompletionPort(INVALID_HANDLE_VALUE,
                NULL, 0, 0);
    }
    if (__handle == NULL) {
        acl_msg_error("CreateIoCompletionPort: %s",
            acl_last_serror());
        return -1;
    }

    overlapped = acl_mycalloc(1, sizeof(OVERLAPPED));

    h2  = CreateIoCompletionPort((HANDLE) fd, __handle,
        (DWORD) overlapped, 0);
    if (h2 == NULL) {
        acl_myfree(overlapped);
    } else if (h2 != __handle) {
        acl_msg_error("invalid h2 by CreateIoCompletionPort: %s",
            acl_last_serror());
        CloseHandle(h2);
        return -1;
    }

    if (ReadFile((HANDLE) fd, NULL, 0, &recvBytes, overlapped) == FALSE
          && acl_last_error() != ERROR_IO_PENDING) {
        acl_msg_warn("%s(%d): ReadFile error(%s)",
            __FUNCTION__, __LINE__, acl_last_serror());
        return -1;
    }

    delay = 6000;

    while (1) {
        isSuccess = GetQueuedCompletionStatus(__handle,
                &bytesTransferred,
                (DWORD*) &fd2,
                (OVERLAPPED**) &lpOverlapped,
                delay);
        if (lpOverlapped == NULL) {
            break;
        }

        if (HasOverlappedIoCompleted(lpOverlapped)) {
            acl_myfree(lpOverlapped);
        }

        if (isSuccess) {
            return 0;
        }
    }

    acl_msg_warn("timeout error: %s", acl_last_serror());
    return -1;
}

#endif

static acl_select_fn __sys_select = select;

void acl_set_select(acl_select_fn fn)
{
    __sys_select = fn;
}

int acl_read_select_wait(ACL_SOCKET fd, int delay)
{
    fd_set  rfds, xfds;
    struct timeval tv;
    struct timeval *tp;
    int  errnum;
    time_t begin;

    /*
     * Sanity checks.
     */
#ifndef ACL_WINDOWS
    if (FD_SETSIZE <= (unsigned) fd)
        acl_msg_fatal("%s(%d), %s: descriptor %d does not fit "
            "FD_SETSIZE %d", __FILE__, __LINE__, __FUNCTION__,
            (int) fd, FD_SETSIZE);
#endif

    /*
     * Guard the write() with select() so we do not depend on alarm()
     * and on signal() handlers. Restart the select when interrupted
     * by some signal. Some select() implementations may reduce the
     * time to wait when interrupted, which is exactly what we want.
     */
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    FD_ZERO(&xfds);
    FD_SET(fd, &xfds);

    if (delay >= 0) {
        tv.tv_usec = 0;
        tv.tv_sec  = delay / 1000;
        tp = &tv;
    } else {
        tp = 0;
    }

    acl_set_error(0);

    for (;;) {
        time(&begin);

#ifdef ACL_WINDOWS
        switch (__sys_select(1, &rfds, (fd_set *) 0, &xfds, tp)) {
#else
        switch (__sys_select(fd + 1, &rfds, (fd_set *) 0, &xfds, tp)) {
#endif
        case -1:
            errnum = acl_last_error();
#ifdef    ACL_WINDOWS
            if (errnum == WSAEINPROGRESS
                || errnum == WSAEWOULDBLOCK
                || errnum == ACL_EINTR) {

                continue;
            }
#else
            if (errnum == ACL_EINTR) {
                continue;
            }
#endif
            acl_msg_error("%s(%d), %s: select error(%s), fd: %d",
                __FILE__, __LINE__, __FUNCTION__,
                acl_last_serror(), (int) fd);
            return -1;
        case 0:
            /*
            acl_msg_warn("%s(%d), %s: poll timeout: %s, fd: %d, "
                "timeout: %d, spent: %ld", __FILE__, __LINE__,
                __FUNCTION__, acl_last_serror(), fd, timeout,
                (long) (time(NULL) - begin));
            */
            if (delay == 0) {
                acl_set_error(ACL_EAGAIN);
            } else {
                acl_set_error(ACL_ETIMEDOUT);
            }
            return -1;
        default:
            return 0;
        }
    }
}

int acl_read_wait(ACL_SOCKET fd, int timeout)
{
    return acl_read_wait_ms(fd, timeout * 1000);
}

/* #define USE_EPOLL */

int acl_read_wait_ms(ACL_SOCKET fd, int timeout)
{
#if defined(ACL_LINUX) && !defined(MINGW) && defined(USE_EPOLL)
    return acl_read_epoll_wait(fd, timeout);
#elif defined(ACL_HAS_POLL)
    return acl_read_poll_wait(fd, timeout);
#else
    return acl_read_select_wait(fd, timeout);
#endif
}
