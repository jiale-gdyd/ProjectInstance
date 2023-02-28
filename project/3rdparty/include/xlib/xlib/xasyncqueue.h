#ifndef __X_ASYNCQUEUE_H__
#define __X_ASYNCQUEUE_H__

#include "xthread.h"

X_BEGIN_DECLS

typedef struct _XAsyncQueue XAsyncQueue;

XLIB_AVAILABLE_IN_ALL
XAsyncQueue *x_async_queue_new(void);

XLIB_AVAILABLE_IN_ALL
XAsyncQueue *x_async_queue_new_full(XDestroyNotify item_free_func);

XLIB_AVAILABLE_IN_ALL
void x_async_queue_lock(XAsyncQueue *queue);

XLIB_AVAILABLE_IN_ALL
void x_async_queue_unlock(XAsyncQueue *queue);

XLIB_AVAILABLE_IN_ALL
XAsyncQueue *x_async_queue_ref(XAsyncQueue *queue);

XLIB_AVAILABLE_IN_ALL
void x_async_queue_unref(XAsyncQueue *queue);

XLIB_DEPRECATED_FOR(x_async_queue_ref)
void x_async_queue_ref_unlocked(XAsyncQueue *queue);

XLIB_DEPRECATED_FOR(x_async_queue_unref)
void x_async_queue_unref_and_unlock(XAsyncQueue *queue);

XLIB_AVAILABLE_IN_ALL
void x_async_queue_push(XAsyncQueue *queue, xpointer data);

XLIB_AVAILABLE_IN_ALL
void x_async_queue_push_unlocked(XAsyncQueue *queue, xpointer data);

XLIB_AVAILABLE_IN_ALL
void x_async_queue_push_sorted(XAsyncQueue *queue, xpointer data, XCompareDataFunc func, xpointer user_data);

XLIB_AVAILABLE_IN_ALL
void x_async_queue_push_sorted_unlocked(XAsyncQueue *queue, xpointer data, XCompareDataFunc func, xpointer user_data);

XLIB_AVAILABLE_IN_ALL
xpointer x_async_queue_pop(XAsyncQueue *queue);

XLIB_AVAILABLE_IN_ALL
xpointer x_async_queue_pop_unlocked(XAsyncQueue *queue);

XLIB_AVAILABLE_IN_ALL
xpointer x_async_queue_try_pop(XAsyncQueue *queue);

XLIB_AVAILABLE_IN_ALL
xpointer x_async_queue_try_pop_unlocked(XAsyncQueue *queue);

XLIB_AVAILABLE_IN_ALL
xpointer x_async_queue_timeout_pop(XAsyncQueue *queue, xuint64 timeout);

XLIB_AVAILABLE_IN_ALL
xpointer x_async_queue_timeout_pop_unlocked(XAsyncQueue *queue, xuint64 timeout);

XLIB_AVAILABLE_IN_ALL
xint x_async_queue_length(XAsyncQueue *queue);

XLIB_AVAILABLE_IN_ALL
xint x_async_queue_length_unlocked(XAsyncQueue *queue);

XLIB_AVAILABLE_IN_ALL
void x_async_queue_sort(XAsyncQueue *queue, XCompareDataFunc func, xpointer user_data);

XLIB_AVAILABLE_IN_ALL
void x_async_queue_sort_unlocked(XAsyncQueue *queue, XCompareDataFunc func, xpointer user_data);

XLIB_AVAILABLE_IN_2_46
xboolean x_async_queue_remove(XAsyncQueue *queue, xpointer item);

XLIB_AVAILABLE_IN_2_46
xboolean x_async_queue_remove_unlocked(XAsyncQueue *queue, xpointer item);

XLIB_AVAILABLE_IN_2_46
void x_async_queue_push_front(XAsyncQueue *queue, xpointer item);

XLIB_AVAILABLE_IN_2_46
void x_async_queue_push_front_unlocked(XAsyncQueue *queue, xpointer item);

X_GNUC_BEGIN_IGNORE_DEPRECATIONS

XLIB_DEPRECATED_FOR(x_async_queue_timeout_pop)
xpointer x_async_queue_timed_pop(XAsyncQueue *queue, XTimeVal *end_time);

XLIB_DEPRECATED_FOR(x_async_queue_timeout_pop_unlocked)
xpointer x_async_queue_timed_pop_unlocked(XAsyncQueue *queue, XTimeVal *end_time);

X_GNUC_END_IGNORE_DEPRECATIONS

X_END_DECLS

#endif
