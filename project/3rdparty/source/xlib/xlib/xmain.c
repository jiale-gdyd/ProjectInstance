#include <time.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <linux/wait.h>
#include <sys/syscall.h>
#include <sys/eventfd.h>

#include <xlib/xlib/config.h>
#include <xlib/xlib/xlibconfig.h>
#include <xlib/xlib/xlib_trace.h>

#include <xlib/xlib/xmain.h>
#include <xlib/xlib/xhash.h>
#include <xlib/xlib/xhook.h>
#include <xlib/xlib/xqueue.h>
#include <xlib/xlib/xarray.h>
#include <xlib/xlib/xwakeup.h>
#include <xlib/xlib/xstrfuncs.h>
#include <xlib/xlib/xlib-init.h>
#include <xlib/xlib/xtestutils.h>
#include <xlib/xlib/xiochannel.h>
#include <xlib/xlib/xlib-private.h>
#include <xlib/xlib/xthreadprivate.h>
#include <xlib/xlib/xtrace-private.h>
#include <xlib/xlib/xmain-internal.h>

#ifndef W_EXITCODE
#define W_EXITCODE(ret, sig)            ((ret) << 8 | (sig))
#endif

#ifndef W_STOPCODE
#define W_STOPCODE(sig)                 ((sig) << 8 | 0x7f)
#endif

#ifndef WCOREFLAG
#define WCOREFLAG                       0x80
#endif

#ifndef __W_CONTINUED
#define __W_CONTINUED                   0xffff
#endif

typedef struct _XPollRec XPollRec;
typedef struct _XIdleSource XIdleSource;
typedef struct _XTimeoutSource XTimeoutSource;
typedef struct _XSourceCallback XSourceCallback;
typedef struct _XChildWatchSource XChildWatchSource;
typedef struct _XUnixSignalWatchSource XUnixSignalWatchSource;

typedef enum {
    X_SOURCE_READY       = 1 << X_HOOK_FLAG_USER_SHIFT,
    X_SOURCE_CAN_RECURSE = 1 << (X_HOOK_FLAG_USER_SHIFT + 1),
    X_SOURCE_BLOCKED     = 1 << (X_HOOK_FLAG_USER_SHIFT + 2)
} XSourceFlags;

typedef struct _XSourceList XSourceList;

struct _XSourceList {
    XSource *head, *tail;
    xint    priority;
};

typedef struct _XMainWaiter XMainWaiter;

struct _XMainWaiter {
    XCond  *cond;
    XMutex *mutex;
};

typedef struct _XMainDispatch XMainDispatch;

struct _XMainDispatch {
    xint    depth;
    XSource *source;
};

struct _XMainContext {
    XMutex            mutex;
    XCond             cond;
    XThread           *owner;
    xuint             owner_count;
    XMainContextFlags flags;
    XSList            *waiters;
    xint              ref_count;
    XHashTable        *sources;
    XPtrArray         *pending_dispatches;
    xint              timeout;
    xuint             next_id;
    XList             *source_lists;
    xint              in_check_or_prepare;
    XPollRec          *poll_records;
    xuint             n_poll_records;
    XPollFD           *cached_poll_array;
    xuint             cached_poll_array_size;
    XWakeup           *wakeup;
    XPollFD           wake_up_rec;
    xboolean          poll_changed;
    XPollFunc         poll_func;
    xint64            time;
    xboolean          time_is_fresh;
};

struct _XSourceCallback {
    xint           ref_count;
    XSourceFunc    func;
    xpointer       data;
    XDestroyNotify notify;
};

struct _XMainLoop {
    XMainContext *context;
    xboolean     is_running;
    xint         ref_count;
};

struct _XIdleSource {
    XSource  source;
    xboolean one_shot;
};

struct _XTimeoutSource {
    XSource  source;
    xuint    interval;
    xboolean seconds;
    xboolean one_shot;
};

struct _XChildWatchSource {
    XSource  source;
    XPid     pid;
    xint     child_status;
    XPollFD  poll;
    xboolean child_exited;
    xboolean using_pidfd;
};

struct _XUnixSignalWatchSource {
    XSource  source;
    int      signum;
    xboolean pending;
};

struct _XPollRec {
    XPollFD  *fd;
    XPollRec *prev;
    XPollRec *next;
    xint     priority;
};

struct _XSourcePrivate {
    XSList             *child_sources;
    XSource            *parent_source;
    xint64             ready_time;
    XSList             *fds;
    XSourceDisposeFunc dispose;
    xboolean           static_name;
};

typedef struct _XSourceIter {
    XMainContext *context;
    xboolean     may_modify;
    XList        *current_list;
    XSource      *source;
} XSourceIter;

#define LOCK_CONTEXT(context)           x_mutex_lock(&context->mutex)
#define UNLOCK_CONTEXT(context)         x_mutex_unlock(&context->mutex)
#define X_THREAD_SELF                   x_thread_self()

#define SOURCE_DESTROYED(source)        (((source)->flags & X_HOOK_FLAG_ACTIVE) == 0)
#define SOURCE_BLOCKED(source)          (((source)->flags & X_SOURCE_BLOCKED) != 0)

static void x_source_unref_internal(XSource *source, XMainContext *context, xboolean have_lock);
static void x_source_destroy_internal(XSource *source, XMainContext *context, xboolean have_lock);
static void x_source_set_priority_unlocked(XSource *source, XMainContext *context, xint priority);

static void x_child_source_remove_internal(XSource *child_source, XMainContext *context);

static void x_main_context_poll(XMainContext *context, xint timeout, xint priority, XPollFD *fds, xint n_fds);
static void x_main_context_add_poll_unlocked(XMainContext *context, xint priority, XPollFD *fd);
static void x_main_context_remove_poll_unlocked(XMainContext *context, XPollFD *fd);

static void x_source_iter_init(XSourceIter *iter, XMainContext *context, xboolean may_modify);

static xboolean x_source_iter_next(XSourceIter *iter, XSource **source);
static void x_source_iter_clear(XSourceIter *iter);

static xboolean x_timeout_dispatch(XSource *source, XSourceFunc callback, xpointer user_data);

static xboolean x_child_watch_check(XSource *source);
static xboolean x_child_watch_prepare(XSource *source, xint *timeout);
static xboolean x_child_watch_dispatch(XSource *source, XSourceFunc callback, xpointer user_data);

static void x_child_watch_finalize(XSource *source);

static void x_unix_signal_handler(int signum);
static void x_unix_signal_watch_finalize(XSource *source);
static xboolean x_unix_signal_watch_check(XSource *source);
static xboolean x_unix_signal_watch_prepare(XSource *source, xint *timeout);
static xboolean x_unix_signal_watch_dispatch(XSource *source, XSourceFunc callback, xpointer user_data);

static xboolean x_idle_check(XSource *source);
static xboolean x_idle_prepare(XSource *source, xint *timeout);
static xboolean x_idle_dispatch(XSource *source, XSourceFunc callback, xpointer user_data);

static void block_source(XSource *source);

static XMainContext *xlib_worker_context;

#if (defined(X_ATOMIC_LOCK_FREE) && defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4)) || !defined(HAVE_SIG_ATOMIC_T)
static volatile int any_unix_signal_pending;
static volatile int unix_signal_pending[NSIG];
#else
static volatile sig_atomic_t any_unix_signal_pending;
static volatile sig_atomic_t unix_signal_pending[NSIG];
#endif

X_LOCK_DEFINE_STATIC(unix_signal_lock);

static XSList *unix_child_watches;
static XSList *unix_signal_watches;
static xuint unix_signal_refcount[NSIG];

XSourceFuncs x_unix_signal_funcs = {
    x_unix_signal_watch_prepare,
    x_unix_signal_watch_check,
    x_unix_signal_watch_dispatch,
    x_unix_signal_watch_finalize,
    NULL, NULL
};

X_LOCK_DEFINE_STATIC(main_context_list);
static XSList *main_context_list = NULL;

XSourceFuncs x_timeout_funcs = {
    NULL,
    NULL,
    x_timeout_dispatch,
    NULL, NULL, NULL
};

XSourceFuncs x_child_watch_funcs = {
    x_child_watch_prepare,
    x_child_watch_check,
    x_child_watch_dispatch,
    x_child_watch_finalize,
    NULL, NULL
};

XSourceFuncs x_idle_funcs = {
    x_idle_prepare,
    x_idle_check,
    x_idle_dispatch,
    NULL, NULL, NULL
};

XMainContext *x_main_context_ref(XMainContext *context)
{
    int old_ref_count;

    x_return_val_if_fail(context != NULL, NULL);
    old_ref_count = x_atomic_int_add(&context->ref_count, 1);
    x_return_val_if_fail(old_ref_count > 0, NULL);

    return context;
}

static inline void poll_rec_list_free(XMainContext *context, XPollRec *list)
{
    x_slice_free_chain(XPollRec, list, next);
}

void x_main_context_unref(XMainContext *context)
{
    xuint i;
    XList *sl_iter;
    XSource *source;
    XSourceIter iter;
    XSourceList *list;
    XSList *s_iter, *remaining_sources = NULL;

    x_return_if_fail(context != NULL);
    x_return_if_fail(x_atomic_int_get(&context->ref_count) > 0);

    if (!x_atomic_int_dec_and_test(&context->ref_count)) {
        return;
    }

    X_LOCK(main_context_list);
    main_context_list = x_slist_remove(main_context_list, context);
    X_UNLOCK(main_context_list);

    for (i = 0; i < context->pending_dispatches->len; i++) {
        x_source_unref_internal((XSource *)context->pending_dispatches->pdata[i], context, FALSE);
    }

    LOCK_CONTEXT(context);

    x_source_iter_init(&iter, context, FALSE);
    while (x_source_iter_next(&iter, &source)) {
        source->context = NULL;
        remaining_sources = x_slist_prepend(remaining_sources, x_source_ref(source));
    }
    x_source_iter_clear(&iter);

    for (s_iter = remaining_sources; s_iter; s_iter = s_iter->next) {
        source = (XSource *)s_iter->data;
        x_source_destroy_internal(source, context, TRUE);
    }

    for (sl_iter = context->source_lists; sl_iter; sl_iter = sl_iter->next) {
        list = (XSourceList *)sl_iter->data;
        x_slice_free(XSourceList, list);
    }
    x_list_free(context->source_lists);

    x_hash_table_destroy(context->sources);

    UNLOCK_CONTEXT(context);
    x_mutex_clear(&context->mutex);

    x_ptr_array_free(context->pending_dispatches, TRUE);
    x_free(context->cached_poll_array);

    poll_rec_list_free(context, context->poll_records);

    x_wakeup_free(context->wakeup);
    x_cond_clear(&context->cond);

    x_free(context);

    for (s_iter = remaining_sources; s_iter; s_iter = s_iter->next) {
        source = (XSource *)s_iter->data;
        x_source_unref_internal(source, NULL, FALSE);
    }
    x_slist_free(remaining_sources);
}

XMainContext *x_main_context_new_with_next_id(xuint next_id)
{
    XMainContext *ret = x_main_context_new();
    ret->next_id = next_id;

    return ret;
}

XMainContext *x_main_context_new(void)
{
    return x_main_context_new_with_flags(X_MAIN_CONTEXT_FLAGS_NONE);
}

XMainContext *x_main_context_new_with_flags(XMainContextFlags flags)
{
    XMainContext *context;
    static xsize initialised;

    if (x_once_init_enter(&initialised)) {
        x_once_init_leave(&initialised, TRUE);
    }

    context = x_new0(XMainContext, 1);

    TRACE(XLIB_MAIN_CONTEXT_NEW(context));

    x_mutex_init(&context->mutex);
    x_cond_init(&context->cond);

    context->sources = x_hash_table_new(NULL, NULL);
    context->owner = NULL;
    context->flags = flags;
    context->waiters = NULL;
    context->ref_count = 1;
    context->next_id = 1;
    context->source_lists = NULL;
    context->poll_func = x_poll;
    context->cached_poll_array = NULL;
    context->cached_poll_array_size = 0;
    context->pending_dispatches = x_ptr_array_new();
    context->time_is_fresh = FALSE;
    context->wakeup = x_wakeup_new();
    x_wakeup_get_pollfd(context->wakeup, &context->wake_up_rec);
    x_main_context_add_poll_unlocked(context, 0, &context->wake_up_rec);

    X_LOCK(main_context_list);
    main_context_list = x_slist_append(main_context_list, context);
    X_UNLOCK(main_context_list);

    return context;
}

XMainContext *x_main_context_default(void)
{
    static XMainContext *default_main_context = NULL;

    if (x_once_init_enter(&default_main_context)) {
        XMainContext *context;

        context = x_main_context_new();
        TRACE(XLIB_MAIN_CONTEXT_DEFAULT(context));
        x_once_init_leave(&default_main_context, context);
    }

    return default_main_context;
}

static void free_context(xpointer data)
{
    XMainContext *context = (XMainContext *)data;

    TRACE(XLIB_MAIN_CONTEXT_FREE(context));

    x_main_context_release(context);
    if (context) {
        x_main_context_unref(context);
    }
}

static void free_context_stack(xpointer data)
{
    x_queue_free_full((XQueue *)data, (XDestroyNotify)free_context);
}

static XPrivate thread_context_stack = X_PRIVATE_INIT(free_context_stack);

void x_main_context_push_thread_default(XMainContext *context)
{
    XQueue *stack;
    xboolean acquired_context;

    acquired_context = x_main_context_acquire(context);
    x_return_if_fail(acquired_context);

    if (context == x_main_context_default()) {
        context = NULL;
    } else if(context) {
        x_main_context_ref(context);
    }

    stack = (XQueue *)x_private_get(&thread_context_stack);
    if (!stack) {
        stack = x_queue_new();
        x_private_set(&thread_context_stack, stack);
    }
    x_queue_push_head(stack, context);

    TRACE(XLIB_MAIN_CONTEXT_PUSH_THREAD_DEFAULT(context));
}

void x_main_context_pop_thread_default(XMainContext *context)
{
    XQueue *stack;

    if (context == x_main_context_default()) {
        context = NULL;
    }

    stack = (XQueue *)x_private_get(&thread_context_stack);

    x_return_if_fail(stack != NULL);
    x_return_if_fail(x_queue_peek_head(stack) == context);

    TRACE(XLIB_MAIN_CONTEXT_POP_THREAD_DEFAULT(context));

    x_queue_pop_head(stack);

    x_main_context_release(context);
    if (context) {
        x_main_context_unref(context);
    }
}

XMainContext *x_main_context_get_thread_default(void)
{
    XQueue *stack;

    stack = (XQueue *)x_private_get(&thread_context_stack);
    if (stack) {
        return (XMainContext *)x_queue_peek_head(stack);
    } else {
        return NULL;
    }
}

XMainContext *x_main_context_ref_thread_default(void)
{
    XMainContext *context;

    context = x_main_context_get_thread_default();
    if (!context) {
        context = x_main_context_default();
    }

    return x_main_context_ref (context);
}

XSource *x_source_new(XSourceFuncs *source_funcs, xuint struct_size)
{
    XSource *source;

    x_return_val_if_fail(source_funcs != NULL, NULL);
    x_return_val_if_fail(struct_size >= sizeof (XSource), NULL);

    source = (XSource *)x_malloc0(struct_size);
    source->priv = x_slice_new0(XSourcePrivate);
    source->source_funcs = source_funcs;
    source->ref_count = 1;
    source->priority = X_PRIORITY_DEFAULT;
    source->flags = X_HOOK_FLAG_ACTIVE;
    source->priv->ready_time = -1;

    TRACE(XLIB_SOURCE_NEW(source, source_funcs->prepare, source_funcs->check, source_funcs->dispatch, source_funcs->finalize, struct_size));
    return source;
}

void x_source_set_dispose_function(XSource *source, XSourceDisposeFunc dispose)
{
    x_return_if_fail(source != NULL);
    x_return_if_fail(source->priv->dispose == NULL);
    x_return_if_fail(x_atomic_int_get(&source->ref_count) > 0);
    source->priv->dispose = dispose;
}

static void x_source_iter_init(XSourceIter  *iter, XMainContext *context, xboolean may_modify)
{
    iter->context = context;
    iter->current_list = NULL;
    iter->source = NULL;
    iter->may_modify = may_modify;
}

static xboolean x_source_iter_next(XSourceIter *iter, XSource **source)
{
    XSource *next_source;

    if (iter->source) {
        next_source = iter->source->next;
    } else {
        next_source = NULL;
    }

    if (!next_source) {
        if (iter->current_list) {
            iter->current_list = iter->current_list->next;
        } else {
            iter->current_list = iter->context->source_lists;
        }

        if (iter->current_list) {
            XSourceList *source_list = (XSourceList *)iter->current_list->data;
            next_source = source_list->head;
        }
    }

    if (next_source && iter->may_modify) {
        x_source_ref(next_source);
    }

    if (iter->source && iter->may_modify) {
        x_source_unref_internal(iter->source, iter->context, TRUE);
    }
    iter->source = next_source;

    *source = iter->source;
    return *source != NULL;
}

static void x_source_iter_clear(XSourceIter *iter)
{
    if (iter->source && iter->may_modify) {
        x_source_unref_internal(iter->source, iter->context, TRUE);
        iter->source = NULL;
    }
}

static XSourceList *find_source_list_for_priority(XMainContext *context, xint priority, xboolean create)
{
    XList *iter, *last;
    XSourceList *source_list;

    last = NULL;
    for (iter = context->source_lists; iter != NULL; last = iter, iter = iter->next) {
        source_list = (XSourceList *)iter->data;

        if (source_list->priority == priority) {
            return source_list;
        }

        if (source_list->priority > priority) {
            if (!create) {
                return NULL;
            }

            source_list = x_slice_new0(XSourceList);
            source_list->priority = priority;
            context->source_lists = x_list_insert_before(context->source_lists, iter, source_list);
            return source_list;
        }
    }

    if (!create) {
        return NULL;
    }

    source_list = x_slice_new0(XSourceList);
    source_list->priority = priority;

    if (!last) {
        context->source_lists = x_list_append(NULL, source_list);
    } else {
        last = x_list_append(last, source_list);
        (void)last;
    }

    return source_list;
}

static void source_add_to_context(XSource *source, XMainContext *context)
{
    XSource *prev, *next;
    XSourceList *source_list;

    source_list = find_source_list_for_priority(context, source->priority, TRUE);

    if (source->priv->parent_source) {
        x_assert(source_list->head != NULL);

        prev = source->priv->parent_source->prev;
        next = source->priv->parent_source;
    } else {
        prev = source_list->tail;
        next = NULL;
    }

    source->next = next;
    if (next) {
        next->prev = source;
    } else {
        source_list->tail = source;
    }

    source->prev = prev;
    if (prev) {
        prev->next = source;
    } else {
        source_list->head = source;
    }
}

static void source_remove_from_context(XSource *source, XMainContext *context)
{
    XSourceList *source_list;

    source_list = find_source_list_for_priority(context, source->priority, FALSE);
    x_return_if_fail(source_list != NULL);

    if (source->prev) {
        source->prev->next = source->next;
    } else {
        source_list->head = source->next;
    }

    if (source->next) {
        source->next->prev = source->prev;
    } else {
        source_list->tail = source->prev;
    }

    source->prev = NULL;
    source->next = NULL;

    if (source_list->head == NULL) {
        context->source_lists = x_list_remove(context->source_lists, source_list);
        x_slice_free(XSourceList, source_list);
    }
}

static xuint x_source_attach_unlocked(XSource *source, XMainContext *context, xboolean do_wakeup)
{
    xuint id;
    XSList *tmp_list;

    do {
        id = context->next_id++;
    } while (id == 0 || x_hash_table_contains(context->sources, XUINT_TO_POINTER(id)));

    source->context = context;
    source->source_id = id;
    x_source_ref(source);

    x_hash_table_insert(context->sources, XUINT_TO_POINTER(id), source);

    source_add_to_context(source, context);

    if (!SOURCE_BLOCKED(source)) {
        tmp_list = source->poll_fds;
        while (tmp_list) {
            x_main_context_add_poll_unlocked(context, source->priority, (XPollFD *)tmp_list->data);
            tmp_list = tmp_list->next;
        }

        for (tmp_list = source->priv->fds; tmp_list; tmp_list = tmp_list->next) {
            x_main_context_add_poll_unlocked(context, source->priority, (XPollFD *)tmp_list->data);
        }
    }

    tmp_list = source->priv->child_sources;
    while (tmp_list) {
        x_source_attach_unlocked((XSource *)tmp_list->data, context, FALSE);
        tmp_list = tmp_list->next;
    }

    if (do_wakeup && (context->flags & X_MAIN_CONTEXT_FLAGS_OWNERLESS_POLLING || (context->owner && context->owner != X_THREAD_SELF))) {
        x_wakeup_signal(context->wakeup);
    }

    x_trace_mark(X_TRACE_CURRENT_TIME, 0, "XLib", "x_source_attach", "%s to context %p", (x_source_get_name(source) != NULL) ? x_source_get_name(source) : "(unnamed)", context);
    return source->source_id;
}

xuint x_source_attach(XSource *source, XMainContext *context)
{
    xuint result = 0;

    x_return_val_if_fail(source != NULL, 0);
    x_return_val_if_fail(x_atomic_int_get(&source->ref_count) > 0, 0);
    x_return_val_if_fail(source->context == NULL, 0);
    x_return_val_if_fail(!SOURCE_DESTROYED(source), 0);

    if (!context) {
        context = x_main_context_default();
    }

    LOCK_CONTEXT(context);
    result = x_source_attach_unlocked(source, context, TRUE);
    TRACE(XLIB_MAIN_SOURCE_ATTACH(x_source_get_name(source), source, context, result));
    UNLOCK_CONTEXT(context);

    return result;
}

static void x_source_destroy_internal(XSource *source, XMainContext *context, xboolean have_lock)
{
    TRACE(XLIB_MAIN_SOURCE_DESTROY(x_source_get_name(source), source, context));

    if (!have_lock) {
        LOCK_CONTEXT(context);
    }

    if (!SOURCE_DESTROYED(source)) {
        XSList *tmp_list;
        xpointer old_cb_data;
        XSourceCallbackFuncs *old_cb_funcs;

        source->flags &= ~X_HOOK_FLAG_ACTIVE;
        old_cb_data = source->callback_data;
        old_cb_funcs = source->callback_funcs;
        source->callback_data = NULL;
        source->callback_funcs = NULL;

        if (old_cb_funcs) {
            UNLOCK_CONTEXT(context);
            old_cb_funcs->unref(old_cb_data);
            LOCK_CONTEXT(context);
        }

        if (!SOURCE_BLOCKED(source)) {
            tmp_list = source->poll_fds;
            while (tmp_list) {
                x_main_context_remove_poll_unlocked(context, (XPollFD *)tmp_list->data);
                tmp_list = tmp_list->next;
            }

            for (tmp_list = source->priv->fds; tmp_list; tmp_list = tmp_list->next) {
                x_main_context_remove_poll_unlocked(context, (XPollFD *)tmp_list->data);
            }
        }

        while (source->priv->child_sources) {
            x_child_source_remove_internal((XSource *)source->priv->child_sources->data, context);
        }

        if (source->priv->parent_source) {
            x_child_source_remove_internal(source, context);
        }

        x_source_unref_internal(source, context, TRUE);
    }

    if (!have_lock) {
        UNLOCK_CONTEXT(context);
    }
}

void x_source_destroy(XSource *source)
{
    XMainContext *context;

    x_return_if_fail(source != NULL);
    x_return_if_fail(x_atomic_int_get(&source->ref_count) > 0);

    context = source->context;
    if (context) {
        x_source_destroy_internal(source, context, FALSE);
    } else {
        source->flags &= ~X_HOOK_FLAG_ACTIVE;
    }
}

xuint x_source_get_id(XSource *source)
{
    xuint result;

    x_return_val_if_fail(source != NULL, 0);
    x_return_val_if_fail(x_atomic_int_get(&source->ref_count) > 0, 0);
    x_return_val_if_fail(source->context != NULL, 0);

    LOCK_CONTEXT(source->context);
    result = source->source_id;
    UNLOCK_CONTEXT(source->context);

    return result;
}

XMainContext *x_source_get_context(XSource *source)
{
    x_return_val_if_fail(source != NULL, NULL);
    x_return_val_if_fail(x_atomic_int_get(&source->ref_count) > 0, NULL);
    x_return_val_if_fail(source->context != NULL || !SOURCE_DESTROYED(source), NULL);

    return source->context;
}

void x_source_add_poll(XSource *source, XPollFD *fd)
{
    XMainContext *context;

    x_return_if_fail(source != NULL);
    x_return_if_fail(x_atomic_int_get(&source->ref_count) > 0);
    x_return_if_fail(fd != NULL);
    x_return_if_fail(!SOURCE_DESTROYED(source));

    context = source->context;
    if (context) {
        LOCK_CONTEXT(context);
    }
    source->poll_fds = x_slist_prepend(source->poll_fds, fd);

    if (context) {
        if (!SOURCE_BLOCKED(source)) {
            x_main_context_add_poll_unlocked(context, source->priority, fd);
        }

        UNLOCK_CONTEXT(context);
    }
}

void x_source_remove_poll(XSource *source, XPollFD *fd)
{
    XMainContext *context;

    x_return_if_fail(source != NULL);
    x_return_if_fail(x_atomic_int_get(&source->ref_count) > 0);
    x_return_if_fail(fd != NULL);
    x_return_if_fail(!SOURCE_DESTROYED(source));

    context = source->context;
    if (context) {
        LOCK_CONTEXT(context);
    }
    
    source->poll_fds = x_slist_remove(source->poll_fds, fd);

    if (context) {
        if (!SOURCE_BLOCKED(source)) {
            x_main_context_remove_poll_unlocked(context, fd);
        }

        UNLOCK_CONTEXT(context);
    }
}

void x_source_add_child_source(XSource *source, XSource *child_source)
{
    XMainContext *context;

    x_return_if_fail(source != NULL);
    x_return_if_fail(x_atomic_int_get(&source->ref_count) > 0);
    x_return_if_fail(child_source != NULL);
    x_return_if_fail(x_atomic_int_get(&child_source->ref_count) > 0);
    x_return_if_fail(!SOURCE_DESTROYED(source));
    x_return_if_fail(!SOURCE_DESTROYED(child_source));
    x_return_if_fail(child_source->context == NULL);
    x_return_if_fail(child_source->priv->parent_source == NULL);

    context = source->context;
    if (context) {
        LOCK_CONTEXT(context);
    }

    TRACE(XLIB_SOURCE_ADD_CHILD_SOURCE(source, child_source));

    source->priv->child_sources = x_slist_prepend(source->priv->child_sources, x_source_ref(child_source));
    child_source->priv->parent_source = source;
    x_source_set_priority_unlocked(child_source, NULL, source->priority);
    if (SOURCE_BLOCKED(source)) {
        block_source(child_source);
    }

    if (context) {
        x_source_attach_unlocked(child_source, context, TRUE);
        UNLOCK_CONTEXT(context);
    }
}

static void x_child_source_remove_internal(XSource *child_source, XMainContext *context)
{
    XSource *parent_source = child_source->priv->parent_source;

    parent_source->priv->child_sources = x_slist_remove(parent_source->priv->child_sources, child_source);
    child_source->priv->parent_source = NULL;

    x_source_destroy_internal(child_source, context, TRUE);
    x_source_unref_internal(child_source, context, TRUE);
}

void x_source_remove_child_source(XSource *source, XSource *child_source)
{
    XMainContext *context;

    x_return_if_fail(source != NULL);
    x_return_if_fail(x_atomic_int_get(&source->ref_count) > 0);
    x_return_if_fail(child_source != NULL);
    x_return_if_fail(x_atomic_int_get(&child_source->ref_count) > 0);
    x_return_if_fail(child_source->priv->parent_source == source);
    x_return_if_fail(!SOURCE_DESTROYED(source));
    x_return_if_fail(!SOURCE_DESTROYED(child_source));

    context = source->context;
    if (context) {
        LOCK_CONTEXT(context);
    }

    x_child_source_remove_internal(child_source, context);
    if (context) {
        UNLOCK_CONTEXT(context);
    }
}

static void x_source_callback_ref(xpointer cb_data)
{
    XSourceCallback *callback = (XSourceCallback *)cb_data;
    x_atomic_int_inc(&callback->ref_count);
}

static void x_source_callback_unref(xpointer cb_data)
{
    XSourceCallback *callback = (XSourceCallback *)cb_data;

    if (x_atomic_int_dec_and_test(&callback->ref_count)) {
        if (callback->notify) {
            callback->notify(callback->data);
        }

        x_free(callback);
    }
}

static void x_source_callback_get(xpointer cb_data, XSource *source,  XSourceFunc *func, xpointer *data)
{
    XSourceCallback *callback = (XSourceCallback *)cb_data;

    *func = callback->func;
    *data = callback->data;
}

static XSourceCallbackFuncs x_source_callback_funcs = {
    x_source_callback_ref,
    x_source_callback_unref,
    x_source_callback_get,
};

void x_source_set_callback_indirect(XSource *source, xpointer callback_data, XSourceCallbackFuncs *callback_funcs)
{
    xpointer old_cb_data;
    XMainContext *context;
    XSourceCallbackFuncs *old_cb_funcs;

    x_return_if_fail(source != NULL);
    x_return_if_fail(x_atomic_int_get(&source->ref_count) > 0);
    x_return_if_fail(callback_funcs != NULL || callback_data == NULL);

    context = source->context;
    if (context) {
        LOCK_CONTEXT(context);
    }

    if (callback_funcs != &x_source_callback_funcs) {
        TRACE(XLIB_SOURCE_SET_CALLBACK_INDIRECT(source, callback_data, callback_funcs->ref, callback_funcs->unref, callback_funcs->get));
    }

    old_cb_data = source->callback_data;
    old_cb_funcs = source->callback_funcs;
    source->callback_data = callback_data;
    source->callback_funcs = callback_funcs;

    if (context) {
        UNLOCK_CONTEXT(context);
    }

    if (old_cb_funcs) {
        old_cb_funcs->unref(old_cb_data);
    }
}

void x_source_set_callback(XSource *source, XSourceFunc func, xpointer data, XDestroyNotify notify)
{
    XSourceCallback *new_callback;

    x_return_if_fail(source != NULL);
    x_return_if_fail(x_atomic_int_get(&source->ref_count) > 0);

    TRACE(XLIB_SOURCE_SET_CALLBACK(source, func, data, notify));

    new_callback = x_new(XSourceCallback, 1);

    new_callback->ref_count = 1;
    new_callback->func = func;
    new_callback->data = data;
    new_callback->notify = notify;

    x_source_set_callback_indirect(source, new_callback, &x_source_callback_funcs);
}

void x_source_set_funcs(XSource *source, XSourceFuncs *funcs)
{
    x_return_if_fail(source != NULL);
    x_return_if_fail(source->context == NULL);
    x_return_if_fail(x_atomic_int_get(&source->ref_count) > 0);
    x_return_if_fail(funcs != NULL);

    source->source_funcs = funcs;
}

static void x_source_set_priority_unlocked(XSource *source, XMainContext *context, xint priority)
{
    XSList *tmp_list;
    
    x_return_if_fail(source->priv->parent_source == NULL || source->priv->parent_source->priority == priority);

    TRACE(XLIB_SOURCE_SET_PRIORITY(source, context, priority));

    if (context) {
        source_remove_from_context(source, source->context);
    }

    source->priority = priority;
    if (context) {
        source_add_to_context(source, source->context);

        if (!SOURCE_BLOCKED(source)) {
            tmp_list = source->poll_fds;
            while (tmp_list) {
                x_main_context_remove_poll_unlocked(context, (XPollFD *)tmp_list->data);
                x_main_context_add_poll_unlocked(context, priority, (XPollFD *)tmp_list->data);
                
                tmp_list = tmp_list->next;
            }

            for (tmp_list = source->priv->fds; tmp_list; tmp_list = tmp_list->next) {
                x_main_context_remove_poll_unlocked(context, (XPollFD *)tmp_list->data);
                x_main_context_add_poll_unlocked(context, priority, (XPollFD *)tmp_list->data);
            }
        }
    }

    if (source->priv->child_sources) {
        tmp_list = source->priv->child_sources;
        while (tmp_list) {
            x_source_set_priority_unlocked((XSource *)tmp_list->data, context, priority);
            tmp_list = tmp_list->next;
        }
    }
}

void x_source_set_priority(XSource *source, xint priority)
{
    XMainContext *context;

    x_return_if_fail(source != NULL);
    x_return_if_fail(x_atomic_int_get(&source->ref_count) > 0);
    x_return_if_fail(source->priv->parent_source == NULL);

    context = source->context;
    if (context) {
        LOCK_CONTEXT(context);
    }

    x_source_set_priority_unlocked(source, context, priority);
    if (context) {
        UNLOCK_CONTEXT(context);
    }
}

xint x_source_get_priority(XSource *source)
{
    x_return_val_if_fail(source != NULL, 0);
    x_return_val_if_fail(x_atomic_int_get(&source->ref_count) > 0, 0);

    return source->priority;
}

void x_source_set_ready_time(XSource *source, xint64 ready_time)
{
    XMainContext *context;

    x_return_if_fail(source != NULL);
    x_return_if_fail(x_atomic_int_get(&source->ref_count) > 0);

    context = source->context;
    if (context) {
        LOCK_CONTEXT(context);
    }

    if (source->priv->ready_time == ready_time) {
        if (context) {
            UNLOCK_CONTEXT(context);
        }

        return;
    }

    source->priv->ready_time = ready_time;

    TRACE(XLIB_SOURCE_SET_READY_TIME(source, ready_time));

    if (context) {
        if (!SOURCE_BLOCKED(source)) {
            x_wakeup_signal(context->wakeup);
        }

        UNLOCK_CONTEXT(context);
    }
}

xint64 x_source_get_ready_time(XSource *source)
{
    x_return_val_if_fail(source != NULL, -1);
    x_return_val_if_fail(x_atomic_int_get(&source->ref_count) > 0, -1);

    return source->priv->ready_time;
}

void x_source_set_can_recurse(XSource *source, xboolean can_recurse)
{
    XMainContext *context;

    x_return_if_fail(source != NULL);
    x_return_if_fail(x_atomic_int_get(&source->ref_count) > 0);

    context = source->context;
    if (context) {
        LOCK_CONTEXT(context);
    }

    if (can_recurse) {
        source->flags |= X_SOURCE_CAN_RECURSE;
    } else {
        source->flags &= ~X_SOURCE_CAN_RECURSE;
    }

    if (context) {
        UNLOCK_CONTEXT(context);
    }
}

xboolean x_source_get_can_recurse(XSource *source)
{
    x_return_val_if_fail(source != NULL, FALSE);
    x_return_val_if_fail(x_atomic_int_get(&source->ref_count) > 0, FALSE);

    return (source->flags & X_SOURCE_CAN_RECURSE) != 0;
}

static void x_source_set_name_full(XSource *source, const char *name, xboolean is_static)
{
    XMainContext *context;

    x_return_if_fail(source != NULL);
    x_return_if_fail(x_atomic_int_get(&source->ref_count) > 0);

    context = source->context;
    if (context) {
        LOCK_CONTEXT(context);
    }

    TRACE(XLIB_SOURCE_SET_NAME(source, name));

    if (!source->priv->static_name) {
        x_free(source->name);
    }

    if (is_static) {
        source->name = (char *)name;
    } else {
        source->name = x_strdup(name);
    }

    source->priv->static_name = is_static;
    if (context) {
        UNLOCK_CONTEXT(context);
    }
}

void x_source_set_name(XSource *source, const char *name)
{
    x_source_set_name_full(source, name, FALSE);
}

void x_source_set_static_name(XSource *source, const char *name)
{
    x_source_set_name_full(source, name, TRUE);
}

const char *x_source_get_name(XSource *source)
{
    x_return_val_if_fail(source != NULL, NULL);
    x_return_val_if_fail(x_atomic_int_get(&source->ref_count) > 0, NULL);

    return source->name;
}

void x_source_set_name_by_id(xuint tag, const char *name)
{
    XSource *source;

    x_return_if_fail(tag > 0);

    source = x_main_context_find_source_by_id(NULL, tag);
    if (source == NULL) {
        return;
    }

    x_source_set_name(source, name);
}

XSource *x_source_ref(XSource *source)
{
    x_return_val_if_fail(source != NULL, NULL);
    x_return_val_if_fail(x_atomic_int_get(&source->ref_count) >= 0, NULL);

    x_atomic_int_inc(&source->ref_count);
    return source;
}

static void x_source_unref_internal(XSource *source, XMainContext *context, xboolean have_lock)
{
    xpointer old_cb_data = NULL;
    XSourceCallbackFuncs *old_cb_funcs = NULL;

    x_return_if_fail(source != NULL);

    if (!have_lock && context) {
        LOCK_CONTEXT(context);
    }

    if (x_atomic_int_dec_and_test(&source->ref_count)) {
        if (source->priv->dispose) {
            x_atomic_int_inc(&source->ref_count);
            if (context) {
                UNLOCK_CONTEXT(context);
            }

            source->priv->dispose (source);
            if (context) {
                LOCK_CONTEXT(context);
            }

            if (!x_atomic_int_dec_and_test(&source->ref_count)) {
                if (!have_lock && context) {
                    UNLOCK_CONTEXT(context);
                }

                return;
            }
        }

        TRACE(XLIB_SOURCE_BEFORE_FREE(source, context, source->source_funcs->finalize));

        old_cb_data = source->callback_data;
        old_cb_funcs = source->callback_funcs;

        source->callback_data = NULL;
        source->callback_funcs = NULL;

        if (context) {
            if (!SOURCE_DESTROYED(source)) {
                x_warning(X_STRLOC ": ref_count == 0, but source was still attached to a context!");
            }

            source_remove_from_context (source, context);
            x_hash_table_remove(context->sources, XUINT_TO_POINTER (source->source_id));
        }

        if (source->source_funcs->finalize) {
            xint old_ref_count;

            x_atomic_int_inc(&source->ref_count);
            if (context) {
                UNLOCK_CONTEXT(context);
            }

            source->source_funcs->finalize (source);
            if (context) {
                LOCK_CONTEXT(context);
            }

            old_ref_count = x_atomic_int_add(&source->ref_count, -1);
            x_warn_if_fail(old_ref_count == 1);
        }

        if (old_cb_funcs) {
            xint old_ref_count;

            x_atomic_int_inc(&source->ref_count);
            if (context) {
                UNLOCK_CONTEXT(context);
            }

            old_cb_funcs->unref(old_cb_data);

            if (context) {
                LOCK_CONTEXT(context);
            }

            old_ref_count = x_atomic_int_add(&source->ref_count, -1);
            x_warn_if_fail(old_ref_count == 1);
        }

        if (!source->priv->static_name) {
            x_free(source->name);
        }
        source->name = NULL;

        x_slist_free(source->poll_fds);
        source->poll_fds = NULL;

        x_slist_free_full(source->priv->fds, x_free);

        while (source->priv->child_sources) {
            XSource *child_source = (XSource *)source->priv->child_sources->data;

            source->priv->child_sources = x_slist_remove(source->priv->child_sources, child_source);
            child_source->priv->parent_source = NULL;

            x_source_unref_internal(child_source, context, TRUE);
        }

        x_slice_free(XSourcePrivate, source->priv);
        source->priv = NULL;

        x_free(source);
    }

    if (!have_lock && context) {
        UNLOCK_CONTEXT(context);
    }
}

void x_source_unref(XSource *source)
{
    x_return_if_fail(source != NULL);
    x_return_if_fail(x_atomic_int_get(&source->ref_count) > 0);

    x_source_unref_internal(source, source->context, FALSE);
}

XSource *x_main_context_find_source_by_id(XMainContext *context, xuint source_id)
{
    XSource *source;

    x_return_val_if_fail(source_id > 0, NULL);

    if (context == NULL) {
        context = x_main_context_default();
    }

    LOCK_CONTEXT(context);
    source = (XSource *)x_hash_table_lookup(context->sources, XUINT_TO_POINTER(source_id));
    UNLOCK_CONTEXT(context);

    if (source && SOURCE_DESTROYED(source)) {
        source = NULL;
    }

    return source;
}

XSource *x_main_context_find_source_by_funcs_user_data(XMainContext *context, XSourceFuncs *funcs, xpointer user_data)
{
    XSource *source;
    XSourceIter iter;

    x_return_val_if_fail(funcs != NULL, NULL);

    if (context == NULL) {
        context = x_main_context_default();
    }

    LOCK_CONTEXT(context);

    x_source_iter_init(&iter, context, FALSE);
    while (x_source_iter_next(&iter, &source)) {
        if (!SOURCE_DESTROYED(source) && source->source_funcs == funcs && source->callback_funcs) {
            XSourceFunc callback;
            xpointer callback_data;

            source->callback_funcs->get(source->callback_data, source, &callback, &callback_data);
            if (callback_data == user_data) {
                break;
            }
        }
    }
    x_source_iter_clear(&iter);

    UNLOCK_CONTEXT(context);
    return source;
}

XSource *x_main_context_find_source_by_user_data(XMainContext *context, xpointer user_data)
{
    XSource *source;
    XSourceIter iter;

    if (context == NULL) {
        context = x_main_context_default();
    }

    LOCK_CONTEXT(context);

    x_source_iter_init(&iter, context, FALSE);
    while (x_source_iter_next(&iter, &source)) {
        if (!SOURCE_DESTROYED(source) && source->callback_funcs) {
            XSourceFunc callback;
            xpointer callback_data = NULL;

            source->callback_funcs->get (source->callback_data, source, &callback, &callback_data);
            if (callback_data == user_data) {
                break;
            }
        }
    }
    x_source_iter_clear(&iter);

    UNLOCK_CONTEXT(context);
    return source;
}

xboolean x_source_remove(xuint tag)
{
    XSource *source;

    x_return_val_if_fail(tag > 0, FALSE);

    source = x_main_context_find_source_by_id(NULL, tag);
    if (source) {
        x_source_destroy(source);
    } else {
        x_critical("Source ID %u was not found when attempting to remove it", tag);
    }

    return source != NULL;
}

xboolean x_source_remove_by_user_data(xpointer user_data)
{
    XSource *source;

    source = x_main_context_find_source_by_user_data(NULL, user_data);
    if (source) {
        x_source_destroy(source);
        return TRUE;
    } else {
        return FALSE;
    }
}

xboolean x_source_remove_by_funcs_user_data(XSourceFuncs *funcs, xpointer user_data)
{
    XSource *source;

    x_return_val_if_fail(funcs != NULL, FALSE);

    source = x_main_context_find_source_by_funcs_user_data(NULL, funcs, user_data);
    if (source) {
        x_source_destroy(source);
        return TRUE;
    } else {
        return FALSE;
    }
}

#undef x_clear_handle_id
void x_clear_handle_id (xuint *tag_ptr, XClearHandleFunc clear_func)
{
    xuint _handle_id;

    _handle_id = *tag_ptr;
    if (_handle_id > 0) {
        *tag_ptr = 0;
        clear_func(_handle_id);
    }
}

xpointer x_source_add_unix_fd(XSource *source, xint fd, XIOCondition events)
{
    XPollFD *poll_fd;
    XMainContext *context;

    x_return_val_if_fail(source != NULL, NULL);
    x_return_val_if_fail(x_atomic_int_get(&source->ref_count) > 0, NULL);
    x_return_val_if_fail(!SOURCE_DESTROYED(source), NULL);

    poll_fd = x_new(XPollFD, 1);
    poll_fd->fd = fd;
    poll_fd->events = events;
    poll_fd->revents = 0;

    context = source->context;
    if (context) {
        LOCK_CONTEXT(context);
    }

    source->priv->fds = x_slist_prepend(source->priv->fds, poll_fd);
    if (context) {
        if (!SOURCE_BLOCKED(source)) {
            x_main_context_add_poll_unlocked(context, source->priority, poll_fd);
        }

        UNLOCK_CONTEXT(context);
    }

    return poll_fd;
}

void x_source_modify_unix_fd(XSource *source, xpointer tag, XIOCondition  new_events)
{
    XPollFD *poll_fd;
    XMainContext *context;

    x_return_if_fail(source != NULL);
    x_return_if_fail(x_atomic_int_get(&source->ref_count) > 0);
    x_return_if_fail(x_slist_find(source->priv->fds, tag));

    context = source->context;
    poll_fd = (XPollFD *)tag;

    poll_fd->events = new_events;
    if (context) {
        x_main_context_wakeup(context);
    }
}

void x_source_remove_unix_fd (XSource *source, xpointer tag)
{
    XPollFD *poll_fd;
    XMainContext *context;

    x_return_if_fail(source != NULL);
    x_return_if_fail(x_atomic_int_get(&source->ref_count) > 0);
    x_return_if_fail(x_slist_find(source->priv->fds, tag));

    context = source->context;
    poll_fd = (XPollFD *)tag;

    if (context) {
        LOCK_CONTEXT(context);
    }

    source->priv->fds = x_slist_remove(source->priv->fds, poll_fd);
    if (context) {
        if (!SOURCE_BLOCKED(source)) {
            x_main_context_remove_poll_unlocked(context, poll_fd);
        }

        UNLOCK_CONTEXT(context);
    }

    x_free(poll_fd);
}

XIOCondition x_source_query_unix_fd(XSource *source, xpointer  tag)
{
    XPollFD *poll_fd;

    x_return_val_if_fail(source != NULL, (XIOCondition)0);
    x_return_val_if_fail(x_atomic_int_get(&source->ref_count) > 0, (XIOCondition)0);
    x_return_val_if_fail(x_slist_find(source->priv->fds, tag), (XIOCondition)0);

    poll_fd = (XPollFD *)tag;
    return (XIOCondition)poll_fd->revents;
}

X_GNUC_BEGIN_IGNORE_DEPRECATIONS
void x_get_current_time(XTimeVal *result)
{
    xint64 tv;

    x_return_if_fail(result != NULL);
    tv = x_get_real_time();

    result->tv_sec = tv / 1000000;
    result->tv_usec = tv % 1000000;
}
X_GNUC_END_IGNORE_DEPRECATIONS

xint64 x_get_real_time(void)
{
    struct timeval r;

    gettimeofday(&r, NULL);
    return (((xint64)r.tv_sec) * 1000000) + r.tv_usec;
}

xint64 x_get_monotonic_time(void)
{
    xint result;
    struct timespec ts;

    result = clock_gettime(CLOCK_MONOTONIC, &ts);

    if X_UNLIKELY(result != 0) {
        x_error("XLib requires working CLOCK_MONOTONIC");
    }

    return (((xint64)ts.tv_sec) * 1000000) + (ts.tv_nsec / 1000);
}

static void x_main_dispatch_free(xpointer dispatch)
{
    x_free(dispatch);
}

static XMainDispatch *get_dispatch(void)
{
    XMainDispatch *dispatch;
    static XPrivate depth_private = X_PRIVATE_INIT(x_main_dispatch_free);

    dispatch = (XMainDispatch *)x_private_get(&depth_private);
    if (!dispatch) {
        dispatch = (XMainDispatch *)x_private_set_alloc0(&depth_private, sizeof(XMainDispatch));
    }

    return dispatch;
}

int x_main_depth(void)
{
    XMainDispatch *dispatch = get_dispatch();
    return dispatch->depth;
}

XSource *x_main_current_source(void)
{
    XMainDispatch *dispatch = get_dispatch();
    return dispatch->source;
}

xboolean x_source_is_destroyed(XSource *source)
{
    x_return_val_if_fail(source != NULL, TRUE);
    x_return_val_if_fail(x_atomic_int_get(&source->ref_count) > 0, TRUE);

    return SOURCE_DESTROYED(source);
}

static void block_source(XSource *source)
{
    XSList *tmp_list;

    x_return_if_fail(!SOURCE_BLOCKED(source));

    source->flags |= X_SOURCE_BLOCKED;

    if (source->context) {
        tmp_list = source->poll_fds;
        while (tmp_list) {
            x_main_context_remove_poll_unlocked(source->context, (XPollFD *)tmp_list->data);
            tmp_list = tmp_list->next;
        }

        for (tmp_list = source->priv->fds; tmp_list; tmp_list = tmp_list->next) {
            x_main_context_remove_poll_unlocked(source->context, (XPollFD *)tmp_list->data);
        }
    }

    if (source->priv && source->priv->child_sources) {
        tmp_list = source->priv->child_sources;
        while (tmp_list) {
            block_source((XSource *)tmp_list->data);
            tmp_list = tmp_list->next;
        }
    }
}

static void unblock_source(XSource *source)
{
    XSList *tmp_list;

    x_return_if_fail(SOURCE_BLOCKED(source));
    x_return_if_fail(!SOURCE_DESTROYED(source));
    
    source->flags &= ~X_SOURCE_BLOCKED;

    tmp_list = source->poll_fds;
    while (tmp_list) {
        x_main_context_add_poll_unlocked(source->context, source->priority, (XPollFD *)tmp_list->data);
        tmp_list = tmp_list->next;
    }

    for (tmp_list = source->priv->fds; tmp_list; tmp_list = tmp_list->next) {
        x_main_context_add_poll_unlocked(source->context, source->priority, (XPollFD *)tmp_list->data);
    }

    if (source->priv && source->priv->child_sources) {
        tmp_list = source->priv->child_sources;
        while (tmp_list) {
            unblock_source((XSource *)tmp_list->data);
            tmp_list = tmp_list->next;
        }
    }
}

static void x_main_dispatch(XMainContext *context)
{
    xuint i;
    XMainDispatch *current = get_dispatch();

    for (i = 0; i < context->pending_dispatches->len; i++) {
        XSource *source = (XSource *)context->pending_dispatches->pdata[i];

        context->pending_dispatches->pdata[i] = NULL;
        x_assert(source);

        source->flags &= ~X_SOURCE_READY;

        if (!SOURCE_DESTROYED(source)) {
            xpointer cb_data;
            XSource *prev_source;
            xboolean was_in_call;
            xboolean need_destroy;
            xpointer user_data = NULL;
            XSourceFunc callback = NULL;
            XSourceCallbackFuncs *cb_funcs;
            xint64 begin_time_nsec X_GNUC_UNUSED;
            xboolean (*dispatch)(XSource *, XSourceFunc, xpointer);

            dispatch = source->source_funcs->dispatch;
            cb_funcs = source->callback_funcs;
            cb_data = source->callback_data;

            if (cb_funcs) {
                cb_funcs->ref(cb_data);
            }

            if ((source->flags & X_SOURCE_CAN_RECURSE) == 0) {
                block_source(source);
            }

            was_in_call = source->flags & X_HOOK_FLAG_IN_CALL;
            source->flags |= X_HOOK_FLAG_IN_CALL;

            if (cb_funcs) {
                cb_funcs->get(cb_data, source, &callback, &user_data);
            }

            UNLOCK_CONTEXT(context);

            prev_source = current->source;
            current->source = source;
            current->depth++;

            begin_time_nsec = X_TRACE_CURRENT_TIME;

            TRACE(XLIB_MAIN_BEFORE_DISPATCH(x_source_get_name(source), source, dispatch, callback, user_data));
            need_destroy = !(*dispatch)(source, callback, user_data);
            TRACE(XLIB_MAIN_AFTER_DISPATCH(x_source_get_name(source), source, dispatch, need_destroy));

            x_trace_mark(begin_time_nsec, X_TRACE_CURRENT_TIME - begin_time_nsec, "XLib", "XSource.dispatch", "%s ⇒ %s", (x_source_get_name(source) != NULL) ? x_source_get_name(source) : "(unnamed)", need_destroy ? "destroy" : "keep");

            current->source = prev_source;
            current->depth--;

            if (cb_funcs) {
                cb_funcs->unref(cb_data);
            }

            LOCK_CONTEXT(context);

            if (!was_in_call) {
                source->flags &= ~X_HOOK_FLAG_IN_CALL;
            }

            if (SOURCE_BLOCKED(source) && !SOURCE_DESTROYED(source)) {
                unblock_source(source);
            }

            if (need_destroy && !SOURCE_DESTROYED(source)) {
                x_assert(source->context == context);
                x_source_destroy_internal(source, context, TRUE);
            }
        }

        x_source_unref_internal(source, context, TRUE);
    }

    x_ptr_array_set_size(context->pending_dispatches, 0);
}

xboolean x_main_context_acquire(XMainContext *context)
{
    xboolean result = FALSE;
    XThread *self = X_THREAD_SELF;

    if (context == NULL) {
        context = x_main_context_default();
    }

    LOCK_CONTEXT(context);

    if (!context->owner) {
        context->owner = self;
        x_assert(context->owner_count == 0);
        TRACE(XLIB_MAIN_CONTEXT_ACQUIRE(context, TRUE));
    }

    if (context->owner == self) {
        context->owner_count++;
        result = TRUE;
    } else {
        TRACE(XLIB_MAIN_CONTEXT_ACQUIRE(context, FALSE));
    }

    UNLOCK_CONTEXT(context); 
    return result;
}

void x_main_context_release(XMainContext *context)
{
    if (context == NULL) {
        context = x_main_context_default();
    }

    LOCK_CONTEXT(context);

    context->owner_count--;
    if (context->owner_count == 0) {
        TRACE(XLIB_MAIN_CONTEXT_RELEASE(context));

        context->owner = NULL;
        if (context->waiters) {
            XMainWaiter *waiter = (XMainWaiter *)context->waiters->data;
            xboolean loop_internal_waiter = (waiter->mutex == &context->mutex);

            context->waiters = x_slist_delete_link(context->waiters, context->waiters);
            if (!loop_internal_waiter) {
                x_mutex_lock(waiter->mutex);
            }

            x_cond_signal(waiter->cond);
            if (!loop_internal_waiter) {
                x_mutex_unlock(waiter->mutex);
            }
        }
    }

    UNLOCK_CONTEXT(context); 
}

static xboolean x_main_context_wait_internal(XMainContext *context, XCond *cond, XMutex *mutex)
{
    xboolean result = FALSE;
    XThread *self = X_THREAD_SELF;
    xboolean loop_internal_waiter;

    loop_internal_waiter = (mutex == &context->mutex);
    if (!loop_internal_waiter) {
        LOCK_CONTEXT(context);
    }

    if (context->owner && context->owner != self) {
        XMainWaiter waiter;

        waiter.cond = cond;
        waiter.mutex = mutex;

        context->waiters = x_slist_append(context->waiters, &waiter);
        
        if (!loop_internal_waiter) {
            UNLOCK_CONTEXT(context);
        }

        x_cond_wait(cond, mutex);
        if (!loop_internal_waiter) {
            LOCK_CONTEXT(context);
        }

        context->waiters = x_slist_remove(context->waiters, &waiter);
    }

    if (!context->owner) {
        context->owner = self;
        x_assert(context->owner_count == 0);
    }

    if (context->owner == self) {
        context->owner_count++;
        result = TRUE;
    }

    if (!loop_internal_waiter) {
        UNLOCK_CONTEXT(context);
    }

    return result;
}

xboolean x_main_context_wait(XMainContext *context, XCond *cond, XMutex *mutex)
{
    if (context == NULL) {
        context = x_main_context_default();
    }

    if (X_UNLIKELY(cond != &context->cond || mutex != &context->mutex)) {
        static xboolean warned;
        if (!warned) {
            x_critical("WARNING!! x_main_context_wait() will be removed in a future release. If you see this message, please file a bug immediately.");
            warned = TRUE;
        }
    }

    return x_main_context_wait_internal(context, cond, mutex);
}

xboolean x_main_context_prepare(XMainContext *context, xint *priority)
{
    xuint i;
    XSource *source;
    XSourceIter iter;
    xint n_ready = 0;
    xint current_priority = X_MAXINT;

    if (context == NULL) {
        context = x_main_context_default();
    }

    LOCK_CONTEXT(context);

    context->time_is_fresh = FALSE;

    if (context->in_check_or_prepare) {
        x_warning("x_main_context_prepare() called recursively from within a source's check() or prepare() member.");
        UNLOCK_CONTEXT(context);
        return FALSE;
    }

    TRACE(XLIB_MAIN_CONTEXT_BEFORE_PREPARE(context));

    for (i = 0; i < context->pending_dispatches->len; i++) {
        if (context->pending_dispatches->pdata[i]) {
            x_source_unref_internal((XSource *)context->pending_dispatches->pdata[i], context, TRUE);
        }
    }
    x_ptr_array_set_size(context->pending_dispatches, 0);

    context->timeout = -1;

    x_source_iter_init(&iter, context, TRUE);
    while (x_source_iter_next(&iter, &source)) {
        xint source_timeout = -1;

        if (SOURCE_DESTROYED(source) || SOURCE_BLOCKED(source)) {
            continue;
        }

        if ((n_ready > 0) && (source->priority > current_priority)) {
            break;
        }

        if (!(source->flags & X_SOURCE_READY)) {
            xboolean result;
            xboolean (*prepare)(XSource *source, xint *timeout);

            prepare = source->source_funcs->prepare;
            if (prepare) {
                xint64 begin_time_nsec X_GNUC_UNUSED;

                context->in_check_or_prepare++;
                UNLOCK_CONTEXT(context);

                begin_time_nsec = X_TRACE_CURRENT_TIME;

                result = (*prepare)(source, &source_timeout);
                TRACE(XLIB_MAIN_AFTER_PREPARE(source, prepare, source_timeout));

                x_trace_mark(begin_time_nsec, X_TRACE_CURRENT_TIME - begin_time_nsec, "XLib", "XSource.prepare", "%s ⇒ %s", (x_source_get_name(source) != NULL) ? x_source_get_name(source) : "(unnamed)", result ? "ready" : "unready");

                LOCK_CONTEXT(context);
                context->in_check_or_prepare--;
            } else {
                source_timeout = -1;
                result = FALSE;
            }

            if (result == FALSE && source->priv->ready_time != -1) {
                if (!context->time_is_fresh) {
                    context->time = x_get_monotonic_time();
                    context->time_is_fresh = TRUE;
                }

                if (source->priv->ready_time <= context->time) {
                    source_timeout = 0;
                    result = TRUE;
                } else {
                    xint64 timeout;

                    timeout = (source->priv->ready_time - context->time + 999) / 1000;
                    if (source_timeout < 0 || timeout < source_timeout) {
                        source_timeout = MIN(timeout, X_MAXINT);
                    }
                }
            }

            if (result) {
                XSource *ready_source = source;

                while (ready_source) {
                    ready_source->flags |= X_SOURCE_READY;
                    ready_source = ready_source->priv->parent_source;
                }
            }
        }

        if (source->flags & X_SOURCE_READY) {
            n_ready++;
            current_priority = source->priority;
            context->timeout = 0;
        }
        
        if (source_timeout >= 0)  {
            if (context->timeout < 0) {
                context->timeout = source_timeout;
            } else {
                context->timeout = MIN(context->timeout, source_timeout);
            }
        }
    }
    x_source_iter_clear(&iter);

    TRACE(XLIB_MAIN_CONTEXT_AFTER_PREPARE(context, current_priority, n_ready));
    UNLOCK_CONTEXT(context);

    if (priority) {
        *priority = current_priority;
    }

    return (n_ready > 0);
}

xint x_main_context_query(XMainContext *context, xint max_priority, xint *timeout, XPollFD *fds, xint n_fds)
{
    xint n_poll;
    xushort events;
    XPollRec *pollrec, *lastpollrec;

    if (context == NULL) {
        context = x_main_context_default();
    }

    LOCK_CONTEXT(context);

    TRACE(XLIB_MAIN_CONTEXT_BEFORE_QUERY(context, max_priority));

    n_poll = 0;
    lastpollrec = NULL;
    for (pollrec = context->poll_records; pollrec; pollrec = pollrec->next) {
        if (pollrec->priority > max_priority) {
            continue;
        }

        events = pollrec->fd->events & ~(X_IO_ERR|X_IO_HUP|X_IO_NVAL);
        if (lastpollrec && pollrec->fd->fd == lastpollrec->fd->fd) {
            if (n_poll - 1 < n_fds) {
                fds[n_poll - 1].events |= events;
            }
        } else {
            if (n_poll < n_fds) {
                fds[n_poll].fd = pollrec->fd->fd;
                fds[n_poll].events = events;
                fds[n_poll].revents = 0;
            }

            n_poll++;
        }

        lastpollrec = pollrec;
    }

    context->poll_changed = FALSE;
    if (timeout) {
        *timeout = context->timeout;
        if (*timeout != 0) {
            context->time_is_fresh = FALSE;
        }
    }

    TRACE(xLIB_MAIN_CONTEXT_AFTER_QUERY(context, context->timeout, fds, n_poll));
    UNLOCK_CONTEXT(context);

    return n_poll;
}

xboolean x_main_context_check(XMainContext *context, xint max_priority, XPollFD *fds, xint n_fds)
{
    xint i;
    XSource *source;
    XSourceIter iter;
    xint n_ready = 0;
    XPollRec *pollrec;

    if (context == NULL) {
        context = x_main_context_default();
    }

    LOCK_CONTEXT(context);

    if (context->in_check_or_prepare) {
        x_warning("x_main_context_check() called recursively from within a source's check() or prepare() member.");
        UNLOCK_CONTEXT(context);
        return FALSE;
    }

    TRACE(XLIB_MAIN_CONTEXT_BEFORE_CHECK(context, max_priority, fds, n_fds));

    for (i = 0; i < n_fds; i++) {
        if (fds[i].fd == context->wake_up_rec.fd) {
            if (fds[i].revents) {
                TRACE(XLIB_MAIN_CONTEXT_WAKEUP_ACKNOWLEDGE(context));
                x_wakeup_acknowledge(context->wakeup);
            }

            break;
        }
    }

    if (context->poll_changed) {
        TRACE(XLIB_MAIN_CONTEXT_AFTER_CHECK(context, 0));

        UNLOCK_CONTEXT(context);
        return FALSE;
    }

    pollrec = context->poll_records;
    i = 0;

    while (pollrec && i < n_fds) {
        x_assert(i <= 0 || fds[i - 1].fd < fds[i].fd);

        while (pollrec && pollrec->fd->fd != fds[i].fd) {
            pollrec = pollrec->next;
        }

        while (pollrec && pollrec->fd->fd == fds[i].fd) {
            if (pollrec->priority <= max_priority) {
                pollrec->fd->revents = fds[i].revents & (pollrec->fd->events | X_IO_ERR | X_IO_HUP | X_IO_NVAL);
            }

            pollrec = pollrec->next;
        }

        i++;
    }

    x_source_iter_init(&iter, context, TRUE);
    while (x_source_iter_next(&iter, &source)) {
        if (SOURCE_DESTROYED(source) || SOURCE_BLOCKED(source)) {
            continue;
        }

        if ((n_ready > 0) && (source->priority > max_priority)) {
            break;
        }

        if (!(source->flags & X_SOURCE_READY)) {
            xboolean result;
            xboolean (*check)(XSource *source);

            check = source->source_funcs->check;
            if (check) {
                xint64 begin_time_nsec X_GNUC_UNUSED;

                context->in_check_or_prepare++;
                UNLOCK_CONTEXT(context);

                begin_time_nsec = X_TRACE_CURRENT_TIME;

                result = (*check)(source);

                TRACE(XLIB_MAIN_AFTER_CHECK(source, check, result));
                x_trace_mark(begin_time_nsec, X_TRACE_CURRENT_TIME - begin_time_nsec, "XLib", "XSource.check", "%s ⇒ %s", (x_source_get_name(source) != NULL) ? x_source_get_name(source) : "(unnamed)", result ? "dispatch" : "ignore");

                LOCK_CONTEXT(context);
                context->in_check_or_prepare--;
            } else {
                result = FALSE;
            }

            if (result == FALSE) {
                XSList *tmp_list;

                for (tmp_list = source->priv->fds; tmp_list; tmp_list = tmp_list->next) {
                    XPollFD *pollfd = (XPollFD *)tmp_list->data;
                    if (pollfd->revents) {
                        result = TRUE;
                        break;
                    }
                }
            }

            if (result == FALSE && source->priv->ready_time != -1) {
                if (!context->time_is_fresh) {
                    context->time = x_get_monotonic_time();
                    context->time_is_fresh = TRUE;
                }

                if (source->priv->ready_time <= context->time) {
                    result = TRUE;
                }
            }

            if (result) {
                XSource *ready_source = source;

                while (ready_source) {
                    ready_source->flags |= X_SOURCE_READY;
                    ready_source = ready_source->priv->parent_source;
                }
            }
        }

        if (source->flags & X_SOURCE_READY) {
            x_source_ref(source);
            x_ptr_array_add(context->pending_dispatches, source);

            n_ready++;
            max_priority = source->priority;
        }
    }
    x_source_iter_clear(&iter);

    TRACE(XLIB_MAIN_CONTEXT_AFTER_CHECK(context, n_ready));
    UNLOCK_CONTEXT(context);

    return n_ready > 0;
}

void x_main_context_dispatch(XMainContext *context)
{
    if (context == NULL) {
        context = x_main_context_default();
    }

    LOCK_CONTEXT(context);
    TRACE(XLIB_MAIN_CONTEXT_BEFORE_DISPATCH(context));

    if (context->pending_dispatches->len > 0) {
        x_main_dispatch(context);
    }

    TRACE(XLIB_MAIN_CONTEXT_AFTER_DISPATCH(context));
    UNLOCK_CONTEXT(context);
}

static xboolean x_main_context_iterate(XMainContext *context, xboolean block, xboolean dispatch, XThread *self)
{
    xint timeout;
    XPollFD *fds = NULL;
    xboolean some_ready;
    xint max_priority = 0;
    xint nfds, allocated_nfds;
    xint64 begin_time_nsec X_GNUC_UNUSED;

    UNLOCK_CONTEXT(context);

    begin_time_nsec = X_TRACE_CURRENT_TIME;

    if (!x_main_context_acquire(context)) {
        xboolean got_ownership;

        LOCK_CONTEXT(context);

        if (!block) {
            return FALSE;
        }

        got_ownership = x_main_context_wait_internal(context, &context->cond, &context->mutex);
        if (!got_ownership) {
            return FALSE;
        }
    } else {
        LOCK_CONTEXT(context);
    }

    if (!context->cached_poll_array) {
        context->cached_poll_array_size = context->n_poll_records;
        context->cached_poll_array = x_new(XPollFD, context->n_poll_records);
    }

    allocated_nfds = context->cached_poll_array_size;
    fds = context->cached_poll_array;

    UNLOCK_CONTEXT(context);

    x_main_context_prepare(context, &max_priority); 

    while ((nfds = x_main_context_query(context, max_priority, &timeout, fds, allocated_nfds)) > allocated_nfds) {
        LOCK_CONTEXT(context);
        x_free(fds);
        context->cached_poll_array_size = allocated_nfds = nfds;
        context->cached_poll_array = fds = x_new(XPollFD, nfds);
        UNLOCK_CONTEXT(context);
    }

    if (!block) {
        timeout = 0;
    }

    x_main_context_poll(context, timeout, max_priority, fds, nfds);
    some_ready = x_main_context_check(context, max_priority, fds, nfds);

    if (dispatch) {
        x_main_context_dispatch (context);
    }
    x_main_context_release(context);

    x_trace_mark(begin_time_nsec, X_TRACE_CURRENT_TIME - begin_time_nsec, "XLib", "x_main_context_iterate", "Context %p, %s ⇒ %s", context, block ? "blocking" : "non-blocking", some_ready ? "dispatched" : "nothing");
    LOCK_CONTEXT(context);

    return some_ready;
}

xboolean x_main_context_pending(XMainContext *context)
{
    xboolean retval;

    if (!context) {
        context = x_main_context_default();
    }

    LOCK_CONTEXT(context);
    retval = x_main_context_iterate(context, FALSE, FALSE, X_THREAD_SELF);
    UNLOCK_CONTEXT(context);

    return retval;
}

xboolean x_main_context_iteration(XMainContext *context, xboolean may_block)
{
    xboolean retval;

    if (!context) {
        context = x_main_context_default();
    }

    LOCK_CONTEXT(context);
    retval = x_main_context_iterate(context, may_block, TRUE, X_THREAD_SELF);
    UNLOCK_CONTEXT(context);

    return retval;
}

XMainLoop *x_main_loop_new(XMainContext *context, xboolean is_running)
{
    XMainLoop *loop;

    if (!context) {
        context = x_main_context_default();
    }
    x_main_context_ref(context);

    loop = x_new0(XMainLoop, 1);
    loop->context = context;
    loop->is_running = is_running != FALSE;
    loop->ref_count = 1;

    TRACE(XLIB_MAIN_LOOP_NEW(loop, context));
    return loop;
}

XMainLoop *x_main_loop_ref(XMainLoop *loop)
{
    x_return_val_if_fail(loop != NULL, NULL);
    x_return_val_if_fail(x_atomic_int_get(&loop->ref_count) > 0, NULL);

    x_atomic_int_inc(&loop->ref_count);
    return loop;
}

void x_main_loop_unref(XMainLoop *loop)
{
    x_return_if_fail(loop != NULL);
    x_return_if_fail(x_atomic_int_get(&loop->ref_count) > 0);

    if (!x_atomic_int_dec_and_test(&loop->ref_count)) {
        return;
    }

    x_main_context_unref(loop->context);
    x_free(loop);
}

void x_main_loop_run(XMainLoop *loop)
{
    XThread *self = X_THREAD_SELF;

    x_return_if_fail(loop != NULL);
    x_return_if_fail(x_atomic_int_get(&loop->ref_count) > 0);

    x_atomic_int_inc(&loop->ref_count);

    if (!x_main_context_acquire(loop->context)) {
        xboolean got_ownership = FALSE;

        LOCK_CONTEXT(loop->context);

        x_atomic_int_set(&loop->is_running, TRUE);

        while (x_atomic_int_get(&loop->is_running) && !got_ownership) {
            got_ownership = x_main_context_wait_internal(loop->context, &loop->context->cond, &loop->context->mutex);
        }

        if (!x_atomic_int_get(&loop->is_running)) {
            UNLOCK_CONTEXT(loop->context);
            if (got_ownership) {
                x_main_context_release(loop->context);
            }

            x_main_loop_unref(loop);
            return;
        }

        x_assert(got_ownership);
    } else {
        LOCK_CONTEXT(loop->context);
    }

    if (loop->context->in_check_or_prepare) {
        x_warning("x_main_loop_run(): called recursively from within a source's check() or prepare() member, iteration not possible.");
        x_main_loop_unref(loop);
        return;
    }

    x_atomic_int_set(&loop->is_running, TRUE);
    while (x_atomic_int_get(&loop->is_running)) {
        x_main_context_iterate(loop->context, TRUE, TRUE, self);
    }

    UNLOCK_CONTEXT(loop->context);

    x_main_context_release(loop->context);
    x_main_loop_unref(loop);
}

void x_main_loop_quit(XMainLoop *loop)
{
    x_return_if_fail(loop != NULL);
    x_return_if_fail(x_atomic_int_get(&loop->ref_count) > 0);

    LOCK_CONTEXT(loop->context);
    x_atomic_int_set(&loop->is_running, FALSE);
    x_wakeup_signal(loop->context->wakeup);

    x_cond_broadcast(&loop->context->cond);
    UNLOCK_CONTEXT(loop->context);

    TRACE(XLIB_MAIN_LOOP_QUIT(loop));
}

xboolean x_main_loop_is_running(XMainLoop *loop)
{
    x_return_val_if_fail(loop != NULL, FALSE);
    x_return_val_if_fail(x_atomic_int_get(&loop->ref_count) > 0, FALSE);

    return x_atomic_int_get(&loop->is_running);
}

XMainContext *x_main_loop_get_context(XMainLoop *loop)
{
    x_return_val_if_fail(loop != NULL, NULL);
    x_return_val_if_fail(x_atomic_int_get(&loop->ref_count) > 0, NULL);

    return loop->context;
}

static void x_main_context_poll(XMainContext *context, xint timeout, xint priority, XPollFD *fds, xint n_fds)
{
    XPollFunc poll_func;

    if (n_fds || timeout != 0) {
        int ret, errsv;

        LOCK_CONTEXT(context);
        poll_func = context->poll_func;
        UNLOCK_CONTEXT(context);

        ret = (*poll_func)(fds, n_fds, timeout);
        errsv = errno;
        if (ret < 0 && errsv != EINTR) {
            x_warning("poll(2) failed due to: %s.", x_strerror(errsv));
        }
    }
}

void x_main_context_add_poll(XMainContext *context, XPollFD *fd, xint priority)
{
    if (!context) {
        context = x_main_context_default();
    }

    x_return_if_fail(x_atomic_int_get(&context->ref_count) > 0);
    x_return_if_fail(fd);

    LOCK_CONTEXT(context);
    x_main_context_add_poll_unlocked(context, priority, fd);
    UNLOCK_CONTEXT(context);
}

static void x_main_context_add_poll_unlocked(XMainContext *context, xint priority, XPollFD *fd)
{
    XPollRec *prevrec, *nextrec;
    XPollRec *newrec = x_slice_new(XPollRec);

    fd->revents = 0;
    newrec->fd = fd;
    newrec->priority = priority;

    prevrec = NULL;
    nextrec = context->poll_records;
    while (nextrec) {
        if (nextrec->fd->fd > fd->fd) {
            break;
        }

        prevrec = nextrec;
        nextrec = nextrec->next;
    }

    if (prevrec) {
        prevrec->next = newrec;
    } else {
        context->poll_records = newrec;
    }

    newrec->prev = prevrec;
    newrec->next = nextrec;

    if (nextrec) {
        nextrec->prev = newrec;
    }

    context->n_poll_records++;
    context->poll_changed = TRUE;

    if (fd != &context->wake_up_rec) {
        x_wakeup_signal(context->wakeup);
    }
}

void x_main_context_remove_poll(XMainContext *context, XPollFD *fd)
{
    if (!context) {
        context = x_main_context_default();
    }

    x_return_if_fail(x_atomic_int_get(&context->ref_count) > 0);
    x_return_if_fail(fd);

    LOCK_CONTEXT(context);
    x_main_context_remove_poll_unlocked(context, fd);
    UNLOCK_CONTEXT(context);
}

static void x_main_context_remove_poll_unlocked(XMainContext *context, XPollFD *fd)
{
    XPollRec *pollrec, *prevrec, *nextrec;

    prevrec = NULL;
    pollrec = context->poll_records;

    while (pollrec) {
        nextrec = pollrec->next;
        if (pollrec->fd == fd) {
            if (prevrec != NULL) {
                prevrec->next = nextrec;
            } else {
                context->poll_records = nextrec;
            }

            if (nextrec != NULL) {
                nextrec->prev = prevrec;
            }
            x_slice_free(XPollRec, pollrec);

            context->n_poll_records--;
            break;
        }

        prevrec = pollrec;
        pollrec = nextrec;
    }

    context->poll_changed = TRUE;
    x_wakeup_signal(context->wakeup);
}

X_GNUC_BEGIN_IGNORE_DEPRECATIONS
void x_source_get_current_time(XSource *source, XTimeVal *timeval)
{
    x_get_current_time(timeval);
}
X_GNUC_END_IGNORE_DEPRECATIONS

xint64 x_source_get_time(XSource *source)
{
    xint64 result;
    XMainContext *context;

    x_return_val_if_fail(source != NULL, 0);
    x_return_val_if_fail(x_atomic_int_get(&source->ref_count) > 0, 0);
    x_return_val_if_fail(source->context != NULL, 0);

    context = source->context;

    LOCK_CONTEXT(context);
    if (!context->time_is_fresh) {
        context->time = x_get_monotonic_time();
        context->time_is_fresh = TRUE;
    }
    result = context->time;
    UNLOCK_CONTEXT(context);

    return result;
}

void x_main_context_set_poll_func(XMainContext *context, XPollFunc func)
{
    if (!context) {
        context = x_main_context_default();
    }

    x_return_if_fail(x_atomic_int_get(&context->ref_count) > 0);

    LOCK_CONTEXT(context);
    if (func) {
        context->poll_func = func;
    } else {
        context->poll_func = x_poll;
    }
    UNLOCK_CONTEXT(context);
}

XPollFunc x_main_context_get_poll_func(XMainContext *context)
{
    XPollFunc result;

    if (!context) {
        context = x_main_context_default();
    }

    x_return_val_if_fail(x_atomic_int_get(&context->ref_count) > 0, NULL);

    LOCK_CONTEXT(context);
    result = context->poll_func;
    UNLOCK_CONTEXT(context);

    return result;
}

void x_main_context_wakeup(XMainContext *context)
{
    if (!context) {
        context = x_main_context_default();
    }

    x_return_if_fail(x_atomic_int_get(&context->ref_count) > 0);
    TRACE(XLIB_MAIN_CONTEXT_WAKEUP(context));

    x_wakeup_signal(context->wakeup);
}

xboolean x_main_context_is_owner(XMainContext *context)
{
    xboolean is_owner;

    if (!context) {
        context = x_main_context_default();
    }

    LOCK_CONTEXT(context);
    is_owner = context->owner == X_THREAD_SELF;
    UNLOCK_CONTEXT(context);

    return is_owner;
}

static void x_timeout_set_expiration(XTimeoutSource *timeout_source, xint64 current_time)
{
    xint64 expiration;

    if (timeout_source->seconds) {
        xint64 remainder;
        static xint timer_perturb = -1;

        if (timer_perturb == -1) {
            const char *session_bus_address = x_getenv("DBUS_SESSION_BUS_ADDRESS");
            if (!session_bus_address) {
                session_bus_address = x_getenv("HOSTNAME");
            }

            if (session_bus_address) {
                timer_perturb = ABS((xint)x_str_hash(session_bus_address)) % 1000000;
            } else {
                timer_perturb = 0;
            }
        }

        expiration = current_time + (xuint64)timeout_source->interval * 1000 * 1000;
        expiration -= timer_perturb;

        remainder = expiration % 1000000;
        if (remainder >= 1000000/4) {
            expiration += 1000000;
        }

        expiration -= remainder;
        expiration += timer_perturb;
    } else {
        expiration = current_time + (xuint64) timeout_source->interval * 1000;
    }

    x_source_set_ready_time((XSource *)timeout_source, expiration);
}

static xboolean x_timeout_dispatch(XSource *source, XSourceFunc callback, xpointer user_data)
{
    xboolean again;
    XTimeoutSource *timeout_source = (XTimeoutSource *)source;

    if (!callback) {
        x_warning("Timeout source dispatched without callback. You must call x_source_set_callback().");
        return FALSE;
    }

    if (timeout_source->one_shot) {
        XSourceOnceFunc once_callback = (XSourceOnceFunc)callback;
        once_callback (user_data);
        again = X_SOURCE_REMOVE;
    } else {
        again = callback (user_data);
    }

    TRACE(XLIB_TIMEOUT_DISPATCH(source, source->context, callback, user_data, again));

    if (again) {
        x_timeout_set_expiration(timeout_source, x_source_get_time(source));
    }

    return again;
}

static XSource *timeout_source_new(xuint interval, xboolean seconds, xboolean one_shot)
{
    XSource *source = x_source_new(&x_timeout_funcs, sizeof(XTimeoutSource));
    XTimeoutSource *timeout_source = (XTimeoutSource *)source;

    timeout_source->interval = interval;
    timeout_source->seconds = seconds;
    timeout_source->one_shot = one_shot;

    x_timeout_set_expiration(timeout_source, x_get_monotonic_time());

    return source;
}

XSource *x_timeout_source_new(xuint interval)
{
    return timeout_source_new(interval, FALSE, FALSE);
}

XSource *x_timeout_source_new_seconds(xuint interval)
{
    return timeout_source_new(interval, TRUE, FALSE);
}

static xuint timeout_add_full(xint priority, xuint interval, xboolean seconds, xboolean one_shot, XSourceFunc function, xpointer data, XDestroyNotify notify)
{
    xuint id;
    XSource *source;

    x_return_val_if_fail(function != NULL, 0);

    source = timeout_source_new(interval, seconds, one_shot);
    if (priority != X_PRIORITY_DEFAULT) {
        x_source_set_priority(source, priority);
    }

    x_source_set_callback(source, function, data, notify);
    id = x_source_attach(source, NULL);

    TRACE(XLIB_TIMEOUT_ADD(source, x_main_context_default(), id, priority, interval, function, data));
    x_source_unref(source);

    return id;
}

xuint x_timeout_add_full(xint priority, xuint interval, XSourceFunc function, xpointer data, XDestroyNotify notify)
{
    return timeout_add_full(priority, interval, FALSE, FALSE, function, data, notify);
}

xuint x_timeout_add(xuint32 interval, XSourceFunc function, xpointer data)
{
    return x_timeout_add_full(X_PRIORITY_DEFAULT, interval, function, data, NULL);
}

xuint x_timeout_add_once(xuint32 interval, XSourceOnceFunc function, xpointer data)
{
    return timeout_add_full(X_PRIORITY_DEFAULT, interval, FALSE, TRUE, (XSourceFunc)function, data, NULL);
}

xuint x_timeout_add_seconds_full(xint priority, xuint32 interval, XSourceFunc function, xpointer data, XDestroyNotify notify)
{
    xuint id;
    XSource *source;

    x_return_val_if_fail(function != NULL, 0);

    source = x_timeout_source_new_seconds(interval);

    if (priority != X_PRIORITY_DEFAULT) {
        x_source_set_priority(source, priority);
    }

    x_source_set_callback(source, function, data, notify);
    id = x_source_attach(source, NULL);
    x_source_unref(source);

    return id;
}

xuint x_timeout_add_seconds(xuint interval, XSourceFunc function, xpointer data)
{
    x_return_val_if_fail(function != NULL, 0);
    return x_timeout_add_seconds_full(X_PRIORITY_DEFAULT, interval, function, data, NULL);
}

static void wake_source(XSource *source)
{
    XMainContext *context;

    X_LOCK(main_context_list);
    context = source->context;
    if (context) {
        x_wakeup_signal(context->wakeup);
    }
    X_UNLOCK(main_context_list);
}

static void dispatch_unix_signals_unlocked(void)
{
    xint i;
    XSList *node;
    xboolean pending[NSIG];

    x_atomic_int_set(&any_unix_signal_pending, 0);

    for (i = 0; i < NSIG; i++) {
        pending[i] = x_atomic_int_compare_and_exchange(&unix_signal_pending[i], 1, 0);
    }

    if (pending[SIGCHLD]) {
        for (node = unix_child_watches; node; node = node->next) {
            XChildWatchSource *source = (XChildWatchSource *)node->data;

            if (!source->using_pidfd && !x_atomic_int_get(&source->child_exited)) {
                pid_t pid;
                do {
                    x_assert(source->pid > 0);

                    pid = waitpid(source->pid, &source->child_status, WNOHANG);
                    if (pid > 0) {
                        x_atomic_int_set(&source->child_exited, TRUE);
                        wake_source ((XSource *) source);
                    } else if (pid == -1 && errno == ECHILD) {
                        x_warning("XChildWatchSource: Exit status of a child process was requested but ECHILD was received by waitpid(). See the documentation of x_child_watch_source_new() for possible causes.");
                        source->child_status = 0;
                        x_atomic_int_set(&source->child_exited, TRUE);
                        wake_source ((XSource *) source);
                    }
                } while (pid == -1 && errno == EINTR);
            }
        }
    }

    for (node = unix_signal_watches; node; node = node->next) {
        XUnixSignalWatchSource *source = (XUnixSignalWatchSource *)node->data;

        if (pending[source->signum] && x_atomic_int_compare_and_exchange(&source->pending, FALSE, TRUE)) {
            wake_source((XSource *)source);
        }
    }
}

static void dispatch_unix_signals(void)
{
    X_LOCK(unix_signal_lock);
    dispatch_unix_signals_unlocked();
    X_UNLOCK(unix_signal_lock);
}

static xboolean x_child_watch_prepare(XSource *source, xint *timeout)
{
    XChildWatchSource *child_watch_source;
    child_watch_source = (XChildWatchSource *)source;

    return x_atomic_int_get(&child_watch_source->child_exited);
}

#ifdef HAVE_PIDFD
static int siginfo_t_to_wait_status (const siginfo_t *info)
{
    switch (info->si_code) {
        case CLD_EXITED:
            return W_EXITCODE(info->si_status, 0);

        case CLD_KILLED:
            return W_EXITCODE(0, info->si_status);

        case CLD_DUMPED:
            return W_EXITCODE(0, info->si_status | WCOREFLAG);

        case CLD_CONTINUED:
            return __W_CONTINUED;

        case CLD_STOPPED:
        case CLD_TRAPPED:
        default:
            return W_STOPCODE(info->si_status);
    }
}
#endif

static xboolean x_child_watch_check(XSource *source)
{
    XChildWatchSource *child_watch_source;
    child_watch_source = (XChildWatchSource *)source;

#ifdef HAVE_PIDFD
    if (child_watch_source->using_pidfd) {
        xboolean child_exited = child_watch_source->poll.revents & X_IO_IN;

        if (child_exited) {
            siginfo_t child_info = { 0, };
            if (waitid(P_PIDFD, child_watch_source->poll.fd, &child_info, WEXITED | WNOHANG) >= 0 && child_info.si_pid != 0) {
                child_watch_source->child_status = siginfo_t_to_wait_status(&child_info);
                child_watch_source->child_exited = TRUE;
            }
        }

        return child_exited;
    }
#endif

    return x_atomic_int_get(&child_watch_source->child_exited);
}

static xboolean x_unix_signal_watch_prepare(XSource *source, xint *timeout)
{
    XUnixSignalWatchSource *unix_signal_source;
    unix_signal_source = (XUnixSignalWatchSource *)source;

    return x_atomic_int_get(&unix_signal_source->pending);
}

static xboolean x_unix_signal_watch_check(XSource *source)
{
    XUnixSignalWatchSource *unix_signal_source;
    unix_signal_source = (XUnixSignalWatchSource *)source;

    return x_atomic_int_get(&unix_signal_source->pending);
}

static xboolean x_unix_signal_watch_dispatch(XSource *source,  XSourceFunc callback, xpointer user_data)
{
    xboolean again;
    XUnixSignalWatchSource *unix_signal_source;

    unix_signal_source = (XUnixSignalWatchSource *)source;
    if (!callback) {
        x_warning("Unix signal source dispatched without callback. You must call x_source_set_callback().");
        return FALSE;
    }

    x_atomic_int_set(&unix_signal_source->pending, FALSE);
    again = (callback)(user_data);

    return again;
}

static void ref_unix_signal_handler_unlocked(int signum)
{
    x_get_worker_context ();
    unix_signal_refcount[signum]++;

    if (unix_signal_refcount[signum] == 1) {
        struct sigaction action;
        action.sa_handler = x_unix_signal_handler;
        sigemptyset(&action.sa_mask);
#ifdef SA_RESTART
        action.sa_flags = SA_RESTART | SA_NOCLDSTOP;
#else
        action.sa_flags = SA_NOCLDSTOP;
#endif
        sigaction(signum, &action, NULL);
    }
}

static void unref_unix_signal_handler_unlocked(int signum)
{
    unix_signal_refcount[signum]--;
    if (unix_signal_refcount[signum] == 0) {
        struct sigaction action;

        memset(&action, 0, sizeof (action));
        action.sa_handler = SIG_DFL;
        sigemptyset(&action.sa_mask);
        sigaction(signum, &action, NULL);
    }
}

static const xchar *signum_to_string(int signum)
{
#define SIGNAL(s)   \
    case (s):       \
        return ("XUnixSignalSource: " #s);

    switch (signum) {
        SIGNAL(SIGABRT)
        SIGNAL(SIGFPE)
        SIGNAL(SIGILL)
        SIGNAL(SIGINT)
        SIGNAL(SIGSEGV)
        SIGNAL(SIGTERM)
#ifdef SIGALRM
        SIGNAL (SIGALRM)
#endif
#ifdef SIGCHLD
        SIGNAL(SIGCHLD)
#endif
#ifdef SIGHUP
        SIGNAL(SIGHUP)
#endif
#ifdef SIGKILL
        SIGNAL(SIGKILL)
#endif
#ifdef SIGPIPE
        SIGNAL(SIGPIPE)
#endif
#ifdef SIGQUIT
        SIGNAL(SIGQUIT)
#endif
#ifdef SIGSTOP
        SIGNAL(SIGSTOP)
#endif
#ifdef SIGUSR1
        SIGNAL(SIGUSR1)
#endif
#ifdef SIGUSR2
        SIGNAL(SIGUSR2)
#endif
#ifdef SIGPOLL
        SIGNAL(SIGPOLL)
#endif
#ifdef SIGPROF
        SIGNAL(SIGPROF)
#endif
#ifdef SIGTRAP
        SIGNAL(SIGTRAP)
#endif
        default:
            return "XUnixSignalSource: Unrecognized signal";
    }
#undef SIGNAL
}

XSource *_x_main_create_unix_signal_watch(int signum)
{
    XSource *source;
    XUnixSignalWatchSource *unix_signal_source;

    source = x_source_new(&x_unix_signal_funcs, sizeof(XUnixSignalWatchSource));
    unix_signal_source = (XUnixSignalWatchSource *)source;

    unix_signal_source->signum = signum;
    unix_signal_source->pending = FALSE;

    x_source_set_static_name(source, signum_to_string(signum));

    X_LOCK(unix_signal_lock);
    ref_unix_signal_handler_unlocked (signum);
    unix_signal_watches = x_slist_prepend(unix_signal_watches, unix_signal_source);
    dispatch_unix_signals_unlocked ();
    X_UNLOCK(unix_signal_lock);

    return source;
}

static void x_unix_signal_watch_finalize(XSource *source)
{
    XUnixSignalWatchSource *unix_signal_source;
    unix_signal_source = (XUnixSignalWatchSource *)source;

    X_LOCK(unix_signal_lock);
    unref_unix_signal_handler_unlocked (unix_signal_source->signum);
    unix_signal_watches = x_slist_remove(unix_signal_watches, source);
    X_UNLOCK(unix_signal_lock);
}

static void x_child_watch_finalize(XSource *source)
{
    XChildWatchSource *child_watch_source = (XChildWatchSource *)source;

    if (child_watch_source->using_pidfd) {
        if (child_watch_source->poll.fd >= 0) {
            close (child_watch_source->poll.fd);
        }

        return;
    }

    X_LOCK(unix_signal_lock);
    unix_child_watches = x_slist_remove(unix_child_watches, source);
    unref_unix_signal_handler_unlocked(SIGCHLD);
    X_UNLOCK(unix_signal_lock);
}

static xboolean x_child_watch_dispatch(XSource *source, XSourceFunc callback, xpointer user_data)
{
    XChildWatchSource *child_watch_source;
    XChildWatchFunc child_watch_callback = (XChildWatchFunc)callback;

    child_watch_source = (XChildWatchSource *)source;

    if (!callback) {
        x_warning("Child watch source dispatched without callback. You must call x_source_set_callback().");
        return FALSE;
    }

    (child_watch_callback)(child_watch_source->pid, child_watch_source->child_status, user_data);
    return FALSE;
}

static void x_unix_signal_handler(int signum)
{
  xint saved_errno = errno;

#if defined(X_ATOMIC_LOCK_FREE) && defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4)
    x_atomic_int_set(&unix_signal_pending[signum], 1);
    x_atomic_int_set(&any_unix_signal_pending, 1);
#else
#warning "Can't use atomics in x_unix_signal_handler(): Unix signal handling will be racy"
    unix_signal_pending[signum] = 1;
    any_unix_signal_pending = 1;
#endif

    x_wakeup_signal(xlib_worker_context->wakeup);
    errno = saved_errno;
}

XSource *x_child_watch_source_new(XPid pid)
{
    XSource *source;
    XChildWatchSource *child_watch_source;
#ifdef HAVE_PIDFD
    int errsv;
#endif

    x_return_val_if_fail(pid > 0, NULL);

    source = x_source_new(&x_child_watch_funcs, sizeof(XChildWatchSource));
    child_watch_source = (XChildWatchSource *)source;

    x_source_set_static_name(source, "XChildWatchSource");
    child_watch_source->pid = pid;

#ifdef HAVE_PIDFD
    child_watch_source->poll.fd = (int)syscall(SYS_pidfd_open, pid, 0);
    errsv = errno;

    if (child_watch_source->poll.fd >= 0) {
        child_watch_source->using_pidfd = TRUE;
        child_watch_source->poll.events = X_IO_IN;
        x_source_add_poll(source, &child_watch_source->poll);

        return source;
    } else {
        x_debug("pidfd_open(%" X_PID_FORMAT ") failed with error: %s", pid, x_strerror(errsv));

    }
#endif

    X_LOCK(unix_signal_lock);
    ref_unix_signal_handler_unlocked (SIGCHLD);
    unix_child_watches = x_slist_prepend(unix_child_watches, child_watch_source);
    if (waitpid(pid, &child_watch_source->child_status, WNOHANG) > 0) {
        child_watch_source->child_exited = TRUE;
    }
    X_UNLOCK(unix_signal_lock);

    return source;
}

xuint x_child_watch_add_full(xint priority, XPid pid, XChildWatchFunc function, xpointer data, XDestroyNotify notify)
{
    xuint id;
    XSource *source;

    x_return_val_if_fail(function != NULL, 0);
    x_return_val_if_fail(pid > 0, 0);

    source = x_child_watch_source_new (pid);
    if (priority != X_PRIORITY_DEFAULT) {
        x_source_set_priority(source, priority);
    }

    x_source_set_callback(source, (XSourceFunc)function, data, notify);
    id = x_source_attach(source, NULL);
    x_source_unref(source);

    return id;
}

xuint x_child_watch_add(XPid pid, XChildWatchFunc function, xpointer data)
{
    return x_child_watch_add_full(X_PRIORITY_DEFAULT, pid, function, data, NULL);
}

static xboolean x_idle_prepare(XSource *source, xint *timeout)
{
    *timeout = 0;
    return TRUE;
}

static xboolean x_idle_check(XSource  *source)
{
    return TRUE;
}

static xboolean x_idle_dispatch(XSource *source, XSourceFunc callback, xpointer user_data)
{
    xboolean again;
    XIdleSource *idle_source = (XIdleSource *)source;

    if (!callback) {
        x_warning("Idle source dispatched without callback. You must call x_source_set_callback().");
        return FALSE;
    }

    if (idle_source->one_shot) {
        XSourceOnceFunc once_callback = (XSourceOnceFunc)callback;
        once_callback (user_data);
        again = X_SOURCE_REMOVE;
    } else {
        again = callback (user_data);
    }

    TRACE(XLIB_IDLE_DISPATCH(source, source->context, callback, user_data, again));
    return again;
}

static XSource *idle_source_new(xboolean one_shot)
{
    XSource *source;
    XIdleSource *idle_source;

    source = x_source_new(&x_idle_funcs, sizeof (XIdleSource));
    idle_source = (XIdleSource *) source;

    idle_source->one_shot = one_shot;
    x_source_set_priority(source, X_PRIORITY_DEFAULT_IDLE);

    x_source_set_static_name(source, "XIdleSource");
    return source;
}

XSource *x_idle_source_new(void)
{
    return idle_source_new(FALSE);
}

static xuint idle_add_full(xint priority, xboolean one_shot, XSourceFunc function, xpointer data, XDestroyNotify notify)
{
    xuint id;
    XSource *source;

    x_return_val_if_fail(function != NULL, 0);

    source = idle_source_new(one_shot);

    if (priority != X_PRIORITY_DEFAULT_IDLE) {
        x_source_set_priority(source, priority);
    }

    x_source_set_callback(source, function, data, notify);
    id = x_source_attach(source, NULL);

    TRACE(XLIB_IDLE_ADD(source, x_main_context_default(), id, priority, function, data));
    x_source_unref(source);

    return id;
}

xuint x_idle_add_full(xint priority, XSourceFunc function, xpointer data, XDestroyNotify notify)
{
    return idle_add_full(priority, FALSE, function, data, notify);
}

xuint x_idle_add(XSourceFunc function, xpointer data)
{
    return x_idle_add_full(X_PRIORITY_DEFAULT_IDLE, function, data, NULL);
}

xuint x_idle_add_once(XSourceOnceFunc function, xpointer data)
{
    return idle_add_full(X_PRIORITY_DEFAULT_IDLE, TRUE, (XSourceFunc)function, data, NULL);
}

xboolean x_idle_remove_by_data(xpointer data)
{
    return x_source_remove_by_funcs_user_data(&x_idle_funcs, data);
}

void x_main_context_invoke(XMainContext *context, XSourceFunc function, xpointer data)
{
    x_main_context_invoke_full(context, X_PRIORITY_DEFAULT, function, data, NULL);
}

void x_main_context_invoke_full(XMainContext *context, xint priority, XSourceFunc function, xpointer data, XDestroyNotify notify)
{
    x_return_if_fail(function != NULL);

    if (!context) {
        context = x_main_context_default();
    }

    if (x_main_context_is_owner(context)) {
        while (function(data));
        if (notify != NULL) {
            notify(data);
        }
    } else {
        XMainContext *thread_default;

        thread_default = x_main_context_get_thread_default();
        if (!thread_default) {
            thread_default = x_main_context_default();
        }

        if (thread_default == context && x_main_context_acquire(context)) {
            while (function(data));

            x_main_context_release(context);
            if (notify != NULL) {
                notify(data);
            }
        } else {
            XSource *source;

            source = x_idle_source_new();
            x_source_set_priority(source, priority);
            x_source_set_callback(source, function, data, notify);
            x_source_attach(source, context);
            x_source_unref(source);
        }
    }
}

static xpointer xlib_worker_main(xpointer data)
{
    while (TRUE) {
        x_main_context_iteration(xlib_worker_context, TRUE);
        if (x_atomic_int_get(&any_unix_signal_pending)) {
            dispatch_unix_signals();
        }
    }

    return NULL;
}

XMainContext *x_get_worker_context(void)
{
    static xsize initialised;

    if (x_once_init_enter(&initialised)) {
        sigset_t all;
        sigset_t prev_mask;

        sigfillset(&all);
        pthread_sigmask(SIG_SETMASK, &all, &prev_mask);

        xlib_worker_context = x_main_context_new();
        x_thread_new("gmain", xlib_worker_main, NULL);

        pthread_sigmask(SIG_SETMASK, &prev_mask, NULL);
        x_once_init_leave(&initialised, TRUE);
    }

    return xlib_worker_context;
}
