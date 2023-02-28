#ifndef __X_CLOSURE_H__
#define __X_CLOSURE_H__

#include "xtype.h"

X_BEGIN_DECLS

#define X_CLOSURE_NEEDS_MARSHAL(closure)            (((XClosure *)(closure))->marshal == NULL)
#define X_CLOSURE_N_NOTIFIERS(cl)                   (((cl)->n_guards << 1L) + (cl)->n_fnotifiers + (cl)->n_inotifiers)
#define X_CCLOSURE_SWAP_DATA(cclosure)	            (((XClosure *)(cclosure))->derivative_flag)

#define X_CALLBACK(f)                               ((XCallback)(f))

typedef struct _XClosure XClosure;
typedef struct _XCClosure XCClosure;
typedef struct _XClosureNotifyData XClosureNotifyData;

typedef void (*XCallback)(void);
typedef void (*XClosureNotify)(xpointer data, XClosure *closure);

typedef void (*XClosureMarshal)(XClosure *closure, XValue *return_value, xuint n_param_values, const XValue *param_values, xpointer invocation_hint, xpointer marshal_data);
typedef void (*XVaClosureMarshal)(XClosure *closure, XValue *return_value, xpointer instance, va_list args, xpointer marshal_data, int n_params, XType *param_types);

struct _XClosureNotifyData {
    xpointer       data;
    XClosureNotify notify;
};

struct _XClosure {
    xuint ref_count : 15;

    xuint meta_marshal_nouse : 1;
    xuint n_guards : 1;
    xuint n_fnotifiers : 2;
    xuint n_inotifiers : 8;
    xuint in_inotify : 1;
    xuint floating : 1;

    xuint derivative_flag : 1;

    xuint in_marshal : 1;
    xuint is_invalid : 1;

    void (*marshal)(XClosure *closure, XValue *return_value, xuint n_param_values, const XValue *param_values, xpointer invocation_hint, xpointer marshal_data);
    xpointer data;

    XClosureNotifyData *notifiers;
};

struct _XCClosure {
    XClosure closure;
    xpointer callback;
};

XLIB_AVAILABLE_IN_ALL
XClosure *x_cclosure_new(XCallback callback_func, xpointer user_data, XClosureNotify destroy_data);

XLIB_AVAILABLE_IN_ALL
XClosure *x_cclosure_new_swap(XCallback callback_func, xpointer user_data, XClosureNotify destroy_data);

XLIB_AVAILABLE_IN_ALL
XClosure *x_signal_type_cclosure_new(XType itype, xuint struct_offset);

XLIB_AVAILABLE_IN_ALL
XClosure *x_closure_ref(XClosure *closure);

XLIB_AVAILABLE_IN_ALL
void x_closure_sink(XClosure *closure);

XLIB_AVAILABLE_IN_ALL
void x_closure_unref(XClosure *closure);

XLIB_AVAILABLE_IN_ALL
XClosure *x_closure_new_simple(xuint sizeof_closure, xpointer data);

XLIB_AVAILABLE_IN_ALL
void x_closure_add_finalize_notifier(XClosure *closure, xpointer  notify_data, XClosureNotify notify_func);

XLIB_AVAILABLE_IN_ALL
void x_closure_remove_finalize_notifier(XClosure *closure, xpointer notify_data, XClosureNotify notify_func);

XLIB_AVAILABLE_IN_ALL
void x_closure_add_invalidate_notifier(XClosure *closure, xpointer notify_data, XClosureNotify notify_func);

XLIB_AVAILABLE_IN_ALL
void x_closure_remove_invalidate_notifier(XClosure *closure, xpointer notify_data, XClosureNotify notify_func);

XLIB_AVAILABLE_IN_ALL
void x_closure_add_marshal_guards(XClosure *closure, xpointer pre_marshal_data, XClosureNotify pre_marshal_notify, xpointer post_marshal_data, XClosureNotify post_marshal_notify);

XLIB_AVAILABLE_IN_ALL
void x_closure_set_marshal(XClosure *closure, XClosureMarshal marshal);

XLIB_AVAILABLE_IN_ALL
void x_closure_set_meta_marshal(XClosure *closure, xpointer marshal_data, XClosureMarshal meta_marshal);

XLIB_AVAILABLE_IN_ALL
void x_closure_invalidate(XClosure *closure);

XLIB_AVAILABLE_IN_ALL
void x_closure_invoke(XClosure *closure, XValue *return_value, xuint n_param_values, const XValue *param_values, xpointer invocation_hint);

XLIB_AVAILABLE_IN_ALL
void x_cclosure_marshal_generic(XClosure *closure, XValue *return_gvalue, xuint n_param_values, const XValue *param_values, xpointer invocation_hint, xpointer marshal_data);

XLIB_AVAILABLE_IN_ALL
void x_cclosure_marshal_generic_va(XClosure *closure, XValue *return_value, xpointer instance, va_list args_list, xpointer marshal_data, int n_params, XType *param_types);

X_END_DECLS

#endif
