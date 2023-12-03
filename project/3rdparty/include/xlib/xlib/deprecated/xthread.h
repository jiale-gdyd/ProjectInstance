#ifndef __X_DEPRECATED_THREAD_H__
#define __X_DEPRECATED_THREAD_H__

#include "../xthread.h"

#include <pthread.h>
#include <sys/types.h>

X_BEGIN_DECLS

X_GNUC_BEGIN_IGNORE_DEPRECATIONS

typedef enum {
    X_THREAD_PRIORITY_LOW,
    X_THREAD_PRIORITY_NORMAL,
    X_THREAD_PRIORITY_HIGH,
    X_THREAD_PRIORITY_URGENT
} XThreadPriority XLIB_DEPRECATED_TYPE_IN_2_32;

struct  _XThread {
    XThreadFunc     func;
    xpointer        data;
    xboolean        joinable;
    XThreadPriority priority;
};

typedef struct _XThreadFunctions XThreadFunctions XLIB_DEPRECATED_TYPE_IN_2_32;

struct _XThreadFunctions
{
    XMutex *(*mutex_new)(void);
    void (*mutex_lock)(XMutex *mutex);
    xboolean (*mutex_trylock)(XMutex *mutex);
    void (*mutex_unlock) (XMutex *mutex);
    void (*mutex_free)(XMutex *mutex);
    XCond *(*cond_new)(void);
    void (*cond_signal)(XCond *cond);
    void (*cond_broadcast)(XCond *cond);
    void (*cond_wait)(XCond *cond, XMutex *mutex);
    xboolean (*cond_timed_wait)(XCond *cond, XMutex *mutex, XTimeVal *end_time);
    void (*cond_free)(XCond *cond);
    XPrivate* (*private_new)(XDestroyNotify destructor);
    xpointer  (*private_get)(XPrivate *private_key);
    void (*private_set)(XPrivate *private_key, xpointer data);
    void (*thread_create)(XThreadFunc func, xpointer data, xulong stack_size, xboolean joinable, xboolean bound, XThreadPriority priority, xpointer thread, XError **error);
    void (*thread_yield)(void);
    void (*thread_join)(xpointer thread);
    void (*thread_exit)(void);
    void (*thread_set_priority)(xpointer thread, XThreadPriority priority);
    void (*thread_self)(xpointer thread);
    xboolean (*thread_equal)(xpointer thread1, xpointer thread2);
} XLIB_DEPRECATED_TYPE_IN_2_32;

XLIB_VAR xboolean x_thread_use_default_impl;
XLIB_VAR XThreadFunctions x_thread_functions_for_xlib_use;

XLIB_VAR xuint64 (*x_thread_gettime)(void);

XLIB_DEPRECATED_IN_2_32_FOR(x_thread_new)
XThread *x_thread_create(XThreadFunc func, xpointer data, xboolean joinable, XError **error);

XLIB_DEPRECATED_IN_2_32_FOR(x_thread_new)
XThread *x_thread_create_full(XThreadFunc func, xpointer data, xulong stack_size, xboolean joinable, xboolean bound, XThreadPriority priority, XError **error);

XLIB_DEPRECATED_IN_2_32
void x_thread_set_priority(XThread *thread, XThreadPriority priority);

XLIB_DEPRECATED_IN_2_32
void x_thread_foreach(XFunc thread_func, xpointer user_data);

#define x_static_mutex_get_mutex            x_static_mutex_get_mutex_impl XLIB_DEPRECATED_MACRO_IN_2_32
#define X_STATIC_MUTEX_INIT                 { NULL, PTHREAD_MUTEX_INITIALIZER } XLIB_DEPRECATED_MACRO_IN_2_32_FOR(x_mutex_init)

typedef struct {
    XMutex          *mutex;
    pthread_mutex_t unused;
} XStaticMutex XLIB_DEPRECATED_TYPE_IN_2_32_FOR(XMutex);

#define x_static_mutex_lock(mutex)          x_mutex_lock(x_static_mutex_get_mutex(mutex)) XLIB_DEPRECATED_MACRO_IN_2_32_FOR(x_mutex_lock)
#define x_static_mutex_trylock(mutex)       x_mutex_trylock(x_static_mutex_get_mutex(mutex)) XLIB_DEPRECATED_MACRO_IN_2_32_FOR(x_mutex_trylock)
#define x_static_mutex_unlock(mutex)        x_mutex_unlock(x_static_mutex_get_mutex(mutex)) XLIB_DEPRECATED_MACRO_IN_2_32_FOR(x_mutex_unlock)

XLIB_DEPRECATED_IN_2_32_FOR(x_mutex_init)
void x_static_mutex_init(XStaticMutex *mutex);

XLIB_DEPRECATED_IN_2_32_FOR(x_mutex_clear)
void x_static_mutex_free(XStaticMutex *mutex);

XLIB_DEPRECATED_IN_2_32_FOR(XMutex)
XMutex *x_static_mutex_get_mutex_impl(XStaticMutex *mutex);

typedef struct _XStaticRecMutex XStaticRecMutex XLIB_DEPRECATED_TYPE_IN_2_32_FOR(XRecMutex);

struct _XStaticRecMutex {
    XStaticMutex mutex;
    xuint        depth;

    union {
        pthread_t owner;
        xdouble   dummy;
    } unused;
} XLIB_DEPRECATED_TYPE_IN_2_32_FOR(XRecMutex);

#define X_STATIC_REC_MUTEX_INIT             { X_STATIC_MUTEX_INIT, 0, { 0 } } XLIB_DEPRECATED_MACRO_IN_2_32_FOR(x_rec_mutex_init)

XLIB_DEPRECATED_IN_2_32_FOR(x_rec_mutex_init)
void x_static_rec_mutex_init(XStaticRecMutex *mutex);

XLIB_DEPRECATED_IN_2_32_FOR(x_rec_mutex_lock)
void x_static_rec_mutex_lock(XStaticRecMutex *mutex);

XLIB_DEPRECATED_IN_2_32_FOR(x_rec_mutex_try_lock)
xboolean x_static_rec_mutex_trylock(XStaticRecMutex *mutex);

XLIB_DEPRECATED_IN_2_32_FOR(x_rec_mutex_unlock)
void x_static_rec_mutex_unlock(XStaticRecMutex *mutex);

XLIB_DEPRECATED_IN_2_32
void x_static_rec_mutex_lock_full(XStaticRecMutex *mutex, xuint depth);

XLIB_DEPRECATED_IN_2_32
xuint x_static_rec_mutex_unlock_full(XStaticRecMutex *mutex);

XLIB_DEPRECATED_IN_2_32_FOR(x_rec_mutex_free)
void x_static_rec_mutex_free(XStaticRecMutex *mutex);

typedef struct _XStaticRWLock XStaticRWLock XLIB_DEPRECATED_TYPE_IN_2_32_FOR(XRWLock);

struct _XStaticRWLock {
    XStaticMutex mutex;
    XCond        *read_cond;
    XCond        *write_cond;
    xuint        read_counter;
    xboolean     have_writer;
    xuint        want_to_read;
    xuint        want_to_write;
} XLIB_DEPRECATED_TYPE_IN_2_32_FOR(XRWLock);

#define X_STATIC_RW_LOCK_INIT               { X_STATIC_MUTEX_INIT, NULL, NULL, 0, FALSE, 0, 0 } XLIB_DEPRECATED_MACRO_IN_2_32_FOR(x_rw_lock_init)

XLIB_DEPRECATED_IN_2_32_FOR(x_rw_lock_init)
void x_static_rw_lock_init(XStaticRWLock *lock);

XLIB_DEPRECATED_IN_2_32_FOR(x_rw_lock_reader_lock)
void x_static_rw_lock_reader_lock(XStaticRWLock *lock);

XLIB_DEPRECATED_IN_2_32_FOR(x_rw_lock_reader_trylock)
xboolean x_static_rw_lock_reader_trylock(XStaticRWLock *lock);

XLIB_DEPRECATED_IN_2_32_FOR(x_rw_lock_reader_unlock)
void x_static_rw_lock_reader_unlock(XStaticRWLock *lock);

XLIB_DEPRECATED_IN_2_32_FOR(x_rw_lock_writer_lock)
void x_static_rw_lock_writer_lock(XStaticRWLock *lock);

XLIB_DEPRECATED_IN_2_32_FOR(x_rw_lock_writer_trylock)
xboolean x_static_rw_lock_writer_trylock(XStaticRWLock *lock);

XLIB_DEPRECATED_IN_2_32_FOR(x_rw_lock_writer_unlock)
void x_static_rw_lock_writer_unlock(XStaticRWLock *lock);

XLIB_DEPRECATED_IN_2_32_FOR(x_rw_lock_free)
void x_static_rw_lock_free(XStaticRWLock *lock);

XLIB_DEPRECATED_IN_2_32
XPrivate *x_private_new(XDestroyNotify notify);

typedef struct _XStaticPrivate XStaticPrivate XLIB_DEPRECATED_TYPE_IN_2_32_FOR(XPrivate);
struct _XStaticPrivate {
    xuint index;
} XLIB_DEPRECATED_TYPE_IN_2_32_FOR(XPrivate);

#define X_STATIC_PRIVATE_INIT                   { 0 } XLIB_DEPRECATED_MACRO_IN_2_32_FOR(X_PRIVATE_INIT)

XLIB_DEPRECATED_IN_2_32
void x_static_private_init(XStaticPrivate *private_key);

XLIB_DEPRECATED_IN_2_32_FOR(x_private_get)
xpointer x_static_private_get(XStaticPrivate *private_key);

XLIB_DEPRECATED_IN_2_32_FOR(x_private_set)
void x_static_private_set(XStaticPrivate *private_key, xpointer data, XDestroyNotify notify);

XLIB_DEPRECATED_IN_2_32
void x_static_private_free(XStaticPrivate *private_key);

XLIB_DEPRECATED_IN_2_32
xboolean x_once_init_enter_impl(volatile xsize *location);

XLIB_DEPRECATED_IN_2_32
void x_thread_init(xpointer vtable);

XLIB_DEPRECATED_IN_2_32
void x_thread_init_with_errorcheck_mutexes(xpointer vtable);

XLIB_DEPRECATED_IN_2_32
xboolean x_thread_get_initialized(void);

XLIB_VAR xboolean x_threads_got_initialized;

#define x_thread_supported()                    (1) XLIB_DEPRECATED_MACRO_IN_2_32

XLIB_DEPRECATED_IN_2_32
XMutex *x_mutex_new(void);

XLIB_DEPRECATED_IN_2_32
void x_mutex_free(XMutex *mutex);

XLIB_DEPRECATED_IN_2_32
XCond *x_cond_new(void);

XLIB_DEPRECATED_IN_2_32
void x_cond_free(XCond *cond);

XLIB_DEPRECATED_IN_2_32
xboolean x_cond_timed_wait(XCond *cond, XMutex *mutex, XTimeVal *abs_time);

X_GNUC_END_IGNORE_DEPRECATIONS

X_END_DECLS

#endif
