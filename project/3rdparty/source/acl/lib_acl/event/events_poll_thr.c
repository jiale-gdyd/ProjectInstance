#include "acl/lib_acl/StdAfx.h"

#ifndef ACL_PREPARE_COMPILE

#include "acl/lib_acl/stdlib/acl_define.h"
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "acl/lib_acl/stdlib/acl_sys_patch.h"
#include "acl/lib_acl/stdlib/acl_mymalloc.h"
#include "acl/lib_acl/stdlib/acl_mystring.h"
#include "acl/lib_acl/stdlib/acl_msg.h"
#include "acl/lib_acl/stdlib/acl_ring.h"
#include "acl/lib_acl/stdlib/acl_vstream.h"
#include "acl/lib_acl/stdlib/acl_iostuff.h"
#include "acl/lib_acl/event/acl_events.h"

#endif

#include "events_define.h"

#if defined(ACL_EVENTS_POLL_STYLE) && !defined(ACL_WINDOWS)
# include <poll.h>
# include <unistd.h>
# include "events_fdtable.h"
# include "events_dog.h"
# include "events.h"

typedef struct EVENT_POLL_THR {
    EVENT_THR event;
    struct pollfd *fds;
    struct pollfd *fdset;
    ACL_FD_MAP *fdmap;
} EVENT_POLL_THR;

static void event_enable_read(ACL_EVENT *eventp, ACL_VSTREAM *stream,
    int timeout, ACL_EVENT_NOTIFY_RDWR callback, void *context)
{
    const char *myname = "event_enable_read";
    EVENT_POLL_THR *event_thr = (EVENT_POLL_THR *) eventp;
    ACL_EVENT_FDTABLE *fdp;
    ACL_SOCKET sockfd;

    sockfd = ACL_VSTREAM_SOCK(stream);
    fdp = stream->fdp;
    if (fdp == NULL) {
        fdp = event_fdtable_alloc();
        fdp->listener = 0;
        fdp->stream = stream;
        stream->fdp = (void *) fdp;
    } else if (fdp->flag & EVENT_FDTABLE_FLAG_WRITE) {
        acl_msg_panic("%s(%d)->%s: fd %d: multiple I/O request",
            __FILE__, __LINE__, myname, sockfd);
    } else {
        fdp->listener = 0;
        fdp->stream = stream;
    }

    if (fdp->r_callback != callback || fdp->r_context != context) {
        fdp->r_callback = callback;
        fdp->r_context = context;
    }

    if (timeout > 0) {
        fdp->r_timeout = ((acl_int64) timeout) * 1000000;
        fdp->r_ttl = eventp->present + fdp->r_timeout;
    } else {
        fdp->r_ttl = 0;
        fdp->r_timeout = 0;
    }

    if ((fdp->flag & EVENT_FDTABLE_FLAG_READ) != 0) {
        return;
    }

    stream->nrefer++;
    fdp->flag = EVENT_FDTABLE_FLAG_READ | EVENT_FDTABLE_FLAG_EXPT;

    THREAD_LOCK(&event_thr->event.tb_mutex);

    fdp->fdidx = eventp->fdcnt;
    eventp->fdtabs[eventp->fdcnt++] = fdp;

    event_thr->fds[fdp->fdidx].fd = sockfd;
    event_thr->fds[fdp->fdidx].events = POLLIN | POLLHUP | POLLERR;
    if (eventp->maxfd == ACL_SOCKET_INVALID || eventp->maxfd < sockfd) {
        eventp->maxfd = sockfd;
    }

    acl_fdmap_add(event_thr->fdmap, sockfd, fdp);

    THREAD_UNLOCK(&event_thr->event.tb_mutex);

    /* 主要是为了减少通知次数 */
    if (event_thr->event.blocked && event_thr->event.evdog
        && event_dog_client(event_thr->event.evdog) != stream) {
        event_dog_notify(event_thr->event.evdog);
    }
}

static void event_enable_listen(ACL_EVENT *eventp, ACL_VSTREAM *stream,
    int timeout, ACL_EVENT_NOTIFY_RDWR callback, void *context)
{
    const char *myname = "event_enable_listen";
    EVENT_POLL_THR *event_thr = (EVENT_POLL_THR *) eventp;
    ACL_EVENT_FDTABLE *fdp;
    ACL_SOCKET sockfd;

    sockfd = ACL_VSTREAM_SOCK(stream);
    fdp = stream->fdp;
    if (fdp == NULL) {
        fdp = event_fdtable_alloc();
        fdp->listener = 1;
        fdp->stream = stream;
        stream->fdp = (void *) fdp;
    } else if (fdp->flag & EVENT_FDTABLE_FLAG_WRITE) {
        acl_msg_panic("%s(%d)->%s: fd %d: multiple I/O request",
            __FILE__, __LINE__, myname, sockfd);
    } else {
        fdp->listener = 1;
        fdp->stream = stream;
    }

    if (fdp->r_callback != callback || fdp->r_context != context) {
        fdp->r_callback = callback;
        fdp->r_context = context;
    }

    if (timeout > 0) {
        fdp->r_timeout = ((acl_int64) timeout) * 1000000;
        fdp->r_ttl = eventp->present + fdp->r_timeout;
    } else {
        fdp->r_ttl = 0;
        fdp->r_timeout = 0;
    }

    if ((fdp->flag & EVENT_FDTABLE_FLAG_READ) != 0) {
        return;
    }

    stream->nrefer++;
    fdp->flag = EVENT_FDTABLE_FLAG_READ | EVENT_FDTABLE_FLAG_EXPT;

    THREAD_LOCK(&event_thr->event.tb_mutex);

    fdp->fdidx = eventp->fdcnt;
    eventp->fdtabs[eventp->fdcnt++] = fdp;

    event_thr->fds[fdp->fdidx].fd = sockfd;
    event_thr->fds[fdp->fdidx].events = POLLIN | POLLHUP | POLLERR;
    if (eventp->maxfd == ACL_SOCKET_INVALID || eventp->maxfd < sockfd) {
        eventp->maxfd = sockfd;
    }

    acl_fdmap_add(event_thr->fdmap, sockfd, fdp);

    THREAD_UNLOCK(&event_thr->event.tb_mutex);
}

static void event_enable_write(ACL_EVENT *eventp, ACL_VSTREAM *stream,
    int timeout, ACL_EVENT_NOTIFY_RDWR callback, void *context)
{
    const char *myname = "event_enable_write";
    EVENT_POLL_THR *event_thr = (EVENT_POLL_THR *) eventp;
    ACL_EVENT_FDTABLE *fdp;
    ACL_SOCKET sockfd;

    sockfd = ACL_VSTREAM_SOCK(stream);
    fdp = (ACL_EVENT_FDTABLE*) stream->fdp;
    if (fdp == NULL) {
        fdp = event_fdtable_alloc();
        fdp->listener = 0;
        fdp->stream = stream;
        stream->fdp = (void *) fdp;
    } else if (fdp->flag & EVENT_FDTABLE_FLAG_READ) {
        acl_msg_panic("%s(%d)->%s: fd %d: multiple I/O request",
            __FILE__, __LINE__, myname, sockfd);
    } else {
        fdp->listener = 0;
        fdp->stream = stream;
    }

    if (fdp->w_callback != callback || fdp->w_context != context) {
        fdp->w_callback = callback;
        fdp->w_context = context;
    }

    if (timeout > 0) {
        fdp->w_timeout = ((acl_int64) timeout) * 1000000;
        fdp->w_ttl = eventp->present + fdp->w_timeout;
    } else {
        fdp->w_ttl = 0;
        fdp->w_timeout = 0;
    }

    if ((fdp->flag & EVENT_FDTABLE_FLAG_WRITE) != 0) {
        return;
    }

    stream->nrefer++;
    fdp->flag = EVENT_FDTABLE_FLAG_WRITE | EVENT_FDTABLE_FLAG_EXPT;

    THREAD_LOCK(&event_thr->event.tb_mutex);

    fdp->fdidx = eventp->fdcnt;
    eventp->fdtabs[eventp->fdcnt++] = fdp;

    event_thr->fds[fdp->fdidx].fd = sockfd;
    event_thr->fds[fdp->fdidx].events = POLLOUT | POLLHUP | POLLERR;
    if (eventp->maxfd == ACL_SOCKET_INVALID || eventp->maxfd < sockfd) {
        eventp->maxfd = sockfd;
    }

    acl_fdmap_add(event_thr->fdmap, sockfd, fdp);

    THREAD_UNLOCK(&event_thr->event.tb_mutex);

    if (event_thr->event.blocked && event_thr->event.evdog
        && event_dog_client(event_thr->event.evdog) != stream) {
        event_dog_notify(event_thr->event.evdog);
    }
}

/* event_disable_readwrite - disable request for read or write events */

static void event_disable_readwrite(ACL_EVENT *eventp, ACL_VSTREAM *stream)
{
    const char *myname = "event_disable_readwrite";
    EVENT_POLL_THR *event_thr = (EVENT_POLL_THR *) eventp;
    ACL_EVENT_FDTABLE *fdp;
    ACL_SOCKET sockfd;

    sockfd = ACL_VSTREAM_SOCK(stream);
    fdp = (ACL_EVENT_FDTABLE *) stream->fdp;
    if (fdp == NULL) {
        acl_msg_error("%s(%d): fdp null", myname, __LINE__);
        return;
    }

    if ((fdp->flag & (EVENT_FDTABLE_FLAG_READ
        | EVENT_FDTABLE_FLAG_WRITE)) == 0) {
        acl_msg_error("%s(%d): sockfd(%d) not be set",
            myname, __LINE__, sockfd);
        return;
    }
    if (fdp->fdidx == -1) {
        acl_msg_fatal("%s(%d): fdidx(%d) invalid",
            myname, __LINE__, fdp->fdidx);
    }

    THREAD_LOCK(&event_thr->event.tb_mutex);

    if (eventp->maxfd == sockfd) {
        eventp->maxfd = ACL_SOCKET_INVALID;
    }

    if (eventp->fdtabs[fdp->fdidx] != fdp) {
        acl_msg_fatal("%s(%d): fdidx(%d)'s fdp invalid",
            myname, __LINE__, fdp->fdidx);
    }

    if (fdp->fdidx < --eventp->fdcnt) {
        eventp->fdtabs[fdp->fdidx] = eventp->fdtabs[eventp->fdcnt];
        eventp->fdtabs[fdp->fdidx]->fdidx = fdp->fdidx;
        event_thr->fds[fdp->fdidx] = event_thr->fds[eventp->fdcnt];
    }

    acl_fdmap_del(event_thr->fdmap, sockfd);

    THREAD_UNLOCK(&event_thr->event.tb_mutex);

    if (fdp->flag & EVENT_FDTABLE_FLAG_READ) {
        stream->nrefer--;
    }
    if (fdp->flag & EVENT_FDTABLE_FLAG_WRITE) {
        stream->nrefer--;
    }

    event_fdtable_reset(fdp);
}

static int event_isrset(ACL_EVENT *eventp acl_unused, ACL_VSTREAM *stream)
{
    ACL_EVENT_FDTABLE *fdp;

    fdp = (ACL_EVENT_FDTABLE *) stream->fdp;
    if (fdp == NULL) {
        return 0;
    }

    return (fdp->flag & EVENT_FDTABLE_FLAG_READ) == 0 ? 0 : 1;
}

static int event_iswset(ACL_EVENT *eventp acl_unused, ACL_VSTREAM *stream)
{
    ACL_EVENT_FDTABLE *fdp;

    fdp = (ACL_EVENT_FDTABLE *) stream->fdp;
    if (fdp == NULL) {
        return 0;
    }

    return (fdp->flag & EVENT_FDTABLE_FLAG_WRITE) == 0 ? 0 : 1;
}

static int event_isxset(ACL_EVENT *eventp acl_unused, ACL_VSTREAM *stream)
{
    ACL_EVENT_FDTABLE *fdp;

    fdp = (ACL_EVENT_FDTABLE *) stream->fdp;
    if (fdp == NULL) {
        return 0;
    }

    return (fdp->flag & EVENT_FDTABLE_FLAG_EXPT) == 0 ? 0 : 1;
}

static void event_loop(ACL_EVENT *eventp)
{
    const char *myname = "event_loop";
    EVENT_POLL_THR *event_thr = (EVENT_POLL_THR *) eventp;
    int   nready, i, revents, fdcnt;
    acl_int64 delay, when;
    ACL_EVENT_FDTABLE *fdp;

    delay = eventp->delay_sec * 1000000 + eventp->delay_usec;

    if (delay < DELAY_MIN) {
        delay = DELAY_MIN;
    }

    SET_TIME(eventp->present);
    THREAD_LOCK(&event_thr->event.tm_mutex);

    /*
    * Find out when the next timer would go off. Timer requests
    * are sorted. If any timer is scheduled, adjust the delay
    * appropriately.
    */
    when = event_timer_when(eventp);
    if (when >= 0) {
        acl_int64 n = when - eventp->present;
        if (n <= 0) {
            delay = 0;
        } else if (n < delay) {
            delay = n;
        }
    }

    THREAD_UNLOCK(&event_thr->event.tm_mutex);

    eventp->ready_cnt = 0;

    if (eventp->present - eventp->last_check >= eventp->check_inter) {
        eventp->last_check = eventp->present;

        THREAD_LOCK(&event_thr->event.tb_mutex);

        if (event_thr_prepare(eventp) == 0) {

            THREAD_UNLOCK(&event_thr->event.tb_mutex);

            if (eventp->ready_cnt == 0) {
                acl_doze(delay > DELAY_MIN ? (int) delay / 1000 : 1);
            }

            nready = 0;
            goto TAG_DONE;
        }

        memcpy(event_thr->fdset, event_thr->fds,
            eventp->fdcnt * sizeof(struct pollfd));
        fdcnt = eventp->fdcnt;

        THREAD_UNLOCK(&event_thr->event.tb_mutex);

        if (eventp->ready_cnt > 0) {
            delay = 0;
        }
    } else {
        THREAD_LOCK(&event_thr->event.tb_mutex);

        memcpy(event_thr->fdset, event_thr->fds,
            eventp->fdcnt * sizeof(struct pollfd));
        fdcnt = eventp->fdcnt;

        THREAD_UNLOCK(&event_thr->event.tb_mutex);
    }

    event_thr->event.blocked = 1;
    nready = poll(event_thr->fdset, fdcnt, (int) (delay / 1000));
    event_thr->event.blocked = 0;

    if (nready < 0) {
        if (acl_last_error() != ACL_EINTR) {
            acl_msg_fatal("%s(%d), %s: event_loop: poll: %s",
                __FILE__, __LINE__, myname, acl_last_serror());
        }
        goto TAG_DONE;
    } else if (nready == 0) {
        goto TAG_DONE;
    }

    THREAD_LOCK(&event_thr->event.tb_mutex);

    for (i = 0; i < fdcnt; i++) {
        fdp = acl_fdmap_ctx(event_thr->fdmap, event_thr->fdset[i].fd);
        if (fdp == NULL || fdp->stream == NULL) {
            continue;
        }
        if ((fdp->event_type & (ACL_EVENT_XCPT | ACL_EVENT_RW_TIMEOUT))) {
            continue;
        }

        revents = event_thr->fdset[i].revents;
        if ((revents & POLLIN) != 0) {
            if ((fdp->event_type & ACL_EVENT_READ) == 0) {
                fdp->event_type |= ACL_EVENT_READ;
                fdp->fdidx_ready = eventp->ready_cnt;
                eventp->ready[eventp->ready_cnt] = fdp;
                eventp->ready_cnt++;
            }

            if (fdp->listener) {
                fdp->event_type |= ACL_EVENT_ACCEPT;
            } else {
                fdp->stream->read_ready = 1;
            }
        } else if ((revents & POLLOUT) != 0) {
            fdp->event_type |= ACL_EVENT_WRITE;
            fdp->fdidx_ready = eventp->ready_cnt;
            eventp->ready[eventp->ready_cnt++] = fdp;
        } else if ((revents & (POLLHUP | POLLERR)) != 0) {
            fdp->event_type |= ACL_EVENT_XCPT;
            fdp->fdidx_ready = eventp->ready_cnt;
            eventp->ready[eventp->ready_cnt++] = fdp;
        }
    }

    THREAD_UNLOCK(&event_thr->event.tb_mutex);

TAG_DONE:

    /* Deliver timer events */
    event_timer_trigger_thr(&event_thr->event);

    if (eventp->ready_cnt > 0) {
        event_thr_fire(eventp);
    }
}

static void event_add_dog(ACL_EVENT *eventp)
{
    EVENT_POLL_THR *event_thr = (EVENT_POLL_THR*) eventp;

    event_thr->event.evdog = event_dog_create((ACL_EVENT*) event_thr, 1);
}

static void event_free(ACL_EVENT *eventp)
{
    const char *myname = "event_free";
    EVENT_POLL_THR *event_thr = (EVENT_POLL_THR *) eventp;

    if (eventp == NULL)
        acl_msg_fatal("%s, %s(%d): eventp null",
            __FILE__, myname, __LINE__);

    LOCK_DESTROY(&event_thr->event.tm_mutex);
    LOCK_DESTROY(&event_thr->event.tb_mutex);

    acl_fdmap_free(event_thr->fdmap);
    acl_myfree(event_thr->fdset);
    acl_myfree(event_thr->fds);
    acl_myfree(eventp);
}

ACL_EVENT *event_poll_alloc_thr(int fdsize)
{
    EVENT_POLL_THR *event_thr;

    event_thr = (EVENT_POLL_THR*) event_alloc(sizeof(EVENT_POLL_THR));

    snprintf(event_thr->event.event.name,
        sizeof(event_thr->event.event.name), "thread events - poll");

    event_thr->event.event.event_mode           = ACL_EVENT_POLL;
    event_thr->event.event.use_thread           = 1;
    event_thr->event.event.loop_fn              = event_loop;
    event_thr->event.event.free_fn              = event_free;
    event_thr->event.event.add_dog_fn           = event_add_dog;
    event_thr->event.event.enable_read_fn       = event_enable_read;
    event_thr->event.event.enable_write_fn      = event_enable_write;
    event_thr->event.event.enable_listen_fn     = event_enable_listen;
    event_thr->event.event.disable_readwrite_fn = event_disable_readwrite;
    event_thr->event.event.isrset_fn            = event_isrset;
    event_thr->event.event.iswset_fn            = event_iswset;
    event_thr->event.event.isxset_fn            = event_isxset;
    event_thr->event.event.timer_request        = event_timer_request_thr;
    event_thr->event.event.timer_cancel         = event_timer_cancel_thr;
    event_thr->event.event.timer_keep           = event_timer_keep_thr;
    event_thr->event.event.timer_ifkeep         = event_timer_ifkeep_thr;

    LOCK_INIT(&event_thr->event.tm_mutex);
    LOCK_INIT(&event_thr->event.tb_mutex);

    event_thr->fds = (struct pollfd *) acl_mycalloc(fdsize + 1,
            sizeof(struct pollfd));
    event_thr->fdset = (struct pollfd *) acl_mycalloc(fdsize + 1,
            sizeof(struct pollfd));
    event_thr->fdmap = acl_fdmap_create(fdsize);
    return (ACL_EVENT *) event_thr;
}

#endif	/* ACL_EVENTS_POLL_STYLE */
