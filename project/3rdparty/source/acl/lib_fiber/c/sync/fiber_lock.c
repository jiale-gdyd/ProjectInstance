#include "../stdafx.h"
#include "../common.h"

#include "acl/lib_fiber/c/libfiber.h"
#include "../fiber.h"

struct ACL_FIBER_LOCK {
    RING       me;
    ACL_FIBER *owner;
    RING       waiting;
};

struct ACL_FIBER_RWLOCK {
    int        readers;
    ACL_FIBER *writer;
    RING       rwaiting;
    RING       wwaiting;
};

ACL_FIBER_LOCK *acl_fiber_lock_create(void)
{
    ACL_FIBER_LOCK *lk = (ACL_FIBER_LOCK *) mem_malloc(sizeof(ACL_FIBER_LOCK));

    lk->owner = NULL;
    ring_init(&lk->me);
    ring_init(&lk->waiting);
    return lk;
}

void acl_fiber_lock_free(ACL_FIBER_LOCK *lk)
{
    mem_free(lk);
}

static int __lock(ACL_FIBER_LOCK *lk, int block)
{
    ACL_FIBER *curr = acl_fiber_running();
    EVENT *ev;

    if (lk->owner == NULL) {
        lk->owner = acl_fiber_running();
#ifdef	DEBUG_LOCK
        ring_prepend(&curr->holding, &lk->me);
#endif
        return 0;
    }

    // xxx: no support recursion lock
    assert(lk->owner != curr);

    if (!block) {
        return -1;
    }

    ring_prepend(&lk->waiting, &curr->me);

#ifdef	DEBUG_LOCK
    curr->waiting = lk;
#endif

    curr->wstatus |= FIBER_WAIT_LOCK;

    ev = fiber_io_event();
    WAITER_INC(ev);  // Just for avoiding fiber_io_loop to exit
    acl_fiber_switch();
    WAITER_DEC(ev);

    curr->wstatus &= ~FIBER_WAIT_LOCK;

    /* if switch to me because other killed me, I should detach myself;
    * else if because other unlock, I'll be detached twice which is
    * hamless because RING can deal with it.
    */
    ring_detach(&curr->me);

    if (lk->owner == curr) {
        return 0;
    }

    if (acl_fiber_canceled(curr)) {
        acl_fiber_set_error(curr->errnum);
    }

    return -1;
}

int acl_fiber_lock_lock(ACL_FIBER_LOCK *lk)
{
    return __lock(lk, 1);
}

int acl_fiber_lock_trylock(ACL_FIBER_LOCK *lk)
{
    return __lock(lk, 0) ? 0 : -1;
}

#define RING_TO_FIBER(r) \
    ((ACL_FIBER *) ((char *) (r) - offsetof(ACL_FIBER, me)))

#define FIRST_FIBER(head) \
    (ring_succ(head) != (head) ? RING_TO_FIBER(ring_succ(head)) : 0)

void acl_fiber_lock_unlock(ACL_FIBER_LOCK *lk)
{
    ACL_FIBER *ready, *curr = acl_fiber_running();
    
    if (lk->owner == NULL) {
        msg_fatal("%s(%d), %s: qunlock: owner NULL",
            __FILE__, __LINE__, __FUNCTION__);
    }
    if (lk->owner != curr) {
        msg_fatal("%s(%d), %s: invalid owner=%p, %p",
            __FILE__, __LINE__, __FUNCTION__, lk->owner, curr);
    }

    ring_detach(&lk->me);
    ready = FIRST_FIBER(&lk->waiting);

    if ((lk->owner = ready) != NULL) {
        ring_detach(&ready->me);
        acl_fiber_ready(ready);
        acl_fiber_yield();
    }
}

/*--------------------------------------------------------------------------*/

ACL_FIBER_RWLOCK *acl_fiber_rwlock_create(void)
{
    ACL_FIBER_RWLOCK *lk = (ACL_FIBER_RWLOCK *)
        mem_malloc(sizeof(ACL_FIBER_RWLOCK));

    lk->readers = 0;
    lk->writer  = NULL;
    ring_init(&lk->rwaiting);
    ring_init(&lk->wwaiting);

    return lk;
}

void acl_fiber_rwlock_free(ACL_FIBER_RWLOCK *lk)
{
    mem_free(lk);
}

static int __rlock(ACL_FIBER_RWLOCK *lk, int block)
{
    ACL_FIBER *curr;
    EVENT *ev;

    if (lk->writer == NULL && FIRST_FIBER(&lk->wwaiting) == NULL) {
        lk->readers++;
        return 0;
    }

    if (!block) {
        return -1;
    }

    curr = acl_fiber_running();
    ring_prepend(&lk->rwaiting, &curr->me);

    curr->wstatus |= FIBER_WAIT_LOCK;

    ev = fiber_io_event();
    WAITER_INC(ev);  // Just for avoiding fiber_io_loop to exit
    acl_fiber_switch();
    WAITER_DEC(ev);

    curr->wstatus &= ~FIBER_WAIT_LOCK;

    /* if switch to me because other killed me, I should detach myself */
    ring_detach(&curr->me);

    if (acl_fiber_canceled(curr)) {
        acl_fiber_set_error(curr->errnum);
        return -1;
    }
    return 0;
}

int acl_fiber_rwlock_rlock(ACL_FIBER_RWLOCK *lk)
{
    return __rlock(lk, 1);
}

int acl_fiber_rwlock_tryrlock(ACL_FIBER_RWLOCK *lk)
{
    return __rlock(lk, 0);
}

static int __wlock(ACL_FIBER_RWLOCK *lk, int block)
{
    ACL_FIBER *curr;
    EVENT *ev;

    if (lk->writer == NULL && lk->readers == 0) {
        lk->writer = acl_fiber_running();
        return 0;
    }

    if (!block) {
        return -1;
    }

    curr = acl_fiber_running();
    ring_prepend(&lk->wwaiting, &curr->me);

    curr->wstatus |= FIBER_WAIT_LOCK;

    ev = fiber_io_event();
    WAITER_INC(ev);  // Just for avoiding fiber_io_loop to exit
    acl_fiber_switch();
    WAITER_DEC(ev);

    curr->wstatus &= ~FIBER_WAIT_LOCK;

    /* if switch to me because other killed me, I should detach myself */
    ring_detach(&curr->me);

    if (acl_fiber_canceled(curr)) {
        acl_fiber_set_error(curr->errnum);
        return -1;
    }
    return 0;
}

int acl_fiber_rwlock_wlock(ACL_FIBER_RWLOCK *lk)
{
    return __wlock(lk, 1);
}

int acl_fiber_rwlock_trywlock(ACL_FIBER_RWLOCK *lk)
{
    return __wlock(lk, 0);
}

void acl_fiber_rwlock_runlock(ACL_FIBER_RWLOCK *lk)
{
    ACL_FIBER *fiber;

    if (--lk->readers == 0 && (fiber = FIRST_FIBER(&lk->wwaiting))) {
        ring_detach(&lk->wwaiting);
        lk->writer = fiber;
        acl_fiber_ready(fiber);
        acl_fiber_yield();
    }
}

void acl_fiber_rwlock_wunlock(ACL_FIBER_RWLOCK *lk)
{
    ACL_FIBER *fiber;
    size_t n = 0;
    
    if (lk->writer == NULL) {
        msg_fatal("%s(%d), %s: wunlock: not locked",
            __FILE__, __LINE__, __FUNCTION__);
    }

    lk->writer = NULL;

    if (lk->readers != 0) {
        msg_fatal("%s(%d), %s: wunlock: readers",
            __FILE__, __LINE__, __FUNCTION__);
    }

    while ((fiber = FIRST_FIBER(&lk->rwaiting)) != NULL) {
        ring_detach(&lk->rwaiting);
        lk->readers++;
        acl_fiber_ready(fiber);
        n++;
    }

    if (lk->readers == 0 && (fiber = FIRST_FIBER(&lk->wwaiting)) != NULL) {
        ring_detach(&lk->wwaiting);
        lk->writer = fiber;
        acl_fiber_ready(fiber);
        n++;
    }

    if (n > 0) {
        acl_fiber_yield();
    }
}