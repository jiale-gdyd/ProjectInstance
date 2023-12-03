#include <xlib/xlib/config.h>
#include <xlib/xlib.h>
#include <xlib/xlib/xlibintl.h>
#include <xlib/xobj/xparamspecs.h>
#include <xlib/xobj/xbindinggroup.h>

struct _XBindingGroup {
    XObject   parent_instance;
    XMutex    mutex;
    XObject   *source;
    XPtrArray *lazy_bindings;
};

typedef struct _XBindingGroupClass {
    XObjectClass parent_class;
} XBindingGroupClass;

typedef struct {
    XBindingGroup  *group;
    const char     *source_property;
    const char     *target_property;
    XObject        *target;
    XBinding       *binding;
    xpointer       user_data;
    XDestroyNotify user_data_destroy;
    xpointer       transform_to;
    xpointer       transform_from;
    XBindingFlags  binding_flags;
    xuint          using_closures : 1;
} LazyBinding;

X_DEFINE_TYPE(XBindingGroup, x_binding_group, X_TYPE_OBJECT)

typedef enum {
    PROP_SOURCE = 1,
    N_PROPS
} XBindingGroupProperty;

static XParamSpec *properties[N_PROPS];

static void lazy_binding_free(xpointer data);

static void x_binding_group_connect(XBindingGroup *self, LazyBinding *lazy_binding)
{
    XBinding *binding;

    x_assert(X_IS_BINDING_GROUP(self));
    x_assert(self->source != NULL);
    x_assert(lazy_binding != NULL);
    x_assert(lazy_binding->binding == NULL);
    x_assert(lazy_binding->target != NULL);
    x_assert(lazy_binding->target_property != NULL);
    x_assert(lazy_binding->source_property != NULL);

    if (!lazy_binding->using_closures) {
        binding = x_object_bind_property_full(self->source, lazy_binding->source_property, lazy_binding->target, lazy_binding->target_property, lazy_binding->binding_flags, (XBindingTransformFunc)lazy_binding->transform_to, (XBindingTransformFunc)lazy_binding->transform_from, lazy_binding->user_data, NULL);
    } else {
        binding = x_object_bind_property_with_closures(self->source, lazy_binding->source_property, lazy_binding->target, lazy_binding->target_property, lazy_binding->binding_flags, (XClosure *)lazy_binding->transform_to, (XClosure *)lazy_binding->transform_from);
    }

    lazy_binding->binding = binding;
}

static void x_binding_group_disconnect(LazyBinding *lazy_binding)
{
    x_assert(lazy_binding != NULL);

    if (lazy_binding->binding != NULL) {
        x_binding_unbind(lazy_binding->binding);
        lazy_binding->binding = NULL;
    }
}

static void x_binding_group__source_weak_notify(xpointer data, XObject *where_object_was)
{
    xuint i;
    XBindingGroup *self = (XBindingGroup *)data;

    x_assert(X_IS_BINDING_GROUP(self));

    x_mutex_lock(&self->mutex);

    self->source = NULL;
    for (i = 0; i < self->lazy_bindings->len; i++) {
        LazyBinding *lazy_binding = (LazyBinding *)x_ptr_array_index(self->lazy_bindings, i);
        lazy_binding->binding = NULL;
    }
    x_mutex_unlock(&self->mutex);
}

static void x_binding_group__target_weak_notify(xpointer data, XObject *where_object_was)
{
    xuint i;
    LazyBinding *to_free = NULL;
    XBindingGroup *self = (XBindingGroup *)data;

    x_assert(X_IS_BINDING_GROUP(self));

    x_mutex_lock(&self->mutex);
    for (i = 0; i < self->lazy_bindings->len; i++) {
        LazyBinding *lazy_binding = (LazyBinding *)x_ptr_array_index(self->lazy_bindings, i);

        if (lazy_binding->target == where_object_was) {
            lazy_binding->target = NULL;
            lazy_binding->binding = NULL;

            to_free = (LazyBinding *)x_ptr_array_steal_index_fast(self->lazy_bindings, i);
            break;
        }
    }
    x_mutex_unlock(&self->mutex);

    if (to_free != NULL) {
        lazy_binding_free(to_free);
    }
}

static void lazy_binding_free(xpointer data)
{
    LazyBinding *lazy_binding = (LazyBinding *)data;

    if (lazy_binding->target != NULL) {
        x_object_weak_unref(lazy_binding->target, x_binding_group__target_weak_notify, lazy_binding->group);
        lazy_binding->target = NULL;
    }

    x_binding_group_disconnect(lazy_binding);

    lazy_binding->group = NULL;
    lazy_binding->source_property = NULL;
    lazy_binding->target_property = NULL;

    if (lazy_binding->user_data_destroy) {
        lazy_binding->user_data_destroy(lazy_binding->user_data);
    }

    if (lazy_binding->using_closures) {
        x_clear_pointer((XClosure **)&lazy_binding->transform_to, x_closure_unref);
        x_clear_pointer((XClosure **)&lazy_binding->transform_from, x_closure_unref);
    }

    x_slice_free(LazyBinding, lazy_binding);
}

static void x_binding_group_dispose(XObject *object)
{
    xsize i;
    xsize len = 0;
    LazyBinding **lazy_bindings = NULL;
    XBindingGroup *self = (XBindingGroup *)object;

    x_assert(X_IS_BINDING_GROUP(self));

    x_mutex_lock(&self->mutex);
    if (self->source != NULL) {
        x_object_weak_unref(self->source, x_binding_group__source_weak_notify, self);
        self->source = NULL;
    }

    if (self->lazy_bindings->len > 0) {
        lazy_bindings = (LazyBinding **)x_ptr_array_steal(self->lazy_bindings, &len);
    }
    x_mutex_unlock(&self->mutex);

    for (i = 0; i < len; i++) {
        lazy_binding_free(lazy_bindings[i]);
    }
    x_free(lazy_bindings);

    X_OBJECT_CLASS(x_binding_group_parent_class)->dispose(object);
}

static void x_binding_group_finalize(XObject *object)
{
    XBindingGroup *self = (XBindingGroup *)object;

    x_assert(self->lazy_bindings != NULL);
    x_assert(self->lazy_bindings->len == 0);

    x_clear_pointer(&self->lazy_bindings, x_ptr_array_unref);
    x_mutex_clear(&self->mutex);

    X_OBJECT_CLASS(x_binding_group_parent_class)->finalize(object);
}

static void x_binding_group_get_property(XObject *object, xuint prop_id, XValue *value, XParamSpec *pspec)
{
    XBindingGroup *self = X_BINDING_GROUP(object);

    switch ((XBindingGroupProperty) prop_id) {
        case PROP_SOURCE:
            x_value_take_object(value, x_binding_group_dup_source(self));
            break;

        default:
            X_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void x_binding_group_set_property(XObject *object, xuint prop_id, const XValue *value, XParamSpec *pspec)
{
    XBindingGroup *self = X_BINDING_GROUP(object);

    switch ((XBindingGroupProperty) prop_id) {
        case PROP_SOURCE:
            x_binding_group_set_source(self, x_value_get_object(value));
            break;

        default:
            X_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void x_binding_group_class_init(XBindingGroupClass *klass)
{
    XObjectClass *object_class = X_OBJECT_CLASS(klass);

    object_class->dispose = x_binding_group_dispose;
    object_class->finalize = x_binding_group_finalize;
    object_class->get_property = x_binding_group_get_property;
    object_class->set_property = x_binding_group_set_property;

    properties[PROP_SOURCE] = x_param_spec_object("source", NULL, NULL, X_TYPE_OBJECT, (XParamFlags)(X_PARAM_READWRITE | X_PARAM_EXPLICIT_NOTIFY | X_PARAM_STATIC_STRINGS));
    x_object_class_install_properties(object_class, N_PROPS, properties);
}

static void x_binding_group_init(XBindingGroup *self)
{
    x_mutex_init(&self->mutex);
    self->lazy_bindings = x_ptr_array_new_with_free_func(lazy_binding_free);
}

XBindingGroup *x_binding_group_new(void)
{
    return (XBindingGroup *)x_object_new(X_TYPE_BINDING_GROUP, NULL);
}

xpointer x_binding_group_dup_source(XBindingGroup *self)
{
    XObject *source;

    x_return_val_if_fail(X_IS_BINDING_GROUP(self), NULL);

    x_mutex_lock(&self->mutex);
    source = self->source ? x_object_ref(self->source) : NULL;
    x_mutex_unlock(&self->mutex);

    return source;
}

static xboolean x_binding_group_check_source(XBindingGroup *self, xpointer source)
{
    xuint i;

    x_assert(X_IS_BINDING_GROUP(self));
    x_assert(!source || X_IS_OBJECT(source));

    for (i = 0; i < self->lazy_bindings->len; i++) {
        LazyBinding *lazy_binding = (LazyBinding *)x_ptr_array_index(self->lazy_bindings, i);
        x_return_val_if_fail(x_object_class_find_property(X_OBJECT_GET_CLASS(source), lazy_binding->source_property) != NULL, FALSE);
    }

    return TRUE;
}

void x_binding_group_set_source(XBindingGroup *self, xpointer source)
{
    xboolean notify = FALSE;

    x_return_if_fail(X_IS_BINDING_GROUP(self));
    x_return_if_fail(!source || X_IS_OBJECT(source));
    x_return_if_fail(source != (xpointer)self);

    x_mutex_lock(&self->mutex);

    if (source == (xpointer)self->source) {
        goto unlock;
    }

    if (self->source != NULL) {
        xuint i;

        x_object_weak_unref(self->source, x_binding_group__source_weak_notify, self);
        self->source = NULL;

        for (i = 0; i < self->lazy_bindings->len; i++) {
            LazyBinding *lazy_binding = (LazyBinding *)x_ptr_array_index(self->lazy_bindings, i);
            x_binding_group_disconnect(lazy_binding);
        }
    }

    if (source != NULL && x_binding_group_check_source(self, source)) {
        xuint i;

        self->source = (XObject *)source;
        x_object_weak_ref(self->source, x_binding_group__source_weak_notify, self);

        for (i = 0; i < self->lazy_bindings->len; i++) {
            LazyBinding *lazy_binding;

            lazy_binding = (LazyBinding *)x_ptr_array_index(self->lazy_bindings, i);
            x_binding_group_connect(self, lazy_binding);
        }
    }

    notify = TRUE;

unlock:
    x_mutex_unlock(&self->mutex);
    if (notify) {
        x_object_notify_by_pspec(X_OBJECT(self), properties[PROP_SOURCE]);
    }
}

static void x_binding_group_bind_helper(XBindingGroup *self, const xchar *source_property, xpointer target, const xchar *target_property, XBindingFlags flags, xpointer transform_to, xpointer transform_from, xpointer user_data, XDestroyNotify user_data_destroy, xboolean using_closures)
{
    LazyBinding *lazy_binding;

    x_return_if_fail(X_IS_BINDING_GROUP(self));
    x_return_if_fail(source_property != NULL);
    x_return_if_fail(self->source == NULL || x_object_class_find_property(X_OBJECT_GET_CLASS(self->source), source_property) != NULL);
    x_return_if_fail(X_IS_OBJECT(target));
    x_return_if_fail(target_property != NULL);
    x_return_if_fail(x_object_class_find_property(X_OBJECT_GET_CLASS(target), target_property) != NULL);
    x_return_if_fail(target != (xpointer)self || strcmp(source_property, target_property) != 0);

    x_mutex_lock(&self->mutex);

    lazy_binding = x_slice_new0(LazyBinding);
    lazy_binding->group = self;
    lazy_binding->source_property = x_intern_string(source_property);
    lazy_binding->target_property = x_intern_string(target_property);
    lazy_binding->target = (XObject *)target;
    lazy_binding->binding_flags = (XBindingFlags)(flags | X_BINDING_SYNC_CREATE);
    lazy_binding->user_data = user_data;
    lazy_binding->user_data_destroy = user_data_destroy;
    lazy_binding->transform_to = transform_to;
    lazy_binding->transform_from = transform_from;

    if (using_closures) {
        lazy_binding->using_closures = TRUE;

        if (transform_to != NULL) {
            x_closure_sink(x_closure_ref((XClosure *)transform_to));
        }

        if (transform_from != NULL) {
            x_closure_sink(x_closure_ref((XClosure *)transform_from));
        }
    }

    x_object_weak_ref((XObject *)target, x_binding_group__target_weak_notify, self);
    x_ptr_array_add(self->lazy_bindings, lazy_binding);

    if (self->source != NULL) {
        x_binding_group_connect(self, lazy_binding);
    }

    x_mutex_unlock(&self->mutex);
}

void x_binding_group_bind(XBindingGroup *self, const xchar *source_property, xpointer target, const xchar *target_property, XBindingFlags flags)
{
    x_binding_group_bind_full(self, source_property, target, target_property, flags, NULL, NULL, NULL, NULL);
}

void x_binding_group_bind_full(XBindingGroup *self, const xchar *source_property, xpointer target, const xchar *target_property, XBindingFlags flags, XBindingTransformFunc transform_to, XBindingTransformFunc transform_from, xpointer user_data, XDestroyNotify user_data_destroy)
{
    x_binding_group_bind_helper(self, source_property, target, target_property, flags, (xpointer)transform_to, (xpointer)transform_from, user_data, user_data_destroy, FALSE);
}

void x_binding_group_bind_with_closures(XBindingGroup *self, const xchar *source_property, xpointer target, const xchar *target_property, XBindingFlags flags, XClosure *transform_to, XClosure *transform_from)
{
    x_binding_group_bind_helper(self, source_property, target, target_property, flags, transform_to, transform_from, NULL, NULL, TRUE);
}
