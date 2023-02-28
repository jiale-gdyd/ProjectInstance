#include <xlib/xlib/config.h>

#include <xlib/xlib.h>
#include <xlib/xlib/xlibintl.h>
#include <xlib/xobj/xparamspecs.h>
#include <xlib/xobj/xvaluetypes.h>
#include <xlib/xobj/xsignalgroup.h>

struct _XSignalGroup {
    XObject   parent_instance;
    XWeakRef  target_ref;
    XRecMutex mutex;
    XPtrArray *handlers;
    XType     target_type;
    xssize    block_count;
    xuint     has_bound_at_least_once : 1;
};

typedef struct _XSignalGroupClass {
    XObjectClass parent_class;
    void (*bind)(XSignalGroup *self, XObject *target);
} XSignalGroupClass;

typedef struct {
    XSignalGroup *group;
    xulong       handler_id;
    XClosure     *closure;
    xuint        signal_id;
    XQuark       signal_detail;
    xuint        connect_after : 1;
} SignalHandler;

X_DEFINE_TYPE(XSignalGroup, x_signal_group, X_TYPE_OBJECT)

typedef enum {
    PROP_TARGET = 1,
    PROP_TARGET_TYPE,
    LAST_PROP
} XSignalGroupProperty;

enum {
    BIND,
    UNBIND,
    LAST_SIGNAL
};

static xuint signals[LAST_SIGNAL];
static XParamSpec *properties[LAST_PROP];

static void x_signal_group_set_target_type(XSignalGroup *self, XType target_type)
{
    x_assert(X_IS_SIGNAL_GROUP(self));
    x_assert(x_type_is_a(target_type, X_TYPE_OBJECT));

    self->target_type = target_type;

    if (X_TYPE_IS_INTERFACE(target_type)) {
        if (x_type_default_interface_peek(target_type) == NULL) {
            x_type_default_interface_unref(x_type_default_interface_ref(target_type));
        }
    } else {
        if (x_type_class_peek(target_type) == NULL) {
            x_type_class_unref(x_type_class_ref(target_type));
        }
    }
}

static void x_signal_group_gc_handlers(XSignalGroup *self)
{
    xuint i;

    x_assert(X_IS_SIGNAL_GROUP(self));

    for (i = self->handlers->len; i > 0; i--) {
        const SignalHandler *handler = (const SignalHandler *)x_ptr_array_index(self->handlers, i - 1);

        x_assert(handler != NULL);
        x_assert(handler->closure != NULL);

        if (handler->closure->is_invalid) {
            x_ptr_array_remove_index(self->handlers, i - 1);
        }
    }
}

static void x_signal_group__target_weak_notify(xpointer data, XObject *where_object_was)
{
    xuint i;
    XSignalGroup *self = (XSignalGroup *)data;

    x_assert(X_IS_SIGNAL_GROUP(self));
    x_assert(where_object_was != NULL);

    x_rec_mutex_lock(&self->mutex);

    x_weak_ref_set(&self->target_ref, NULL);

    for (i = 0; i < self->handlers->len; i++) {
        SignalHandler *handler = (SignalHandler *)x_ptr_array_index(self->handlers, i);
        handler->handler_id = 0;
    }

    x_signal_emit(self, signals[UNBIND], 0);
    x_object_notify_by_pspec(X_OBJECT(self), properties[PROP_TARGET]);

    x_rec_mutex_unlock(&self->mutex);
}

static void x_signal_group_bind_handler(XSignalGroup *self, SignalHandler *handler, XObject *target)
{
    xssize i;

    x_assert(self != NULL);
    x_assert(X_IS_OBJECT(target));
    x_assert(handler != NULL);
    x_assert(handler->signal_id != 0);
    x_assert(handler->closure != NULL);
    x_assert(handler->closure->is_invalid == 0);
    x_assert(handler->handler_id == 0);

    handler->handler_id = x_signal_connect_closure_by_id(target, handler->signal_id, handler->signal_detail, handler->closure, handler->connect_after);
    x_assert(handler->handler_id != 0);

    for (i = 0; i < self->block_count; i++) {
        x_signal_handler_block(target, handler->handler_id);
    }
}

static void x_signal_group_bind(XSignalGroup *self, XObject *target)
{
    xuint i;
    XObject *hold;

    x_assert(X_IS_SIGNAL_GROUP(self));
    x_assert(!target || X_IS_OBJECT(target));

    if (target == NULL) {
        return;
    }

    self->has_bound_at_least_once = TRUE;
    hold = x_object_ref(target);

    x_weak_ref_set(&self->target_ref, hold);
    x_object_weak_ref(hold, x_signal_group__target_weak_notify, self);

    x_signal_group_gc_handlers(self);

    for (i = 0; i < self->handlers->len; i++) {
        SignalHandler *handler = (SignalHandler *)x_ptr_array_index(self->handlers, i);
        x_signal_group_bind_handler(self, handler, hold);
    }

    x_signal_emit(self, signals [BIND], 0, hold);
    x_object_unref(hold);
}

static void x_signal_group_unbind(XSignalGroup *self)
{
    xuint i;
    XObject *target;

    x_return_if_fail(X_IS_SIGNAL_GROUP(self));

    target = (XObject *)x_weak_ref_get(&self->target_ref);
    if (target != NULL) {
        x_weak_ref_set(&self->target_ref, NULL);
        x_object_weak_unref(target, x_signal_group__target_weak_notify, self);
    }

    x_signal_group_gc_handlers(self);

    for (i = 0; i < self->handlers->len; i++) {
        xulong handler_id;
        SignalHandler *handler;

        handler = (SignalHandler *)x_ptr_array_index(self->handlers, i);

        x_assert(handler != NULL);
        x_assert(handler->signal_id != 0);
        x_assert(handler->closure != NULL);

        handler_id = handler->handler_id;
        handler->handler_id = 0;

        if (target != NULL && handler_id != 0) {
            x_signal_handler_disconnect(target, handler_id);
        }
    }

    x_signal_emit(self, signals[UNBIND], 0);
    x_clear_object(&target);
}

static xboolean x_signal_group_check_target_type(XSignalGroup *self, xpointer target)
{
    if ((target != NULL) && !x_type_is_a(X_OBJECT_TYPE(target), self->target_type)) {
        x_critical("Failed to set XSignalGroup of target type %s using target %p of type %s", x_type_name(self->target_type), target, X_OBJECT_TYPE_NAME(target));
        return FALSE;
    }

    return TRUE;
}

void x_signal_group_block(XSignalGroup *self)
{
    xuint i;
    XObject *target;

    x_return_if_fail(X_IS_SIGNAL_GROUP(self));
    x_return_if_fail(self->block_count >= 0);

    x_rec_mutex_lock(&self->mutex);

    self->block_count++;
    target = (XObject *)x_weak_ref_get(&self->target_ref);
    if (target == NULL) {
        goto unlock;
    }

    for (i = 0; i < self->handlers->len; i++) {
        const SignalHandler *handler = (const SignalHandler *)x_ptr_array_index(self->handlers, i);

        x_assert(handler != NULL);
        x_assert(handler->signal_id != 0);
        x_assert(handler->closure != NULL);
        x_assert(handler->handler_id != 0);

        x_signal_handler_block(target, handler->handler_id);
    }

    x_object_unref(target);

unlock:
    x_rec_mutex_unlock(&self->mutex);
}

void x_signal_group_unblock(XSignalGroup *self)
{
    xuint i;
    XObject *target;

    x_return_if_fail(X_IS_SIGNAL_GROUP(self));
    x_return_if_fail(self->block_count > 0);

    x_rec_mutex_lock(&self->mutex);

    self->block_count--;

    target = (XObject *)x_weak_ref_get(&self->target_ref);
    if (target == NULL) {
        goto unlock;
    }

    for (i = 0; i < self->handlers->len; i++) {
        const SignalHandler *handler = (const SignalHandler *)x_ptr_array_index(self->handlers, i);

        x_assert(handler != NULL);
        x_assert(handler->signal_id != 0);
        x_assert(handler->closure != NULL);
        x_assert(handler->handler_id != 0);

        x_signal_handler_unblock(target, handler->handler_id);
    }

    x_object_unref(target);

unlock:
    x_rec_mutex_unlock(&self->mutex);
}

xpointer x_signal_group_dup_target(XSignalGroup *self)
{
    XObject *target;

    x_return_val_if_fail(X_IS_SIGNAL_GROUP(self), NULL);

    x_rec_mutex_lock(&self->mutex);
    target = (XObject *)x_weak_ref_get(&self->target_ref);
    x_rec_mutex_unlock(&self->mutex);

    return target;
}

void x_signal_group_set_target(XSignalGroup *self, xpointer target)
{
    XObject *object;

    x_return_if_fail(X_IS_SIGNAL_GROUP(self));

    x_rec_mutex_lock(&self->mutex);

    object = (XObject *)x_weak_ref_get(&self->target_ref);
    if (object == (XObject *)target) {
        goto cleanup;
    }

    if (!x_signal_group_check_target_type(self, target)) {
        goto cleanup;
    }

    if (self->has_bound_at_least_once) {
        x_signal_group_unbind(self);
    }

    x_signal_group_bind(self, (XObject *)target);
    x_object_notify_by_pspec(X_OBJECT(self), properties[PROP_TARGET]);

cleanup:
    x_clear_object(&object);
    x_rec_mutex_unlock(&self->mutex);
}

static void signal_handler_free(xpointer data)
{
    SignalHandler *handler = (SignalHandler *)data;

    if (handler->closure != NULL) {
        x_closure_invalidate(handler->closure);
    }

    handler->handler_id = 0;
    handler->signal_id = 0;
    handler->signal_detail = 0;
    x_clear_pointer(&handler->closure, x_closure_unref);
    x_slice_free(SignalHandler, handler);
}

static void x_signal_group_constructed(XObject *object)
{
    XObject *target;
    XSignalGroup *self = (XSignalGroup *)object;

    x_rec_mutex_lock(&self->mutex);

    target = (XObject *)x_weak_ref_get(&self->target_ref);
    if (!x_signal_group_check_target_type(self, target)) {
        x_signal_group_set_target(self, NULL);
    }

    X_OBJECT_CLASS(x_signal_group_parent_class)->constructed(object);

    x_clear_object(&target);

    x_rec_mutex_unlock(&self->mutex);
}

static void x_signal_group_dispose(XObject *object)
{
    XSignalGroup *self = (XSignalGroup *)object;

    x_rec_mutex_lock(&self->mutex);
    x_signal_group_gc_handlers(self);
    if (self->has_bound_at_least_once) {
        x_signal_group_unbind(self);
    }
    x_clear_pointer(&self->handlers, x_ptr_array_unref);
    x_rec_mutex_unlock(&self->mutex);

    X_OBJECT_CLASS(x_signal_group_parent_class)->dispose(object);
}

static void x_signal_group_finalize(XObject *object)
{
    XSignalGroup *self = (XSignalGroup *)object;

    x_weak_ref_clear(&self->target_ref);
    x_rec_mutex_clear(&self->mutex);

    X_OBJECT_CLASS(x_signal_group_parent_class)->finalize(object);
}

static void x_signal_group_get_property(XObject *object, xuint prop_id, XValue *value, XParamSpec *pspec)
{
    XSignalGroup *self = X_SIGNAL_GROUP(object);

    switch ((XSignalGroupProperty)prop_id) {
        case PROP_TARGET:
            x_value_take_object(value, x_signal_group_dup_target(self));
            break;

        case PROP_TARGET_TYPE:
            x_value_set_gtype(value, self->target_type);
            break;

        default:
            X_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void x_signal_group_set_property(XObject *object, xuint prop_id, const XValue *value, XParamSpec *pspec)
{
    XSignalGroup *self = X_SIGNAL_GROUP(object);

    switch ((XSignalGroupProperty)prop_id) {
        case PROP_TARGET:
            x_signal_group_set_target(self, x_value_get_object(value));
            break;

        case PROP_TARGET_TYPE:
            x_signal_group_set_target_type(self, x_value_get_gtype(value));
            break;

        default:
            X_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void x_signal_group_class_init(XSignalGroupClass *klass)
{
    XObjectClass *object_class = X_OBJECT_CLASS(klass);

    object_class->constructed = x_signal_group_constructed;
    object_class->dispose = x_signal_group_dispose;
    object_class->finalize = x_signal_group_finalize;
    object_class->get_property = x_signal_group_get_property;
    object_class->set_property = x_signal_group_set_property;

    properties[PROP_TARGET] = x_param_spec_object("target", "Target", "The target instance used when connecting signals.", X_TYPE_OBJECT, (XParamFlags)(X_PARAM_READWRITE | X_PARAM_EXPLICIT_NOTIFY | X_PARAM_STATIC_STRINGS));
    properties[PROP_TARGET_TYPE] = x_param_spec_gtype("target-type", "Target Type", "The XType of the target property.", X_TYPE_OBJECT, (XParamFlags)(X_PARAM_READWRITE | X_PARAM_CONSTRUCT_ONLY | X_PARAM_STATIC_STRINGS));

    x_object_class_install_properties(object_class, LAST_PROP, properties);

    signals[BIND] = x_signal_new("bind", X_TYPE_FROM_CLASS(klass), X_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, X_TYPE_NONE, 1, X_TYPE_OBJECT);
    signals[UNBIND] = x_signal_new("unbind", X_TYPE_FROM_CLASS(klass), X_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, X_TYPE_NONE, 0);
}

static void x_signal_group_init(XSignalGroup *self)
{
    x_rec_mutex_init(&self->mutex);
    self->handlers = x_ptr_array_new_with_free_func(signal_handler_free);
    self->target_type = X_TYPE_OBJECT;
}

XSignalGroup *x_signal_group_new(XType target_type)
{
    x_return_val_if_fail(x_type_is_a(target_type, X_TYPE_OBJECT), NULL);
    return (XSignalGroup *)x_object_new(X_TYPE_SIGNAL_GROUP, "target-type", target_type, NULL);
}

static xboolean x_signal_group_connect_closure_(XSignalGroup *self, const xchar *detailed_signal, XClosure *closure, xboolean after)
{
    xuint signal_id;
    XObject *target;
    XQuark signal_detail;
    SignalHandler *handler;

    x_return_val_if_fail(X_IS_SIGNAL_GROUP(self), FALSE);
    x_return_val_if_fail(detailed_signal != NULL, FALSE);
    x_return_val_if_fail(x_signal_parse_name(detailed_signal, self->target_type, &signal_id, &signal_detail, TRUE) != 0, FALSE);
    x_return_val_if_fail(closure != NULL, FALSE);

    x_rec_mutex_lock(&self->mutex);

    if (self->has_bound_at_least_once) {
        x_critical("Cannot add signals after setting target");
        x_rec_mutex_unlock(&self->mutex);
        return FALSE;
    }

    handler = x_slice_new0(SignalHandler);
    handler->group = self;
    handler->signal_id = signal_id;
    handler->signal_detail = signal_detail;
    handler->closure = x_closure_ref(closure);
    handler->connect_after = after;

    x_closure_sink(closure);
    x_ptr_array_add(self->handlers, handler);

    target = (XObject *)x_weak_ref_get(&self->target_ref);

    if (target != NULL) {
        x_signal_group_bind_handler(self, handler, target);
        x_object_unref(target);
    }

    x_signal_group_gc_handlers(self);

    x_rec_mutex_unlock(&self->mutex);
    return TRUE;
}

void x_signal_group_connect_closure(XSignalGroup *self, const xchar *detailed_signal, XClosure *closure, xboolean after)
{
    x_signal_group_connect_closure_(self, detailed_signal, closure, after);
}

static void x_signal_group_connect_full(XSignalGroup *self, const xchar *detailed_signal, XCallback c_handler, xpointer data, XClosureNotify notify, XConnectFlags flags, xboolean is_object)
{
    XClosure *closure;

    x_return_if_fail(c_handler != NULL);
    x_return_if_fail(!is_object || X_IS_OBJECT(data));

    if ((flags & X_CONNECT_SWAPPED) != 0) {
        closure = x_cclosure_new_swap(c_handler, data, notify);
    } else {
        closure = x_cclosure_new(c_handler, data, notify);
    }

    if (is_object) {
        x_object_watch_closure((XObject *)data, closure);
    }

    if (!x_signal_group_connect_closure_(self, detailed_signal, closure, (flags & X_CONNECT_AFTER) != 0)) {
        x_closure_unref(closure);
    }
}

void x_signal_group_connect_object(XSignalGroup *self, const xchar *detailed_signal, XCallback c_handler, xpointer object, XConnectFlags flags)
{
    x_return_if_fail(X_IS_OBJECT(object));
    x_signal_group_connect_full(self, detailed_signal, c_handler, object, NULL, flags, TRUE);
}

void x_signal_group_connect_data(XSignalGroup *self, const xchar *detailed_signal, XCallback c_handler, xpointer data, XClosureNotify notify, XConnectFlags flags)
{
    x_signal_group_connect_full(self, detailed_signal, c_handler, data, notify, flags, FALSE);
}

void x_signal_group_connect(XSignalGroup *self, const xchar *detailed_signal, XCallback c_handler, xpointer data)
{
    x_signal_group_connect_full(self, detailed_signal, c_handler, data, NULL, (XConnectFlags)0, FALSE);
}

void x_signal_group_connect_after(XSignalGroup *self, const xchar *detailed_signal, XCallback c_handler, xpointer data)
{
    x_signal_group_connect_full(self, detailed_signal, c_handler, data, NULL, X_CONNECT_AFTER, FALSE);
}

void x_signal_group_connect_swapped(XSignalGroup *self, const xchar *detailed_signal, XCallback c_handler, xpointer data)
{
    x_signal_group_connect_full(self, detailed_signal, c_handler, data, NULL, X_CONNECT_SWAPPED, FALSE);
}
