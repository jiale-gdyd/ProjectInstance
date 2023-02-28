#include <xlib/xlib/config.h>
#include <xlib/xobj/xboxed.h>
#include <xlib/xobj/xenums.h>
#include <xlib/xobj/xvalue.h>
#include <xlib/xobj/xmarshal.h>
#include <xlib/xlib/xlib-unix.h>
#include <xlib/xobj/xvaluetypes.h>
#include <xlib/xobj/xsourceclosure.h>

X_DEFINE_BOXED_TYPE(XIOChannel, x_io_channel, x_io_channel_ref, x_io_channel_unref)

XType x_io_condition_get_type(void)
{
    static XType etype = 0;

    if (x_once_init_enter(&etype)) {
        static const XFlagsValue values[] = {
            { X_IO_IN,   "X_IO_IN",   "in" },
            { X_IO_OUT,  "X_IO_OUT",  "out" },
            { X_IO_PRI,  "X_IO_PRI",  "pri" },
            { X_IO_ERR,  "X_IO_ERR",  "err" },
            { X_IO_HUP,  "X_IO_HUP",  "hup" },
            { X_IO_NVAL, "X_IO_NVAL", "nval" },
            { 0, NULL, NULL }
        };

        XType type_id = x_flags_register_static("XIOCondition", values);
        x_once_init_leave(&etype, type_id);
    }

    return etype;
}

static void source_closure_marshal_BOOLEAN__VOID(XClosure *closure, XValue *return_value, xuint n_param_values, const XValue *param_values, xpointer invocation_hint, xpointer marshal_data)
{
    xboolean v_return;
    XSourceFunc callback;
    XCClosure *cc = (XCClosure *)closure;

    x_return_if_fail(return_value != NULL);
    x_return_if_fail(n_param_values == 0);

    callback = (XSourceFunc)(marshal_data ? marshal_data : cc->callback);
    v_return = callback(closure->data);

    x_value_set_boolean(return_value, v_return);
}

static xboolean io_watch_closure_callback(XIOChannel *channel, XIOCondition condition, xpointer data)
{
    XClosure *closure = (XClosure *)data;

    XValue params[2] = { X_VALUE_INIT, X_VALUE_INIT };
    XValue result_value = X_VALUE_INIT;
    xboolean result;

    x_value_init(&result_value, X_TYPE_BOOLEAN);
    x_value_init(&params[0], X_TYPE_IO_CHANNEL);
    x_value_set_boxed(&params[0], channel);

    x_value_init(&params[1], X_TYPE_IO_CONDITION);
    x_value_set_flags(&params[1], condition);

    x_closure_invoke (closure, &result_value, 2, params, NULL);

    result = x_value_get_boolean(&result_value);
    x_value_unset(&result_value);
    x_value_unset(&params[0]);
    x_value_unset(&params[1]);

    return result;
}

static xboolean x_child_watch_closure_callback(XPid pid, xint status, xpointer data)
{
    xboolean result;
    XValue result_value = X_VALUE_INIT;
    XClosure *closure = (XClosure *)data;
    XValue params[2] = { X_VALUE_INIT, X_VALUE_INIT };

    x_value_init(&result_value, X_TYPE_BOOLEAN);

    x_value_init(&params[0], X_TYPE_ULONG);
    x_value_set_ulong(&params[0], pid);

    x_value_init(&params[1], X_TYPE_INT);
    x_value_set_int(&params[1], status);

    x_closure_invoke(closure, &result_value, 2, params, NULL);

    result = x_value_get_boolean(&result_value);
    x_value_unset(&result_value);
    x_value_unset(&params[0]);
    x_value_unset(&params[1]);

    return result;
}

static xboolean x_unix_fd_source_closure_callback(int fd, XIOCondition condition, xpointer data)
{
    xboolean result;
    XValue result_value = X_VALUE_INIT;
    XClosure *closure = (XClosure *)data;
    XValue params[2] = { X_VALUE_INIT, X_VALUE_INIT };

    x_value_init(&result_value, X_TYPE_BOOLEAN);

    x_value_init(&params[0], X_TYPE_INT);
    x_value_set_int(&params[0], fd);

    x_value_init(&params[1], X_TYPE_IO_CONDITION);
    x_value_set_flags(&params[1], condition);

    x_closure_invoke(closure, &result_value, 2, params, NULL);

    result = x_value_get_boolean(&result_value);
    x_value_unset(&result_value);
    x_value_unset(&params[0]);
    x_value_unset(&params[1]);

    return result;
}

static xboolean source_closure_callback(xpointer data)
{
    xboolean result;
    XValue result_value = X_VALUE_INIT;
    XClosure *closure = (XClosure *)data;

    x_value_init(&result_value, X_TYPE_BOOLEAN);
    x_closure_invoke(closure, &result_value, 0, NULL, NULL);

    result = x_value_get_boolean(&result_value);
    x_value_unset(&result_value);

    return result;
}

static void closure_callback_get(xpointer cb_data, XSource *source, XSourceFunc *func, xpointer *data)
{
    XSourceFunc closure_callback = source->source_funcs->closure_callback;

    if (!closure_callback) {
        if (source->source_funcs == &x_io_watch_funcs) {
            closure_callback = (XSourceFunc)io_watch_closure_callback;
        } else if (source->source_funcs == &x_child_watch_funcs) {
            closure_callback = (XSourceFunc)x_child_watch_closure_callback;
        } else if (source->source_funcs == &x_unix_fd_source_funcs) {
            closure_callback = (XSourceFunc)x_unix_fd_source_closure_callback;
        } else if (source->source_funcs == &x_timeout_funcs || source->source_funcs == &x_unix_signal_funcs || source->source_funcs == &x_idle_funcs) {
            closure_callback = source_closure_callback;
        }
    }

    *func = closure_callback;
    *data = cb_data;
}

static XSourceCallbackFuncs closure_callback_funcs = {
    (void (*)(xpointer))x_closure_ref,
    (void (*)(xpointer))x_closure_unref,
    closure_callback_get
};

static void closure_invalidated(xpointer user_data, XClosure *closure)
{
    x_source_destroy((XSource *)user_data);
}

void x_source_set_closure(XSource *source, XClosure *closure)
{
    x_return_if_fail(source != NULL);
    x_return_if_fail(closure != NULL);

    if (!source->source_funcs->closure_callback
        && source->source_funcs != &x_unix_fd_source_funcs
        && source->source_funcs != &x_unix_signal_funcs
        && source->source_funcs != &x_child_watch_funcs
        && source->source_funcs != &x_io_watch_funcs
        && source->source_funcs != &x_timeout_funcs
        && source->source_funcs != &x_idle_funcs)
    {
        x_critical(X_STRLOC ": closure cannot be set on XSource without XSourceFuncs::closure_callback");
        return;
    }

    x_closure_ref(closure);
    x_closure_sink(closure);
    x_source_set_callback_indirect(source, closure, &closure_callback_funcs);

    x_closure_add_invalidate_notifier(closure, source, closure_invalidated);

    if (X_CLOSURE_NEEDS_MARSHAL(closure)) {
        XClosureMarshal marshal = (XClosureMarshal)source->source_funcs->closure_marshal;
        if (marshal) {
            x_closure_set_marshal(closure, marshal);
        } else if (source->source_funcs == &x_idle_funcs || source->source_funcs == &x_unix_signal_funcs || source->source_funcs == &x_timeout_funcs) {
            x_closure_set_marshal(closure, source_closure_marshal_BOOLEAN__VOID);
        } else {
            x_closure_set_marshal(closure, x_cclosure_marshal_generic);
        }
    }
}

static void dummy_closure_marshal(XClosure *closure, XValue *return_value, xuint n_param_values, const XValue *param_values, xpointer invocation_hint, xpointer marshal_data)
{
    if (X_VALUE_HOLDS_BOOLEAN(return_value)) {
        x_value_set_boolean(return_value, TRUE);
    }
}

void x_source_set_dummy_callback(XSource *source)
{
    XClosure *closure;

    closure = x_closure_new_simple(sizeof(XClosure), NULL);
    x_closure_set_meta_marshal(closure, NULL, dummy_closure_marshal);
    x_source_set_closure(source, closure);
}
