#ifndef __X_THREADPRIVATE_H__
#define __X_THREADPRIVATE_H__

#include "config.h"
#include "deprecated/xthread.h"

typedef struct _XRealThread XRealThread;
struct _XRealThread {
    XThread  thread;
    xint     ref_count;
    xboolean ours;
    xchar    *name;
    xpointer retval;
};

#if defined(HAVE_FUTEX) || defined(HAVE_FUTEX_TIME64)
#include <errno.h>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>

#ifndef FUTEX_WAIT_PRIVATE
#define FUTEX_WAIT_PRIVATE  FUTEX_WAIT
#define FUTEX_WAKE_PRIVATE  FUTEX_WAKE
#endif

#if defined(HAVE_FUTEX) && defined(HAVE_FUTEX_TIME64)
#if defined(__ANDROID__)
#define x_futex_simple(uaddr, futex_op, ...)                                                    \
    X_STMT_START {                                                                              \
        int saved_errno = errno;                                                                \
        int res = 0;                                                                            \
        if (__builtin_available(android 30, *)) {                                               \
            res = syscall(__NR_futex_time64, uaddr, (xsize)futex_op, __VA_ARGS__);              \
            if (res < 0 && errno == ENOSYS) {                                                   \
                errno = saved_errno;                                                            \
                res = syscall(__NR_futex, uaddr, (xsize)futex_op, __VA_ARGS__);                 \
            }                                                                                   \
        } else {                                                                                \
            res = syscall(__NR_futex, uaddr, (xsize)futex_op, __VA_ARGS__);                     \
        }                                                                                       \
        if (res < 0 && errno == EAGAIN) {                                                       \
            errno = saved_errno;                                                                \
        }                                                                                       \
    } X_STMT_END
#else
#define x_futex_simple(uaddr, futex_op, ...)                                                    \
    X_STMT_START {                                                                              \
        int saved_errno = errno;                                                                \
        int res = syscall(__NR_futex_time64, uaddr, (xsize)futex_op, __VA_ARGS__);              \
        if ((res < 0) && (errno == ENOSYS)) {                                                   \
            errno = saved_errno;                                                                \
            res = syscall(__NR_futex, uaddr, (xsize)futex_op, __VA_ARGS__);                     \
        }                                                                                       \
        if ((res < 0) && (errno == EAGAIN)) {                                                   \
            errno = saved_errno;                                                                \
        }                                                                                       \
    } X_STMT_END
#endif
#elif defined(HAVE_FUTEX_TIME64)
#define x_futex_simple(uaddr, futex_op, ...)                                                    \
    X_STMT_START {                                                                              \
        int saved_errno = errno;                                                                \
        int res = syscall(__NR_futex_time64, uaddr, (xsize)futex_op, __VA_ARGS__);              \
        if ((res < 0) && (errno == EAGAIN)) {                                                   \
            errno = saved_errno;                                                                \
        }                                                                                       \
  } X_STMT_END
#elif defined(HAVE_FUTEX)
#define x_futex_simple(uaddr, futex_op, ...)                                                    \
    X_STMT_START {                                                                              \
        int saved_errno = errno;                                                                \
        int res = syscall(__NR_futex, uaddr, (xsize)futex_op, __VA_ARGS__);                     \
        if ((res < 0) && (errno == EAGAIN)) {                                                   \
            errno = saved_errno;                                                                \
        }                                                                                       \
    } X_STMT_END
#else
#error "Neither __NR_futex nor __NR_futex_time64 are available"
#endif
#endif

void x_system_thread_wait(XRealThread  *thread);

XRealThread *x_system_thread_new(XThreadFunc proxy, xulong stack_size, const char *name, XThreadFunc func, xpointer data, XError **error);

void x_system_thread_free(XRealThread  *thread);

X_NORETURN void x_system_thread_exit(void);
void x_system_thread_set_name(const xchar *name);

XThread *x_thread_new_internal(const xchar *name, XThreadFunc proxy, XThreadFunc func, xpointer data, xsize stack_size, XError **error);

xpointer x_thread_proxy(xpointer thread);
xuint x_thread_n_created(void);

xpointer x_private_set_alloc0(XPrivate *key, xsize size);

void x_mutex_init_impl(XMutex *mutex);
void x_mutex_clear_impl(XMutex *mutex);
void x_mutex_lock_impl(XMutex *mutex);
void x_mutex_unlock_impl(XMutex *mutex);
xboolean x_mutex_trylock_impl(XMutex *mutex);

void x_rec_mutex_init_impl(XRecMutex *rec_mutex);
void x_rec_mutex_clear_impl(XRecMutex *rec_mutex);
void x_rec_mutex_lock_impl(XRecMutex *mutex);
void x_rec_mutex_unlock_impl(XRecMutex *rec_mutex);
xboolean x_rec_mutex_trylock_impl(XRecMutex *rec_mutex);

void x_rw_lock_init_impl(XRWLock *rw_lock);
void x_rw_lock_clear_impl(XRWLock *rw_lock);
void x_rw_lock_writer_lock_impl(XRWLock *rw_lock);
xboolean x_rw_lock_writer_trylock_impl(XRWLock *rw_lock);
void x_rw_lock_writer_unlock_impl(XRWLock *rw_lock);
void x_rw_lock_reader_lock_impl(XRWLock *rw_lock);
xboolean x_rw_lock_reader_trylock_impl(XRWLock *rw_lock);
void x_rw_lock_reader_unlock_impl(XRWLock *rw_lock);

void x_cond_init_impl(XCond *cond);
void x_cond_clear_impl(XCond *cond);
void x_cond_wait_impl(XCond *cond, XMutex *mutex);
void x_cond_signal_impl(XCond *cond);
void x_cond_broadcast_impl(XCond *cond);
xboolean x_cond_wait_until_impl(XCond *cond, XMutex *mutex, xint64 end_time);

xpointer x_private_get_impl(XPrivate *key);
void x_private_set_impl(XPrivate *key, xpointer value);
void x_private_replace_impl(XPrivate *key, xpointer value);

void x_thread_yield_impl(void);

#endif
