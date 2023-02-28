#include <xlib/xlib/config.h>

#ifndef XLIB_DISABLE_DEPRECATION_WARNINGS
#define XLIB_DISABLE_DEPRECATION_WARNINGS
#endif

#include <xlib/xlib/xmain.h>
#include <xlib/xlib/xarray.h>
#include <xlib/xlib/xutils.h>
#include <xlib/xlib/xslice.h>
#include <xlib/xlib/xthread.h>
#include <xlib/xlib/xmessages.h>
#include <xlib/xlib/xthreadprivate.h>
#include <xlib/xlib/deprecated/xthread.h>

xboolean x_thread_use_default_impl = FALSE;

XThreadFunctions x_thread_functions_for_xlib_use = {
    x_mutex_new,
    x_mutex_lock,
    x_mutex_trylock,
    x_mutex_unlock,
    x_mutex_free,
    x_cond_new,
    x_cond_signal,
    x_cond_broadcast,
    x_cond_wait,
    x_cond_timed_wait,
    x_cond_free,
    x_private_new,
    x_private_get,
    x_private_set,
    NULL,
    x_thread_yield,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};

static xuint64 gettime(void)
{
    return x_get_monotonic_time() * 1000;
}

xuint64 (*x_thread_gettime)(void) = gettime;

xboolean x_threads_got_initialized = TRUE;

xboolean x_thread_get_initialized(void)
{
    return x_thread_supported();
}

XLIB_AVAILABLE_IN_ALL
void x_thread_init_xlib(void);
void x_thread_init_xlib(void) { }

static XSList *x_thread_all_threads = NULL;
static XSList *x_thread_free_indices = NULL;

X_LOCK_DEFINE_STATIC(x_static_mutex);
X_LOCK_DEFINE_STATIC(x_thread);

void x_thread_set_priority(XThread *thread, XThreadPriority priority)
{

}

void x_thread_foreach(XFunc thread_func, xpointer user_data)
{
    XRealThread *thread;
    XSList *slist = NULL;

    x_return_if_fail(thread_func != NULL);

    X_LOCK(x_thread);
    slist = x_slist_copy(x_thread_all_threads);
    X_UNLOCK(x_thread);

    while (slist) {
        XSList *node = slist;
        slist = node->next;

        X_LOCK(x_thread);
        if (x_slist_find(x_thread_all_threads, node->data)) {
            thread = (XRealThread *)node->data;
        } else {
            thread = NULL;
        }
        X_UNLOCK(x_thread);

        if (thread) {
            thread_func(thread, user_data);
        }

        x_slist_free_1(node);
    }
}

static void x_enumerable_thread_remove(xpointer data)
{
    XRealThread *thread = (XRealThread *)data;

    X_LOCK(x_thread);
    x_thread_all_threads = x_slist_remove(x_thread_all_threads, thread);
    X_UNLOCK(x_thread);
}

XPrivate enumerable_thread_private = X_PRIVATE_INIT(x_enumerable_thread_remove);

static void x_enumerable_thread_add(XRealThread *thread)
{
    X_LOCK(x_thread);
    x_thread_all_threads = x_slist_prepend(x_thread_all_threads, thread);
    X_UNLOCK(x_thread);

    x_private_set(&enumerable_thread_private, thread);
}

static xpointer x_deprecated_thread_proxy(xpointer data)
{
    XRealThread *real = (XRealThread *)data;

    x_enumerable_thread_add(real);
    return x_thread_proxy(data);
}

XThread *x_thread_create(XThreadFunc func, xpointer data, xboolean joinable, XError **error)
{
    return x_thread_create_full(func, data, 0, joinable, 0, (XThreadPriority)0, error);
}

XThread *x_thread_create_full(XThreadFunc func, xpointer data, xulong stack_size, xboolean joinable, xboolean bound, XThreadPriority priority, XError **error)
{
    XThread *thread;

    thread = x_thread_new_internal(NULL, x_deprecated_thread_proxy, func, data, stack_size, error);
    if (thread && !joinable) {
        thread->joinable = FALSE;
        x_thread_unref(thread);
    }

    return thread;
}

xboolean x_once_init_enter_impl(volatile xsize *location)
{
    return (x_once_init_enter)(location);
}

void x_static_mutex_init(XStaticMutex *mutex)
{
    static const XStaticMutex init_mutex = X_STATIC_MUTEX_INIT;

    x_return_if_fail(mutex);
    *mutex = init_mutex;
}

XMutex *x_static_mutex_get_mutex_impl(XStaticMutex *mutex)
{
    XMutex *result;

    if (!x_thread_supported()) {
        return NULL;
    }

    result = x_atomic_pointer_get(&mutex->mutex);
    if (!result) {
        X_LOCK(x_static_mutex);

        result = mutex->mutex;
        if (!result) {
            result = x_mutex_new();
            x_atomic_pointer_set(&mutex->mutex, result);
        }

        X_UNLOCK(x_static_mutex);
    }

    return result;
}

void x_static_mutex_free(XStaticMutex *mutex)
{
    XMutex **runtime_mutex;

    x_return_if_fail(mutex);

    runtime_mutex = ((XMutex **)mutex);
    if (*runtime_mutex) {
        x_mutex_free(*runtime_mutex);
    }
    *runtime_mutex = NULL;
}

void x_static_rec_mutex_init(XStaticRecMutex *mutex)
{
    static const XStaticRecMutex init_mutex = X_STATIC_REC_MUTEX_INIT;

    x_return_if_fail(mutex);
    *mutex = init_mutex;
}

static XRecMutex *x_static_rec_mutex_get_rec_mutex_impl(XStaticRecMutex *mutex)
{
    XRecMutex *result;

    if (!x_thread_supported()) {
        return NULL;
    }

    result = (XRecMutex *)x_atomic_pointer_get(&mutex->mutex.mutex);
    if (!result) {
        X_LOCK(x_static_mutex);

        result = (XRecMutex *) mutex->mutex.mutex;
        if (!result) {
            result = x_slice_new(XRecMutex);
            x_rec_mutex_init(result);
            x_atomic_pointer_set(&mutex->mutex.mutex, (XMutex *)result);
        }

        X_UNLOCK(x_static_mutex);
    }

    return result;
}

void x_static_rec_mutex_lock(XStaticRecMutex *mutex)
{
    XRecMutex *rm;

    rm = x_static_rec_mutex_get_rec_mutex_impl(mutex);
    x_rec_mutex_lock (rm);
    mutex->depth++;
}

xboolean x_static_rec_mutex_trylock(XStaticRecMutex *mutex)
{
    XRecMutex *rm;
    rm = x_static_rec_mutex_get_rec_mutex_impl(mutex);

    if (x_rec_mutex_trylock(rm)) {
        mutex->depth++;
        return TRUE;
    } else {
        return FALSE;
    }
}

void x_static_rec_mutex_unlock(XStaticRecMutex *mutex)
{
    XRecMutex *rm;

    rm = x_static_rec_mutex_get_rec_mutex_impl(mutex);
    mutex->depth--;
    x_rec_mutex_unlock(rm);
}

void x_static_rec_mutex_lock_full(XStaticRecMutex *mutex, xuint depth)
{
    XRecMutex *rm;

    rm = x_static_rec_mutex_get_rec_mutex_impl(mutex);
    while (depth--) {
        x_rec_mutex_lock(rm);
        mutex->depth++;
    }
}

xuint x_static_rec_mutex_unlock_full(XStaticRecMutex *mutex)
{
    xint i;
    xint depth;
    XRecMutex *rm;

    rm = x_static_rec_mutex_get_rec_mutex_impl(mutex);

    depth = mutex->depth;
    i = mutex->depth;
    mutex->depth = 0;

    while (i--) {
        x_rec_mutex_unlock(rm);
    }

    return depth;
}

void x_static_rec_mutex_free(XStaticRecMutex *mutex)
{
    x_return_if_fail(mutex);

    if (mutex->mutex.mutex) {
        XRecMutex *rm = (XRecMutex *)mutex->mutex.mutex;

        x_rec_mutex_clear(rm);
        x_slice_free(XRecMutex, rm);
    }
}

void x_static_rw_lock_init(XStaticRWLock *lock)
{
    static const XStaticRWLock init_lock = X_STATIC_RW_LOCK_INIT;

    x_return_if_fail(lock);
    *lock = init_lock;
}

inline static void x_static_rw_lock_wait(XCond **cond, XStaticMutex *mutex)
{
    if (!*cond) {
        *cond = x_cond_new();
    }

    x_cond_wait(*cond, x_static_mutex_get_mutex(mutex));
}

inline static void x_static_rw_lock_signal(XStaticRWLock *lock)
{
    if (lock->want_to_write && lock->write_cond) {
        x_cond_signal(lock->write_cond);
    } else if (lock->want_to_read && lock->read_cond) {
        x_cond_broadcast(lock->read_cond);
    }
}

void x_static_rw_lock_reader_lock(XStaticRWLock *lock)
{
    x_return_if_fail(lock);

    if (!x_threads_got_initialized) {
        return;
    }

    x_static_mutex_lock(&lock->mutex);
    lock->want_to_read++;
    while (lock->have_writer || lock->want_to_write) {
        x_static_rw_lock_wait(&lock->read_cond, &lock->mutex);
    }
    lock->want_to_read--;
    lock->read_counter++;
    x_static_mutex_unlock(&lock->mutex);
}

xboolean x_static_rw_lock_reader_trylock(XStaticRWLock *lock)
{
    xboolean ret_val = FALSE;

    x_return_val_if_fail(lock, FALSE);

    if (!x_threads_got_initialized) {
        return TRUE;
    }

    x_static_mutex_lock(&lock->mutex);
    if (!lock->have_writer && !lock->want_to_write) {
        lock->read_counter++;
        ret_val = TRUE;
    }
    x_static_mutex_unlock(&lock->mutex);
    return ret_val;
}

void x_static_rw_lock_reader_unlock(XStaticRWLock *lock)
{
    x_return_if_fail(lock);

    if (!x_threads_got_initialized) {
        return;
    }

    x_static_mutex_lock(&lock->mutex);
    lock->read_counter--;
    if (lock->read_counter == 0) {
        x_static_rw_lock_signal(lock);
    }
    x_static_mutex_unlock(&lock->mutex);
}

void x_static_rw_lock_writer_lock(XStaticRWLock *lock)
{
    x_return_if_fail(lock);

    if (!x_threads_got_initialized) {
        return;
    }

    x_static_mutex_lock(&lock->mutex);
    lock->want_to_write++;
    while (lock->have_writer || lock->read_counter) {
        x_static_rw_lock_wait(&lock->write_cond, &lock->mutex);
    }
    lock->want_to_write--;
    lock->have_writer = TRUE;
    x_static_mutex_unlock(&lock->mutex);
}

xboolean x_static_rw_lock_writer_trylock(XStaticRWLock *lock)
{
    xboolean ret_val = FALSE;

    x_return_val_if_fail(lock, FALSE);

    if (!x_threads_got_initialized) {
        return TRUE;
    }

    x_static_mutex_lock(&lock->mutex);
    if (!lock->have_writer && !lock->read_counter) {
        lock->have_writer = TRUE;
        ret_val = TRUE;
    }
    x_static_mutex_unlock(&lock->mutex);
    return ret_val;
}

void x_static_rw_lock_writer_unlock(XStaticRWLock *lock)
{
    x_return_if_fail(lock);

    if (!x_threads_got_initialized) {
        return;
    }

    x_static_mutex_lock(&lock->mutex);
    lock->have_writer = FALSE;
    x_static_rw_lock_signal(lock);
    x_static_mutex_unlock(&lock->mutex);
}

void x_static_rw_lock_free(XStaticRWLock *lock)
{
    x_return_if_fail(lock);

    if (lock->read_cond) {
        x_cond_free(lock->read_cond);
        lock->read_cond = NULL;
    }

    if (lock->write_cond) {
        x_cond_free(lock->write_cond);
        lock->write_cond = NULL;
    }
    x_static_mutex_free(&lock->mutex);
}

XPrivate *x_private_new(XDestroyNotify notify)
{
    XPrivate tmp = X_PRIVATE_INIT(notify);
    XPrivate *key;

    key = x_slice_new(XPrivate);
    *key = tmp;

    return key;
}

typedef struct _XStaticPrivateNode XStaticPrivateNode;
struct _XStaticPrivateNode {
    xpointer       data;
    XDestroyNotify destroy;
    XStaticPrivate *owner;
};

static void x_static_private_cleanup(xpointer data)
{
    xuint i;
    XArray *array = (XArray *)data;

    for (i = 0; i < array->len; i++) {
        XStaticPrivateNode *node = &x_array_index(array, XStaticPrivateNode, i);
        if (node->destroy) {
            node->destroy(node->data);
        }
    }

    x_array_free(array, TRUE);
}

XPrivate static_private_private = X_PRIVATE_INIT(x_static_private_cleanup);

void x_static_private_init(XStaticPrivate *private_key)
{
    private_key->index = 0;
}

xpointer x_static_private_get(XStaticPrivate *private_key)
{
    XArray *array;
    xpointer ret = NULL;

    array = (XArray *)x_private_get(&static_private_private);

    if (array && private_key->index != 0 && private_key->index <= array->len) {
        XStaticPrivateNode *node;

        node = &x_array_index(array, XStaticPrivateNode, private_key->index - 1);
        if (X_UNLIKELY(node->owner != private_key)) {
            if (node->destroy) {
                node->destroy(node->data);
            }
    
            node->destroy = NULL;
            node->data = NULL;
            node->owner = NULL;
        }

        ret = node->data;
    }

    return ret;
}

void x_static_private_set(XStaticPrivate *private_key, xpointer data, XDestroyNotify notify)
{
    XArray *array;
    XStaticPrivateNode *node;
    static xuint next_index = 0;

    if (!private_key->index) {
        X_LOCK(x_thread);

        if (!private_key->index) {
            if (x_thread_free_indices) {
                private_key->index = XPOINTER_TO_UINT(x_thread_free_indices->data);
                x_thread_free_indices = x_slist_delete_link(x_thread_free_indices, x_thread_free_indices);
            } else {
                private_key->index = ++next_index;
            }
        }

        X_UNLOCK(x_thread);
    }

    array = (XArray *)x_private_get(&static_private_private);
    if (!array) {
        array = x_array_new(FALSE, TRUE, sizeof(XStaticPrivateNode));
        x_private_set(&static_private_private, array);
    }

    if (private_key->index > array->len) {
        x_array_set_size(array, private_key->index);
    }

    node = &x_array_index(array, XStaticPrivateNode, private_key->index - 1);
    if (node->destroy) {
        node->destroy(node->data);
    }

    node->data = data;
    node->destroy = notify;
    node->owner = private_key;
}

void x_static_private_free(XStaticPrivate *private_key)
{
    xuint idx = private_key->index;

    if (!idx) {
        return;
    }

    private_key->index = 0;

    X_LOCK(x_thread);
    x_thread_free_indices = x_slist_prepend(x_thread_free_indices, XUINT_TO_POINTER(idx));
    X_UNLOCK(x_thread);
}

XMutex *x_mutex_new(void)
{
    XMutex *mutex;

    mutex = x_slice_new(XMutex);
    x_mutex_init(mutex);

    return mutex;
}

void x_mutex_free(XMutex *mutex)
{
    x_mutex_clear(mutex);
    x_slice_free(XMutex, mutex);
}

XCond *x_cond_new(void)
{
    XCond *cond;

    cond = x_slice_new(XCond);
    x_cond_init(cond);

    return cond;
}

void x_cond_free(XCond *cond)
{
    x_cond_clear(cond);
    x_slice_free(XCond, cond);
}

xboolean x_cond_timed_wait(XCond *cond, XMutex *mutex, XTimeVal *abs_time)
{
    xint64 end_time;

    if (abs_time == NULL) {
        x_cond_wait(cond, mutex);
        return TRUE;
    }

    end_time = abs_time->tv_sec;
    end_time *= 1000000;
    end_time += abs_time->tv_usec;

    end_time += x_get_monotonic_time() - x_get_real_time();

    return x_cond_wait_until(cond, mutex, end_time);
}
