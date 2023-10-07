#include <string.h>

#include <xlib/xlib/config.h>
#include <xlib/xobj/xenums.h>
#include <xlib/xobj/xobject.h>
#include <xlib/xobj/xsignal.h>
#include <xlib/xobj/xmarshal.h>
#include <xlib/xlib/xlibintl.h>
#include <xlib/xobj/xbinding.h>
#include <xlib/xobj/xparamspecs.h>
#include <xlib/xobj/xvaluetypes.h>

XType x_binding_flags_get_type(void)
{
    static XType static_x_define_type_id = 0;

    if (x_once_init_enter_pointer(&static_x_define_type_id)) {
        static const XFlagsValue values[] = {
            { X_BINDING_DEFAULT, "X_BINDING_DEFAULT", "default" },
            { X_BINDING_BIDIRECTIONAL, "X_BINDING_BIDIRECTIONAL", "bidirectional" },
            { X_BINDING_SYNC_CREATE, "X_BINDING_SYNC_CREATE", "sync-create" },
            { X_BINDING_INVERT_BOOLEAN, "X_BINDING_INVERT_BOOLEAN", "invert-boolean" },
            { 0, NULL, NULL }
        };

        XType x_define_type_id = x_flags_register_static(x_intern_static_string("XBindingFlags"), values);
        x_once_init_leave_pointer(&static_x_define_type_id, x_define_type_id);
    }

    return static_x_define_type_id;
}

typedef struct {
    XWeakRef binding;
    XWeakRef source;
    XWeakRef target;
    xboolean binding_removed;
} BindingContext;

static BindingContext *binding_context_ref(BindingContext *context)
{
    return x_atomic_rc_box_acquire(context);
}

static void binding_context_clear(BindingContext *context)
{
    x_weak_ref_clear(&context->binding);
    x_weak_ref_clear(&context->source);
    x_weak_ref_clear(&context->target);
}

static void binding_context_unref(BindingContext *context)
{
    x_atomic_rc_box_release_full(context, (XDestroyNotify)binding_context_clear);
}

typedef struct {
    XBindingTransformFunc transform_s2t;
    XBindingTransformFunc transform_t2s;
    xpointer              transform_data;
    XDestroyNotify        destroy_notify;
} TransformFunc;

static TransformFunc *transform_func_new (XBindingTransformFunc transform_s2t, XBindingTransformFunc transform_t2s, xpointer transform_data, XDestroyNotify destroy_notify)
{
    TransformFunc *func = x_atomic_rc_box_new0(TransformFunc);

    func->transform_s2t = transform_s2t;
    func->transform_t2s= transform_t2s;
    func->transform_data = transform_data;
    func->destroy_notify = destroy_notify;

    return func;
}

static TransformFunc *transform_func_ref(TransformFunc *func)
{
    return x_atomic_rc_box_acquire(func);
}

static void transform_func_clear(TransformFunc *func)
{
    if (func->destroy_notify) {
        func->destroy_notify(func->transform_data);
    }
}

static void transform_func_unref(TransformFunc *func)
{
    x_atomic_rc_box_release_full(func, (XDestroyNotify)transform_func_clear);
}

#define X_BINDING_CLASS(klass)          (X_TYPE_CHECK_CLASS_CAST((klass), X_TYPE_BINDING, XBindingClass))
#define X_IS_BINDING_CLASS(klass)       (X_TYPE_CHECK_CLASS_TYPE((klass), X_TYPE_BINDING))
#define X_BINDING_GET_CLASS(obj)        (X_TYPE_INSTANCE_GET_CLASS((obj), X_TYPE_BINDING, XBindingClass))

typedef struct _XBindingClass XBindingClass;

struct _XBinding {
    XObject        parent_instance;
    BindingContext *context;
    XMutex         unbind_lock;
    TransformFunc  *transform_func;
    const xchar    *source_property;
    const xchar    *target_property;
    XParamSpec     *source_pspec;
    XParamSpec     *target_pspec;
    XBindingFlags  flags;
    xuint          source_notify;
    xuint          target_notify;
    xboolean       target_weak_notify_installed;
    xuint          is_frozen : 1;
};

struct _XBindingClass {
    XObjectClass parent_class;
};

enum {
    PROP_0,
    PROP_SOURCE,
    PROP_TARGET,
    PROP_SOURCE_PROPERTY,
    PROP_TARGET_PROPERTY,
    PROP_FLAGS
};

static xuint xobject_notify_signal_id;

X_DEFINE_TYPE(XBinding, x_binding, X_TYPE_OBJECT)

static void weak_unbind(xpointer user_data, XObject *where_the_object_was);

static xboolean unbind_internal_locked(BindingContext *context, XBinding *binding, XObject *source, XObject *target)
{
    xboolean binding_was_removed = FALSE;

    x_assert(context != NULL);
    x_assert(binding != NULL);

    if (source) {
        if (binding->source_notify != 0) {
            x_signal_handler_disconnect(source, binding->source_notify);

            x_object_weak_unref(source, weak_unbind, context);
            binding_context_unref(context);

            binding->source_notify = 0;
        }

        x_weak_ref_set(&context->source, NULL);
    }

    if (target) {
        if (binding->target_notify != 0) {
            x_signal_handler_disconnect(target, binding->target_notify);
            binding->target_notify = 0;
        }
        x_weak_ref_set(&context->target, NULL);

        if (binding->target_weak_notify_installed) {
            x_object_weak_unref(target, weak_unbind, context);
            binding_context_unref(context);
            binding->target_weak_notify_installed = FALSE;
        }
    }

    if (!context->binding_removed) {
        context->binding_removed = TRUE;
        binding_was_removed = TRUE;
    }

    return binding_was_removed;
}

static void weak_unbind(xpointer user_data, XObject *where_the_object_was)
{
    XBinding *binding;
    XObject *source, *target;
    TransformFunc *transform_func;
    xboolean binding_was_removed = FALSE;
    BindingContext *context = (BindingContext *)user_data;

    binding = (XBinding *)x_weak_ref_get(&context->binding);
    if (!binding) {
        binding_context_unref(context);
        return;
    }

    x_mutex_lock(&binding->unbind_lock);

    transform_func = x_steal_pointer(&binding->transform_func);

    source = (XObject *)x_weak_ref_get(&context->source);
    target = (XObject *)x_weak_ref_get(&context->target);

    if (source == where_the_object_was) {
        x_weak_ref_set(&context->source, NULL);
        x_clear_object(&source);
    }

    if (target == where_the_object_was) {
        x_weak_ref_set(&context->target, NULL);
        x_clear_object(&target);
    }

    binding_was_removed = unbind_internal_locked(context, binding, source, target);

    x_mutex_unlock(&binding->unbind_lock);

    x_clear_object(&target);
    x_clear_object(&source);

    x_clear_pointer(&transform_func, transform_func_unref);
    x_object_unref(binding);

    if (binding_was_removed) {
        x_object_unref(binding);
    }

    binding_context_unref(context);
}

static xboolean default_transform(XBinding *binding, const XValue *value_a, XValue *value_b, xpointer user_data X_GNUC_UNUSED)
{
    if (!x_type_is_a(X_VALUE_TYPE(value_a), X_VALUE_TYPE(value_b))) {
        if (x_value_type_compatible(X_VALUE_TYPE(value_a), X_VALUE_TYPE(value_b))) {
            x_value_copy(value_a, value_b);
            return TRUE;
        }

        if (x_value_type_transformable(X_VALUE_TYPE(value_a), X_VALUE_TYPE(value_b))) {
            if (x_value_transform(value_a, value_b)) {
                return TRUE;
            }
        }

        x_critical("%s: Unable to convert a value of type %s to a value of type %s", X_STRLOC, x_type_name(X_VALUE_TYPE(value_a)), x_type_name(X_VALUE_TYPE(value_b)));
        return FALSE;
    }

    x_value_copy(value_a, value_b);
    return TRUE;
}

static xboolean default_invert_boolean_transform(XBinding *binding, const XValue *value_a, XValue *value_b, xpointer user_data X_GNUC_UNUSED)
{
    xboolean value;

    x_assert(X_VALUE_HOLDS_BOOLEAN(value_a));
    x_assert(X_VALUE_HOLDS_BOOLEAN(value_b));

    value = x_value_get_boolean(value_a);
    value = !value;

    x_value_set_boolean(value_b, value);
    return TRUE;
}

static void on_source_notify(XObject *source, XParamSpec *pspec, BindingContext *context)
{
    xboolean res;
    XObject *target;
    XBinding *binding;
    TransformFunc *transform_func;
    XValue to_value = X_VALUE_INIT;
    XValue from_value = X_VALUE_INIT;

    binding = (XBinding *)x_weak_ref_get(&context->binding);
    if (!binding) {
        return;
    }

    if (binding->is_frozen) {
        x_object_unref(binding);
        return;
    }

    target = (XObject *)x_weak_ref_get(&context->target);
    if (!target) {
        x_object_unref(binding);
        return;
    }

    x_mutex_lock(&binding->unbind_lock);
    if (!binding->transform_func) {
        x_mutex_unlock(&binding->unbind_lock);
        return;
    }

    transform_func = transform_func_ref(binding->transform_func);
    x_mutex_unlock(&binding->unbind_lock);

    x_value_init(&from_value, X_PARAM_SPEC_VALUE_TYPE(binding->source_pspec));
    x_value_init(&to_value, X_PARAM_SPEC_VALUE_TYPE(binding->target_pspec));

    x_object_get_property(source, binding->source_pspec->name, &from_value);

    res = transform_func->transform_s2t(binding, &from_value, &to_value, transform_func->transform_data);

    transform_func_unref(transform_func);

    if (res) {
        binding->is_frozen = TRUE;
        (void)x_param_value_validate(binding->target_pspec, &to_value);
        x_object_set_property(target, binding->target_pspec->name, &to_value);
        binding->is_frozen = FALSE;
    }

    x_value_unset(&from_value);
    x_value_unset(&to_value);

    x_object_unref(target);
    x_object_unref(binding);
}

static void on_target_notify(XObject *target, XParamSpec *pspec, BindingContext *context)
{
    xboolean res;
    XObject *source;
    XBinding *binding;
    TransformFunc *transform_func;
    XValue to_value = X_VALUE_INIT;
    XValue from_value = X_VALUE_INIT;

    binding = (XBinding *)x_weak_ref_get(&context->binding);
    if (!binding) {
        return;
    }

    if (binding->is_frozen) {
        x_object_unref(binding);
        return;
    }

    source = (XObject *)x_weak_ref_get(&context->source);
    if (!source) {
        x_object_unref(binding);
        return;
    }

    x_mutex_lock(&binding->unbind_lock);
    if (!binding->transform_func) {
        x_mutex_unlock(&binding->unbind_lock);
        return;
    }

    transform_func = transform_func_ref(binding->transform_func);
    x_mutex_unlock(&binding->unbind_lock);

    x_value_init(&from_value, X_PARAM_SPEC_VALUE_TYPE(binding->target_pspec));
    x_value_init(&to_value, X_PARAM_SPEC_VALUE_TYPE(binding->source_pspec));

    x_object_get_property(target, binding->target_pspec->name, &from_value);

    res = transform_func->transform_t2s(binding, &from_value, &to_value, transform_func->transform_data);
    transform_func_unref(transform_func);

    if (res) {
        binding->is_frozen = TRUE;
        (void)x_param_value_validate(binding->source_pspec, &to_value);
        x_object_set_property(source, binding->source_pspec->name, &to_value);
        binding->is_frozen = FALSE;
    }

    x_value_unset(&from_value);
    x_value_unset(&to_value);

    x_object_unref(source);
    x_object_unref(binding);
}

static inline void x_binding_unbind_internal(XBinding *binding, xboolean unref_binding)
{
    XObject *source, *target;
    TransformFunc *transform_func;
    xboolean binding_was_removed = FALSE;
    BindingContext *context = binding->context;

    x_mutex_lock(&binding->unbind_lock);
    transform_func = x_steal_pointer(&binding->transform_func);

    source = (XObject *)x_weak_ref_get(&context->source);
    target = (XObject *)x_weak_ref_get(&context->target);

    binding_was_removed = unbind_internal_locked(context, binding, source, target);
    x_mutex_unlock(&binding->unbind_lock);

    x_clear_object(&target);
    x_clear_object(&source);

    x_clear_pointer(&transform_func, transform_func_unref);

    if (binding_was_removed && unref_binding) {
        x_object_unref(binding);
    }
}

static void x_binding_finalize(XObject *gobject)
{
    XBinding *binding = X_BINDING(gobject);

    x_binding_unbind_internal(binding, FALSE);
    binding_context_unref(binding->context);

    x_mutex_clear(&binding->unbind_lock);

    X_OBJECT_CLASS(x_binding_parent_class)->finalize(gobject);
}

static void canonicalize_key(xchar *key)
{
    xchar *p;

    for (p = key; *p != 0; p++) {
        xchar transform_func_new = *p;
        if (transform_func_new == '_') {
            *p = '-';
        }
    }
}

static xboolean is_canonical(const xchar *key)
{
    return (strchr(key, '_') == NULL);
}

static void x_binding_set_property(XObject *gobject, xuint prop_id, const XValue *value, XParamSpec *pspec)
{
    XBinding *binding = X_BINDING(gobject);

    switch (prop_id) {
        case PROP_SOURCE:
            x_weak_ref_set(&binding->context->source, x_value_get_object(value));
            break;

        case PROP_TARGET:
            x_weak_ref_set(&binding->context->target, x_value_get_object(value));
            break;

        case PROP_SOURCE_PROPERTY:
        case PROP_TARGET_PROPERTY: {
            const xchar **dest;
            xchar *name_copy = NULL;
            const xchar *name = x_value_get_string(value);

            if (!is_canonical(name)) {
                name_copy = x_value_dup_string(value);
                canonicalize_key(name_copy);
                name = name_copy;
            }

            if (prop_id == PROP_SOURCE_PROPERTY) {
                dest = &binding->source_property;
            } else {
                dest = &binding->target_property;
            }

            *dest = x_intern_string(name);
            x_free(name_copy);
            break;
        }

        case PROP_FLAGS:
            binding->flags = (XBindingFlags)x_value_get_flags(value);
            break;

        default:
            X_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, prop_id, pspec);
            break;
    }
}

static void x_binding_get_property(XObject *gobject, xuint prop_id, XValue *value, XParamSpec *pspec)
{
    XBinding *binding = X_BINDING(gobject);

    switch (prop_id) {
        case PROP_SOURCE:
            x_value_take_object(value, x_weak_ref_get(&binding->context->source));
            break;

        case PROP_SOURCE_PROPERTY:
            x_value_set_interned_string(value, binding->source_property);
            break;

        case PROP_TARGET:
            x_value_take_object(value, x_weak_ref_get(&binding->context->target));
            break;

        case PROP_TARGET_PROPERTY:
            x_value_set_interned_string(value, binding->target_property);
            break;

        case PROP_FLAGS:
            x_value_set_flags(value, binding->flags);
            break;

        default:
            X_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, prop_id, pspec);
            break;
    }
}

static void x_binding_constructed(XObject *gobject)
{
    XObject *source, *target;
    XQuark source_property_detail;
    XClosure *source_notify_closure;
    XBinding *binding = X_BINDING(gobject);
    XBindingTransformFunc transform_func = default_transform;

    source = (XObject *)x_weak_ref_get(&binding->context->source);
    target = (XObject *)x_weak_ref_get(&binding->context->target);

    x_assert(source != NULL);
    x_assert(target != NULL);
    x_assert(binding->source_property != NULL);
    x_assert(binding->target_property != NULL);

    binding->source_pspec = x_object_class_find_property(X_OBJECT_GET_CLASS(source), binding->source_property);
    binding->target_pspec = x_object_class_find_property(X_OBJECT_GET_CLASS(target), binding->target_property);
    x_assert(binding->source_pspec != NULL);
    x_assert(binding->target_pspec != NULL);

    if (binding->flags & X_BINDING_INVERT_BOOLEAN) {
        transform_func = default_invert_boolean_transform;
    }

    binding->transform_func = transform_func_new(transform_func, transform_func, NULL, NULL);

    source_property_detail = x_quark_from_string(binding->source_property);
    source_notify_closure = x_cclosure_new(X_CALLBACK (on_source_notify), binding_context_ref(binding->context), (XClosureNotify)binding_context_unref);
    binding->source_notify = x_signal_connect_closure_by_id(source, xobject_notify_signal_id, source_property_detail, source_notify_closure, FALSE);

    x_object_weak_ref(source, weak_unbind, binding_context_ref(binding->context));

    if (binding->flags & X_BINDING_BIDIRECTIONAL) {
        XQuark target_property_detail;
        XClosure *target_notify_closure;

        target_property_detail = x_quark_from_string (binding->target_property);
        target_notify_closure = x_cclosure_new (X_CALLBACK(on_target_notify), binding_context_ref(binding->context), (XClosureNotify)binding_context_unref);
        binding->target_notify = x_signal_connect_closure_by_id(target, xobject_notify_signal_id, target_property_detail, target_notify_closure, FALSE);
    }

    if (target != source) {
        x_object_weak_ref(target, weak_unbind, binding_context_ref(binding->context));
        binding->target_weak_notify_installed = TRUE;
    }

    x_object_unref(source);
    x_object_unref(target);
}

static void x_binding_class_init(XBindingClass *klass)
{
    XObjectClass *xobject_class = X_OBJECT_CLASS(klass);

    xobject_notify_signal_id = x_signal_lookup("notify", X_TYPE_OBJECT);
    x_assert(xobject_notify_signal_id != 0);

    xobject_class->constructed = x_binding_constructed;
    xobject_class->set_property = x_binding_set_property;
    xobject_class->get_property = x_binding_get_property;
    xobject_class->finalize = x_binding_finalize;

    x_object_class_install_property(xobject_class, PROP_SOURCE, x_param_spec_object("source", P_("Source"), P_("The source of the binding"), X_TYPE_OBJECT, (XParamFlags)(X_PARAM_CONSTRUCT_ONLY | X_PARAM_READWRITE | X_PARAM_STATIC_STRINGS)));
    x_object_class_install_property(xobject_class, PROP_TARGET, x_param_spec_object("target", P_("Target"), P_("The target of the binding"), X_TYPE_OBJECT, (XParamFlags)(X_PARAM_CONSTRUCT_ONLY | X_PARAM_READWRITE | X_PARAM_STATIC_STRINGS)));
    x_object_class_install_property(xobject_class, PROP_SOURCE_PROPERTY, x_param_spec_string("source-property", P_("Source Property"), P_("The property on the source to bind"), NULL, (XParamFlags)(X_PARAM_CONSTRUCT_ONLY | X_PARAM_READWRITE | X_PARAM_STATIC_STRINGS)));
    x_object_class_install_property(xobject_class, PROP_TARGET_PROPERTY, x_param_spec_string("target-property", P_("Target Property"), P_("The property on the target to bind"), NULL, (XParamFlags)(X_PARAM_CONSTRUCT_ONLY | X_PARAM_READWRITE | X_PARAM_STATIC_STRINGS)));
    x_object_class_install_property(xobject_class, PROP_FLAGS, x_param_spec_flags("flags", P_("Flags"), P_("The binding flags"), X_TYPE_BINDING_FLAGS, X_BINDING_DEFAULT, (XParamFlags)(X_PARAM_CONSTRUCT_ONLY | X_PARAM_READWRITE | X_PARAM_STATIC_STRINGS)));
}

static void x_binding_init(XBinding *binding)
{
    x_mutex_init(&binding->unbind_lock);

    binding->context = x_atomic_rc_box_new0(BindingContext);
    x_weak_ref_init(&binding->context->binding, binding);
    x_weak_ref_init(&binding->context->source, NULL);
    x_weak_ref_init(&binding->context->target, NULL);
}

XBindingFlags x_binding_get_flags(XBinding *binding)
{
    x_return_val_if_fail(X_IS_BINDING(binding), X_BINDING_DEFAULT);
    return binding->flags;
}

XObject *x_binding_get_source(XBinding *binding)
{
    XObject *source;

    x_return_val_if_fail(X_IS_BINDING(binding), NULL);

    source = (XObject *)x_weak_ref_get(&binding->context->source);
    if (source) {
        x_object_unref(source);
    }

    return source;
}

XObject *x_binding_dup_source(XBinding *binding)
{
    x_return_val_if_fail(X_IS_BINDING(binding), NULL);
    return (XObject *)x_weak_ref_get(&binding->context->source);
}

XObject *x_binding_get_target(XBinding *binding)
{
    XObject *target;

    x_return_val_if_fail(X_IS_BINDING(binding), NULL);

    target = (XObject *)x_weak_ref_get(&binding->context->target);
    if (target) {
        x_object_unref(target);
    }

    return target;
}

XObject *x_binding_dup_target(XBinding *binding)
{
    x_return_val_if_fail(X_IS_BINDING(binding), NULL);
    return (XObject *)x_weak_ref_get(&binding->context->target);
}

const xchar *x_binding_get_source_property(XBinding *binding)
{
    x_return_val_if_fail(X_IS_BINDING(binding), NULL);
    return binding->source_property;
}

const xchar *x_binding_get_target_property(XBinding *binding)
{
    x_return_val_if_fail(X_IS_BINDING(binding), NULL);
    return binding->target_property;
}

void x_binding_unbind(XBinding *binding)
{
    x_return_if_fail(X_IS_BINDING(binding));
    x_binding_unbind_internal(binding, TRUE);
}

XBinding *x_object_bind_property_full(xpointer source, const xchar *source_property, xpointer target, const xchar *target_property, XBindingFlags flags, XBindingTransformFunc transform_to, XBindingTransformFunc transform_from, xpointer user_data, XDestroyNotify notify)
{
    XParamSpec *pspec;
    XBinding *binding;

    x_return_val_if_fail(X_IS_OBJECT(source), NULL);
    x_return_val_if_fail(source_property != NULL, NULL);
    x_return_val_if_fail(x_param_spec_is_valid_name(source_property), NULL);
    x_return_val_if_fail(X_IS_OBJECT(target), NULL);
    x_return_val_if_fail(target_property != NULL, NULL);
    x_return_val_if_fail(x_param_spec_is_valid_name(target_property), NULL);

    if (source == target && x_strcmp0(source_property, target_property) == 0) {
        x_critical("Unable to bind the same property on the same instance");
        return NULL;
    }

    if ((flags & X_BINDING_INVERT_BOOLEAN) && (transform_to != NULL || transform_from != NULL)) {
        flags = (XBindingFlags)(flags & ~X_BINDING_INVERT_BOOLEAN);
    }

    pspec = x_object_class_find_property(X_OBJECT_GET_CLASS(source), source_property);
    if (pspec == NULL) {
        x_critical("%s: The source object of type %s has no property called '%s'", X_STRLOC, X_OBJECT_TYPE_NAME(source), source_property);
        return NULL;
    }

    if (!(pspec->flags & X_PARAM_READABLE)) {
        x_critical("%s: The source object of type %s has no readable property called '%s'", X_STRLOC, X_OBJECT_TYPE_NAME(source), source_property);
        return NULL;
    }

    if ((flags & X_BINDING_BIDIRECTIONAL) && ((pspec->flags & X_PARAM_CONSTRUCT_ONLY) || !(pspec->flags & X_PARAM_WRITABLE))) {
        x_critical("%s: The source object of type %s has no writable property called '%s'", X_STRLOC, X_OBJECT_TYPE_NAME(source), source_property);
        return NULL;
    }

    if ((flags & X_BINDING_INVERT_BOOLEAN) && !(X_PARAM_SPEC_VALUE_TYPE(pspec) == X_TYPE_BOOLEAN)) {
        x_critical("%s: The X_BINDING_INVERT_BOOLEAN flag can only be used when binding boolean properties; the source property '%s' is of type '%s'", X_STRLOC, source_property, x_type_name(X_PARAM_SPEC_VALUE_TYPE(pspec)));
        return NULL;
    }

    pspec = x_object_class_find_property(X_OBJECT_GET_CLASS(target), target_property);
    if (pspec == NULL) {
        x_critical("%s: The target object of type %s has no property called '%s'", X_STRLOC, X_OBJECT_TYPE_NAME(target), target_property);
        return NULL;
    }

    if ((pspec->flags & X_PARAM_CONSTRUCT_ONLY) || !(pspec->flags & X_PARAM_WRITABLE)) {
        x_critical("%s: The target object of type %s has no writable property called '%s'", X_STRLOC, X_OBJECT_TYPE_NAME(target), target_property);
        return NULL;
    }

    if ((flags & X_BINDING_BIDIRECTIONAL) && !(pspec->flags & X_PARAM_READABLE)) {
        x_critical("%s: The target object of type %s has no readable property called '%s'", X_STRLOC, X_OBJECT_TYPE_NAME(target), target_property);
        return NULL;
    }

    if ((flags & X_BINDING_INVERT_BOOLEAN) && !(X_PARAM_SPEC_VALUE_TYPE(pspec) == X_TYPE_BOOLEAN)) {
        x_critical("%s: The X_BINDING_INVERT_BOOLEAN flag can only be used when binding boolean properties; the target property '%s' is of type '%s'", X_STRLOC, target_property, x_type_name(X_PARAM_SPEC_VALUE_TYPE(pspec)));
        return NULL;
    }

    binding = (XBinding *)x_object_new(X_TYPE_BINDING, "source", source, "source-property", source_property, "target", target, "target-property", target_property, "flags", flags, NULL);
    x_assert(binding->transform_func != NULL);

    if (transform_to == NULL) {
        transform_to = binding->transform_func->transform_s2t;
    }

    if (transform_from == NULL) {
        transform_from = binding->transform_func->transform_t2s;
    }

    x_clear_pointer(&binding->transform_func, transform_func_unref);
    binding->transform_func = transform_func_new(transform_to, transform_from, user_data, notify);

    if (flags & X_BINDING_SYNC_CREATE) {
        on_source_notify((XObject *)source, binding->source_pspec, binding->context);
    }

    return binding;
}

XBinding *x_object_bind_property(xpointer source, const xchar *source_property, xpointer target, const xchar *target_property, XBindingFlags flags)
{
  return x_object_bind_property_full(source, source_property, target, target_property, flags, NULL, NULL, NULL, NULL);
}

typedef struct _TransformData {
    XClosure *transform_to_closure;
    XClosure *transform_from_closure;
} TransformData;

static xboolean bind_with_closures_transform_to(XBinding *binding, const XValue *source, XValue *target, xpointer data)
{
    xboolean res;
    XValue retval = X_VALUE_INIT;
    TransformData *t_data = (TransformData *)data;
    XValue params[3] = { X_VALUE_INIT, X_VALUE_INIT, X_VALUE_INIT };

    x_value_init(&params[0], X_TYPE_BINDING);
    x_value_set_object(&params[0], binding);

    x_value_init(&params[1], X_TYPE_VALUE);
    x_value_set_boxed(&params[1], source);

    x_value_init(&params[2], X_TYPE_VALUE);
    x_value_set_boxed(&params[2], target);

    x_value_init(&retval, X_TYPE_BOOLEAN);
    x_value_set_boolean(&retval, FALSE);

    x_closure_invoke(t_data->transform_to_closure, &retval, 3, params, NULL);

    res = x_value_get_boolean(&retval);
    if (res) {
        const XValue *out_value = (const XValue *)x_value_get_boxed(&params[2]);
        x_assert(out_value != NULL);
        x_value_copy(out_value, target);
    }

    x_value_unset(&params[0]);
    x_value_unset(&params[1]);
    x_value_unset(&params[2]);
    x_value_unset(&retval);

    return res;
}

static xboolean bind_with_closures_transform_from(XBinding *binding, const XValue *source, XValue *target, xpointer data)
{
    xboolean res;
    XValue retval = X_VALUE_INIT;
    TransformData *t_data = (TransformData *)data;
    XValue params[3] = { X_VALUE_INIT, X_VALUE_INIT, X_VALUE_INIT };

    x_value_init(&params[0], X_TYPE_BINDING);
    x_value_set_object(&params[0], binding);

    x_value_init(&params[1], X_TYPE_VALUE);
    x_value_set_boxed(&params[1], source);

    x_value_init(&params[2], X_TYPE_VALUE);
    x_value_set_boxed(&params[2], target);

    x_value_init(&retval, X_TYPE_BOOLEAN);
    x_value_set_boolean(&retval, FALSE);

    x_closure_invoke(t_data->transform_from_closure, &retval, 3, params, NULL);

    res = x_value_get_boolean(&retval);
    if (res) {
        const XValue *out_value = (const XValue *)x_value_get_boxed(&params[2]);
        x_assert(out_value != NULL);
        x_value_copy(out_value, target);
    }

    x_value_unset(&params[0]);
    x_value_unset(&params[1]);
    x_value_unset(&params[2]);
    x_value_unset(&retval);

    return res;
}

static void bind_with_closures_free_func(xpointer data)
{
    TransformData *t_data = (TransformData *)data;

    if (t_data->transform_to_closure != NULL) {
        x_closure_unref(t_data->transform_to_closure);
    }

    if (t_data->transform_from_closure != NULL) {
        x_closure_unref(t_data->transform_from_closure);
    }

    x_slice_free(TransformData, t_data);
}

XBinding *x_object_bind_property_with_closures(xpointer source, const xchar *source_property, xpointer target, const xchar *target_property, XBindingFlags flags, XClosure *transform_to, XClosure *transform_from)
{
    TransformData *data;

    data = x_slice_new0(TransformData);

    if (transform_to != NULL) {
        if (X_CLOSURE_NEEDS_MARSHAL(transform_to)) {
            x_closure_set_marshal(transform_to, x_cclosure_marshal_BOOLEAN__BOXED_BOXED);
        }

        data->transform_to_closure = x_closure_ref(transform_to);
        x_closure_sink(data->transform_to_closure);
    }

    if (transform_from != NULL) {
        if (X_CLOSURE_NEEDS_MARSHAL(transform_from)) {
            x_closure_set_marshal(transform_from, x_cclosure_marshal_BOOLEAN__BOXED_BOXED);
        }

        data->transform_from_closure = x_closure_ref(transform_from);
        x_closure_sink(data->transform_from_closure);
    }

    return x_object_bind_property_full(source, source_property, target, target_property, flags, transform_to != NULL ? bind_with_closures_transform_to : NULL, transform_from != NULL ? bind_with_closures_transform_from : NULL, data, bind_with_closures_free_func);
}
