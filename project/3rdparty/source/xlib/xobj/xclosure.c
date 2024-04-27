#include "libffi/ffi.h"

#include <string.h>
#include <xlib/xlib/config.h>
#include <xlib/xobj/xboxed.h>
#include <xlib/xobj/xenums.h>
#include <xlib/xobj/xvalue.h>
#include <xlib/xobj/xobject.h>
#include <xlib/xobj/xclosure.h>
#include <xlib/xobj/xvaluetypes.h>
#include <xlib/xobj/xtype-private.h>

#define CLOSURE_MAX_REF_COUNT                       ((1 << 15) - 1)
#define CLOSURE_MAX_N_GUARDS                        ((1 << 1) - 1)
#define CLOSURE_MAX_N_FNOTIFIERS                    ((1 << 2) - 1)
#define CLOSURE_MAX_N_INOTIFIERS                    ((1 << 8) - 1)
#define CLOSURE_N_MFUNCS(cl)                        (((cl)->n_guards << 1L))

#define CLOSURE_N_NOTIFIERS(cl)                     (CLOSURE_N_MFUNCS(cl) + (cl)->n_fnotifiers + (cl)->n_inotifiers)

typedef union {
    XClosure closure;
    xint     vint;
} ClosureInt;

#define ATOMIC_CHANGE_FIELD(_closure, _field, _OP, _value, _must_set, _SET_OLD, _SET_NEW)              \
    X_STMT_START {                                                                              \
        ClosureInt *cunion = (ClosureInt *)_closure;                                            \
        xint new_int, old_int, success;                                                         \
        do {                                                                                    \
            ClosureInt tmp;                                                                     \
            tmp.vint = old_int = cunion->vint;                                                  \
            _SET_OLD tmp.closure._field;                                                        \
            tmp.closure._field _OP _value;                                                      \
            _SET_NEW tmp.closure._field;                                                        \
            new_int = tmp.vint;                                                                 \
            success = x_atomic_int_compare_and_exchange(&cunion->vint, old_int, new_int);       \
        } while (!success && _must_set);                                                        \
    } X_STMT_END

#define ATOMIC_SWAP(_closure, _field, _value, _oldv)   ATOMIC_CHANGE_FIELD(_closure, _field, =, _value, TRUE, *(_oldv) =,     (void) )
#define ATOMIC_SET(_closure, _field, _value)           ATOMIC_CHANGE_FIELD(_closure, _field, =, _value, TRUE,     (void),     (void) )
#define ATOMIC_INC(_closure, _field)                   ATOMIC_CHANGE_FIELD(_closure, _field, +=,     1, TRUE,     (void),     (void) )
#define ATOMIC_INC_ASSIGN(_closure, _field, _newv)     ATOMIC_CHANGE_FIELD(_closure, _field, +=,     1, TRUE,     (void), *(_newv) = )
#define ATOMIC_DEC(_closure, _field)                   ATOMIC_CHANGE_FIELD(_closure, _field, -=,     1, TRUE,     (void),     (void) )
#define ATOMIC_DEC_ASSIGN(_closure, _field, _newv)     ATOMIC_CHANGE_FIELD(_closure, _field, -=,     1, TRUE,     (void), *(_newv) = )

enum {
    FNOTIFY,
    INOTIFY,
    PRE_NOTIFY,
    POST_NOTIFY
};

XClosure *x_closure_new_simple(xuint sizeof_closure, xpointer data)
{
    xchar *allocated;
    XClosure *closure;
    xint private_size;

    x_return_val_if_fail(sizeof_closure >= sizeof(XClosure), NULL);

    private_size = sizeof(XRealClosure) - sizeof(XClosure);
    allocated = (xchar *)x_malloc0(private_size + sizeof_closure);
    closure = (XClosure *)(allocated + private_size);

    ATOMIC_SET(closure, ref_count, 1);
    ATOMIC_SET(closure, floating, TRUE);
    closure->data = data;

    return closure;
}

static inline void closure_invoke_notifiers(XClosure *closure, xuint notify_type)
{
    xuint i, offs;
    XClosureNotifyData *ndata;

    switch (notify_type) {
        case FNOTIFY:
            while (closure->n_fnotifiers) {
                xuint n;
                ATOMIC_DEC_ASSIGN(closure, n_fnotifiers, &n);

                ndata = closure->notifiers + CLOSURE_N_MFUNCS(closure) + n;
                closure->marshal = (XClosureMarshal)ndata->notify;
                closure->data = ndata->data;
                ndata->notify(ndata->data, closure);
            }

            closure->marshal = NULL;
            closure->data = NULL;
            break;

        case INOTIFY:
            ATOMIC_SET(closure, in_inotify, TRUE);
            while (closure->n_inotifiers) {
                xuint n;
                ATOMIC_DEC_ASSIGN(closure, n_inotifiers, &n);

                ndata = closure->notifiers + CLOSURE_N_MFUNCS(closure) + closure->n_fnotifiers + n;
                closure->marshal = (XClosureMarshal)ndata->notify;
                closure->data = ndata->data;
                ndata->notify(ndata->data, closure);
            }

            closure->marshal = NULL;
            closure->data = NULL;
            ATOMIC_SET(closure, in_inotify, FALSE);
            break;

        case PRE_NOTIFY:
            i = closure->n_guards;
            offs = 0;
            while (i--) {
                ndata = closure->notifiers + offs + i;
                ndata->notify(ndata->data, closure);
            }
            break;

        case POST_NOTIFY:
            i = closure->n_guards;
            offs = i;
            while (i--) {
                ndata = closure->notifiers + offs + i;
                ndata->notify(ndata->data, closure);
            }
            break;
    }
}

static void x_closure_set_meta_va_marshal(XClosure *closure, XVaClosureMarshal va_meta_marshal)
{
    XRealClosure *real_closure;

    x_return_if_fail(closure != NULL);
    x_return_if_fail(va_meta_marshal != NULL);
    x_return_if_fail(closure->is_invalid == FALSE);
    x_return_if_fail(closure->in_marshal == FALSE);

    real_closure = X_REAL_CLOSURE(closure);

    x_return_if_fail(real_closure->meta_marshal != NULL);
    real_closure->va_meta_marshal = va_meta_marshal;
}

void x_closure_set_meta_marshal(XClosure *closure, xpointer marshal_data, XClosureMarshal meta_marshal)
{
    XRealClosure *real_closure;

    x_return_if_fail(closure != NULL);
    x_return_if_fail(meta_marshal != NULL);
    x_return_if_fail(closure->is_invalid == FALSE);
    x_return_if_fail(closure->in_marshal == FALSE);

    real_closure = X_REAL_CLOSURE(closure);

    x_return_if_fail(real_closure->meta_marshal == NULL);

    real_closure->meta_marshal = meta_marshal;
    real_closure->meta_marshal_data = marshal_data;
}

void x_closure_add_marshal_guards(XClosure *closure, xpointer pre_marshal_data, XClosureNotify pre_marshal_notify, xpointer post_marshal_data, XClosureNotify post_marshal_notify)
{
    xuint i;

    x_return_if_fail(closure != NULL);
    x_return_if_fail(pre_marshal_notify != NULL);
    x_return_if_fail(post_marshal_notify != NULL);
    x_return_if_fail(closure->is_invalid == FALSE);
    x_return_if_fail(closure->in_marshal == FALSE);
    x_return_if_fail(closure->n_guards < CLOSURE_MAX_N_GUARDS);

    closure->notifiers = x_renew(XClosureNotifyData, closure->notifiers, CLOSURE_N_NOTIFIERS(closure) + 2);
    if (closure->n_inotifiers) {
        closure->notifiers[(CLOSURE_N_MFUNCS(closure) + closure->n_fnotifiers + closure->n_inotifiers + 1)] = closure->notifiers[(CLOSURE_N_MFUNCS(closure) + closure->n_fnotifiers + 0)];
    }

    if (closure->n_inotifiers > 1) {
        closure->notifiers[(CLOSURE_N_MFUNCS(closure) + closure->n_fnotifiers + closure->n_inotifiers)] = closure->notifiers[(CLOSURE_N_MFUNCS(closure) + closure->n_fnotifiers + 1)];
    }

    if (closure->n_fnotifiers) {
        closure->notifiers[(CLOSURE_N_MFUNCS(closure) + closure->n_fnotifiers + 1)] = closure->notifiers[CLOSURE_N_MFUNCS(closure) + 0];
    }

    if (closure->n_fnotifiers > 1) {
        closure->notifiers[(CLOSURE_N_MFUNCS(closure) + closure->n_fnotifiers)] = closure->notifiers[CLOSURE_N_MFUNCS(closure) + 1];
    }

    if (closure->n_guards) {
        closure->notifiers[(closure->n_guards + closure->n_guards + 1)] = closure->notifiers[closure->n_guards];
    }

    i = closure->n_guards;
    closure->notifiers[i].data = pre_marshal_data;
    closure->notifiers[i].notify = pre_marshal_notify;
    closure->notifiers[i + 1].data = post_marshal_data;
    closure->notifiers[i + 1].notify = post_marshal_notify;
    ATOMIC_INC(closure, n_guards);
}

void x_closure_add_finalize_notifier(XClosure *closure, xpointer notify_data, XClosureNotify notify_func)
{
    xuint i;

    x_return_if_fail(closure != NULL);
    x_return_if_fail(notify_func != NULL);
    x_return_if_fail(closure->n_fnotifiers < CLOSURE_MAX_N_FNOTIFIERS);

    closure->notifiers = x_renew(XClosureNotifyData, closure->notifiers, CLOSURE_N_NOTIFIERS(closure) + 1);
    if (closure->n_inotifiers) {
        closure->notifiers[(CLOSURE_N_MFUNCS(closure) + closure->n_fnotifiers + closure->n_inotifiers)] = closure->notifiers[(CLOSURE_N_MFUNCS(closure) + closure->n_fnotifiers + 0)];
    }

    i = CLOSURE_N_MFUNCS(closure) + closure->n_fnotifiers;
    closure->notifiers[i].data = notify_data;
    closure->notifiers[i].notify = notify_func;
    ATOMIC_INC(closure, n_fnotifiers);
}

void x_closure_add_invalidate_notifier(XClosure *closure, xpointer notify_data, XClosureNotify notify_func)
{
    xuint i;

    x_return_if_fail(closure != NULL);
    x_return_if_fail(notify_func != NULL);
    x_return_if_fail(closure->is_invalid == FALSE);
    x_return_if_fail(closure->n_inotifiers < CLOSURE_MAX_N_INOTIFIERS);

    closure->notifiers = x_renew(XClosureNotifyData, closure->notifiers, CLOSURE_N_NOTIFIERS(closure) + 1);
    i = CLOSURE_N_MFUNCS(closure) + closure->n_fnotifiers + closure->n_inotifiers;
    closure->notifiers[i].data = notify_data;
    closure->notifiers[i].notify = notify_func;
    ATOMIC_INC(closure, n_inotifiers);
}

static inline xboolean closure_try_remove_inotify(XClosure *closure, xpointer notify_data, XClosureNotify notify_func)
{
    XClosureNotifyData *ndata, *nlast;

    nlast = closure->notifiers + CLOSURE_N_NOTIFIERS(closure) - 1;
    for (ndata = nlast + 1 - closure->n_inotifiers; ndata <= nlast; ndata++) {
        if (ndata->notify == notify_func && ndata->data == notify_data) {
            ATOMIC_DEC(closure, n_inotifiers);
            if (ndata < nlast) {
                *ndata = *nlast;
            }

            return TRUE;
        }
    }

    return FALSE;
}

static inline xboolean closure_try_remove_fnotify(XClosure *closure, xpointer notify_data, XClosureNotify notify_func)
{
    XClosureNotifyData *ndata, *nlast;

    nlast = closure->notifiers + CLOSURE_N_NOTIFIERS(closure) - closure->n_inotifiers - 1;
    for (ndata = nlast + 1 - closure->n_fnotifiers; ndata <= nlast; ndata++) {
        if (ndata->notify == notify_func && ndata->data == notify_data) {
            ATOMIC_DEC(closure, n_fnotifiers);
            if (ndata < nlast) {
                *ndata = *nlast;
            }

            if (closure->n_inotifiers) {
                closure->notifiers[(CLOSURE_N_MFUNCS(closure) + closure->n_fnotifiers)] = closure->notifiers[(CLOSURE_N_MFUNCS(closure) + closure->n_fnotifiers + closure->n_inotifiers)];
            }

            return TRUE;
        }
    }

    return FALSE;
}

XClosure *x_closure_ref(XClosure *closure)
{
    xuint new_ref_count;

    x_return_val_if_fail(closure != NULL, NULL);
    x_return_val_if_fail(closure->ref_count > 0, NULL);
    x_return_val_if_fail(closure->ref_count < CLOSURE_MAX_REF_COUNT, NULL);

    ATOMIC_INC_ASSIGN(closure, ref_count, &new_ref_count);
    x_return_val_if_fail(new_ref_count > 1, NULL);

    return closure;
}

static void closure_invalidate_internal(XClosure *closure)
{
    xboolean was_invalid;

    ATOMIC_SWAP(closure, is_invalid, TRUE, &was_invalid);
    if (!was_invalid) {
        closure_invoke_notifiers(closure, INOTIFY);
    }
}

void x_closure_invalidate(XClosure *closure)
{
    x_return_if_fail(closure != NULL);

    if (!closure->is_invalid) {
        x_closure_ref(closure);
        closure_invalidate_internal(closure);
        x_closure_unref(closure);
    }
}

void x_closure_unref(XClosure *closure)
{
    xuint new_ref_count;

    x_return_if_fail(closure != NULL);
    x_return_if_fail(closure->ref_count > 0);

    if (closure->ref_count == 1 && !closure->is_invalid) {
        closure_invalidate_internal(closure);
    }

    ATOMIC_DEC_ASSIGN(closure, ref_count, &new_ref_count);

    if (new_ref_count == 0) {
        closure_invoke_notifiers(closure, FNOTIFY);
        x_free(closure->notifiers);
        x_free(X_REAL_CLOSURE(closure));
    }
}

void x_closure_sink(XClosure *closure)
{
    x_return_if_fail(closure != NULL);
    x_return_if_fail(closure->ref_count > 0);

    if (closure->floating) {
        xboolean was_floating;
        ATOMIC_SWAP(closure, floating, FALSE, &was_floating);
        if (was_floating) {
            x_closure_unref(closure);
        }
    }
}

void x_closure_remove_invalidate_notifier(XClosure *closure, xpointer notify_data, XClosureNotify notify_func)
{
    x_return_if_fail(closure != NULL);
    x_return_if_fail(notify_func != NULL);

    if (closure->is_invalid && closure->in_inotify && ((xpointer) closure->marshal) == ((xpointer) notify_func) && closure->data == notify_data) {
        closure->marshal = NULL;
    } else if (!closure_try_remove_inotify(closure, notify_data, notify_func)) {
        x_critical(X_STRLOC ": unable to remove uninstalled invalidation notifier: %p (%p)", notify_func, notify_data);
    }
}

void x_closure_remove_finalize_notifier(XClosure *closure, xpointer notify_data, XClosureNotify notify_func)
{
    x_return_if_fail(closure != NULL);
    x_return_if_fail(notify_func != NULL);

    if (closure->is_invalid && !closure->in_inotify &&  ((xpointer)closure->marshal) == ((xpointer)notify_func) && closure->data == notify_data) {
        closure->marshal = NULL;
    } else if (!closure_try_remove_fnotify(closure, notify_data, notify_func)) {
        x_critical(X_STRLOC ": unable to remove uninstalled finalization notifier: %p (%p)", notify_func, notify_data);
    }
}

void x_closure_invoke(XClosure *closure, XValue *return_value, xuint n_param_values, const XValue *param_values, xpointer invocation_hint)
{
    XRealClosure *real_closure;

    x_return_if_fail(closure != NULL);

    real_closure = X_REAL_CLOSURE(closure);

    x_closure_ref(closure);
    if (!closure->is_invalid) {
        xpointer marshal_data;
        XClosureMarshal marshal;
        xboolean in_marshal = closure->in_marshal;

        x_return_if_fail(closure->marshal || real_closure->meta_marshal);

        ATOMIC_SET(closure, in_marshal, TRUE);
        if (real_closure->meta_marshal) {
            marshal_data = real_closure->meta_marshal_data;
            marshal = real_closure->meta_marshal;
        } else {
            marshal_data = NULL;
            marshal = closure->marshal;
        }

        if (!in_marshal) {
            closure_invoke_notifiers(closure, PRE_NOTIFY);
        }

        marshal(closure, return_value, n_param_values, param_values, invocation_hint, marshal_data);
        if (!in_marshal) {
            closure_invoke_notifiers(closure, POST_NOTIFY);
        }

        ATOMIC_SET(closure, in_marshal, in_marshal);
    }
    x_closure_unref(closure);
}

xboolean _x_closure_supports_invoke_va(XClosure *closure)
{
    XRealClosure *real_closure;

    x_return_val_if_fail(closure != NULL, FALSE);
    real_closure = X_REAL_CLOSURE(closure);

    return real_closure->va_marshal != NULL && (real_closure->meta_marshal == NULL || real_closure->va_meta_marshal != NULL);
}

void _x_closure_invoke_va(XClosure *closure, XValue *return_value, xpointer instance, va_list args, int n_params, XType *param_types)
{
    XRealClosure *real_closure;

    x_return_if_fail(closure != NULL);

    real_closure = X_REAL_CLOSURE(closure);

    x_closure_ref(closure);
    if (!closure->is_invalid) {
        xpointer marshal_data;
        XVaClosureMarshal marshal;
        xboolean in_marshal = closure->in_marshal;

        x_return_if_fail(closure->marshal || real_closure->meta_marshal);

        ATOMIC_SET(closure, in_marshal, TRUE);
        if (real_closure->va_meta_marshal) {
            marshal_data = real_closure->meta_marshal_data;
            marshal = real_closure->va_meta_marshal;
        } else {
            marshal_data = NULL;
            marshal = real_closure->va_marshal;
        }

        if (!in_marshal) {
            closure_invoke_notifiers(closure, PRE_NOTIFY);
        }

        marshal(closure, return_value, instance, args, marshal_data, n_params, param_types);
        if (!in_marshal) {
            closure_invoke_notifiers(closure, POST_NOTIFY);
        }

        ATOMIC_SET(closure, in_marshal, in_marshal);
    }
    x_closure_unref(closure);
}

void x_closure_set_marshal(XClosure *closure, XClosureMarshal marshal)
{
    x_return_if_fail(closure != NULL);
    x_return_if_fail(marshal != NULL);

    if (closure->marshal && closure->marshal != marshal) {
        x_critical("attempt to override closure->marshal (%p) with new marshal (%p)", closure->marshal, marshal);
    } else {
        closure->marshal = marshal;
    }
}

void _x_closure_set_va_marshal(XClosure *closure, XVaClosureMarshal marshal)
{
    XRealClosure *real_closure;

    x_return_if_fail(closure != NULL);
    x_return_if_fail(marshal != NULL);

    real_closure = X_REAL_CLOSURE(closure);

    if (real_closure->va_marshal && real_closure->va_marshal != marshal) {
        x_critical("attempt to override closure->va_marshal (%p) with new marshal (%p)", real_closure->va_marshal, marshal);
    } else {
        real_closure->va_marshal = marshal;
    }
}

XClosure *x_cclosure_new(XCallback callback_func, xpointer user_data, XClosureNotify destroy_data)
{
    XClosure *closure;

    x_return_val_if_fail(callback_func != NULL, NULL);

    closure = x_closure_new_simple(sizeof(XCClosure), user_data);
    if (destroy_data) {
        x_closure_add_finalize_notifier(closure, user_data, destroy_data);
    }
    ((XCClosure *)closure)->callback = (xpointer)callback_func;

    return closure;
}

XClosure *x_cclosure_new_swap(XCallback callback_func, xpointer user_data, XClosureNotify destroy_data)
{
    XClosure *closure;

    x_return_val_if_fail(callback_func != NULL, NULL);

    closure = x_closure_new_simple(sizeof(XCClosure), user_data);
    if (destroy_data) {
        x_closure_add_finalize_notifier(closure, user_data, destroy_data);
    }
    ((XCClosure *)closure)->callback = (xpointer)callback_func;
    ATOMIC_SET(closure, derivative_flag, TRUE);

    return closure;
}

static void x_type_class_meta_marshal(XClosure *closure, XValue *return_value, xuint n_param_values, const XValue *param_values, xpointer invocation_hint, xpointer marshal_data)
{
    xpointer callback;
    XTypeClass *classt;
    xuint offset = XPOINTER_TO_UINT(marshal_data);

    classt = X_TYPE_INSTANCE_GET_CLASS(x_value_peek_pointer(param_values + 0), itype, XTypeClass);
    callback = X_STRUCT_MEMBER(xpointer, classt, offset);
    if (callback) {
        closure->marshal(closure, return_value, n_param_values, param_values, invocation_hint, callback);
    }
}

static void x_type_class_meta_marshalv(XClosure *closure, XValue *return_value, xpointer instance, va_list args, xpointer marshal_data, int n_params, XType *param_types)
{
    xpointer callback;
    XTypeClass *classt;
    XRealClosure *real_closure;
    xuint offset = XPOINTER_TO_UINT(marshal_data);

    real_closure = X_REAL_CLOSURE(closure);

    classt = X_TYPE_INSTANCE_GET_CLASS(instance, itype, XTypeClass);
    callback = X_STRUCT_MEMBER(xpointer, classt, offset);
    if (callback) {
        real_closure->va_marshal(closure, return_value, instance, args, callback, n_params, param_types);
    }
}

static void x_type_iface_meta_marshal(XClosure *closure, XValue *return_value, xuint n_param_values, const XValue *param_values, xpointer invocation_hint, xpointer marshal_data)
{
    xpointer callback;
    XTypeClass *classt;
    XType itype = (XType)closure->data;
    xuint offset = XPOINTER_TO_UINT(marshal_data);

    classt = X_TYPE_INSTANCE_GET_INTERFACE(x_value_peek_pointer(param_values + 0), itype, XTypeClass);
    callback = X_STRUCT_MEMBER(xpointer, classt, offset);
    if (callback) {
        closure->marshal(closure, return_value, n_param_values, param_values, invocation_hint, callback);
    }
}

xboolean _x_closure_is_void(XClosure *closure, xpointer instance)
{
    XType itype;
    xuint offset;
    xpointer callback;
    XTypeClass *classt;
    XRealClosure *real_closure;

    if (closure->is_invalid) {
        return TRUE;
    }

    real_closure = X_REAL_CLOSURE(closure);
    if (real_closure->meta_marshal == x_type_iface_meta_marshal) {
        itype = (XType)closure->data;
        offset = XPOINTER_TO_UINT(real_closure->meta_marshal_data);

        classt = X_TYPE_INSTANCE_GET_INTERFACE(instance, itype, XTypeClass);
        callback = X_STRUCT_MEMBER(xpointer, classt, offset);
        return callback == NULL;
    } else if (real_closure->meta_marshal == x_type_class_meta_marshal) {
        offset = XPOINTER_TO_UINT(real_closure->meta_marshal_data);

        classt = X_TYPE_INSTANCE_GET_CLASS(instance, itype, XTypeClass);
        callback = X_STRUCT_MEMBER(xpointer, classt, offset);
        return callback == NULL;
    }

    return FALSE;
}

static void x_type_iface_meta_marshalv(XClosure *closure, XValue *return_value, xpointer instance, va_list args, xpointer marshal_data, int n_params, XType *param_types)
{
    xpointer callback;
    XTypeClass *classt;
    XRealClosure *real_closure;
    XType itype = (XType)closure->data;
    xuint offset = XPOINTER_TO_UINT(marshal_data);

    real_closure = X_REAL_CLOSURE(closure);

    classt = X_TYPE_INSTANCE_GET_INTERFACE(instance, itype, XTypeClass);
    callback = X_STRUCT_MEMBER(xpointer, classt, offset);
    if (callback) {
        real_closure->va_marshal(closure, return_value, instance, args, callback, n_params, param_types);
    }
}

XClosure *x_signal_type_cclosure_new(XType itype, xuint struct_offset)
{
    XClosure *closure;

    x_return_val_if_fail(X_TYPE_IS_CLASSED(itype) || X_TYPE_IS_INTERFACE(itype), NULL);
    x_return_val_if_fail(struct_offset >= sizeof(XTypeClass), NULL);

    closure = x_closure_new_simple(sizeof(XClosure), XTYPE_TO_POINTER(itype));
    if (X_TYPE_IS_INTERFACE(itype)) {
        x_closure_set_meta_marshal(closure, XUINT_TO_POINTER(struct_offset), x_type_iface_meta_marshal);
        x_closure_set_meta_va_marshal(closure, x_type_iface_meta_marshalv);
    } else {
        x_closure_set_meta_marshal(closure, XUINT_TO_POINTER(struct_offset), x_type_class_meta_marshal);
        x_closure_set_meta_va_marshal(closure, x_type_class_meta_marshalv);
    }

    return closure;
}

static ffi_type *value_to_ffi_type(const XValue *gvalue, xpointer *value, xint *enum_tmpval, xboolean *tmpval_used)
{
    ffi_type *rettype = NULL;
    XType type = x_type_fundamental(X_VALUE_TYPE(gvalue));

    x_assert(type != X_TYPE_INVALID);

    if (enum_tmpval) {
        x_assert(tmpval_used != NULL);
        *tmpval_used = FALSE;
    }

    switch (type) {
        case X_TYPE_BOOLEAN:
        case X_TYPE_CHAR:
        case X_TYPE_INT:
            rettype = &ffi_type_sint;
            *value = (xpointer)&(gvalue->data[0].v_int);
            break;

        case X_TYPE_ENUM:
            x_assert(enum_tmpval != NULL);
            rettype = &ffi_type_sint;
            *enum_tmpval = x_value_get_enum(gvalue);
            *value = enum_tmpval;
            *tmpval_used = TRUE;
            break;

        case X_TYPE_FLAGS:
            x_assert(enum_tmpval != NULL);
            rettype = &ffi_type_uint;
            *enum_tmpval = x_value_get_flags(gvalue);
            *value = enum_tmpval;
            *tmpval_used = TRUE;
            break;

        case X_TYPE_UCHAR:
        case X_TYPE_UINT:
            rettype = &ffi_type_uint;
            *value = (xpointer)&(gvalue->data[0].v_uint);
            break;

        case X_TYPE_STRING:
        case X_TYPE_OBJECT:
        case X_TYPE_BOXED:
        case X_TYPE_PARAM:
        case X_TYPE_POINTER:
        case X_TYPE_INTERFACE:
        case X_TYPE_VARIANT:
            rettype = &ffi_type_pointer;
            *value = (xpointer)&(gvalue->data[0].v_pointer);
            break;

        case X_TYPE_FLOAT:
            rettype = &ffi_type_float;
            *value = (xpointer)&(gvalue->data[0].v_float);
            break;

        case X_TYPE_DOUBLE:
            rettype = &ffi_type_double;
            *value = (xpointer)&(gvalue->data[0].v_double);
            break;

        case X_TYPE_LONG:
            rettype = &ffi_type_slong;
            *value = (xpointer)&(gvalue->data[0].v_long);
            break;

        case X_TYPE_ULONG:
            rettype = &ffi_type_ulong;
            *value = (xpointer)&(gvalue->data[0].v_ulong);
            break;

        case X_TYPE_INT64:
            rettype = &ffi_type_sint64;
            *value = (xpointer)&(gvalue->data[0].v_int64);
            break;

        case X_TYPE_UINT64:
            rettype = &ffi_type_uint64;
            *value = (xpointer)&(gvalue->data[0].v_uint64);
            break;

        default:
            rettype = &ffi_type_pointer;
            *value = NULL;
            x_critical("value_to_ffi_type: Unsupported fundamental type: %s", x_type_name(type));
            break;
    }

    return rettype;
}

static void value_from_ffi_type(XValue *gvalue, xpointer *value)
{
    XType type;
    ffi_arg *int_val = (ffi_arg *)value;

    type = X_VALUE_TYPE(gvalue);

restart:
    switch (x_type_fundamental(type)) {
        case X_TYPE_INT:
            x_value_set_int(gvalue, (xint)*int_val);
            break;

        case X_TYPE_FLOAT:
            x_value_set_float(gvalue, *(xfloat *)value);
            break;

        case X_TYPE_DOUBLE:
            x_value_set_double(gvalue, *(xdouble *)value);
            break;

        case X_TYPE_BOOLEAN:
            x_value_set_boolean(gvalue, (xboolean)*int_val);
            break;

        case X_TYPE_STRING:
            x_value_take_string(gvalue, *(xchar **)value);
            break;

        case X_TYPE_CHAR:
            x_value_set_schar(gvalue, (xint8)*int_val);
            break;

        case X_TYPE_UCHAR:
            x_value_set_uchar (gvalue, (xuchar)*int_val);
            break;

        case X_TYPE_UINT:
            x_value_set_uint(gvalue, (xuint)*int_val);
            break;

        case X_TYPE_POINTER:
            x_value_set_pointer(gvalue, *(xpointer *)value);
            break;

        case X_TYPE_LONG:
            x_value_set_long(gvalue, (xlong)*int_val);
            break;

        case X_TYPE_ULONG:
            x_value_set_ulong(gvalue, (xulong)*int_val);
            break;

        case X_TYPE_INT64:
            x_value_set_int64(gvalue, (xint64)*int_val);
            break;

        case X_TYPE_UINT64:
            x_value_set_uint64(gvalue, (xuint64)*int_val);
            break;

        case X_TYPE_BOXED:
            x_value_take_boxed(gvalue, *(xpointer *)value);
            break;

        case X_TYPE_ENUM:
            x_value_set_enum(gvalue, (xint)*int_val);
            break;

        case X_TYPE_FLAGS:
            x_value_set_flags(gvalue, (xuint)*int_val);
            break;

        case X_TYPE_PARAM:
            x_value_take_param(gvalue, (XParamSpec *)value);
            break;

        case X_TYPE_OBJECT:
            x_value_take_object(gvalue, *(xpointer *)value);
            break;

        case X_TYPE_VARIANT:
            x_value_take_variant(gvalue, (XVariant *)value);
            break;

        case X_TYPE_INTERFACE:
            type = x_type_interface_instantiatable_prerequisite(type);
            if (type) {
                goto restart;
            }
            X_GNUC_FALLTHROUGH;

        default:
            x_critical("value_from_ffi_type: Unsupported fundamental type %s for type %s", x_type_name(x_type_fundamental(X_VALUE_TYPE(gvalue))), x_type_name(X_VALUE_TYPE(gvalue)));
    }
}

typedef union {
    xpointer _gpointer;
    float    _float;
    double   _double;
    xint     _gint;
    xuint    _guint;
    xlong    _glong;
    xulong   _gulong;
    xint64   _gint64;
    xuint64  _guint64;
} va_arg_storage;

static ffi_type *va_to_ffi_type(XType gtype, va_list *va, va_arg_storage *storage)
{
    ffi_type *rettype = NULL;
    XType type = x_type_fundamental(gtype);

    x_assert(type != X_TYPE_INVALID);

    switch (type) {
        case X_TYPE_BOOLEAN:
        case X_TYPE_CHAR:
        case X_TYPE_INT:
        case X_TYPE_ENUM:
            rettype = &ffi_type_sint;
            storage->_gint = va_arg(*va, xint);
            break;

        case X_TYPE_UCHAR:
        case X_TYPE_UINT:
        case X_TYPE_FLAGS:
            rettype = &ffi_type_uint;
            storage->_guint = va_arg(*va, xuint);
            break;

        case X_TYPE_STRING:
        case X_TYPE_OBJECT:
        case X_TYPE_BOXED:
        case X_TYPE_PARAM:
        case X_TYPE_POINTER:
        case X_TYPE_INTERFACE:
        case X_TYPE_VARIANT:
            rettype = &ffi_type_pointer;
            storage->_gpointer = va_arg(*va, xpointer);
            break;

        case X_TYPE_FLOAT:
            rettype = &ffi_type_float;
            storage->_float = (float)va_arg(*va, double);
            break;

        case X_TYPE_DOUBLE:
            rettype = &ffi_type_double;
            storage->_double = va_arg(*va, double);
            break;

        case X_TYPE_LONG:
            rettype = &ffi_type_slong;
            storage->_glong = va_arg(*va, xlong);
            break;

        case X_TYPE_ULONG:
            rettype = &ffi_type_ulong;
            storage->_gulong = va_arg(*va, xulong);
            break;

        case X_TYPE_INT64:
            rettype = &ffi_type_sint64;
            storage->_gint64 = va_arg(*va, xint64);
            break;

        case X_TYPE_UINT64:
            rettype = &ffi_type_uint64;
            storage->_guint64 = va_arg(*va, xuint64);
            break;

        default:
            rettype = &ffi_type_pointer;
            storage->_guint64 = 0;
            x_critical("va_to_ffi_type: Unsupported fundamental type: %s", x_type_name(type));
            break;
    }

    return rettype;
}

void x_cclosure_marshal_generic(XClosure *closure, XValue *return_gvalue, xuint n_param_values, const XValue *param_values, xpointer invocation_hint, xpointer marshal_data)
{
    int i;
    int n_args;
    void **args;
    ffi_cif cif;
    void *rvalue;
    ffi_type *rtype;
    xint *enum_tmpval;
    ffi_type **atypes;
    xboolean tmpval_used = FALSE;
    XCClosure *cc = (XCClosure *)closure;

    enum_tmpval = (xint *)x_alloca(sizeof(xint));
    if (return_gvalue && X_VALUE_TYPE(return_gvalue)) {
        rtype = value_to_ffi_type(return_gvalue, &rvalue, enum_tmpval, &tmpval_used);
    } else {
        rtype = &ffi_type_void;
    }

    rvalue = x_alloca(MAX(rtype->size, sizeof(ffi_arg)));

    n_args = n_param_values + 1;
    atypes = (ffi_type **)x_alloca(sizeof(ffi_type *) * n_args);
    args = (void **)x_alloca(sizeof(xpointer) * n_args);

    if (tmpval_used) {
        enum_tmpval = (xint *)x_alloca(sizeof(xint));
    }

    if (X_CCLOSURE_SWAP_DATA(closure)) {
        atypes[n_args - 1] = value_to_ffi_type(param_values + 0, &args[n_args - 1], enum_tmpval, &tmpval_used);
        atypes[0] = &ffi_type_pointer;
        args[0] = &closure->data;
    } else {
        atypes[0] = value_to_ffi_type(param_values + 0, &args[0], enum_tmpval, &tmpval_used);
        atypes[n_args - 1] = &ffi_type_pointer;
        args[n_args - 1] = &closure->data;
    }

    for (i = 1; i < n_args - 1; i++) {
        if (tmpval_used) {
            enum_tmpval = (xint *)x_alloca(sizeof(xint));
        }
    
        atypes[i] = value_to_ffi_type(param_values + i, &args[i], enum_tmpval, &tmpval_used);
    }

    if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, n_args, rtype, atypes) != FFI_OK) {
        return;
    }

    ffi_call(&cif, marshal_data ? (void (*)(void))marshal_data : (void (*)(void))cc->callback, rvalue, args);

    if (return_gvalue && X_VALUE_TYPE(return_gvalue)) {
        value_from_ffi_type(return_gvalue, (xpointer *)rvalue);
    }
}

void x_cclosure_marshal_generic_va(XClosure *closure, XValue *return_value, xpointer instance, va_list args_list, xpointer marshal_data, int n_params, XType *param_types)
{
    int i;
    int n_args;
    void **args;
    ffi_cif cif;
    void *rvalue;
    ffi_type *rtype;
    xint *enum_tmpval;
    ffi_type **atypes;
    va_list args_copy;
    va_arg_storage *storage;
    xboolean tmpval_used = FALSE;
    XCClosure *cc = (XCClosure *)closure;

    enum_tmpval = (xint *)x_alloca(sizeof(xint));
    if (return_value && X_VALUE_TYPE(return_value)) {
        rtype = value_to_ffi_type(return_value, &rvalue, enum_tmpval, &tmpval_used);
    } else {
        rtype = &ffi_type_void;
    }

    rvalue = x_alloca(MAX(rtype->size, sizeof(ffi_arg)));

    n_args = n_params + 2;
    atypes = (ffi_type **)x_alloca(sizeof(ffi_type *) * n_args);
    args = (void **)x_alloca(sizeof(xpointer) * n_args);
    storage = (va_arg_storage *)x_alloca(sizeof(va_arg_storage) * n_params);

    if (X_CCLOSURE_SWAP_DATA(closure)) {
        atypes[n_args-1] = &ffi_type_pointer;
        args[n_args-1] = &instance;
        atypes[0] = &ffi_type_pointer;
        args[0] = &closure->data;
    } else {
        atypes[0] = &ffi_type_pointer;
        args[0] = &instance;
        atypes[n_args-1] = &ffi_type_pointer;
        args[n_args-1] = &closure->data;
    }

    va_copy(args_copy, args_list);
    for (i = 0; i < n_params; i++) {
        XType type = param_types[i]  & ~X_SIGNAL_TYPE_STATIC_SCOPE;
        XType fundamental = X_TYPE_FUNDAMENTAL(type);

        atypes[i + 1] = va_to_ffi_type(type, &args_copy, &storage[i]);
        args[i + 1] = &storage[i];

        if ((param_types[i]  & X_SIGNAL_TYPE_STATIC_SCOPE) == 0) {
            if (fundamental == X_TYPE_STRING && storage[i]._gpointer != NULL) {
                storage[i]._gpointer = x_strdup((const xchar *)storage[i]._gpointer);
            } else if (fundamental == X_TYPE_PARAM && storage[i]._gpointer != NULL) {
                storage[i]._gpointer = x_param_spec_ref((XParamSpec *)storage[i]._gpointer);
            } else if (fundamental == X_TYPE_BOXED && storage[i]._gpointer != NULL) {
                storage[i]._gpointer = x_boxed_copy(type, storage[i]._gpointer);
            } else if (fundamental == X_TYPE_VARIANT && storage[i]._gpointer != NULL) {
                storage[i]._gpointer = x_variant_ref_sink((XVariant *)storage[i]._gpointer);
            }
        }

        if (fundamental == X_TYPE_OBJECT && storage[i]._gpointer != NULL) {
            storage[i]._gpointer = x_object_ref(storage[i]._gpointer);
        }
    }
    va_end(args_copy);

    if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, n_args, rtype, atypes) != FFI_OK) {
        return;
    }

    ffi_call(&cif, marshal_data ? (void (*)(void))marshal_data : (void (*)(void))cc->callback, rvalue, args);

    for (i = 0; i < n_params; i++) {
        XType type = param_types[i]  & ~X_SIGNAL_TYPE_STATIC_SCOPE;
        XType fundamental = X_TYPE_FUNDAMENTAL(type);

        if ((param_types[i] & X_SIGNAL_TYPE_STATIC_SCOPE) == 0) {
            if (fundamental == X_TYPE_STRING && storage[i]._gpointer != NULL) {
                x_free(storage[i]._gpointer);
            } else if (fundamental == X_TYPE_PARAM && storage[i]._gpointer != NULL) {
                x_param_spec_unref((XParamSpec *)storage[i]._gpointer);
            } else if (fundamental == X_TYPE_BOXED && storage[i]._gpointer != NULL) {
                x_boxed_free(type, storage[i]._gpointer);
            } else if (fundamental == X_TYPE_VARIANT && storage[i]._gpointer != NULL) {
                x_variant_unref((XVariant *)storage[i]._gpointer);
            }
        }

        if (fundamental == X_TYPE_OBJECT && storage[i]._gpointer != NULL) {
            x_object_unref(storage[i]._gpointer);
        }
    }

    if (return_value && X_VALUE_TYPE(return_value)) {
        value_from_ffi_type(return_value, (xpointer *)rvalue);
    }
}
