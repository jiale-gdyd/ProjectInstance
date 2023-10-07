#define X_IMPLEMENT_INLINES         1
#define __X_THREAD_C__

#include <time.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include <xlib/xlib/config.h>
#include <xlib/xlib/xslice.h>
#include <xlib/xlib/xthread.h>
#include <xlib/xlib/xstrfuncs.h>
#include <xlib/xlib/xtestutils.h>
#include <xlib/xlib/xlib_trace.h>
#include <xlib/xlib/xtrace-private.h>
#include <xlib/xlib/xthreadprivate.h>

X_DEFINE_QUARK(x_thread_error, x_thread_error)

static XCond x_once_cond;
static XMutex x_once_mutex;
static XSList *x_once_init_list = NULL;
static xuint x_thread_n_created_counter = 0;

static void x_thread_cleanup(xpointer data);
static XPrivate x_thread_specific_private = X_PRIVATE_INIT(x_thread_cleanup);

xpointer x_private_set_alloc0(XPrivate *key, xsize size)
{
    xpointer allocated = x_malloc0(size);
    x_private_set(key, allocated);

    return x_steal_pointer(&allocated);
}

xpointer x_once_impl(XOnce *once, XThreadFunc func, xpointer arg)
{
    x_mutex_lock(&x_once_mutex);

    while (once->status == X_ONCE_STATUS_PROGRESS) {
        x_cond_wait(&x_once_cond, &x_once_mutex);
    }

    if (once->status != X_ONCE_STATUS_READY) {
        xpointer retval;

        once->status = X_ONCE_STATUS_PROGRESS;
        x_mutex_unlock(&x_once_mutex);

        retval = func(arg);

        x_mutex_lock(&x_once_mutex);

#if defined(X_ATOMIC_LOCK_FREE) && defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4) && defined(__ATOMIC_SEQ_CST)
        once->retval = retval;
        __atomic_store_n(&once->status, X_ONCE_STATUS_READY, __ATOMIC_RELEASE);
#else
        once->retval = retval;
        once->status = X_ONCE_STATUS_READY;
#endif
        x_cond_broadcast(&x_once_cond);
    }

    x_mutex_unlock(&x_once_mutex);

    return once->retval;
}

xboolean (x_once_init_enter)(volatile void *location)
{
    xboolean need_init = FALSE;
    xsize *value_location = (xsize *)location;

    x_mutex_lock(&x_once_mutex);
    if (x_atomic_pointer_get(value_location) == 0) {
        if (!x_slist_find(x_once_init_list, (void *)value_location)) {
            need_init = TRUE;
            x_once_init_list = x_slist_prepend(x_once_init_list, (void *)value_location);
        } else {
            do {
                x_cond_wait(&x_once_cond, &x_once_mutex);
            } while (x_slist_find(x_once_init_list, (void *)value_location));
        }
    }
    x_mutex_unlock(&x_once_mutex);

    return need_init;
}

xboolean (x_once_init_enter_pointer)(xpointer location)
{
    xboolean need_init = FALSE;
    xpointer *value_location = (xpointer *)location;

    x_mutex_lock(&x_once_mutex);
    if (x_atomic_pointer_get(value_location) == 0) {
        if (!x_slist_find(x_once_init_list, (void *)value_location)) {
            need_init = TRUE;
            x_once_init_list = x_slist_prepend(x_once_init_list, (void *)value_location);
        } else {
            do {
                x_cond_wait(&x_once_cond, &x_once_mutex);
            } while (x_slist_find(x_once_init_list, (void *)value_location));
        }
    }
    x_mutex_unlock(&x_once_mutex);

    return need_init;
}

void (x_once_init_leave)(volatile void *location, xsize result)
{
    xsize old_value;
    xsize *value_location = (xsize *)location;

    x_return_if_fail(result != 0);

    old_value = (xsize)x_atomic_pointer_exchange(value_location, result);
    x_return_if_fail(old_value == 0);

    x_mutex_lock(&x_once_mutex);
    x_return_if_fail(x_once_init_list != NULL);
    x_once_init_list = x_slist_remove(x_once_init_list, (void*) value_location);
    x_cond_broadcast(&x_once_cond);
    x_mutex_unlock(&x_once_mutex);
}

void (x_once_init_leave_pointer)(xpointer location, xpointer result)
{
    xpointer old_value;
    xpointer *value_location = (xpointer *)location;

    x_return_if_fail(result != 0);

    old_value = x_atomic_pointer_exchange(value_location, result);
    x_return_if_fail(old_value == 0);

    x_mutex_lock(&x_once_mutex);
    x_return_if_fail(x_once_init_list != NULL);
    x_once_init_list = x_slist_remove(x_once_init_list, (void *)value_location);
    x_cond_broadcast(&x_once_cond);
    x_mutex_unlock(&x_once_mutex);
}

XThread *x_thread_ref(XThread *thread)
{
    XRealThread *real = (XRealThread *)thread;
    x_atomic_int_inc(&real->ref_count);
    return thread;
}

void x_thread_unref(XThread *thread)
{
    XRealThread *real = (XRealThread *)thread;

    if (x_atomic_int_dec_and_test(&real->ref_count)) {
        if (real->ours) {
            x_system_thread_free(real);
        } else {
            x_slice_free(XRealThread, real);
        }
    }
}

static void x_thread_cleanup(xpointer data)
{
    x_thread_unref((XThread *)data);
}

xpointer x_thread_proxy(xpointer data)
{
    XRealThread *thread = (XRealThread *)data;

    x_assert(data);
    x_private_set(&x_thread_specific_private, data);

    TRACE(XLIB_THREAD_SPAWNED(thread->thread.func, thread->thread.data, thread->name));

    if (thread->name) {
        x_system_thread_set_name(thread->name);
        x_free(thread->name);
        thread->name = NULL;
    }

    thread->retval = thread->thread.func (thread->thread.data);
    return NULL;
}

xuint x_thread_n_created(void)
{
    return x_atomic_int_get(&x_thread_n_created_counter);
}

XThread *x_thread_new(const xchar *name, XThreadFunc func, xpointer data)
{
    XThread *thread;
    XError *error = NULL;

    thread = x_thread_new_internal(name, x_thread_proxy, func, data, 0, &error);
    if X_UNLIKELY(thread == NULL) {
        x_error("creating thread '%s': %s", name ? name : "", error->message);
    }

    return thread;
}

XThread *x_thread_try_new(const xchar *name, XThreadFunc func, xpointer data, XError **error)
{
    return x_thread_new_internal(name, x_thread_proxy, func, data, 0, error);
}

XThread *x_thread_new_internal(const xchar *name, XThreadFunc proxy, XThreadFunc func, xpointer data, xsize stack_size, XError **error)
{
    x_return_val_if_fail(func != NULL, NULL);

    x_atomic_int_inc(&x_thread_n_created_counter);
    x_trace_mark(X_TRACE_CURRENT_TIME, 0, "XLib", "XThread created", "%s", name ? name : "(unnamed)");

    return (XThread *)x_system_thread_new(proxy, stack_size, name, func, data, error);
}

void x_thread_exit(xpointer retval)
{
    XRealThread *real = (XRealThread *)x_thread_self();

    if X_UNLIKELY(!real->ours) {
        x_error("attempt to x_thread_exit() a thread not created by XLib");
    }
    real->retval = retval;

    x_system_thread_exit();
}

xpointer x_thread_join(XThread *thread)
{
    xpointer retval;
    XRealThread *real = (XRealThread *)thread;

    x_return_val_if_fail(thread, NULL);
    x_return_val_if_fail(real->ours, NULL);

    x_system_thread_wait(real);

    retval = real->retval;
    thread->joinable = 0;

    x_thread_unref(thread);

    return retval;
}

XThread *x_thread_self(void)
{
    XRealThread *thread = (XRealThread *)x_private_get(&x_thread_specific_private);
    if (!thread) {
        thread = x_slice_new0(XRealThread);
        thread->ref_count = 1;

        x_private_set(&x_thread_specific_private, thread);
    }

    return (XThread *)thread;
}

xuint x_get_num_processors(void)
{
#if defined(_SC_NPROCESSORS_ONLN)
    {
        int count;

        count = sysconf(_SC_NPROCESSORS_ONLN);
        if (count > 0) {
            return count;
        }
    }
#elif defined HW_NCPU
    {
    xpointer retval;
        int mib[2], count = 0;
        size_t len;

        mib[0] = CTL_HW;
        mib[1] = HW_NCPU;
        len = sizeof(count);

        if (sysctl(mib, 2, &count, &len, NULL, 0) == 0 && count > 0) {
            return count;
        }
    }
#endif

    return 1;
}

void x_thread_init(xpointer init)
{
    if (init != NULL) {
        x_warning("XThread system no longer supports custom thread implementations.");
    }
}

void x_thread_init_with_errorcheck_mutexes(xpointer vtable)
{
    x_assert(vtable == NULL);
    x_warning("XThread system no longer supports errorcheck mutexes.");
}
