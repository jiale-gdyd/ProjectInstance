#ifndef __X_THREAD_H__
#define __X_THREAD_H__

#include "xatomic.h"
#include "xerror.h"
#include "xutils.h"

X_BEGIN_DECLS

#define X_THREAD_ERROR              x_thread_error_quark()

XLIB_AVAILABLE_IN_ALL
XQuark x_thread_error_quark(void);

typedef enum {
    X_THREAD_ERROR_AGAIN
} XThreadError;

typedef xpointer (*XThreadFunc)(xpointer data);

typedef struct _XCond XCond;
typedef struct _XOnce XOnce;
typedef union  _XMutex XMutex;
typedef struct _XThread XThread;
typedef struct _XRWLock XRWLock;
typedef struct _XPrivate XPrivate;
typedef struct _XRecMutex XRecMutex;

union _XMutex {
    xpointer p;
    xuint    i[2];
};

struct _XRWLock {
    xpointer p;
    xuint    i[2];
};

struct _XCond {
    xpointer p;
    xuint    i[2];
};

struct _XRecMutex {
    xpointer p;
    xuint    i[2];
};

struct _XPrivate {
    xpointer       p;
    XDestroyNotify notify;
    xpointer       future[2];
};

typedef enum {
    X_ONCE_STATUS_NOTCALLED,
    X_ONCE_STATUS_PROGRESS,
    X_ONCE_STATUS_READY
} XOnceStatus;

struct _XOnce {
    volatile XOnceStatus status;
    volatile xpointer    retval;
};

#define X_ONCE_INIT                                 { X_ONCE_STATUS_NOTCALLED, NULL }
#define X_PRIVATE_INIT(notify)                      { NULL, (notify), { NULL, NULL } }

#define X_LOCK_NAME(name)                           x__ ## name ## _lock
#define X_LOCK_DEFINE_STATIC(name)                  static X_LOCK_DEFINE(name)
#define X_LOCK_DEFINE(name)                         XMutex X_LOCK_NAME(name)
#define X_LOCK_EXTERN(name)                         extern XMutex X_LOCK_NAME(name)

#define X_LOCK(name)                                x_mutex_lock(&X_LOCK_NAME(name))
#define X_UNLOCK(name)                              x_mutex_unlock(&X_LOCK_NAME(name))
#define X_TRYLOCK(name)                             x_mutex_trylock(&X_LOCK_NAME(name))

XLIB_AVAILABLE_IN_2_32
XThread *x_thread_ref(XThread *thread);

XLIB_AVAILABLE_IN_2_32
void x_thread_unref(XThread *thread);

XLIB_AVAILABLE_IN_2_32
XThread *x_thread_new(const xchar *name, XThreadFunc func, xpointer data);

XLIB_AVAILABLE_IN_2_32
XThread *x_thread_try_new(const xchar *name, XThreadFunc func, xpointer data, XError **error);

XLIB_AVAILABLE_IN_ALL
XThread *x_thread_self(void);

X_NORETURN XLIB_AVAILABLE_IN_ALL
void x_thread_exit(xpointer retval);

XLIB_AVAILABLE_IN_ALL
xpointer x_thread_join(XThread *thread);

XLIB_AVAILABLE_IN_ALL
void x_thread_yield(void);

XLIB_AVAILABLE_IN_2_32
void x_mutex_init(XMutex *mutex);

XLIB_AVAILABLE_IN_2_32
void x_mutex_clear(XMutex *mutex);

XLIB_AVAILABLE_IN_ALL
void x_mutex_lock(XMutex *mutex);

XLIB_AVAILABLE_IN_ALL
xboolean x_mutex_trylock(XMutex *mutex);

XLIB_AVAILABLE_IN_ALL
void x_mutex_unlock(XMutex *mutex);

XLIB_AVAILABLE_IN_2_32
void x_rw_lock_init(XRWLock *rw_lock);

XLIB_AVAILABLE_IN_2_32
void x_rw_lock_clear(XRWLock *rw_lock);

XLIB_AVAILABLE_IN_2_32
void x_rw_lock_writer_lock(XRWLock *rw_lock);

XLIB_AVAILABLE_IN_2_32
xboolean x_rw_lock_writer_trylock(XRWLock *rw_lock);

XLIB_AVAILABLE_IN_2_32
void x_rw_lock_writer_unlock(XRWLock *rw_lock);

XLIB_AVAILABLE_IN_2_32
void x_rw_lock_reader_lock(XRWLock *rw_lock);

XLIB_AVAILABLE_IN_2_32
xboolean x_rw_lock_reader_trylock(XRWLock *rw_lock);

XLIB_AVAILABLE_IN_2_32
void x_rw_lock_reader_unlock(XRWLock *rw_lock);

XLIB_AVAILABLE_IN_2_32
void x_rec_mutex_init(XRecMutex *rec_mutex);

XLIB_AVAILABLE_IN_2_32
void x_rec_mutex_clear(XRecMutex *rec_mutex);

XLIB_AVAILABLE_IN_2_32
void x_rec_mutex_lock(XRecMutex *rec_mutex);

XLIB_AVAILABLE_IN_2_32
xboolean x_rec_mutex_trylock(XRecMutex *rec_mutex);

XLIB_AVAILABLE_IN_2_32
void x_rec_mutex_unlock(XRecMutex *rec_mutex);

XLIB_AVAILABLE_IN_2_32
void x_cond_init(XCond *cond);

XLIB_AVAILABLE_IN_2_32
void x_cond_clear(XCond *cond);

XLIB_AVAILABLE_IN_ALL
void x_cond_wait(XCond *cond, XMutex *mutex);

XLIB_AVAILABLE_IN_ALL
void x_cond_signal(XCond *cond);

XLIB_AVAILABLE_IN_ALL
void x_cond_broadcast(XCond *cond);

XLIB_AVAILABLE_IN_2_32
xboolean x_cond_wait_until(XCond *cond, XMutex *mutex, xint64 end_time);

XLIB_AVAILABLE_IN_ALL
xpointer x_private_get(XPrivate *key);

XLIB_AVAILABLE_IN_ALL
void x_private_set(XPrivate*key, xpointer value);

XLIB_AVAILABLE_IN_2_32
void x_private_replace(XPrivate *key, xpointer value);

XLIB_AVAILABLE_IN_ALL
xpointer x_once_impl(XOnce *once, XThreadFunc func, xpointer arg);

XLIB_AVAILABLE_IN_ALL
xboolean x_once_init_enter(volatile void *location);

XLIB_AVAILABLE_IN_ALL
void x_once_init_leave(volatile void *location, xsize result);

#if defined(X_ATOMIC_LOCK_FREE) && defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4) && defined(__ATOMIC_SEQ_CST)
#define x_once(once, func, arg)                 ((__atomic_load_n(&(once)->status, __ATOMIC_ACQUIRE) == X_ONCE_STATUS_READY) ? (once)->retval : x_once_impl((once), (func), (arg)))
#else
#define x_once(once, func, arg)                 x_once_impl((once), (func), (arg))
#endif

#ifdef __GNUC__
#define x_once_init_enter(location)                                         \
    (X_GNUC_EXTENSION ({                                                    \
        X_STATIC_ASSERT(sizeof *(location) == sizeof(xpointer));            \
        (void)(0 ? (xpointer) *(location) : NULL);                          \
        (!x_atomic_pointer_get(location) && x_once_init_enter(location));   \
    }))

#define x_once_init_leave(location, result)                                 \
    (X_GNUC_EXTENSION ({                                                    \
        X_STATIC_ASSERT (sizeof *(location) == sizeof(xpointer));           \
        0 ? (void)(*(location) = (result)) : (void) 0;                      \
        x_once_init_leave((location), (xsize)(result));                     \
    }))
#else
#define x_once_init_enter(location)             (x_once_init_enter((location)))
#define x_once_init_leave(location, result)     (x_once_init_leave((location), (xsize) (result)))
#endif

XLIB_AVAILABLE_IN_2_36
xuint x_get_num_processors(void);

typedef void XMutexLocker;

XLIB_AVAILABLE_STATIC_INLINE_IN_2_44
static inline XMutexLocker *x_mutex_locker_new(XMutex *mutex)
{
    x_mutex_lock(mutex);
    return (XMutexLocker *)mutex;
}

XLIB_AVAILABLE_STATIC_INLINE_IN_2_44
static inline void x_mutex_locker_free(XMutexLocker *locker)
{
    x_mutex_unlock((XMutex *)locker);
}

typedef void XRecMutexLocker;

X_GNUC_BEGIN_IGNORE_DEPRECATIONS
XLIB_AVAILABLE_STATIC_INLINE_IN_2_60
static inline XRecMutexLocker *x_rec_mutex_locker_new(XRecMutex *rec_mutex)
{
    x_rec_mutex_lock(rec_mutex);
    return (XRecMutexLocker *)rec_mutex;
}
X_GNUC_END_IGNORE_DEPRECATIONS

X_GNUC_BEGIN_IGNORE_DEPRECATIONS
XLIB_AVAILABLE_STATIC_INLINE_IN_2_60
static inline void x_rec_mutex_locker_free(XRecMutexLocker *locker)
{
    x_rec_mutex_unlock((XRecMutex *)locker);
}
X_GNUC_END_IGNORE_DEPRECATIONS

typedef void XRWLockWriterLocker;

X_GNUC_BEGIN_IGNORE_DEPRECATIONS
XLIB_AVAILABLE_STATIC_INLINE_IN_2_62
static inline XRWLockWriterLocker *x_rw_lock_writer_locker_new(XRWLock *rw_lock)
{
    x_rw_lock_writer_lock(rw_lock);
    return (XRWLockWriterLocker *)rw_lock;
}
X_GNUC_END_IGNORE_DEPRECATIONS

X_GNUC_BEGIN_IGNORE_DEPRECATIONS
XLIB_AVAILABLE_STATIC_INLINE_IN_2_62
static inline void x_rw_lock_writer_locker_free(XRWLockWriterLocker *locker)
{
    x_rw_lock_writer_unlock((XRWLock *)locker);
}
X_GNUC_END_IGNORE_DEPRECATIONS

typedef void XRWLockReaderLocker;

X_GNUC_BEGIN_IGNORE_DEPRECATIONS
XLIB_AVAILABLE_STATIC_INLINE_IN_2_62
static inline XRWLockReaderLocker *x_rw_lock_reader_locker_new(XRWLock *rw_lock)
{
    x_rw_lock_reader_lock(rw_lock);
    return (XRWLockReaderLocker *)rw_lock;
}
X_GNUC_END_IGNORE_DEPRECATIONS

X_GNUC_BEGIN_IGNORE_DEPRECATIONS
XLIB_AVAILABLE_STATIC_INLINE_IN_2_62
static inline void x_rw_lock_reader_locker_free(XRWLockReaderLocker *locker)
{
    x_rw_lock_reader_unlock((XRWLock *)locker);
}
X_GNUC_END_IGNORE_DEPRECATIONS

X_END_DECLS

#endif
