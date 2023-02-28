#include <xlib/xlib/config.h>
#include <xlib/xlib/xmem.h>
#include <xlib/xlib/xmain.h>
#include <xlib/xlib/xqueue.h>
#include <xlib/xlib/xtimer.h>
#include <xlib/xlib/xtypes.h>
#include <xlib/xlib/xthread.h>
#include <xlib/xlib/xtestutils.h>
#include <xlib/xlib/xasyncqueue.h>
#include <xlib/xlib/xasyncqueueprivate.h>
#include <xlib/xlib/deprecated/xthread.h>

struct _XAsyncQueue {
    XMutex         mutex;
    XCond          cond;
    XQueue         queue;
    XDestroyNotify item_free_func;
    xuint          waiting_threads;
    xint           ref_count;
};

typedef struct {
    XCompareDataFunc func;
    xpointer         user_data;
} SortData;

XAsyncQueue *x_async_queue_new(void)
{
    return x_async_queue_new_full(NULL);
}

XAsyncQueue *x_async_queue_new_full(XDestroyNotify item_free_func)
{
    XAsyncQueue *queue;

    queue = x_new(XAsyncQueue, 1);
    x_mutex_init(&queue->mutex);
    x_cond_init(&queue->cond);
    x_queue_init(&queue->queue);
    queue->waiting_threads = 0;
    queue->ref_count = 1;
    queue->item_free_func = item_free_func;

    return queue;
}

XAsyncQueue *x_async_queue_ref(XAsyncQueue *queue)
{
    x_return_val_if_fail(queue, NULL);

    x_atomic_int_inc(&queue->ref_count);
    return queue;
}

void x_async_queue_ref_unlocked(XAsyncQueue *queue)
{
    x_return_if_fail(queue);
    x_atomic_int_inc(&queue->ref_count);
}

void x_async_queue_unref_and_unlock(XAsyncQueue *queue)
{
    x_return_if_fail(queue);

    x_mutex_unlock(&queue->mutex);
    x_async_queue_unref(queue);
}

void x_async_queue_unref(XAsyncQueue *queue)
{
    x_return_if_fail(queue);

    if (x_atomic_int_dec_and_test(&queue->ref_count)) {
        x_return_if_fail(queue->waiting_threads == 0);
        x_mutex_clear(&queue->mutex);
        x_cond_clear(&queue->cond);
        if (queue->item_free_func) {
            x_queue_foreach(&queue->queue, (XFunc)queue->item_free_func, NULL);
        }

        x_queue_clear(&queue->queue);
        x_free(queue);
    }
}

void x_async_queue_lock(XAsyncQueue *queue)
{
    x_return_if_fail (queue);
    x_mutex_lock (&queue->mutex);
}

void x_async_queue_unlock(XAsyncQueue *queue)
{
    x_return_if_fail (queue);
    x_mutex_unlock (&queue->mutex);
}

void x_async_queue_push(XAsyncQueue *queue, xpointer data)
{
    x_return_if_fail(queue);
    x_return_if_fail(data);

    x_mutex_lock(&queue->mutex);
    x_async_queue_push_unlocked(queue, data);
    x_mutex_unlock(&queue->mutex);
}

void x_async_queue_push_unlocked(XAsyncQueue *queue, xpointer data)
{
    x_return_if_fail(queue);
    x_return_if_fail(data);

    x_queue_push_head(&queue->queue, data);
    if (queue->waiting_threads > 0) {
        x_cond_signal(&queue->cond);
    }
}

void x_async_queue_push_sorted(XAsyncQueue *queue, xpointer data, XCompareDataFunc func, xpointer user_data)
{
    x_return_if_fail(queue != NULL);

    x_mutex_lock(&queue->mutex);
    x_async_queue_push_sorted_unlocked(queue, data, func, user_data);
    x_mutex_unlock(&queue->mutex);
}

static xint x_async_queue_invert_compare(xpointer v1, xpointer v2, SortData *sd)
{
    return -sd->func(v1, v2, sd->user_data);
}

void x_async_queue_push_sorted_unlocked(XAsyncQueue *queue, xpointer data, XCompareDataFunc func, xpointer user_data)
{
    SortData sd;

    x_return_if_fail(queue != NULL);

    sd.func = func;
    sd.user_data = user_data;

    x_queue_insert_sorted(&queue->queue, data, (XCompareDataFunc)x_async_queue_invert_compare, &sd);
    if (queue->waiting_threads > 0) {
        x_cond_signal(&queue->cond);
    }
}

static xpointer x_async_queue_pop_intern_unlocked(XAsyncQueue *queue, xboolean wait, xint64 end_time)
{
    xpointer retval;

    if (!x_queue_peek_tail_link(&queue->queue) && wait) {
        queue->waiting_threads++;
        while (!x_queue_peek_tail_link (&queue->queue)) {
            if (end_time == -1) {
                x_cond_wait(&queue->cond, &queue->mutex);
            } else {
                if (!x_cond_wait_until(&queue->cond, &queue->mutex, end_time)) {
                    break;
                }
            }
        }
        queue->waiting_threads--;
    }

    retval = x_queue_pop_tail(&queue->queue);
    x_assert(retval || !wait || end_time > 0);

    return retval;
}

xpointer x_async_queue_pop(XAsyncQueue *queue)
{
    xpointer retval;

    x_return_val_if_fail(queue, NULL);

    x_mutex_lock(&queue->mutex);
    retval = x_async_queue_pop_intern_unlocked(queue, TRUE, -1);
    x_mutex_unlock(&queue->mutex);

    return retval;
}

xpointer x_async_queue_pop_unlocked(XAsyncQueue *queue)
{
    x_return_val_if_fail(queue, NULL);
    return x_async_queue_pop_intern_unlocked(queue, TRUE, -1);
}

xpointer x_async_queue_try_pop(XAsyncQueue *queue)
{
    xpointer retval;

    x_return_val_if_fail(queue, NULL);

    x_mutex_lock(&queue->mutex);
    retval = x_async_queue_pop_intern_unlocked(queue, FALSE, -1);
    x_mutex_unlock(&queue->mutex);

    return retval;
}

xpointer x_async_queue_try_pop_unlocked(XAsyncQueue *queue)
{
    x_return_val_if_fail(queue, NULL);
    return x_async_queue_pop_intern_unlocked(queue, FALSE, -1);
}

xpointer x_async_queue_timeout_pop(XAsyncQueue *queue, xuint64 timeout)
{
    xpointer retval;
    xint64 end_time = x_get_monotonic_time() + timeout;

    x_return_val_if_fail (queue != NULL, NULL);

    x_mutex_lock(&queue->mutex);
    retval = x_async_queue_pop_intern_unlocked(queue, TRUE, end_time);
    x_mutex_unlock(&queue->mutex);

    return retval;
}

xpointer x_async_queue_timeout_pop_unlocked(XAsyncQueue *queue, xuint64 timeout)
{
    xint64 end_time = x_get_monotonic_time() + timeout;
    x_return_val_if_fail(queue != NULL, NULL);

    return x_async_queue_pop_intern_unlocked(queue, TRUE, end_time);
}

X_GNUC_BEGIN_IGNORE_DEPRECATIONS

xpointer x_async_queue_timed_pop(XAsyncQueue *queue, XTimeVal *end_time)
{
    xpointer retval;
    xint64 m_end_time;

    x_return_val_if_fail(queue, NULL);

    if (end_time != NULL) {
        m_end_time = x_get_monotonic_time() + ((xint64)end_time->tv_sec * X_USEC_PER_SEC + end_time->tv_usec - x_get_real_time());
    } else {
        m_end_time = -1;
    }

    x_mutex_lock(&queue->mutex);
    retval = x_async_queue_pop_intern_unlocked(queue, TRUE, m_end_time);
    x_mutex_unlock(&queue->mutex);

    return retval;
}

X_GNUC_END_IGNORE_DEPRECATIONS

X_GNUC_BEGIN_IGNORE_DEPRECATIONS

xpointer x_async_queue_timed_pop_unlocked(XAsyncQueue *queue, XTimeVal *end_time)
{
    xint64 m_end_time;

    x_return_val_if_fail (queue, NULL);

    if (end_time != NULL) {
        m_end_time = x_get_monotonic_time() + ((xint64)end_time->tv_sec * X_USEC_PER_SEC + end_time->tv_usec - x_get_real_time());
    } else {
        m_end_time = -1;
    }

    return x_async_queue_pop_intern_unlocked(queue, TRUE, m_end_time);
}

X_GNUC_END_IGNORE_DEPRECATIONS

xint x_async_queue_length(XAsyncQueue *queue)
{
    xint retval;

    x_return_val_if_fail (queue, 0);

    x_mutex_lock(&queue->mutex);
    retval = queue->queue.length - queue->waiting_threads;
    x_mutex_unlock(&queue->mutex);

    return retval;
}

xint x_async_queue_length_unlocked(XAsyncQueue *queue)
{
    x_return_val_if_fail(queue, 0);
    return queue->queue.length - queue->waiting_threads;
}

void x_async_queue_sort(XAsyncQueue *queue, XCompareDataFunc func, xpointer user_data)
{
    x_return_if_fail(queue != NULL);
    x_return_if_fail(func != NULL);

    x_mutex_lock(&queue->mutex);
    x_async_queue_sort_unlocked(queue, func, user_data);
    x_mutex_unlock(&queue->mutex);
}

void x_async_queue_sort_unlocked(XAsyncQueue *queue, XCompareDataFunc func, xpointer user_data)
{
    SortData sd;

    x_return_if_fail(queue != NULL);
    x_return_if_fail(func != NULL);

    sd.func = func;
    sd.user_data = user_data;

    x_queue_sort(&queue->queue, (XCompareDataFunc)x_async_queue_invert_compare, &sd);
}

xboolean x_async_queue_remove(XAsyncQueue *queue, xpointer item)
{
    xboolean ret;

    x_return_val_if_fail(queue != NULL, FALSE);
    x_return_val_if_fail(item != NULL, FALSE);

    x_mutex_lock(&queue->mutex);
    ret = x_async_queue_remove_unlocked(queue, item);
    x_mutex_unlock(&queue->mutex);

    return ret;
}

xboolean x_async_queue_remove_unlocked(XAsyncQueue *queue, xpointer item)
{
    x_return_val_if_fail(queue != NULL, FALSE);
    x_return_val_if_fail(item != NULL, FALSE);

    return x_queue_remove(&queue->queue, item);
}

void x_async_queue_push_front(XAsyncQueue *queue, xpointer item)
{
    x_return_if_fail(queue != NULL);
    x_return_if_fail(item != NULL);

    x_mutex_lock(&queue->mutex);
    x_async_queue_push_front_unlocked(queue, item);
    x_mutex_unlock(&queue->mutex);
}

void x_async_queue_push_front_unlocked(XAsyncQueue *queue, xpointer item)
{
    x_return_if_fail(queue != NULL);
    x_return_if_fail(item != NULL);

    x_queue_push_tail(&queue->queue, item);
    if (queue->waiting_threads > 0) {
        x_cond_signal(&queue->cond);
    }
}

XMutex *_x_async_queue_get_mutex(XAsyncQueue *queue)
{
    x_return_val_if_fail(queue, NULL);
    return &queue->mutex;
}
