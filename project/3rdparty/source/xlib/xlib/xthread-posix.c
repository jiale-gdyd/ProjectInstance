#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <errno.h>
#include <sched.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/syscall.h>

#include <xlib/xlib/config.h>

#include <xlib/xlib/xmain.h>
#include <xlib/xlib/xutils.h>
#include <xlib/xlib/xslice.h>
#include <xlib/xlib/xthread.h>
#include <xlib/xlib/xmessages.h>
#include <xlib/xlib/xstrfuncs.h>
#include <xlib/xlib/xtestutils.h>
#include <xlib/xlib/xthreadprivate.h>

#define USE_NATIVE_MUTEX

static void x_thread_abort(xint status, const xchar *function)
{
    fprintf(stderr, "XLib (xthread-posix.c): Unexpected error from C library during '%s': %s.  Aborting.\n", function, strerror(status));
    x_abort();
}

#if !defined(USE_NATIVE_MUTEX)

static pthread_mutex_t *x_mutex_impl_new(void)
{
    xint status;
    pthread_mutex_t *mutex;
    pthread_mutexattr_t *pattr = NULL;
#ifdef PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP
    pthread_mutexattr_t attr;
#endif

    mutex = malloc(sizeof(pthread_mutex_t));
    if X_UNLIKELY(mutex == NULL) {
        x_thread_abort(errno, "malloc");
    }

#ifdef PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP
    pthread_mutexattr_init (&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ADAPTIVE_NP);
    pattr = &attr;
#endif

    if X_UNLIKELY((status = pthread_mutex_init(mutex, pattr)) != 0) {
        x_thread_abort(status, "pthread_mutex_init");
    }

#ifdef PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP
    pthread_mutexattr_destroy(&attr);
#endif

    return mutex;
}

static void x_mutex_impl_free(pthread_mutex_t *mutex)
{
    pthread_mutex_destroy(mutex);
    free(mutex);
}

static inline pthread_mutex_t *x_mutex_get_impl(XMutex *mutex)
{
    pthread_mutex_t *impl = x_atomic_pointer_get(&mutex->p);

    if X_UNLIKELY(impl == NULL) {
        impl = x_mutex_impl_new();
        if (!x_atomic_pointer_compare_and_exchange(&mutex->p, NULL, impl)) {
            x_mutex_impl_free(impl);
        }
        impl = mutex->p;
    }

    return impl;
}

void x_mutex_init_impl(XMutex *mutex)
{
    mutex->p = x_mutex_impl_new();
}

void x_mutex_clear_impl(XMutex *mutex)
{
    x_mutex_impl_free(mutex->p);
}

void x_mutex_lock_impl(XMutex *mutex)
{
    xint status;

    if X_UNLIKELY((status = pthread_mutex_lock(x_mutex_get_impl(mutex))) != 0) {
        x_thread_abort(status, "pthread_mutex_lock");
    }
}

void x_mutex_unlock_impl(XMutex *mutex)
{
    xint status;

    if X_UNLIKELY((status = pthread_mutex_unlock(x_mutex_get_impl(mutex))) != 0) {
        x_thread_abort(status, "pthread_mutex_unlock");
    }
}

xboolean x_mutex_trylock_impl(XMutex *mutex)
{
    xint status;

    if X_LIKELY((status = pthread_mutex_trylock(x_mutex_get_impl(mutex))) == 0) {
        return TRUE;
    }

    if X_UNLIKELY(status != EBUSY) {
        x_thread_abort(status, "pthread_mutex_trylock");
    }

    return FALSE;
}
#endif

static pthread_mutex_t *x_rec_mutex_impl_new(void)
{
    pthread_mutex_t *mutex;
    pthread_mutexattr_t attr;

    mutex = malloc(sizeof(pthread_mutex_t));
    if X_UNLIKELY(mutex == NULL) {
        x_thread_abort(errno, "malloc");
    }

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(mutex, &attr);
    pthread_mutexattr_destroy(&attr);

    return mutex;
}

static void x_rec_mutex_impl_free(pthread_mutex_t *mutex)
{
    pthread_mutex_destroy(mutex);
    free(mutex);
}

static inline pthread_mutex_t *x_rec_mutex_get_impl(XRecMutex *rec_mutex)
{
    pthread_mutex_t *impl = x_atomic_pointer_get(&rec_mutex->p);

    if X_UNLIKELY(impl == NULL) {
        impl = x_rec_mutex_impl_new();
        if (!x_atomic_pointer_compare_and_exchange(&rec_mutex->p, NULL, impl)) {
            x_rec_mutex_impl_free(impl);
        }

        impl = rec_mutex->p;
    }

    return impl;
}

void x_rec_mutex_init_impl(XRecMutex *rec_mutex)
{
    rec_mutex->p = x_rec_mutex_impl_new();
}

void x_rec_mutex_clear_impl(XRecMutex *rec_mutex)
{
    x_rec_mutex_impl_free(rec_mutex->p);
}

void x_rec_mutex_lock_impl(XRecMutex *mutex)
{
    pthread_mutex_lock(x_rec_mutex_get_impl(mutex));
}

void x_rec_mutex_unlock_impl(XRecMutex *rec_mutex)
{
    pthread_mutex_unlock(rec_mutex->p);
}

xboolean x_rec_mutex_trylock_impl(XRecMutex *rec_mutex)
{
    if (pthread_mutex_trylock(x_rec_mutex_get_impl(rec_mutex)) != 0) {
        return FALSE;
    }

    return TRUE;
}

static pthread_rwlock_t *x_rw_lock_impl_new(void)
{
    xint status;
    pthread_rwlock_t *rwlock;

    rwlock = malloc(sizeof(pthread_rwlock_t));
    if X_UNLIKELY(rwlock == NULL) {
        x_thread_abort(errno, "malloc");
    }

    if X_UNLIKELY((status = pthread_rwlock_init(rwlock, NULL)) != 0) {
        x_thread_abort(status, "pthread_rwlock_init");
    }

    return rwlock;
}

static void x_rw_lock_impl_free(pthread_rwlock_t *rwlock)
{
    pthread_rwlock_destroy(rwlock);
    free(rwlock);
}

static inline pthread_rwlock_t *x_rw_lock_get_impl(XRWLock *lock)
{
    pthread_rwlock_t *impl = x_atomic_pointer_get(&lock->p);

    if X_UNLIKELY(impl == NULL) {
        impl = x_rw_lock_impl_new();
        if (!x_atomic_pointer_compare_and_exchange(&lock->p, NULL, impl)) {
            x_rw_lock_impl_free(impl);
        }
        impl = lock->p;
    }

    return impl;
}

void x_rw_lock_init_impl(XRWLock *rw_lock)
{
    rw_lock->p = x_rw_lock_impl_new();
}

void x_rw_lock_clear_impl(XRWLock *rw_lock)
{
    x_rw_lock_impl_free(rw_lock->p);
}

void x_rw_lock_writer_lock_impl(XRWLock *rw_lock)
{
    int retval = pthread_rwlock_wrlock(x_rw_lock_get_impl(rw_lock));
    if (retval != 0) {
        x_critical("Failed to get RW lock %p: %s", rw_lock, x_strerror(retval));
    }
}

xboolean x_rw_lock_writer_trylock_impl(XRWLock *rw_lock)
{
    if (pthread_rwlock_trywrlock(x_rw_lock_get_impl(rw_lock)) != 0) {
        return FALSE;
    }

    return TRUE;
}

void x_rw_lock_writer_unlock_impl(XRWLock *rw_lock)
{
    pthread_rwlock_unlock(x_rw_lock_get_impl(rw_lock));
}

void x_rw_lock_reader_lock_impl(XRWLock *rw_lock)
{
    int retval = pthread_rwlock_rdlock (x_rw_lock_get_impl(rw_lock));
    if (retval != 0) {
        x_critical("Failed to get RW lock %p: %s", rw_lock, x_strerror(retval));
    }
}

xboolean x_rw_lock_reader_trylock_impl(XRWLock *rw_lock)
{
    if (pthread_rwlock_tryrdlock(x_rw_lock_get_impl(rw_lock)) != 0) {
        return FALSE;
    }

    return TRUE;
}

void x_rw_lock_reader_unlock_impl(XRWLock *rw_lock)
{
    pthread_rwlock_unlock(x_rw_lock_get_impl(rw_lock));
}

#if !defined(USE_NATIVE_MUTEX)
static pthread_cond_t *x_cond_impl_new(void)
{
    xint status;
    pthread_cond_t *cond;
    pthread_condattr_t attr;

    pthread_condattr_init (&attr);

#ifdef HAVE_PTHREAD_COND_TIMEDWAIT_RELATIVE_NP
#elif defined (HAVE_PTHREAD_CONDATTR_SETCLOCK) && defined (CLOCK_MONOTONIC)
    if X_UNLIKELY((status = pthread_condattr_setclock(&attr, CLOCK_MONOTONIC)) != 0) {
        x_thread_abort(status, "pthread_condattr_setclock");
    }
#else
    #error Cannot support XCond on your platform.
#endif

    cond = malloc(sizeof(pthread_cond_t));
    if X_UNLIKELY(cond == NULL) {
        x_thread_abort(errno, "malloc");
    }

    if X_UNLIKELY((status = pthread_cond_init (cond, &attr)) != 0) {
        x_thread_abort(status, "pthread_cond_init");
    }

    pthread_condattr_destroy(&attr);
    return cond;
}

static void x_cond_impl_free(pthread_cond_t *cond)
{
    pthread_cond_destroy(cond);
    free(cond);
}

static inline pthread_cond_t *x_cond_get_impl(XCond *cond)
{
    pthread_cond_t *impl = x_atomic_pointer_get(&cond->p);

    if X_UNLIKELY(impl == NULL) {
        impl = x_cond_impl_new();
        if (!x_atomic_pointer_compare_and_exchange(&cond->p, NULL, impl)) {
            x_cond_impl_free(impl);
        }
        impl = cond->p;
    }

    return impl;
}

void x_cond_init_impl(XCond *cond)
{
    cond->p = x_cond_impl_new();
}

void x_cond_clear_impl(XCond *cond)
{
    x_cond_impl_free(cond->p);
}

void x_cond_wait_impl(XCond *cond, XMutex *mutex)
{
    xint status;

    if X_UNLIKELY((status = pthread_cond_wait(x_cond_get_impl(cond), x_mutex_get_impl(mutex))) != 0) {
        x_thread_abort(status, "pthread_cond_wait");
    }
}

void x_cond_signal_impl(XCond *cond)
{
    xint status;

    if X_UNLIKELY((status = pthread_cond_signal(x_cond_get_impl(cond))) != 0) {
        x_thread_abort(status, "pthread_cond_signal");
    }
}

void x_cond_broadcast_impl(XCond *cond)
{
    xint status;

    if X_UNLIKELY((status = pthread_cond_broadcast(g_cond_get_impl(cond))) != 0) {
        x_thread_abort(status, "pthread_cond_broadcast");
    }
}

xboolean x_cond_wait_until_impl(XCond *cond, XMutex *mutex, xint64 end_time)
{
    xint status;
    struct timespec ts;

#ifdef HAVE_PTHREAD_COND_TIMEDWAIT_RELATIVE_NP
    {
        xint64 relative;
        xint64 now = x_get_monotonic_time();

        if (end_time <= now) {
            return FALSE;
        }

        relative = end_time - now;

        ts.tv_sec = relative / 1000000;
        ts.tv_nsec = (relative % 1000000) * 1000;

        if ((status = pthread_cond_timedwait_relative_np(x_cond_get_impl(cond), x_mutex_get_impl(mutex), &ts)) == 0) {
            return TRUE;
        }
    }
#elif defined (HAVE_PTHREAD_CONDATTR_SETCLOCK) && defined (CLOCK_MONOTONIC)
    {
        ts.tv_sec = end_time / 1000000;
        ts.tv_nsec = (end_time % 1000000) * 1000;

        if ((status = pthread_cond_timedwait(x_cond_get_impl(cond), x_mutex_get_impl(mutex), &ts)) == 0) {
            return TRUE;
        }
    }
#else
    #error Cannot support XCond on your platform.
#endif

    if X_UNLIKELY(status != ETIMEDOUT) {
        x_thread_abort(status, "pthread_cond_timedwait");
    }

    return FALSE;
}
#endif

static pthread_key_t *x_private_impl_new(XDestroyNotify notify)
{
    xint status;
    pthread_key_t *key;

    key = malloc(sizeof(pthread_key_t));
    if X_UNLIKELY(key == NULL) {
        x_thread_abort(errno, "malloc");
    }

    status = pthread_key_create(key, notify);
    if X_UNLIKELY(status != 0) {
        x_thread_abort(status, "pthread_key_create");
    }

    return key;
}

static void x_private_impl_free(pthread_key_t *key)
{
    xint status;

    status = pthread_key_delete(*key);
    if X_UNLIKELY(status != 0) {
        x_thread_abort(status, "pthread_key_delete");
    }

    free(key);
}

static xpointer x_private_impl_new_direct(XDestroyNotify notify)
{
    xint status;
    pthread_key_t key;
    xpointer impl = (void *)(xssize)-1;

    status = pthread_key_create(&key, notify);
    if X_UNLIKELY(status != 0) {
        x_thread_abort(status, "pthread_key_create");
    }

    memcpy(&impl, &key, sizeof(pthread_key_t));

    if (sizeof(pthread_key_t) == sizeof(xpointer)) {
        if X_UNLIKELY(impl == NULL) {
            status = pthread_key_create(&key, notify);
            if X_UNLIKELY(status != 0) {
                x_thread_abort(status, "pthread_key_create");
            }

            memcpy(&impl, &key, sizeof(pthread_key_t));

            if X_UNLIKELY(impl == NULL) {
                x_thread_abort(status, "pthread_key_create (gave NULL result twice)");
            }
        }
    }

    return impl;
}

static void x_private_impl_free_direct(xpointer impl)
{
    xint status;
    pthread_key_t tmp;

    memcpy(&tmp, &impl, sizeof(pthread_key_t));

    status = pthread_key_delete(tmp);
    if X_UNLIKELY(status != 0) {
        x_thread_abort(status, "pthread_key_delete");
    }
}

static inline pthread_key_t _x_private_get_impl(XPrivate *key)
{
    if (sizeof(pthread_key_t) > sizeof(xpointer)) {
        pthread_key_t *impl = x_atomic_pointer_get(&key->p);
        if X_UNLIKELY(impl == NULL) {
            impl = x_private_impl_new(key->notify);
            if (!x_atomic_pointer_compare_and_exchange(&key->p, NULL, impl)) {
                x_private_impl_free(impl);
                impl = key->p;
            }
        }

        return *impl;
    } else {
        pthread_key_t tmp;
        xpointer impl = x_atomic_pointer_get(&key->p);

        if X_UNLIKELY(impl == NULL) {
            impl = x_private_impl_new_direct(key->notify);
            if (!x_atomic_pointer_compare_and_exchange(&key->p, NULL, impl)) {
                x_private_impl_free_direct(impl);
                impl = key->p;
            }
        }

        memcpy(&tmp, &impl, sizeof(pthread_key_t));
        return tmp;
    }
}

xpointer x_private_get_impl(XPrivate *key)
{
    return pthread_getspecific(_x_private_get_impl(key));
}

void x_private_set_impl(XPrivate *key, xpointer value)
{
    xint status;

    if X_UNLIKELY((status = pthread_setspecific (_x_private_get_impl(key), value)) != 0) {
        x_thread_abort(status, "pthread_setspecific");
    }
}

void x_private_replace_impl(XPrivate *key, xpointer value)
{
    xint status;
    xpointer old;
    pthread_key_t impl = _x_private_get_impl(key);

    old = pthread_getspecific(impl);

    if X_UNLIKELY((status = pthread_setspecific(impl, value)) != 0) {
        x_thread_abort(status, "pthread_setspecific");
    }

    if (old && key->notify) {
        key->notify (old);
    }
}

#define posix_check_err(err, name)                                                                                              \
    X_STMT_START {                                                                                                              \
        int error = (err);                                                                                                      \
        if (error) {                                                                                                            \
            x_error("file %s: line %d (%s): error '%s' during '%s'", __FILE__, __LINE__, X_STRFUNC, x_strerror(error), name);   \
        }                                                                                                                       \
    } X_STMT_END

#define posix_check_cmd(cmd)    posix_check_err(cmd, #cmd)

typedef struct {
    XRealThread thread;
    pthread_t   system_thread;
    xboolean    joined;
    XMutex      lock;
    void *(*proxy)(void *);
} XThreadPosix;

void x_system_thread_free(XRealThread *thread)
{
    XThreadPosix *pt = (XThreadPosix *)thread;
    if (!pt->joined) {
        pthread_detach(pt->system_thread);
    }

    x_mutex_clear(&pt->lock);
    x_slice_free(XThreadPosix, pt);
}

XRealThread *x_system_thread_new(XThreadFunc proxy, xulong stack_size, const char *name, XThreadFunc func, xpointer data, XError **error)
{
    xint ret;
    pthread_attr_t attr;
    XThreadPosix *thread;
    XRealThread *base_thread;

    thread = x_slice_new0(XThreadPosix);
    base_thread = (XRealThread*)thread;
    base_thread->ref_count = 2;
    base_thread->ours = TRUE;
    base_thread->thread.joinable = TRUE;
    base_thread->thread.func = func;
    base_thread->thread.data = data;
    base_thread->name = x_strdup(name);
    thread->proxy = proxy;

    posix_check_cmd(pthread_attr_init(&attr));

#ifdef HAVE_PTHREAD_ATTR_SETSTACKSIZE
    if (stack_size) {
#ifdef _SC_THREAD_STACK_MIN
        long min_stack_size = sysconf(_SC_THREAD_STACK_MIN);
        if (min_stack_size >= 0) {
            stack_size = MAX((xulong)min_stack_size, stack_size);
        }
#endif
        pthread_attr_setstacksize (&attr, stack_size);
    }
#endif

#ifdef HAVE_PTHREAD_ATTR_SETINHERITSCHED
    {
        pthread_attr_setinheritsched(&attr, PTHREAD_INHERIT_SCHED);
    }
#endif

    ret = pthread_create(&thread->system_thread, &attr, (void *(*)(void *))proxy, thread);
    posix_check_cmd(pthread_attr_destroy (&attr));

    if (ret == EAGAIN) {
        x_set_error(error, X_THREAD_ERROR, X_THREAD_ERROR_AGAIN, "Error creating thread: %s", x_strerror(ret));
        x_free(thread->thread.name);
        x_slice_free(XThreadPosix, thread);
        return NULL;
    }

    posix_check_err(ret, "pthread_create");
    x_mutex_init(&thread->lock);

    return (XRealThread *)thread;
}

void x_thread_yield_impl(void)
{
    sched_yield();
}

void x_system_thread_wait(XRealThread *thread)
{
    XThreadPosix *pt = (XThreadPosix *)thread;

    x_mutex_lock(&pt->lock);
    if (!pt->joined) {
        posix_check_cmd(pthread_join(pt->system_thread, NULL));
        pt->joined = TRUE;
    }
    x_mutex_unlock(&pt->lock);
}

void x_system_thread_exit(void)
{
    pthread_exit(NULL);
}

void x_system_thread_set_name(const xchar *name)
{
#if defined(HAVE_PTHREAD_SETNAME_NP_WITHOUT_TID)
    pthread_setname_np(name);
#elif defined(HAVE_PTHREAD_SETNAME_NP_WITH_TID)
    pthread_setname_np(pthread_self(), name);
#elif defined(HAVE_PTHREAD_SETNAME_NP_WITH_TID_AND_ARG)
    pthread_setname_np(pthread_self(), "%s", (xchar *)name);
#elif defined(HAVE_PTHREAD_SET_NAME_NP)
    pthread_set_name_np(pthread_self(), name);
#endif
}

#if defined(USE_NATIVE_MUTEX)
#ifdef HAVE_STDATOMIC_H

#include <stdatomic.h>

#define exchange_acquire(ptr, new)                  atomic_exchange_explicit((atomic_uint *)(ptr), (new), __ATOMIC_ACQUIRE)
#define compare_exchange_acquire(ptr, old, new)     atomic_compare_exchange_strong_explicit((atomic_uint *)(ptr), (old), (new), __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)

#define exchange_release(ptr, new)                  atomic_exchange_explicit((atomic_uint *)(ptr), (new), __ATOMIC_RELEASE)
#define store_release(ptr, new)                     atomic_store_explicit((atomic_uint *)(ptr), (new), __ATOMIC_RELEASE)

#else

#define exchange_acquire(ptr, new)                  __atomic_exchange_4((ptr), (new), __ATOMIC_ACQUIRE)
#define compare_exchange_acquire(ptr, old, new)     __atomic_compare_exchange_4((ptr), (old), (new), 0, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)

#define exchange_release(ptr, new)                  __atomic_exchange_4((ptr), (new), __ATOMIC_RELEASE)
#define store_release(ptr, new)                     __atomic_store_4((ptr), (new), __ATOMIC_RELEASE)

#endif

typedef enum {
    X_MUTEX_STATE_EMPTY = 0,
    X_MUTEX_STATE_OWNED,
    X_MUTEX_STATE_CONTENDED,
} XMutexState;

void x_mutex_init_impl(XMutex *mutex)
{
    mutex->i[0] = X_MUTEX_STATE_EMPTY;
}

void x_mutex_clear_impl(XMutex *mutex)
{
    if X_UNLIKELY(mutex->i[0] != X_MUTEX_STATE_EMPTY) {
        fprintf(stderr, "x_mutex_clear() called on uninitialised or locked mutex\n");
        x_abort();
    }
}

X_GNUC_NO_INLINE
static void x_mutex_lock_slowpath(XMutex *mutex)
{
    while (exchange_acquire(&mutex->i[0], X_MUTEX_STATE_CONTENDED) != X_MUTEX_STATE_EMPTY) {
        x_futex_simple(&mutex->i[0], (xsize)FUTEX_WAIT_PRIVATE, X_MUTEX_STATE_CONTENDED, NULL);
    }
}

X_GNUC_NO_INLINE
static void x_mutex_unlock_slowpath(XMutex *mutex, xuint prev)
{
    if X_UNLIKELY(prev == X_MUTEX_STATE_EMPTY) {
        fprintf(stderr, "Attempt to unlock mutex that was not locked\n");
        x_abort();
    }

    x_futex_simple(&mutex->i[0], (xsize)FUTEX_WAKE_PRIVATE, (xsize) 1, NULL);
}

void x_mutex_lock_impl(XMutex *mutex)
{
    if X_UNLIKELY(!x_atomic_int_compare_and_exchange(&mutex->i[0], X_MUTEX_STATE_EMPTY, X_MUTEX_STATE_OWNED)) {
        x_mutex_lock_slowpath(mutex);
    }
}

void x_mutex_unlock_impl(XMutex *mutex)
{
    xuint prev;

    prev = exchange_release(&mutex->i[0], X_MUTEX_STATE_EMPTY);
    if X_UNLIKELY(prev != X_MUTEX_STATE_OWNED) {
        x_mutex_unlock_slowpath(mutex, prev);
    }
}

xboolean x_mutex_trylock_impl(XMutex *mutex)
{
    XMutexState empty = X_MUTEX_STATE_EMPTY;
    return compare_exchange_acquire(&mutex->i[0], &empty, X_MUTEX_STATE_OWNED);
}

void x_cond_init_impl(XCond *cond)
{
    cond->i[0] = 0;
}

void x_cond_clear_impl(XCond *cond)
{

}

void x_cond_wait_impl(XCond *cond, XMutex *mutex)
{
    xuint sampled = (xuint)x_atomic_int_get(&cond->i[0]);

    x_mutex_unlock(mutex);
    x_futex_simple(&cond->i[0], (xsize)FUTEX_WAIT_PRIVATE, (xsize)sampled, NULL);
    x_mutex_lock(mutex);
}

void x_cond_signal_impl(XCond *cond)
{
    x_atomic_int_inc(&cond->i[0]);
    x_futex_simple(&cond->i[0], (xsize)FUTEX_WAKE_PRIVATE, (xsize)1, NULL);
}

void x_cond_broadcast_impl(XCond *cond)
{
    x_atomic_int_inc(&cond->i[0]);
    x_futex_simple(&cond->i[0], (xsize)FUTEX_WAKE_PRIVATE, (xsize)INT_MAX, NULL);
}

xboolean x_cond_wait_until_impl(XCond *cond, XMutex *mutex, xint64 end_time)
{
    int res;
    xuint sampled;
    xboolean success;
    struct timespec now;
    struct timespec span;

    if (end_time < 0) {
        return FALSE;
    }

    clock_gettime(CLOCK_MONOTONIC, &now);
    span.tv_sec = (end_time / 1000000) - now.tv_sec;
    span.tv_nsec = ((end_time % 1000000) * 1000) - now.tv_nsec;
    if (span.tv_nsec < 0) {
        span.tv_nsec += 1000000000;
        span.tv_sec--;
    }

    if (span.tv_sec < 0) {
        return FALSE;
    }

    sampled = cond->i[0];
    x_mutex_unlock(mutex);

#if defined(HAVE_FUTEX_TIME64)
    {
        struct {
            xint64 tv_sec;
            xint64 tv_nsec;
        } span_arg;

        span_arg.tv_sec = span.tv_sec;
        span_arg.tv_nsec = span.tv_nsec;

        res = syscall(__NR_futex_time64, &cond->i[0], (xsize)FUTEX_WAIT_PRIVATE, (xsize)sampled, &span_arg);

#if defined(HAVE_FUTEX)
        if ((res >= 0) || (errno != ENOSYS))
#endif
        {
            success = (res < 0 && errno == ETIMEDOUT) ? FALSE : TRUE;
            x_mutex_lock(mutex);
            return success;
        }
    }
#endif

#if defined(HAVE_FUTEX)
    {
#ifdef __kernel_long_t
#define KERNEL_SPAN_SEC_TYPE    __kernel_long_t
        struct {
        __kernel_long_t tv_sec;
        __kernel_long_t tv_nsec;
        } span_arg;
#else
#define KERNEL_SPAN_SEC_TYPE    __kernel_time_t
        struct {
            __kernel_time_t tv_sec;
            long            tv_nsec;
        } span_arg;
#endif
        if (X_UNLIKELY((sizeof(KERNEL_SPAN_SEC_TYPE) < 8) && (span.tv_sec > X_MAXINT32))) {
            x_error("%s: Can’t wait for more than %us", X_STRFUNC, X_MAXINT32);
        }

        span_arg.tv_sec = span.tv_sec;
        span_arg.tv_nsec = span.tv_nsec;

        res = syscall(__NR_futex, &cond->i[0], (xsize)FUTEX_WAIT_PRIVATE, (xsize)sampled, &span_arg);
        success = (res < 0 && errno == ETIMEDOUT) ? FALSE : TRUE;
        x_mutex_lock(mutex);

        return success;
    }
#undef KERNEL_SPAN_SEC_TYPE
#endif

    x_assert_not_reached();
}
#endif
