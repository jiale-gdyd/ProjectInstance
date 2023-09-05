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

#if defined(__NR_futex) && defined(__NR_futex_time64)
#define x_futex_simple(uaddr, futex_op, ...)                                                    \
    X_STMT_START {                                                                              \
        int saved_errno = errno;                                                                \
        int res = syscall(__NR_futex_time64, uaddr, (xsize)futex_op, __VA_ARGS__);              \
        if ((res < 0) && (errno == ENOSYS)) {                                                   \
            errno = saved_errno;                                                                \
            res = syscall(__NR_futex, uaddr, (gsize) futex_op, __VA_ARGS__);                    \
        }                                                                                       \
        if ((res < 0) && (errno == EAGAIN)) {                                                   \
            errno = saved_errno;                                                                \
        }                                                                                       \
    } X_STMT_END
#elif defined(__NR_futex_time64)
#define x_futex_simple(uaddr, futex_op, ...)                                                    \
    X_STMT_START {                                                                              \
        int saved_errno = errno;                                                                \
        int res = syscall(__NR_futex_time64, uaddr, (gsize) futex_op, __VA_ARGS__);             \
        if ((res < 0) && (errno == EAGAIN)) {                                                   \
            errno = saved_errno;                                                                \
        }                                                                                       \
  } X_STMT_END
#elif defined(__NR_futex)
#define x_futex_simple(uaddr, futex_op, ...)                                                    \
    X_STMT_START {                                                                              \
        int saved_errno = errno;                                                                \
        int res = syscall(__NR_futex, uaddr, (xsize)futex_op, __VA_ARGS__);                     \
        if ((res < 0) && (errno == EAGAIN)) {                                                   \
            errno = saved_errno;                                                                \
        }                                                                                       \
    } X_STMT_END
#else
#error "Neither __NR_futex nor __NR_futex_time64 are defined but were found by meson"
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

#endif
