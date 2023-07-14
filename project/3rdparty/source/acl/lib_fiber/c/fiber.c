#include "stdafx.h"
#include "acl/lib_fiber/c/libfiber.h"
#include "common.h"

#ifdef USE_VALGRIND
#include <valgrind/valgrind.h>
#endif

//#define FIBER_STACK_GUARD
#ifdef	FIBER_STACK_GUARD
#include <sys/mman.h>
#endif

#include "fiber.h"

#define	MAX_CACHE	1000

#ifdef	HOOK_ERRNO
typedef int  *(*errno_fn)(void);

static errno_fn __sys_errno     = NULL;
#endif

typedef struct THREAD {
    RING       ready;       /* ready fiber queue */
    RING       dead;        /* dead fiber queue */
    ACL_FIBER **fibers;
    unsigned   size;
    unsigned   slot;
    int        exitcode;
    ACL_FIBER *running;
    ACL_FIBER *original;
    int        errnum;
    size_t     switched;
    size_t     switched_old;
    int        nlocal;

#ifdef	SHARE_STACK
# ifdef	USE_VALGRIND
    unsigned int vid;
# endif
    char      *stack_buff;
    size_t     stack_size;
    size_t     stack_dlen;
#endif
} THREAD;

static ATOMIC   *__idgen_atomic = NULL;
static long long __idgen_value  = 0;

static THREAD          *__main_fiber   = NULL;
static __thread THREAD *__thread_fiber = NULL;
static __thread int __scheduled = 0;
static __thread int __schedule_auto = 0;
__thread int var_hook_sys_api   = 0;

#ifdef	SHARE_STACK
static size_t __shared_stack_size = 10240000;
#endif

static pthread_key_t __fiber_key;

int acl_fiber_scheduled(void)
{
    return __scheduled;
}

static void thread_free(void *ctx)
{
    THREAD *tf = (THREAD *) ctx;
    RING *head;
    ACL_FIBER *fiber;
    unsigned int i;

    if (__thread_fiber == NULL) {
        return;
    }

    /* Free fiber object in the dead fibers link */
    while ((head = ring_pop_head(&__thread_fiber->dead))) {
        fiber = RING_TO_APPL(head, ACL_FIBER, me);
        fiber_free(fiber);
    }

    /* Free all possible aliving fiber object */
    for (i = 0; i < __thread_fiber->slot; i++) {
        fiber_free(__thread_fiber->fibers[i]);
    }

    if (tf->fibers) {
        mem_free(tf->fibers);
    }

#ifdef	SHARE_STACK
# ifdef	USE_VALGRIND
    VALGRIND_STACK_DEREGISTER(tf->vid);
# endif
    mem_free(tf->stack_buff);
#endif

    fiber_real_free(tf->original);
    mem_free(tf);

    if (__main_fiber == __thread_fiber) {
        __main_fiber = NULL;
    }

    __thread_fiber = NULL;
}

static void fiber_schedule_main_free(void)
{
    if (__main_fiber) {
        thread_free(__main_fiber);
        if (__thread_fiber == __main_fiber) {
            __thread_fiber = NULL;
        }
        __main_fiber = NULL;
    }
}

static void free_atomic_onexit(void)
{
    if (__idgen_atomic != NULL) {
        atomic_free(__idgen_atomic);
        __idgen_atomic = NULL;
    }
}

static void thread_init(void)
{
    if (pthread_key_create(&__fiber_key, thread_free) != 0) {
        msg_fatal("%s(%d), %s: pthread_key_create error %s",
            __FILE__, __LINE__, __FUNCTION__, last_serror());
    }

    __idgen_atomic = atomic_new();
    atomic_set(__idgen_atomic, &__idgen_value);
    atomic_int64_set(__idgen_atomic, 0);
    atexit(free_atomic_onexit);
}

static pthread_once_t __once_control = PTHREAD_ONCE_INIT;

static void fiber_check(void)
{
    if (__thread_fiber != NULL) {
        return;
    }

    if (pthread_once(&__once_control, thread_init) != 0) {
        printf("%s(%d), %s: pthread_once error %s\r\n",
            __FILE__, __LINE__, __FUNCTION__, last_serror());
        abort();
    }

    __thread_fiber = (THREAD *) mem_calloc(1, sizeof(THREAD));

    __thread_fiber->original = fiber_real_origin();
    __thread_fiber->fibers   = NULL;
    __thread_fiber->size     = 0;
    __thread_fiber->slot     = 0;
    __thread_fiber->nlocal   = 0;

#ifdef	SHARE_STACK
    __thread_fiber->stack_size = __shared_stack_size;
    __thread_fiber->stack_buff = mem_malloc(__thread_fiber->stack_size);
# ifdef	USE_VALGRIND
    __thread_fiber->vid = VALGRIND_STACK_REGISTER(__thread_fiber->stack_buff,
            __thread_fiber->stack_buff + __shared_stack_size);
# endif
    __thread_fiber->stack_dlen = 0;
#endif

    ring_init(&__thread_fiber->ready);
    ring_init(&__thread_fiber->dead);

    if (thread_self() == main_thread_self()) {
        __main_fiber = __thread_fiber;
        atexit(fiber_schedule_main_free);
    } else if (pthread_setspecific(__fiber_key, __thread_fiber) != 0) {
        printf("pthread_setspecific error!\r\n");
        abort();
    }
}

ACL_FIBER *fiber_origin(void)
{
    return __thread_fiber->original;
}

#ifdef	SHARE_STACK

char *fiber_share_stack_addr(void)
{
    return __thread_fiber->stack_buff;
}

char *fiber_share_stack_bottom(void)
{
    return __thread_fiber->stack_buff + __thread_fiber->stack_size;
}

size_t fiber_share_stack_size(void)
{
    return __thread_fiber->stack_size;
}

size_t fiber_share_stack_dlen(void)
{
    return __thread_fiber->stack_dlen;
}

void fiber_share_stack_set_dlen(size_t dlen)
{
    __thread_fiber->stack_dlen = dlen;
}

#endif

#ifdef	HOOK_ERRNO

static void fiber_init(void)
{
#ifdef SYS_UNIX

    static pthread_mutex_t __lock = PTHREAD_MUTEX_INITIALIZER;
    static int __called = 0;

    (void) pthread_mutex_lock(&__lock);

    if (__called != 0) {
        (void) pthread_mutex_unlock(&__lock);
        return;
    }

    __called++;

#ifdef ANDROID
    __sys_errno   = (errno_fn) dlsym(RTLD_NEXT, "__errno");
#else
    __sys_errno   = (errno_fn) dlsym(RTLD_NEXT, "__errno_location");
#endif

    (void) pthread_mutex_unlock(&__lock);
#endif
}

/* See /usr/include/bits/errno.h for __errno_location */
#ifdef ANDROID
volatile int*   __errno(void)
#else
int *__errno_location(void)
#endif
{
    if (!var_hook_sys_api) {
        if (__sys_errno == NULL) {
            fiber_init();
        }

        return __sys_errno();
    }

    if (__thread_fiber == NULL) {
        fiber_check();
    }

    if (__thread_fiber->running) {
        return &__thread_fiber->running->errnum;
    } else {
        return &__thread_fiber->original.errnum;
    }
}

#endif

void acl_fiber_set_errno(ACL_FIBER *fiber, int errnum)
{
    if (fiber == NULL) {
        fiber = acl_fiber_running();
    }
    if (fiber) {
        fiber->errnum = errnum;
    }
}

int acl_fiber_errno(ACL_FIBER *fiber)
{
    if (fiber == NULL) {
        fiber = acl_fiber_running();
    }
    return fiber ? fiber->errnum : 0;
}

void acl_fiber_keep_errno(ACL_FIBER *fiber, int yesno)
{
    if (fiber == NULL) {
        fiber = acl_fiber_running();
    }
    if (fiber) {
        if (yesno) {
            fiber->flag |= FIBER_F_SAVE_ERRNO;
        } else {
            fiber->flag &= ~FIBER_F_SAVE_ERRNO;
        }
    }
}

void fiber_save_errno(int errnum)
{
    ACL_FIBER *curr;

    if (__thread_fiber == NULL) {
        fiber_check();
    }

    if ((curr = __thread_fiber->running) == NULL) {
        curr = __thread_fiber->original;
    }

    if (curr->flag & FIBER_F_SAVE_ERRNO) {
        //curr->flag &= ~FIBER_F_SAVE_ERRNO;
        return;
    }

    acl_fiber_set_errno(curr, errnum);
}

static void fiber_kick(int max)
{
    RING *head;
    ACL_FIBER *fiber;

    while (max > 0) {
        head = ring_pop_head(&__thread_fiber->dead);
        if (head == NULL) {
            break;
        }
        fiber = RING_TO_APPL(head, ACL_FIBER, me);
        fiber_free(fiber);
        max--;
    }
}

static void fiber_swap(ACL_FIBER *from, ACL_FIBER *to)
{
    if (from->status == FIBER_STATUS_EXITING) {
        size_t slot = from->slot;
        int n = ring_size(&__thread_fiber->dead);

        /* If the cached dead fibers reached the limit,
        * some will be freed
        */
        if (n > MAX_CACHE) {
            n -= MAX_CACHE;
            fiber_kick(n);
        }

        __thread_fiber->fibers[slot] =
            __thread_fiber->fibers[--__thread_fiber->slot];
        __thread_fiber->fibers[slot]->slot = (unsigned) slot;

        ring_prepend(&__thread_fiber->dead, &from->me);
    } else {
        from->status = FIBER_STATUS_SUSPEND;
    }

    if (to->status != FIBER_STATUS_EXITING) {
        to->status = FIBER_STATUS_RUNNING;
    }

    fiber_real_swap(from, to);
}

static void check_timer(ACL_FIBER *fiber fiber_unused, void *ctx)
{
    size_t *intptr = (size_t *) ctx;
    size_t  max = *intptr;

    mem_free(intptr);
    while (1) {
#ifdef SYS_WIN
        Sleep(1000);
#else
        sleep(1);
#endif
        fiber_kick((int) max);
    }
}

void acl_fiber_check_timer(size_t max)
{
    size_t *intptr = (size_t *) mem_malloc(sizeof(int));

    *intptr = max;
    acl_fiber_create(check_timer, intptr, 64000);
}

ACL_FIBER *acl_fiber_running(void)
{
    fiber_check();
    return __thread_fiber->running;
}

int acl_fiber_killed(ACL_FIBER *fiber)
{
    if (!fiber) {
        fiber = acl_fiber_running();
    }
    return fiber && (fiber->flag & FIBER_F_KILLED);
}

int acl_fiber_signaled(ACL_FIBER *fiber)
{
    if (!fiber) {
        fiber = acl_fiber_running();
    }
    return fiber && (fiber->flag & FIBER_F_SIGNALED);
}

int acl_fiber_closed(ACL_FIBER *fiber)
{
    if (!fiber) {
        fiber = acl_fiber_running();
    }
    return fiber && (fiber->flag & FIBER_F_CLOSED);
}

int acl_fiber_canceled(ACL_FIBER *fiber)
{
    if (!fiber) {
        fiber = acl_fiber_running();
    }
    return fiber && (fiber->flag & FIBER_F_CANCELED);
}

void acl_fiber_clear(ACL_FIBER *fiber)
{
    if (fiber) {
        fiber->errnum = 0;
        fiber->flag &= ~FIBER_F_CANCELED;
    }
}

void acl_fiber_kill(ACL_FIBER *fiber)
{
    fiber->errnum = ECANCELED;
    acl_fiber_signal(fiber, SIGTERM);
}

void acl_fiber_signal(ACL_FIBER *fiber, int signum)
{
    ACL_FIBER *curr = __thread_fiber->running;

    if (fiber == NULL) {
        msg_error("%s(%d), %s: fiber NULL",
            __FILE__, __LINE__, __FUNCTION__);
        return;
    }

    if (curr == NULL) {
        msg_error("%s(%d), %s: current fiber NULL",
            __FILE__, __LINE__, __FUNCTION__);
        return;
    }

#ifdef SYS_WIN
    if (signum == SIGTERM) {
#else
    if (signum == SIGKILL || signum == SIGTERM || signum == SIGQUIT) {
#endif
        fiber->flag |= FIBER_F_KILLED | FIBER_F_SIGNALED;
    } else {
        fiber->flag |= FIBER_F_SIGNALED;
    }

    fiber->signum = signum;

    // Just only wakeup the suspended fiber.
    if (fiber->status == FIBER_STATUS_SUSPEND) {
        ring_detach(&fiber->me); // This is safety!
        
        acl_fiber_ready(fiber);
    }
}

int acl_fiber_signum(ACL_FIBER *fiber)
{
    if (fiber) {
        fiber = acl_fiber_running();
    }
    return fiber ? fiber->signum : 0;
}

void fiber_exit(int exit_code)
{
    fiber_check();

    __thread_fiber->exitcode = exit_code;
    __thread_fiber->running->status = FIBER_STATUS_EXITING;

    acl_fiber_switch();
}

void acl_fiber_ready(ACL_FIBER *fiber)
{
    if (fiber->status != FIBER_STATUS_EXITING) {
        if (fiber->status == FIBER_STATUS_READY) {
            ring_detach(&fiber->me);
        }

        fiber->status = FIBER_STATUS_READY;
        assert(__thread_fiber);
        ring_prepend(&__thread_fiber->ready, &fiber->me);
    }
}

int acl_fiber_yield(void)
{
    if (ring_size(&__thread_fiber->ready) == 0) {
        return 0;
    }

    __thread_fiber->switched_old = __thread_fiber->switched;

    // Reset the current fiber's status in order to be added to
    // ready queue again.
    __thread_fiber->running->status = FIBER_STATUS_NONE;
    acl_fiber_ready(__thread_fiber->running);
    acl_fiber_switch();

    return 1;
}

unsigned acl_fiber_ndead(void)
{
    if (__thread_fiber == NULL) {
        return 0;
    }
    return (unsigned) ring_size(&__thread_fiber->dead);
}

unsigned acl_fiber_number(void)
{
    if (__thread_fiber == NULL) {
        return 0;
    }
    return __thread_fiber->slot;
}

static void fbase_init(FIBER_BASE *fbase, int flag)
{
    fbase->flag      = flag;
    fbase->event_in  = -1;
    fbase->event_out = -1;
    ring_init(&fbase->event_waiter);
}

FIBER_BASE *fbase_alloc(unsigned flag)
{
    FIBER_BASE *fbase = (FIBER_BASE *) mem_calloc(1, sizeof(FIBER_BASE));

    fbase_init(fbase, flag);
    return fbase;
}

void fbase_free(FIBER_BASE *fbase)
{
    fbase_event_close(fbase); // Not closed? try again!
    mem_free(fbase);
}

void fiber_free(ACL_FIBER *fiber)
{
    if (fiber->base) {
        fbase_event_close(fiber->base);
    }
    fiber_real_free(fiber);
}

void fiber_start(ACL_FIBER *fiber, void (*fn)(ACL_FIBER *, void *), void *arg)
{
    int i;

    fn(fiber, arg);

    for (i = 0; i < fiber->nlocal; i++) {
        if (fiber->locals[i] == NULL) {
            continue;
        }
        if (fiber->locals[i]->free_fn) {
            fiber->locals[i]->free_fn(fiber->locals[i]->ctx);
        }
        mem_free(fiber->locals[i]);
    }

    if (fiber->locals) {
        mem_free(fiber->locals);
        fiber->locals = NULL;
        fiber->nlocal = 0;
    }

    fiber_exit(0);
}

ACL_FIBER *acl_fiber_alloc(size_t size, void **pptr)
{
    ACL_FIBER *fiber = (ACL_FIBER *) mem_calloc(1, sizeof(ACL_FIBER) + size);
    *pptr = ((char*) fiber) + sizeof(ACL_FIBER);
    return fiber;
}

static ACL_FIBER *fiber_alloc(void (*fn)(ACL_FIBER *, void *),
    void *arg, const ACL_FIBER_ATTR *attr)
{
    ACL_FIBER *fiber = NULL;
    RING *head;
    unsigned long id;

    fiber_check();

#define	APPL	RING_TO_APPL

    /* Try to reuse the fiber memory in dead queue */
    head = ring_pop_head(&__thread_fiber->dead);
    if (head == NULL) {
        fiber = fiber_real_alloc(attr);
        fiber->tid = thread_self();
    } else {
        fiber = APPL(head, ACL_FIBER, me);
    }

    id = (unsigned long) atomic_int64_add_fetch(__idgen_atomic, 1);
    if (id <= 0) {  /* Overflow ? */
        atomic_int64_set(__idgen_atomic, 0);
        id = (unsigned long) atomic_int64_add_fetch(__idgen_atomic, 1);
    }

    fiber->fid     = id;
    fiber->errnum  = 0;
    fiber->signum  = 0;
    fiber->oflag   = attr ? attr->oflag : 0;
    fiber->flag    = 0;
    fiber->status  = FIBER_STATUS_NONE;
    fiber->wstatus = FIBER_WAIT_NONE;

#ifdef	DEBUG_LOCK
    fiber->waiting = NULL;
    ring_init(&fiber->holding);
#endif
    fiber_real_init(fiber, attr ? attr->stack_size : 128000, fn, arg);

    return fiber;
}

void acl_fiber_schedule_init(int on)
{
    __schedule_auto = on;
}

void acl_fiber_attr_init(ACL_FIBER_ATTR *attr)
{
    attr->oflag = 0;
    attr->stack_size = 128000;
}

void acl_fiber_attr_setstacksize(ACL_FIBER_ATTR *attr, size_t size)
{
    attr->stack_size = size;
}

void acl_fiber_attr_setsharestack(ACL_FIBER_ATTR *attr, int on)
{
    if (on) {
        attr->oflag |= ACL_FIBER_ATTR_SHARE_STACK;
    } else {
        attr->oflag &= ~ACL_FIBER_ATTR_SHARE_STACK;
    }
}

ACL_FIBER *acl_fiber_create(void (*fn)(ACL_FIBER *, void *),
    void *arg, size_t size)
{
    ACL_FIBER_ATTR attr;

    acl_fiber_attr_init(&attr);
    attr.stack_size = size;
    return acl_fiber_create2(&attr, fn, arg);
}

ACL_FIBER *acl_fiber_create2(const ACL_FIBER_ATTR *attr,
    void (*fn)(ACL_FIBER *, void *), void *arg)
{
    ACL_FIBER *fiber = fiber_alloc(fn, arg, attr);

    if (__thread_fiber->slot >= __thread_fiber->size) {
        __thread_fiber->size  += 128;
        __thread_fiber->fibers = (ACL_FIBER **) mem_realloc(
            __thread_fiber->fibers, 
            __thread_fiber->size * sizeof(ACL_FIBER *));
    }

    fiber->slot = __thread_fiber->slot;
    __thread_fiber->fibers[__thread_fiber->slot++] = fiber;

    acl_fiber_ready(fiber);
    if (__schedule_auto && !acl_fiber_scheduled()) {
        acl_fiber_schedule();
    }
    return fiber;
}

int acl_fiber_use_share_stack(const ACL_FIBER *fiber)
{
    return fiber->oflag & ACL_FIBER_ATTR_SHARE_STACK ? 1 : 0;
}

unsigned int acl_fiber_id(const ACL_FIBER *fiber)
{
    return fiber ? fiber->fid : 0;
}

unsigned int acl_fiber_self(void)
{
    ACL_FIBER *curr = acl_fiber_running();
    return acl_fiber_id(curr);
}

int acl_fiber_status(const ACL_FIBER *fiber)
{
    if (fiber == NULL) {
        fiber = acl_fiber_running();
    }
    return fiber ? fiber->status : 0;
}

void acl_fiber_set_shared_stack_size(size_t size)
{
    if (size >= 1024) {
#ifdef	SHARE_STACK
        __shared_stack_size = size;
#endif
    }
}

size_t acl_fiber_get_shared_stack_size(void)
{
#if	defined(SHARE_STACK)
    return __shared_stack_size;
#else
    return 0;
#endif
}

static void fiber_hook_api(int on)
{
    var_hook_sys_api = on;
}

void acl_fiber_schedule_set_event(int event_mode)
{
    event_set(event_mode);
}

void acl_fiber_schedule_with(int event_mode)
{
    acl_fiber_schedule_set_event(event_mode);
    acl_fiber_schedule();
}

void acl_fiber_schedule(void)
{
    ACL_FIBER *fiber;
    RING *head;

    if (__scheduled) {
        return;
    }

#if defined(USE_FAST_TIME)
    set_time_metric(1000);
#endif

    fiber_check();
    fiber_hook_api(1);
    __scheduled = 1;

    for (;;) {
        head = ring_pop_head(&__thread_fiber->ready);
        if (head == NULL) {
            msg_info("thread-%lu: NO FIBER NOW", thread_self());
            break;
        }

        fiber = RING_TO_APPL(head, ACL_FIBER, me);
        fiber->status = FIBER_STATUS_READY;

        __thread_fiber->running = fiber;
        __thread_fiber->switched++;

        fiber_swap(__thread_fiber->original, fiber);
        __thread_fiber->running = NULL;
    }

    /* Release dead fiber */
    while ((head = ring_pop_head(&__thread_fiber->dead)) != NULL) {
        fiber = RING_TO_APPL(head, ACL_FIBER, me);
        fiber_free(fiber);
    }

    fiber_io_clear();
    fiber_hook_api(0);
    __scheduled = 0;
}

void acl_fiber_switch(void)
{
    ACL_FIBER *fiber, *current = __thread_fiber->running;
    RING *head;

#ifdef _DEBUG
    assert(current);
#endif

    head = ring_pop_head(&__thread_fiber->ready);
    if (head == NULL) {
        msg_info("thread-%lu: NO FIBER in ready", thread_self());
        fiber_swap(current, __thread_fiber->original);
        return;
    }

    fiber = RING_TO_APPL(head, ACL_FIBER, me);
    //fiber->status = FIBER_STATUS_READY;

    __thread_fiber->running = fiber;
    __thread_fiber->switched++;

    fiber_swap(current, __thread_fiber->running);
}

int acl_fiber_set_specific(int *key, void *ctx, void (*free_fn)(void *))
{
    FIBER_LOCAL *local;
    ACL_FIBER *curr;

    if (key == NULL) {
        msg_error("%s(%d), %s: key NULL",
            __FILE__, __LINE__, __FUNCTION__);
        return -1;
    }

    if (__thread_fiber == NULL) {
        msg_error("%s(%d), %s: __thread_fiber: NULL",
            __FILE__, __LINE__, __FUNCTION__);
        return -1;
    } else if (__thread_fiber->running == NULL) {
        msg_error("%s(%d), %s: running: NULL",
            __FILE__, __LINE__, __FUNCTION__);
        return -1;
    } else {
        curr = __thread_fiber->running;
    }

    if (*key <= 0) {
        *key = ++__thread_fiber->nlocal;
    } else if (*key > __thread_fiber->nlocal) {
        msg_error("%s(%d), %s: invalid key: %d > nlocal: %d",
            __FILE__, __LINE__, __FUNCTION__,
            *key, __thread_fiber->nlocal);
        return -1;
    }

    if (curr->nlocal < __thread_fiber->nlocal) {
        int i, n = curr->nlocal;
        curr->nlocal = __thread_fiber->nlocal;
        curr->locals = (FIBER_LOCAL **) mem_realloc(curr->locals,
            curr->nlocal * sizeof(FIBER_LOCAL*));
        for (i = n; i < curr->nlocal; i++) {
            curr->locals[i] = NULL;
        }
    }

    local = (FIBER_LOCAL *) mem_calloc(1, sizeof(FIBER_LOCAL));
    local->ctx = ctx;
    local->free_fn = free_fn;
    curr->locals[*key - 1] = local;

    return *key;
}

void *acl_fiber_get_specific(int key)
{
    FIBER_LOCAL *local;
    ACL_FIBER *curr;

    if (key <= 0) {
        return NULL;
    }

    if (__thread_fiber == NULL) {
        msg_error("%s(%d), %s: __thread_fiber NULL",
            __FILE__, __LINE__, __FUNCTION__);
        return NULL;
    } else if (__thread_fiber->running == NULL) {
        msg_error("%s(%d), %s: running fiber NULL",
            __FILE__, __LINE__, __FUNCTION__);
        return NULL;
    } else {
        curr = __thread_fiber->running;
    }

    if (key > curr->nlocal) {
        return NULL;
    }

    local = curr->locals[key - 1];

    return local ? local->ctx : NULL;
}

int acl_fiber_last_error(void)
{
#ifdef	SYS_WIN
    int   error;

    error = WSAGetLastError();
    WSASetLastError(error);
    return error;
#else
    return errno;
#endif
}

void acl_fiber_set_error(int errnum)
{
#ifdef	SYS_WIN
    WSASetLastError(errnum);
#endif
    errno = errnum;
}

void acl_fiber_memstat(void)
{
    mem_stat();
}