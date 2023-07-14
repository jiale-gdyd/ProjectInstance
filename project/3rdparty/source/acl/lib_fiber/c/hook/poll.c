#include "../stdafx.h"
#include "../common.h"

#include "../event.h"
#include "../fiber.h"
#include "hook.h"

#ifdef HAS_POLL

struct POLLFD {
    RING me;
    FILE_EVENT *fe;
    POLL_EVENT *pe;
    struct pollfd *pfd;
};

/****************************************************************************/

#define TO_APPL ring_to_appl

/**
 * The callback is set by poll_event_set() when user calls acl_fiber_poll().
 * The callback will be called when the fd included by FILE_EVENT is ready,
 * and POLLIN flag will be set in the specified FILE_EVENT that will be used
 * by the application called acl_fiber_poll().
 */
static void handle_poll_read(EVENT *ev, FILE_EVENT *fe, POLLFD *pfd)
{
    assert(pfd->pfd->events & POLLIN);

    pfd->pfd->revents |= POLLIN;

    if (fe->mask & EVENT_ERR) {
        pfd->pfd->revents |= POLLERR;
    }
    if (fe->mask & EVENT_HUP) {
        pfd->pfd->revents |= POLLHUP;
    }
    if (fe->mask & EVENT_NVAL) {
        pfd->pfd->revents |= POLLNVAL;
    }

    pfd->fe->fiber_r = NULL;

    if (!(pfd->pfd->events & POLLOUT)) {
        ring_detach(&pfd->me);
        pfd->fe = NULL;
    }

    /*
    * If any fe has been ready, the pe holding fe should be removed from
    * ev->poll_list to avoid to be called in timeout process.
    * We should just remove pe only once by checking if the value of
    * pe->nready is 0. After the pe has been removed from the
    * ev->poll_list, the pe's callback will not be called again in the
    * timeout process in event_process_poll() in event.c.
    */
    if (pfd->pe->nready == 0) {
        /* The cache timer has been be set when timeout >= 0. */
        if (pfd->pe->expire >= 0) {
            timer_cache_remove(ev->poll_list, pfd->pe->expire,
                &pfd->pe->me);
        }
        ring_prepend(&ev->poll_ready, &pfd->pe->me);
    }

    pfd->pe->nready++;
}

static void read_callback(EVENT *ev, FILE_EVENT *fe)
{
    POLLFD *pfd;
    //RING_ITER iter;
    RING *iter = fe->pfds.succ, *next = iter;

    event_del_read(ev, fe);
    SET_READABLE(fe);

#if 1
    // Walk througth the RING list, handle each poll event, and one RING
    // node maybe be detached after it has been handled without any poll
    // event bound with it again.
    for (; iter != &fe->pfds; iter = next) {
        next = next->succ;
        pfd = ring_to_appl(iter, POLLFD, me);
        if (pfd->pfd->events & POLLIN) {
            handle_poll_read(ev, fe, pfd);
        }
    }
#else
    ring_foreach(iter, &fe->pfds) {
        pfd = ring_to_appl(iter.ptr, POLLFD, me);
        if (pfd->pfd->events & POLLIN) {
            handle_poll_read(ev, fe, pfd);
            break;
        }
    }
#endif
}

/**
 * Similiar to read_callback except that the POLLOUT flag will be set in it.
 */
static void handle_poll_write(EVENT *ev, FILE_EVENT *fe, POLLFD *pfd)
{
    assert(pfd->pfd->events & POLLOUT);

    pfd->pfd->revents |= POLLOUT;

    if (fe->mask & EVENT_ERR) {
        pfd->pfd->revents |= POLLERR;
    }
    if (fe->mask & EVENT_HUP) {
        pfd->pfd->revents |= POLLHUP;
    }
    if (fe->mask & EVENT_NVAL) {
        pfd->pfd->revents |= POLLNVAL;
    }

    pfd->fe->fiber_w = NULL;

    if (!(pfd->pfd->events & POLLIN)) {
        ring_detach(&pfd->me);
        pfd->fe = NULL;
    }

    if (pfd->pe->nready == 0) {
        if (pfd->pe->expire >= 0) {
            timer_cache_remove(ev->poll_list, pfd->pe->expire,
                &pfd->pe->me);
        }
        ring_prepend(&ev->poll_ready, &pfd->pe->me);
    }

    pfd->pe->nready++;
}

static void write_callback(EVENT *ev, FILE_EVENT *fe)
{
    POLLFD *pfd;
    //RING_ITER iter;
    RING *iter = fe->pfds.succ, *next = iter;

    event_del_write(ev, fe);
    SET_WRITABLE(fe);

#if 1
    for (; iter != &fe->pfds; iter = next) {
        next = next->succ;
        pfd = ring_to_appl(iter, POLLFD, me);
        if (pfd->pfd->events & POLLOUT) {
            handle_poll_write(ev, fe, pfd);
        }
    }
#else
    ring_foreach(iter, &fe->pfds) {
        pfd = ring_to_appl(iter.ptr, POLLFD, me);
        if (pfd->pfd->events & POLLOUT) {
            handle_poll_write(ev, fe, pfd);
            break;
        }
    }
#endif
}

/**
 * Set all fds' callbacks in POLL_EVENT, thease callbacks will be called
 * by event_wait() of different event engines for different OS platforms.
 */
static void poll_event_set(EVENT *ev, POLL_EVENT *pe, int timeout)
{
    int i;

    for (i = 0; i < pe->nfds; i++) {
        POLLFD *pfd = &pe->fds[i];

        if (pfd->pfd->events & POLLIN) {
#ifdef	HAS_IO_URING
            pfd->fe->mask |= EVENT_POLLIN;
            pfd->fe->r_timeout = timeout;
#endif
            event_add_read(ev, pfd->fe, read_callback);
            pfd->fe->fiber_r = acl_fiber_running();
            SET_READWAIT(pfd->fe);
        }
        if (pfd->pfd->events & POLLOUT) {
#ifdef	HAS_IO_URING
            pfd->fe->mask |= EVENT_POLLOUT;
            pfd->fe->w_timeout = timeout;
#endif
            event_add_write(ev, pfd->fe, write_callback);
            pfd->fe->fiber_w = acl_fiber_running();
            SET_WRITEWAIT(pfd->fe);
        }

        // Append the POLLFD to the FILE_EVENT's list.
        ring_prepend(&pfd->fe->pfds, &pfd->me);
        pfd->pfd->revents = 0;

        // Add one reference to avoid fe being freeed in advanced.
        //file_event_refer(pfd->fe);
    }

    if (timeout >= 0) {
        pe->expire = event_get_stamp(ev) + timeout;
        if (ev->timeout < 0 || timeout < ev->timeout) {
            ev->timeout = timeout;
        }
    } else {
        pe->expire = -1;
    }
}

static void poll_event_clean(EVENT *ev, POLL_EVENT *pe)
{
    int i;

    for (i = 0; i < pe->nfds; i++) {
        POLLFD *pfd = &pe->fds[i];

        /* Maybe has been cleaned in read_callback/write_callback. */
        if (pfd->fe == NULL) {
            continue;
        }

        CLR_POLLING(pfd->fe);

        if (pfd->pfd->events & POLLIN) {
            CLR_READWAIT(pfd->fe);
#ifdef	HAS_IO_URING
            pfd->fe->mask &= ~EVENT_POLLIN;
#endif
            event_del_read(ev, pfd->fe);
            pfd->fe->fiber_r = NULL;
        }
        if (pfd->pfd->events & POLLOUT) {
            CLR_WRITEWAIT(pfd->fe);
#ifdef	HAS_IO_URING
            pfd->fe->mask &= ~EVENT_POLLOUT;
#endif
            event_del_write(ev, pfd->fe);
            pfd->fe->fiber_w = NULL;
        }

        // Unrefer the fe because we don't need it again.
        //file_event_unrefer(pfd->fe);

        // Remove the POLLFD from the FILE_EVENT.
        ring_detach(&pfd->me);
        pfd->fe = NULL;
    }
}

/**
 * This callback will be called from event_process_poll() in event.c and the
 * fiber blocked after calling acl_fiber_switch() in acl_fiber_poll() will
 * wakeup and continue to run.
 */
static void poll_callback(EVENT *ev fiber_unused, POLL_EVENT *pe)
{
    if (pe->fiber->status != FIBER_STATUS_READY) {
        acl_fiber_ready(pe->fiber);
    }
}

static POLLFD *pollfd_alloc(POLL_EVENT *pe, struct pollfd *fds, nfds_t nfds)
{
    POLLFD *pfds = (POLLFD *) mem_malloc(nfds * sizeof(POLLFD));
    nfds_t  i;

    for (i = 0; i < nfds; i++) {
        if (fds[i].events & POLLIN) {
            pfds[i].fe = fiber_file_open_read(fds[i].fd);
        } else {
            pfds[i].fe = fiber_file_open_write(fds[i].fd);
        }
#ifdef HAS_IOCP
        pfds[i].fe->rbuf  = NULL;
        pfds[i].fe->rsize = 0;
#endif
        pfds[i].pe  = pe;
        pfds[i].pfd = &fds[i];
        pfds[i].pfd->revents = 0;
        ring_init(&pfds[i].me);
        SET_POLLING(pfds[i].fe);
    }

    return pfds;
}

static void pollfd_free(POLLFD *pfds)
{
    mem_free(pfds);
}

#ifdef SHARE_STACK

typedef struct pollfds {
    struct pollfd *fds;
    nfds_t nfds;
    size_t size;
} pollfds;

static void fiber_on_exit(void *ctx)
{
    pollfds *pfds = (pollfds *) ctx;

    mem_free(pfds->fds);
    mem_free(pfds);
}

static __thread int __local_key;

static pollfds *pollfds_save(const struct pollfd *fds, nfds_t nfds)
{
    pollfds *pfds = (pollfds *) acl_fiber_get_specific(__local_key);

    if (pfds == NULL) {
        pfds = (pollfds *) mem_malloc(sizeof(pollfds));
        pfds->size = nfds + 1;
        pfds->fds  = mem_malloc(sizeof(struct pollfds) * pfds->size);
        acl_fiber_set_specific(&__local_key, pfds, fiber_on_exit);
    } else if (pfds->size < (size_t) nfds) {
        mem_free(pfds->fds);
        pfds->size = nfds + 1;
        pfds->fds  = mem_malloc(sizeof(struct pollfd) * pfds->size);
    }

    pfds->nfds = nfds;
    memcpy(pfds->fds, fds, sizeof(struct pollfd) * nfds);
    return pfds;
}

static void pollfds_copy(struct pollfd *fds, const pollfds *pfds)
{
    memcpy(fds, pfds->fds, sizeof(struct pollfd) * pfds->nfds);
}

#endif // SHARE_STACK

int WINAPI acl_fiber_poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
    long long now;
    EVENT *ev;
    int old_timeout, nready;
    ACL_FIBER *curr;

#ifdef SHARE_STACK
    // In shared stack mode, we should use heap memory for pe to hold
    // all the fds, because the pe will be operated by the event fiber,
    // but the shared stack can be used by only one fiber, the stack
    // memory of the current fiber will be occupied by the other fiber
    // after switching the other fiber. So, we use heap memory to hold
    // pe to avoid stack memory collision.
    pollfds    *pfds;
    POLL_EVENT  pevent, *pe;
#else
    POLL_EVENT  pevent, *pe;
#endif

    if (sys_poll == NULL) {
        hook_once();
    }

    if (!var_hook_sys_api) {
        return sys_poll ? (*sys_poll)(fds, nfds, timeout) : -1;
    }

    curr        = acl_fiber_running();
    ev          = fiber_io_event();
    old_timeout = ev->timeout;

#ifdef SHARE_STACK
    if (curr->oflag & ACL_FIBER_ATTR_SHARE_STACK) {
        pfds    = pollfds_save(fds, nfds);
        pe      = (POLL_EVENT *) mem_malloc(sizeof(POLL_EVENT));
        pe->fds = pollfd_alloc(pe, pfds->fds, nfds);
    } else {
        pfds    = NULL;
        pe      = &pevent;
        pe->fds = pollfd_alloc(pe, fds, nfds);
    }
#else
    pe        = &pevent;
    pe->fds   = pollfd_alloc(pe, fds, nfds);
#endif

    pe->nfds  = nfds;
    pe->fiber = curr;
    pe->proc  = poll_callback;

    poll_event_set(ev, pe, timeout);

    while (1) {
        /* The cache timer should be set when timeout >= 0. */
        if (pe->expire >= 0) {
            timer_cache_add(ev->poll_list, pe->expire, &pe->me);
        }

        pe->nready = 0;
        pe->fiber->wstatus |= FIBER_WAIT_POLL;

        WAITER_INC(ev);
        acl_fiber_switch();
        WAITER_DEC(ev);

        pe->fiber->wstatus &= ~FIBER_WAIT_POLL;

        if (pe->nready == 0 && pe->expire >= 0) {
            timer_cache_remove(ev->poll_list, pe->expire, &pe->me);
        }

        ev->timeout = old_timeout;

        if (acl_fiber_killed(pe->fiber)) {
            acl_fiber_set_error(pe->fiber->errnum);
            if (pe->nready == 0) {
                pe->nready = -1;
            }
            break;
        }

        if (timer_cache_size(ev->poll_list) == 0) {
            ev->timeout = -1;
        }

        if (pe->nready != 0 || timeout == 0) {
            break;
        }

        now = event_get_stamp(ev);
        if (pe->expire > 0 && now >= pe->expire) {
            acl_fiber_set_error(FIBER_ETIME);
            break;
        }
    }

    poll_event_clean(ev, pe);
    pollfd_free(pe->fds);

    nready = pe->nready;

#ifdef SHARE_STACK
    if (curr->oflag & ACL_FIBER_ATTR_SHARE_STACK) {
        mem_free(pe);
        pollfds_copy(fds, pfds);
    }
#endif

    return nready;
}

#ifdef SYS_UNIX
int poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
    return acl_fiber_poll(fds, nfds, timeout);
}
#endif

#endif // HAS_POLL
