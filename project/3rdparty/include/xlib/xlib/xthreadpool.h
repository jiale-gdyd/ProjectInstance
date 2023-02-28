#ifndef __X_THREADPOOL_H__
#define __X_THREADPOOL_H__

#include "xthread.h"

X_BEGIN_DECLS

typedef struct _XThreadPool XThreadPool;

struct _XThreadPool {
    XFunc    func;
    xpointer user_data;
    xboolean exclusive;
};

XLIB_AVAILABLE_IN_ALL
XThreadPool *x_thread_pool_new(XFunc func, xpointer user_data, xint max_threads, xboolean exclusive, XError **error);

XLIB_AVAILABLE_IN_2_70
XThreadPool *x_thread_pool_new_full(XFunc func, xpointer user_data, XDestroyNotify item_free_func, xint max_threads, xboolean exclusive, XError **error);

XLIB_AVAILABLE_IN_ALL
void x_thread_pool_free(XThreadPool *pool, xboolean immediate, xboolean wait_);

XLIB_AVAILABLE_IN_ALL
xboolean x_thread_pool_push(XThreadPool *pool, xpointer data, XError **error);

XLIB_AVAILABLE_IN_ALL
xuint x_thread_pool_unprocessed(XThreadPool *pool);

XLIB_AVAILABLE_IN_ALL
void x_thread_pool_set_sort_function(XThreadPool *pool, XCompareDataFunc func, xpointer user_data);

XLIB_AVAILABLE_IN_2_46
xboolean x_thread_pool_move_to_front(XThreadPool *pool, xpointer data);

XLIB_AVAILABLE_IN_ALL
xboolean x_thread_pool_set_max_threads(XThreadPool *pool, xint max_threads, XError **error);

XLIB_AVAILABLE_IN_ALL
xint x_thread_pool_get_max_threads(XThreadPool *pool);

XLIB_AVAILABLE_IN_ALL
xuint x_thread_pool_get_num_threads(XThreadPool *pool);

XLIB_AVAILABLE_IN_ALL
void x_thread_pool_set_max_unused_threads(xint max_threads);

XLIB_AVAILABLE_IN_ALL
xint x_thread_pool_get_max_unused_threads(void);

XLIB_AVAILABLE_IN_ALL
xuint x_thread_pool_get_num_unused_threads(void);

XLIB_AVAILABLE_IN_ALL
void x_thread_pool_stop_unused_threads(void);

XLIB_AVAILABLE_IN_ALL
void x_thread_pool_set_max_idle_time(xuint interval);

XLIB_AVAILABLE_IN_ALL
xuint x_thread_pool_get_max_idle_time(void);

X_END_DECLS

#endif
