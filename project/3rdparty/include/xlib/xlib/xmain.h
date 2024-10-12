#ifndef __X_MAIN_H__
#define __X_MAIN_H__

#include "xpoll.h"
#include "xtypes.h"
#include "xslist.h"
#include "xthread.h"

X_BEGIN_DECLS

typedef enum {
    X_IO_IN   XLIB_SYSDEF_POLLIN,
    X_IO_OUT  XLIB_SYSDEF_POLLOUT,
    X_IO_PRI  XLIB_SYSDEF_POLLPRI,
    X_IO_ERR  XLIB_SYSDEF_POLLERR,
    X_IO_HUP  XLIB_SYSDEF_POLLHUP,
    X_IO_NVAL XLIB_SYSDEF_POLLNVAL
} XIOCondition;

XLIB_AVAILABLE_TYPE_IN_2_72
typedef enum {
    X_MAIN_CONTEXT_FLAGS_NONE              = 0,
    X_MAIN_CONTEXT_FLAGS_OWNERLESS_POLLING = 1
} XMainContextFlags;

typedef struct _XMainLoop XMainLoop;
typedef struct _XMainContext XMainContext;

typedef struct _XSource XSource;
typedef struct _XSourcePrivate XSourcePrivate;

typedef struct _XSourceFuncs XSourceFuncs;
typedef struct _XSourceCallbackFuncs XSourceCallbackFuncs;

typedef xboolean (*XSourceFunc)(xpointer user_data);
typedef void (*XSourceOnceFunc)(xpointer user_data);
typedef void (*XChildWatchFunc)(XPid pid, xint wait_status, xpointer user_data);

#define X_SOURCE_FUNC(f)                        ((XSourceFunc)(void (*)(void))(f)) XLIB_AVAILABLE_MACRO_IN_2_58

XLIB_AVAILABLE_TYPE_IN_2_64
typedef void (*XSourceDisposeFunc)(XSource *source);

struct _XSource {
    xpointer             callback_data;
    XSourceCallbackFuncs *callback_funcs;

    const XSourceFuncs   *source_funcs;
    xuint                ref_count;

    XMainContext         *context;

    xint                 priority;
    xuint                flags;
    xuint                source_id;

    XSList               *poll_fds;
    
    XSource              *prev;
    XSource              *next;

    char                 *name;
    XSourcePrivate       *priv;
};

struct _XSourceCallbackFuncs
{
  void (*ref)(xpointer cb_data);
  void (*unref)(xpointer cb_data);
  void (*get)(xpointer cb_data, XSource *source, XSourceFunc *func, xpointer *data);
};

typedef void (*XSourceDummyMarshal)(void);
typedef xboolean (*XSourceFuncsPrepareFunc)(XSource *source, xint *timeout_);
typedef xboolean (*XSourceFuncsCheckFunc)(XSource *source);
typedef xboolean (*XSourceFuncsDispatchFunc)(XSource *source, XSourceFunc callback, xpointer user_data);
typedef void (*XSourceFuncsFinalizeFunc)(XSource *source);

struct _XSourceFuncs {
    XSourceFuncsPrepareFunc  prepare;
    XSourceFuncsCheckFunc    check;
    XSourceFuncsDispatchFunc dispatch;
    XSourceFuncsFinalizeFunc finalize;
    XSourceFunc              closure_callback;
    XSourceDummyMarshal      closure_marshal;
};

#define X_PRIORITY_HIGH                         -100
#define X_PRIORITY_DEFAULT                      0
#define X_PRIORITY_HIGH_IDLE                    100
#define X_PRIORITY_DEFAULT_IDLE                 200
#define X_PRIORITY_LOW                          300

#define X_SOURCE_REMOVE                         FALSE
#define X_SOURCE_CONTINUE                       TRUE

XLIB_AVAILABLE_IN_ALL
XMainContext *x_main_context_new(void);

X_GNUC_BEGIN_IGNORE_DEPRECATIONS

XLIB_AVAILABLE_IN_2_72
XMainContext *x_main_context_new_with_flags(XMainContextFlags flags);

X_GNUC_END_IGNORE_DEPRECATIONS

XLIB_AVAILABLE_IN_ALL
XMainContext *x_main_context_ref(XMainContext *context);

XLIB_AVAILABLE_IN_ALL
void x_main_context_unref(XMainContext *context);

XLIB_AVAILABLE_IN_ALL
XMainContext *x_main_context_default(void);

XLIB_AVAILABLE_IN_ALL
xboolean x_main_context_iteration(XMainContext *context, xboolean may_block);

XLIB_AVAILABLE_IN_ALL
xboolean x_main_context_pending(XMainContext *context);

XLIB_AVAILABLE_IN_ALL
XSource *x_main_context_find_source_by_id(XMainContext *context, xuint source_id);

XLIB_AVAILABLE_IN_ALL
XSource *x_main_context_find_source_by_user_data(XMainContext *context, xpointer user_data);

XLIB_AVAILABLE_IN_ALL
XSource *x_main_context_find_source_by_funcs_user_data(XMainContext *context, XSourceFuncs *funcs, xpointer user_data);

XLIB_AVAILABLE_IN_ALL
void x_main_context_wakeup(XMainContext *context);

XLIB_AVAILABLE_IN_ALL
xboolean x_main_context_acquire(XMainContext *context);

XLIB_AVAILABLE_IN_ALL
void x_main_context_release(XMainContext *context);

XLIB_AVAILABLE_IN_ALL
xboolean x_main_context_is_owner(XMainContext *context);

XLIB_DEPRECATED_IN_2_58_FOR(x_main_context_is_owner)
xboolean x_main_context_wait(XMainContext *context, XCond *cond, XMutex *mutex);

XLIB_AVAILABLE_IN_ALL
xboolean x_main_context_prepare(XMainContext *context, xint *priority);

XLIB_AVAILABLE_IN_ALL
xint x_main_context_query(XMainContext *context, xint max_priority, xint *timeout_, XPollFD *fds, xint n_fds);

XLIB_AVAILABLE_IN_ALL
xboolean x_main_context_check(XMainContext *context, xint max_priority, XPollFD *fds, xint n_fds);

XLIB_AVAILABLE_IN_ALL
void x_main_context_dispatch(XMainContext *context);

XLIB_AVAILABLE_IN_ALL
void x_main_context_set_poll_func(XMainContext *context, XPollFunc func);

XLIB_AVAILABLE_IN_ALL
XPollFunc x_main_context_get_poll_func(XMainContext *context);

XLIB_AVAILABLE_IN_ALL
void x_main_context_add_poll(XMainContext *context, XPollFD *fd, xint priority);

XLIB_AVAILABLE_IN_ALL
void x_main_context_remove_poll(XMainContext *context, XPollFD *fd);

XLIB_AVAILABLE_IN_ALL
xint x_main_depth(void);

XLIB_AVAILABLE_IN_ALL
XSource *x_main_current_source(void);

XLIB_AVAILABLE_IN_ALL
void x_main_context_push_thread_default(XMainContext *context);

XLIB_AVAILABLE_IN_ALL
void x_main_context_pop_thread_default(XMainContext *context);

XLIB_AVAILABLE_IN_ALL
XMainContext *x_main_context_get_thread_default(void);

XLIB_AVAILABLE_IN_ALL
XMainContext *x_main_context_ref_thread_default(void);

typedef void XMainContextPusher XLIB_AVAILABLE_TYPE_IN_2_64;

X_GNUC_BEGIN_IGNORE_DEPRECATIONS
XLIB_AVAILABLE_STATIC_INLINE_IN_2_64
static inline XMainContextPusher *x_main_context_pusher_new(XMainContext *main_context);

XLIB_AVAILABLE_STATIC_INLINE_IN_2_64
static inline XMainContextPusher *x_main_context_pusher_new(XMainContext *main_context)
{
    x_main_context_push_thread_default(main_context);
    return (XMainContextPusher *)main_context;
}
X_GNUC_END_IGNORE_DEPRECATIONS

X_GNUC_BEGIN_IGNORE_DEPRECATIONS
XLIB_AVAILABLE_STATIC_INLINE_IN_2_64
static inline void x_main_context_pusher_free(XMainContextPusher *pusher);

XLIB_AVAILABLE_STATIC_INLINE_IN_2_64
static inline void x_main_context_pusher_free(XMainContextPusher *pusher)
{
    x_main_context_pop_thread_default((XMainContext *)pusher);
}
X_GNUC_END_IGNORE_DEPRECATIONS

XLIB_AVAILABLE_IN_ALL
XMainLoop *x_main_loop_new(XMainContext *context, xboolean is_running);

XLIB_AVAILABLE_IN_ALL
void x_main_loop_run(XMainLoop *loop);

XLIB_AVAILABLE_IN_ALL
void x_main_loop_quit(XMainLoop *loop);

XLIB_AVAILABLE_IN_ALL
XMainLoop *x_main_loop_ref(XMainLoop *loop);

XLIB_AVAILABLE_IN_ALL
void x_main_loop_unref(XMainLoop *loop);

XLIB_AVAILABLE_IN_ALL
xboolean x_main_loop_is_running(XMainLoop *loop);

XLIB_AVAILABLE_IN_ALL
XMainContext *x_main_loop_get_context(XMainLoop *loop);

XLIB_AVAILABLE_IN_ALL
XSource *x_source_new(XSourceFuncs *source_funcs, xuint struct_size);

X_GNUC_BEGIN_IGNORE_DEPRECATIONS
XLIB_AVAILABLE_IN_2_64
void x_source_set_dispose_function(XSource *source, XSourceDisposeFunc dispose);
X_GNUC_END_IGNORE_DEPRECATIONS

XLIB_AVAILABLE_IN_ALL
XSource *x_source_ref(XSource *source);

XLIB_AVAILABLE_IN_ALL
void x_source_unref(XSource *source);

XLIB_AVAILABLE_IN_ALL
xuint x_source_attach(XSource *source, XMainContext *context);

XLIB_AVAILABLE_IN_ALL
void x_source_destroy(XSource *source);

XLIB_AVAILABLE_IN_ALL
void x_source_set_priority(XSource *source, xint priority);

XLIB_AVAILABLE_IN_ALL
xint x_source_get_priority(XSource *source);

XLIB_AVAILABLE_IN_ALL
void x_source_set_can_recurse(XSource *source, xboolean can_recurse);

XLIB_AVAILABLE_IN_ALL
xboolean x_source_get_can_recurse(XSource *source);

XLIB_AVAILABLE_IN_ALL
xuint x_source_get_id(XSource *source);

XLIB_AVAILABLE_IN_ALL
XMainContext *x_source_get_context(XSource *source);

XLIB_AVAILABLE_IN_ALL
void x_source_set_callback(XSource *source, XSourceFunc func, xpointer data, XDestroyNotify notify);

XLIB_AVAILABLE_IN_ALL
void x_source_set_funcs(XSource *source, XSourceFuncs *funcs);

XLIB_AVAILABLE_IN_ALL
xboolean x_source_is_destroyed(XSource *source);

XLIB_AVAILABLE_IN_ALL
void x_source_set_name(XSource *source, const char *name);

XLIB_AVAILABLE_IN_2_70
void x_source_set_static_name(XSource *source, const char *name);

XLIB_AVAILABLE_IN_ALL
const char *x_source_get_name(XSource *source);

XLIB_AVAILABLE_IN_ALL
void x_source_set_name_by_id(xuint tag, const char *name);

XLIB_AVAILABLE_IN_2_36
void x_source_set_ready_time(XSource *source, xint64 ready_time);

XLIB_AVAILABLE_IN_2_36
xint64 x_source_get_ready_time(XSource *source);

XLIB_AVAILABLE_IN_2_36
xpointer x_source_add_unix_fd(XSource *source, xint fd, XIOCondition events);

XLIB_AVAILABLE_IN_2_36
void x_source_modify_unix_fd(XSource *source, xpointer tag, XIOCondition new_events);

XLIB_AVAILABLE_IN_2_36
void x_source_remove_unix_fd (XSource *source, xpointer tag);

XLIB_AVAILABLE_IN_2_36
XIOCondition x_source_query_unix_fd(XSource *source, xpointer tag);

XLIB_AVAILABLE_IN_ALL
void x_source_set_callback_indirect(XSource *source, xpointer callback_data, XSourceCallbackFuncs *callback_funcs);

XLIB_AVAILABLE_IN_ALL
void x_source_add_poll(XSource *source, XPollFD *fd);

XLIB_AVAILABLE_IN_ALL
void x_source_remove_poll(XSource *source, XPollFD *fd);

XLIB_AVAILABLE_IN_ALL
void x_source_add_child_source(XSource *source, XSource *child_source);

XLIB_AVAILABLE_IN_ALL
void x_source_remove_child_source(XSource *source, XSource *child_source);

X_GNUC_BEGIN_IGNORE_DEPRECATIONS
XLIB_DEPRECATED_IN_2_28_FOR(x_source_get_time)
void x_source_get_current_time(XSource *source, XTimeVal *timeval);
X_GNUC_END_IGNORE_DEPRECATIONS

XLIB_AVAILABLE_IN_ALL
xint64 x_source_get_time(XSource *source);

XLIB_AVAILABLE_IN_ALL
XSource *x_idle_source_new(void);

XLIB_AVAILABLE_IN_ALL
XSource *x_child_watch_source_new (XPid pid);

XLIB_AVAILABLE_IN_ALL
XSource *x_timeout_source_new(xuint interval);

XLIB_AVAILABLE_IN_ALL
XSource *x_timeout_source_new_seconds(xuint interval);

X_GNUC_BEGIN_IGNORE_DEPRECATIONS
XLIB_DEPRECATED_IN_2_62_FOR(x_get_real_time)
void x_get_current_time(XTimeVal *result);
X_GNUC_END_IGNORE_DEPRECATIONS

XLIB_AVAILABLE_IN_ALL
xint64 x_get_monotonic_time(void);

XLIB_AVAILABLE_IN_ALL
xint64 x_get_real_time(void);

XLIB_AVAILABLE_IN_ALL
xboolean x_source_remove(xuint tag);

XLIB_AVAILABLE_IN_ALL
xboolean x_source_remove_by_user_data(xpointer user_data);

XLIB_AVAILABLE_IN_ALL
xboolean x_source_remove_by_funcs_user_data(XSourceFuncs *funcs, xpointer user_data);

typedef void (*XClearHandleFunc)(xuint handle_id);

XLIB_AVAILABLE_IN_2_56
void x_clear_handle_id(xuint *tag_ptr, XClearHandleFunc clear_func);

#define x_clear_handle_id(tag_ptr, clear_func)                  \
    X_STMT_START {                                              \
        X_STATIC_ASSERT(sizeof *(tag_ptr) == sizeof(xuint));    \
        xuint *_tag_ptr = (xuint *)(tag_ptr);                   \
        xuint _handle_id;                                       \
                                                                \
        _handle_id = *_tag_ptr;                                 \
        if (_handle_id > 0) {                                   \
            *_tag_ptr = 0;                                      \
            clear_func(_handle_id);                             \
        }                                                       \
    } X_STMT_END                                                \
    XLIB_AVAILABLE_MACRO_IN_2_56

XLIB_AVAILABLE_STATIC_INLINE_IN_2_84
static inline unsigned int x_steal_handle_id(unsigned int *handle_pointer);

XLIB_AVAILABLE_STATIC_INLINE_IN_2_84
static inline unsigned int x_steal_handle_id(unsigned int *handle_pointer)
{
    unsigned int handle;
    handle = *handle_pointer;
    *handle_pointer = 0;
    return handle;
}

XLIB_AVAILABLE_IN_ALL
xuint x_timeout_add_full(xint priority, xuint interval, XSourceFunc function, xpointer data, XDestroyNotify notify);

XLIB_AVAILABLE_IN_ALL
xuint x_timeout_add(xuint interval, XSourceFunc function, xpointer data);

XLIB_AVAILABLE_IN_2_74
xuint x_timeout_add_once(xuint interval, XSourceOnceFunc function, xpointer data);

XLIB_AVAILABLE_IN_ALL
xuint x_timeout_add_seconds_full(xint priority, xuint interval, XSourceFunc function, xpointer data, XDestroyNotify notify);

XLIB_AVAILABLE_IN_ALL
xuint x_timeout_add_seconds(xuint interval, XSourceFunc function, xpointer data);

XLIB_AVAILABLE_IN_2_78
xuint x_timeout_add_seconds_once(xuint interval, XSourceOnceFunc function, xpointer data);

XLIB_AVAILABLE_IN_ALL
xuint x_child_watch_add_full(xint priority, XPid pid, XChildWatchFunc function, xpointer data, XDestroyNotify notify);

XLIB_AVAILABLE_IN_ALL
xuint x_child_watch_add(XPid pid, XChildWatchFunc function, xpointer data);

XLIB_AVAILABLE_IN_ALL
xuint x_idle_add(XSourceFunc function, xpointer data);

XLIB_AVAILABLE_IN_ALL
xuint x_idle_add_full(xint priority, XSourceFunc function, xpointer data, XDestroyNotify notify);

XLIB_AVAILABLE_IN_2_74
xuint x_idle_add_once(XSourceOnceFunc function, xpointer data);

XLIB_AVAILABLE_IN_ALL
xboolean x_idle_remove_by_data(xpointer data);

XLIB_AVAILABLE_IN_ALL
void x_main_context_invoke_full(XMainContext *context, xint priority, XSourceFunc function, xpointer data, XDestroyNotify notify);

XLIB_AVAILABLE_IN_ALL
void x_main_context_invoke(XMainContext *context, XSourceFunc function, xpointer data);

XLIB_AVAILABLE_STATIC_INLINE_IN_2_70
static inline int x_steal_fd(int *fd_ptr);

XLIB_AVAILABLE_STATIC_INLINE_IN_2_70
static inline int x_steal_fd(int *fd_ptr)
{
    int fd = *fd_ptr;
    *fd_ptr = -1;
    return fd;
}

XLIB_VAR XSourceFuncs x_idle_funcs;
XLIB_VAR XSourceFuncs x_timeout_funcs;
XLIB_VAR XSourceFuncs x_child_watch_funcs;
XLIB_VAR XSourceFuncs x_unix_signal_funcs;
XLIB_VAR XSourceFuncs x_unix_fd_source_funcs;

X_END_DECLS

#endif
