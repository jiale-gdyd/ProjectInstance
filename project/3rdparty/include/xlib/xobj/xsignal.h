#ifndef __X_SIGNAL_H__
#define __X_SIGNAL_H__

#include "xvalue.h"
#include "xparam.h"
#include "xclosure.h"
#include "xmarshal.h"

X_BEGIN_DECLS

typedef struct _XSignalQuery XSignalQuery;
typedef struct _XSignalInvocationHint XSignalInvocationHint;

typedef XClosureMarshal XSignalCMarshaller;
typedef XVaClosureMarshal XSignalCVaMarshaller;

typedef xboolean (*XSignalEmissionHook)(XSignalInvocationHint *ihint, xuint n_param_values, const XValue *param_values, xpointer data);
typedef xboolean (*XSignalAccumulator)(XSignalInvocationHint *ihint, XValue *return_accu, const XValue *handler_return, xpointer data);

typedef enum {
    X_SIGNAL_RUN_FIRST             = 1 << 0,
    X_SIGNAL_RUN_LAST              = 1 << 1,
    X_SIGNAL_RUN_CLEANUP           = 1 << 2,
    X_SIGNAL_NO_RECURSE            = 1 << 3,
    X_SIGNAL_DETAILED              = 1 << 4,
    X_SIGNAL_ACTION                = 1 << 5,
    X_SIGNAL_NO_HOOKS              = 1 << 6,
    X_SIGNAL_MUST_COLLECT          = 1 << 7,
    X_SIGNAL_DEPRECATED            = 1 << 8,
    X_SIGNAL_ACCUMULATOR_FIRST_RUN = 1 << 17,
} XSignalFlags;

#define X_SIGNAL_FLAGS_MASK             0x1ff

typedef enum {
    X_CONNECT_DEFAULT XLIB_AVAILABLE_ENUMERATOR_IN_2_74 = 0,
    X_CONNECT_AFTER   = 1 << 0,
    X_CONNECT_SWAPPED = 1 << 1
} XConnectFlags;

typedef enum {
    X_SIGNAL_MATCH_ID        = 1 << 0,
    X_SIGNAL_MATCH_DETAIL    = 1 << 1,
    X_SIGNAL_MATCH_CLOSURE   = 1 << 2,
    X_SIGNAL_MATCH_FUNC      = 1 << 3,
    X_SIGNAL_MATCH_DATA      = 1 << 4,
    X_SIGNAL_MATCH_UNBLOCKED = 1 << 5
} XSignalMatchType;

#define X_SIGNAL_MATCH_MASK             0x3f
#define X_SIGNAL_TYPE_STATIC_SCOPE      (X_TYPE_FLAG_RESERVED_ID_BIT)

struct _XSignalInvocationHint {
    xuint        signal_id;
    XQuark       detail;
    XSignalFlags run_type;
};

struct _XSignalQuery {
    xuint        signal_id;
    const xchar  *signal_name;
    XType        itype;
    XSignalFlags signal_flags;
    XType        return_type;
    xuint        n_params;
    const XType  *param_types;
};

XLIB_AVAILABLE_IN_ALL
xuint x_signal_newv(const xchar *signal_name, XType itype, XSignalFlags signal_flags, XClosure *class_closure, XSignalAccumulator accumulator, xpointer accu_data, XSignalCMarshaller c_marshaller, XType return_type, xuint n_params, XType *param_types);

XLIB_AVAILABLE_IN_ALL
xuint x_signal_new_valist(const xchar *signal_name, XType itype, XSignalFlags signal_flags, XClosure *class_closure, XSignalAccumulator accumulator, xpointer accu_data, XSignalCMarshaller c_marshaller, XType return_type, xuint n_params, va_list args);

XLIB_AVAILABLE_IN_ALL
xuint x_signal_new(const xchar *signal_name, XType itype, XSignalFlags signal_flags, xuint class_offset, XSignalAccumulator accumulator, xpointer accu_data, XSignalCMarshaller c_marshaller, XType return_type, xuint n_params, ...);

XLIB_AVAILABLE_IN_ALL
xuint x_signal_new_class_handler(const xchar *signal_name, XType itype, XSignalFlags signal_flags, XCallback class_handler, XSignalAccumulator accumulator, xpointer accu_data, XSignalCMarshaller c_marshaller, XType return_type, xuint n_params, ...);

XLIB_AVAILABLE_IN_ALL
void x_signal_set_va_marshaller(xuint signal_id, XType instance_type, XSignalCVaMarshaller va_marshaller);

XLIB_AVAILABLE_IN_ALL
void x_signal_emitv(const XValue *instance_and_params, xuint signal_id, XQuark detail, XValue *return_value);

XLIB_AVAILABLE_IN_ALL
void x_signal_emit_valist(xpointer instance, xuint signal_id, XQuark detail, va_list var_args);

XLIB_AVAILABLE_IN_ALL
void x_signal_emit(xpointer instance, xuint signal_id, XQuark detail, ...);

XLIB_AVAILABLE_IN_ALL
void x_signal_emit_by_name(xpointer instance, const xchar *detailed_signal, ...);

XLIB_AVAILABLE_IN_ALL
xuint x_signal_lookup(const xchar *name, XType itype);

XLIB_AVAILABLE_IN_ALL
const xchar *x_signal_name(xuint signal_id);

XLIB_AVAILABLE_IN_ALL
void x_signal_query(xuint signal_id, XSignalQuery *query);

XLIB_AVAILABLE_IN_ALL
xuint *x_signal_list_ids(XType itype, xuint *n_ids);

XLIB_AVAILABLE_IN_2_66
xboolean x_signal_is_valid_name(const xchar *name);

XLIB_AVAILABLE_IN_ALL
xboolean x_signal_parse_name(const xchar *detailed_signal, XType itype, xuint *signal_id_p, XQuark *detail_p, xboolean force_detail_quark);

XLIB_AVAILABLE_IN_ALL
XSignalInvocationHint *x_signal_get_invocation_hint(xpointer instance);

XLIB_AVAILABLE_IN_ALL
void x_signal_stop_emission(xpointer instance, xuint signal_id, XQuark detail);

XLIB_AVAILABLE_IN_ALL
void x_signal_stop_emission_by_name(xpointer instance, const xchar *detailed_signal);

XLIB_AVAILABLE_IN_ALL
xulong x_signal_add_emission_hook(xuint signal_id, XQuark detail, XSignalEmissionHook hook_func, xpointer hook_data, XDestroyNotify data_destroy);

XLIB_AVAILABLE_IN_ALL
void x_signal_remove_emission_hook(xuint signal_id, xulong hook_id);

XLIB_AVAILABLE_IN_ALL
xboolean x_signal_has_handler_pending(xpointer instance, xuint signal_id, XQuark detail, xboolean may_be_blocked);

XLIB_AVAILABLE_IN_ALL
xulong x_signal_connect_closure_by_id(xpointer instance, xuint signal_id, XQuark detail, XClosure *closure, xboolean after);

XLIB_AVAILABLE_IN_ALL
xulong x_signal_connect_closure(xpointer instance, const xchar *detailed_signal, XClosure *closure, xboolean after);

XLIB_AVAILABLE_IN_ALL
xulong x_signal_connect_data(xpointer instance, const xchar *detailed_signal, XCallback c_handler, xpointer data, XClosureNotify destroy_data, XConnectFlags connect_flags);

XLIB_AVAILABLE_IN_ALL
void x_signal_handler_block(xpointer instance, xulong handler_id);

XLIB_AVAILABLE_IN_ALL
void x_signal_handler_unblock(xpointer instance, xulong handler_id);

XLIB_AVAILABLE_IN_ALL
void x_signal_handler_disconnect(xpointer instance, xulong handler_id);

XLIB_AVAILABLE_IN_ALL
xboolean x_signal_handler_is_connected(xpointer instance, xulong handler_id);

XLIB_AVAILABLE_IN_ALL
xulong x_signal_handler_find(xpointer instance, XSignalMatchType mask, xuint signal_id, XQuark detail, XClosure *closure, xpointer func, xpointer data);

XLIB_AVAILABLE_IN_ALL
xuint x_signal_handlers_block_matched(xpointer instance, XSignalMatchType mask, xuint signal_id, XQuark detail, XClosure *closure, xpointer func, xpointer data);

XLIB_AVAILABLE_IN_ALL
xuint x_signal_handlers_unblock_matched(xpointer instance, XSignalMatchType mask, xuint signal_id, XQuark detail, XClosure *closure, xpointer func, xpointer data);

XLIB_AVAILABLE_IN_ALL
xuint x_signal_handlers_disconnect_matched(xpointer instance, XSignalMatchType mask, xuint signal_id, XQuark detail, XClosure *closure, xpointer func, xpointer data);

XLIB_AVAILABLE_IN_2_62
void x_clear_signal_handler(xulong *handler_id_ptr, xpointer instance);

#define x_clear_signal_handler(handler_id_ptr, instance)            \
    X_STMT_START {                                                  \
        xpointer const _instance = (instance);                      \
        xulong *const _handler_id_ptr = (handler_id_ptr);           \
        const xulong _handler_id = *_handler_id_ptr;                \
                                                                    \
        if (_handler_id > 0) {                                      \
            *_handler_id_ptr = 0;                                   \
            x_signal_handler_disconnect(_instance, _handler_id);    \
        }                                                           \
    } X_STMT_END                                                    \
    XLIB_AVAILABLE_MACRO_IN_2_62

XLIB_AVAILABLE_IN_ALL
void x_signal_override_class_closure(xuint signal_id, XType instance_type, XClosure *class_closure);

XLIB_AVAILABLE_IN_ALL
void x_signal_override_class_handler(const xchar *signal_name, XType instance_type, XCallback class_handler);

XLIB_AVAILABLE_IN_ALL
void c_signal_chain_from_overridden(const XValue *instance_and_params, XValue *return_value);

XLIB_AVAILABLE_IN_ALL
void x_signal_chain_from_overridden_handler(xpointer instance, ...);

#define x_signal_connect(instance, detailed_signal, c_handler, data)            \
    x_signal_connect_data((instance), (detailed_signal), (c_handler), (data), NULL, (XConnectFlags)0)

#define x_signal_connect_after(instance, detailed_signal, c_handler, data)      \
    x_signal_connect_data((instance), (detailed_signal), (c_handler), (data), NULL, X_CONNECT_AFTER)

#define x_signal_connect_swapped(instance, detailed_signal, c_handler, data)    \
    x_signal_connect_data((instance), (detailed_signal), (c_handler), (data), NULL, X_CONNECT_SWAPPED)

#define x_signal_handlers_disconnect_by_func(instance, func, data)              \
    x_signal_handlers_disconnect_matched((instance), (XSignalMatchType)(X_SIGNAL_MATCH_FUNC | X_SIGNAL_MATCH_DATA), 0, 0, NULL, (func), (data))

#define x_signal_handlers_disconnect_by_data(instance, data)                    \
    x_signal_handlers_disconnect_matched((instance), X_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, (data))

#define x_signal_handlers_block_by_func(instance, func, data)                   \
    x_signal_handlers_block_matched((instance), (XSignalMatchType)(X_SIGNAL_MATCH_FUNC | X_SIGNAL_MATCH_DATA), 0, 0, NULL, (func), (data))

#define x_signal_handlers_unblock_by_func(instance, func, data)                 \
    x_signal_handlers_unblock_matched((instance), (XSignalMatchType)(X_SIGNAL_MATCH_FUNC | X_SIGNAL_MATCH_DATA), 0, 0, NULL, (func), (data))

XLIB_AVAILABLE_IN_ALL
xboolean x_signal_accumulator_true_handled(XSignalInvocationHint *ihint, XValue *return_accu, const XValue *handler_return, xpointer dummy);

XLIB_AVAILABLE_IN_ALL
xboolean x_signal_accumulator_first_wins(XSignalInvocationHint *ihint, XValue *return_accu, const XValue *handler_return, xpointer dummy);

XLIB_AVAILABLE_IN_ALL
void x_signal_handlers_destroy(xpointer instance);

void _x_signals_destroy(XType itype);

X_END_DECLS

#endif
