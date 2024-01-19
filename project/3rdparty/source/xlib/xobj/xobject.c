#include <string.h>
#include <signal.h>

#include <xlib/xlib/config.h>
#include <xlib/xobj/xobject.h>
#include <xlib/xobj/xsignal.h>
#include <xlib/xobj/xparamspecs.h>
#include <xlib/xobj/xvaluetypes.h>
#include <xlib/xlib/xconstructor.h>
#include <xlib/xlib/xlib-private.h>
#include <xlib/xobj/xobject_trace.h>
#include <xlib/xobj/xtype-private.h>
#include <xlib/xobj/xvaluecollector.h>

#define PARAM_SPEC_PARAM_ID(pspec)                      ((pspec)->param_id)
#define PARAM_SPEC_SET_PARAM_ID(pspec, id)              ((pspec)->param_id = (id))

#define OBJECT_HAS_TOGGLE_REF_FLAG                      0x1
#define OBJECT_HAS_TOGGLE_REF(object)                   ((x_datalist_get_flags(&(object)->qdata) & OBJECT_HAS_TOGGLE_REF_FLAG) != 0)
#define OBJECT_FLOATING_FLAG                            0x2

#define CLASS_HAS_PROPS_FLAG                            0x1
#define CLASS_HAS_PROPS(classt)                         ((classt)->flags & CLASS_HAS_PROPS_FLAG)
#define CLASS_HAS_CUSTOM_CONSTRUCTOR(classt)            ((classt)->constructor != x_object_constructor)
#define CLASS_HAS_CUSTOM_CONSTRUCTED(classt)            ((classt)->constructed != x_object_constructed)
#define CLASS_HAS_NOTIFY(classt)                        ((classt)->notify != NULL)
#define CLASS_HAS_CUSTOM_DISPATCH(classt)               ((classt)->dispatch_properties_changed != x_object_dispatch_properties_changed)
#define CLASS_NEEDS_NOTIFY(classt)                      (CLASS_HAS_NOTIFY(classt) || CLASS_HAS_CUSTOM_DISPATCH(classt))

#define CLASS_HAS_DERIVED_CLASS_FLAG                    0x2
#define CLASS_HAS_DERIVED_CLASS(classt)                 ((classt)->flags & CLASS_HAS_DERIVED_CLASS_FLAG)

enum {
    NOTIFY,
    LAST_SIGNAL
};

enum {
    PROP_NONE
};

#define _OPTIONAL_BIT_LOCK                              3

#define OPTIONAL_FLAG_IN_CONSTRUCTION                   (1 << 0)
#define OPTIONAL_FLAG_HAS_SIGNAL_HANDLER                (1 << 1)
#define OPTIONAL_FLAG_HAS_NOTIFY_HANDLER                (1 << 2)
#define OPTIONAL_FLAG_LOCK                              (1 << 3)

#define OPTIONAL_BIT_LOCK_WEAK_REFS                     1
#define OPTIONAL_BIT_LOCK_NOTIFY                        2
#define OPTIONAL_BIT_LOCK_TOGGLE_REFS                   3
#define OPTIONAL_BIT_LOCK_CLOSURE_ARRAY                 4

#if SIZEOF_INT == 4 && XLIB_SIZEOF_VOID_P >= 8
#define HAVE_OPTIONAL_FLAGS_IN_GOBJECT                  1
#else
#define HAVE_OPTIONAL_FLAGS_IN_GOBJECT                  0
#endif

#define HAVE_PRIVATE (!HAVE_OPTIONAL_FLAGS_IN_GOBJECT)

#if HAVE_PRIVATE
typedef struct {
#if !HAVE_OPTIONAL_FLAGS_IN_GOBJECT
    xuint optional_flags;
#endif
} XObjectPrivate;

static int XObject_private_offset;
#endif

typedef struct {
    XTypeInstance x_type_instance;
    xuint         ref_count;
#if HAVE_OPTIONAL_FLAGS_IN_GOBJECT
    xuint         optional_flags;
#endif
    XData         *qdata;
} XObjectReal;

X_STATIC_ASSERT(sizeof(XObject) == sizeof(XObjectReal));
X_STATIC_ASSERT(X_STRUCT_OFFSET(XObject, ref_count) == X_STRUCT_OFFSET(XObjectReal, ref_count));
X_STATIC_ASSERT(X_STRUCT_OFFSET(XObject, qdata) == X_STRUCT_OFFSET(XObjectReal, qdata));

static void x_object_base_class_init(XObjectClass *classt);
static void x_object_base_class_finalize(XObjectClass *classt);
static void x_object_do_class_init(XObjectClass	*classt);
static void x_object_init(XObject *object, XObjectClass *classt);
static XObject *x_object_constructor(XType type, xuint n_construct_properties, XObjectConstructParam *construct_params);
static void x_object_constructed(XObject *object);
static void x_object_real_dispose(XObject *object);
static void x_object_finalize(XObject *object);
static void x_object_do_set_property(XObject *object, xuint property_id, const XValue *value, XParamSpec *pspec);
static void x_object_do_get_property(XObject *object, xuint property_id, XValue *value, XParamSpec *pspec);
static void x_value_object_init(XValue *value);
static void x_value_object_free_value(XValue *value);
static void x_value_object_copy_value(const XValue *src_value, XValue *dest_value);
static void x_value_object_transform_value(const XValue *src_value, XValue *dest_value);
static xpointer x_value_object_peek_pointer(const XValue *value);
static xchar *x_value_object_collect_value(XValue *value, xuint n_collect_values, XTypeCValue *collect_values, xuint collect_flags);
static xchar *x_value_object_lcopy_value(const XValue *value, xuint n_collect_values, XTypeCValue *collect_values, xuint collect_flags);
static void x_object_dispatch_properties_changed(XObject *object, xuint n_pspecs, XParamSpec **pspecs);
static xuint object_floating_flag_handler(XObject *object, xint job);

static void object_interface_check_properties(xpointer check_data, xpointer x_iface);
static void weak_locations_free_unlocked(XSList **weak_locations);

typedef struct _XObjectNotifyQueue XObjectNotifyQueue;

struct _XObjectNotifyQueue {
    XSList  *pspecs;
    xuint16 n_pspecs;
    xuint16 freeze_count;
};

static XQuark quark_notify_queue;
static XQuark quark_toggle_refs = 0;
static XQuark quark_closure_array = 0;
static XQuark quark_weak_notifies = 0;

static XParamSpecPool *pspec_pool = NULL;
static xulong xobject_signals[LAST_SIGNAL] = { 0, };
static xuint (*floating_flag_handler)(XObject *, xint) = object_floating_flag_handler;

static XRWLock weak_locations_lock;
static XQuark quark_weak_locations = 0;

#if HAVE_PRIVATE
X_ALWAYS_INLINE static inline XObjectPrivate *x_object_get_instance_private(XObject *object)
{
    return X_STRUCT_MEMBER_P(object, GObject_private_offset);
}
#endif

X_ALWAYS_INLINE static inline xuint *object_get_optional_flags_p(XObject *object)
{
#if HAVE_OPTIONAL_FLAGS_IN_GOBJECT
    return &(((XObjectReal *)object)->optional_flags);
#else
    return &x_object_get_instance_private(object)->optional_flags;
#endif
}

#if defined(X_ENABLE_DEBUG) && defined(X_THREAD_LOCAL)
static X_THREAD_LOCAL xuint _object_bit_is_locked;
#endif

static void object_bit_lock(XObject *object, xuint lock_bit)
{
#if defined(X_ENABLE_DEBUG) && defined(X_THREAD_LOCAL)
    x_assert(lock_bit > 0);
    x_assert(_object_bit_is_locked == 0);
    _object_bit_is_locked = lock_bit;
#endif

    x_bit_lock((xint *)object_get_optional_flags_p(object), _OPTIONAL_BIT_LOCK);
}

static void object_bit_unlock(XObject *object, xuint lock_bit)
{
#if defined(X_ENABLE_DEBUG) && defined(X_THREAD_LOCAL)
    x_assert(lock_bit > 0);
    x_assert(_object_bit_is_locked == lock_bit);
    _object_bit_is_locked = 0;
#endif

    x_bit_unlock((xint *)object_get_optional_flags_p(object), _OPTIONAL_BIT_LOCK);
}

static void x_object_notify_queue_free(xpointer data)
{
    XObjectNotifyQueue *nqueue = (XObjectNotifyQueue *)data;

    x_slist_free(nqueue->pspecs);
    x_free_sized(nqueue, sizeof(XObjectNotifyQueue));
}

static XObjectNotifyQueue *x_object_notify_queue_create_queue_frozen(XObject *object)
{
    XObjectNotifyQueue *nqueue;

    nqueue = x_new0(XObjectNotifyQueue, 1);
    *nqueue = (XObjectNotifyQueue){
        .freeze_count = 1,
    };

    x_datalist_id_set_data_full(&object->qdata, quark_notify_queue, nqueue, x_object_notify_queue_free);
    return nqueue;
}

static XObjectNotifyQueue *x_object_notify_queue_freeze(XObject *object)
{
    XObjectNotifyQueue *nqueue;

    object_bit_lock(object, OPTIONAL_BIT_LOCK_NOTIFY);
    nqueue = (XObjectNotifyQueue *)x_datalist_id_get_data(&object->qdata, quark_notify_queue);
    if (!nqueue) {
        nqueue = x_object_notify_queue_create_queue_frozen(object);
        goto out;
    }

    if (nqueue->freeze_count >= 65535) {
        x_critical("Free queue for %s (%p) is larger than 65535, called x_object_freeze_notify() too often. Forgot to call x_object_thaw_notify() or infinite loop", X_OBJECT_TYPE_NAME(object), object);
    } else {
        nqueue->freeze_count++;
    }
out:
    object_bit_unlock(object, OPTIONAL_BIT_LOCK_NOTIFY);

    return nqueue;
}

static void x_object_notify_queue_thaw(XObject *object, XObjectNotifyQueue *nqueue, xboolean take_ref)
{
    XSList *slist;
    xuint n_pspecs = 0;
    XParamSpec *pspecs_mem[16], **pspecs, **free_me = NULL;

    object_bit_lock(object, OPTIONAL_BIT_LOCK_NOTIFY);

    if (!nqueue) {
        nqueue = x_datalist_id_get_data(&object->qdata, quark_notify_queue);
    }

    if (X_UNLIKELY(!nqueue || nqueue->freeze_count == 0)) {
        object_bit_unlock(object, OPTIONAL_BIT_LOCK_NOTIFY);
        x_critical("%s: property-changed notification for %s(%p) is not frozen", X_STRFUNC, X_OBJECT_TYPE_NAME(object), object);
        return;
    }

    nqueue->freeze_count--;
    if (nqueue->freeze_count) {
        object_bit_unlock(object, OPTIONAL_BIT_LOCK_NOTIFY);
        return;
    }

    pspecs = nqueue->n_pspecs > 16 ? free_me = x_new(XParamSpec *, nqueue->n_pspecs) : pspecs_mem;

    for (slist = nqueue->pspecs; slist; slist = slist->next) {
        pspecs[n_pspecs++] = (XParamSpec *)slist->data;
    }
    x_datalist_id_set_data(&object->qdata, quark_notify_queue, NULL);

    object_bit_unlock(object, OPTIONAL_BIT_LOCK_NOTIFY);

    if (n_pspecs) {
        if (take_ref) {
            x_object_ref(object);
        }

        X_OBJECT_GET_CLASS(object)->dispatch_properties_changed(object, n_pspecs, pspecs);
        if (take_ref) {
            x_object_unref(object);
        }
    }

    x_free(free_me);
}

static xboolean x_object_notify_queue_add(XObject *object, XObjectNotifyQueue *nqueue, XParamSpec *pspec, xboolean in_init)
{
    object_bit_lock(object, OPTIONAL_BIT_LOCK_NOTIFY);

    if (!nqueue) {
        nqueue = x_datalist_id_get_data(&object->qdata, quark_notify_queue);
        if (!nqueue) {
            if (!in_init) {
                object_bit_unlock(object, OPTIONAL_BIT_LOCK_NOTIFY);
                return FALSE;
            }

            nqueue = x_object_notify_queue_create_queue_frozen(object);
        }
    }

    x_assert(nqueue->n_pspecs < 65535);

    if (x_slist_find(nqueue->pspecs, pspec) == NULL) {
        nqueue->pspecs = x_slist_prepend(nqueue->pspecs, pspec);
        nqueue->n_pspecs++;
    }

    object_bit_unlock(object, OPTIONAL_BIT_LOCK_NOTIFY);
    return TRUE;
}

void _x_object_type_init(void)
{
    static xboolean initialized = FALSE;
    static const XTypeFundamentalInfo finfo = {
        (XTypeFundamentalFlags)(X_TYPE_FLAG_CLASSED | X_TYPE_FLAG_INSTANTIATABLE | X_TYPE_FLAG_DERIVABLE | X_TYPE_FLAG_DEEP_DERIVABLE),
    };

    XTypeInfo info = {
        sizeof(XObjectClass),
        (XBaseInitFunc)x_object_base_class_init,
        (XBaseFinalizeFunc)x_object_base_class_finalize,
        (XClassInitFunc)x_object_do_class_init,
        NULL,
        NULL,
        sizeof(XObject),
        0,
        (XInstanceInitFunc)x_object_init,
        NULL,
    };

    static const XTypeValueTable value_table = {
        x_value_object_init,
        x_value_object_free_value,
        x_value_object_copy_value,
        x_value_object_peek_pointer,
        "p",
        x_value_object_collect_value,
        "p",
        x_value_object_lcopy_value,
    };

    XType type X_GNUC_UNUSED;
    x_return_if_fail(initialized == FALSE);
    initialized = TRUE;

    info.value_table = &value_table;
    type = x_type_register_fundamental(X_TYPE_OBJECT, x_intern_static_string("XObject"), &info, &finfo, (XTypeFlags)0);
    x_assert(type == X_TYPE_OBJECT);
    x_value_register_transform_func(X_TYPE_OBJECT, X_TYPE_OBJECT, x_value_object_transform_value);

#if HAVE_PRIVATE
    XObject_private_offset = x_type_add_instance_private(X_TYPE_OBJECT, sizeof(XObjectPrivate));
#endif
}

static inline void x_object_init_pspec_pool(void)
{
    if (X_UNLIKELY(x_atomic_pointer_get(&pspec_pool) == NULL)) {
        XParamSpecPool *pool = x_param_spec_pool_new(TRUE);
        if (!x_atomic_pointer_compare_and_exchange(&pspec_pool, NULL, pool)) {
            x_param_spec_pool_free(pool);
        }
    }
}

static void x_object_base_class_init(XObjectClass *classt)
{
    XObjectClass *pclass = (XObjectClass *)x_type_class_peek_parent(classt);

    classt->flags &= ~CLASS_HAS_DERIVED_CLASS_FLAG;
    if (pclass) {
        pclass->flags |= CLASS_HAS_DERIVED_CLASS_FLAG;
    }

    classt->construct_properties = pclass ? x_slist_copy(pclass->construct_properties) : NULL;
    classt->n_construct_properties = x_slist_length(classt->construct_properties);
    classt->get_property = NULL;
    classt->set_property = NULL;
    classt->pspecs = NULL;
    classt->n_pspecs = 0;
}

static void x_object_base_class_finalize(XObjectClass *classt)
{
    XList *list, *node;

    _x_signals_destroy(X_OBJECT_CLASS_TYPE(classt));

    x_slist_free(classt->construct_properties);
    classt->construct_properties = NULL;
    classt->n_construct_properties = 0;
    list = x_param_spec_pool_list_owned(pspec_pool, X_OBJECT_CLASS_TYPE(classt));

    for (node = list; node; node = node->next) {
        XParamSpec *pspec = (XParamSpec *)node->data;

        x_param_spec_pool_remove(pspec_pool, pspec);
        PARAM_SPEC_SET_PARAM_ID(pspec, 0);
        x_param_spec_unref(pspec);
    }

    x_list_free(list);
}

static void x_object_do_class_init(XObjectClass *classt)
{
    quark_closure_array = x_quark_from_static_string("XObject-closure-array");

    quark_weak_notifies = x_quark_from_static_string("XObject-weak-notifies");
    quark_weak_locations = x_quark_from_static_string("XObject-weak-locations");
    quark_toggle_refs = x_quark_from_static_string("XObject-toggle-references");
    quark_notify_queue = x_quark_from_static_string("XObject-notify-queue");

    x_object_init_pspec_pool();

    classt->constructor = x_object_constructor;
    classt->constructed = x_object_constructed;
    classt->set_property = x_object_do_set_property;
    classt->get_property = x_object_do_get_property;
    classt->dispose = x_object_real_dispose;
    classt->finalize = x_object_finalize;
    classt->dispatch_properties_changed = x_object_dispatch_properties_changed;
    classt->notify = NULL;

    xobject_signals[NOTIFY] =
        x_signal_new(x_intern_static_string("notify"),
            X_TYPE_FROM_CLASS(classt),
            (XSignalFlags)(X_SIGNAL_RUN_FIRST | X_SIGNAL_NO_RECURSE | X_SIGNAL_DETAILED | X_SIGNAL_NO_HOOKS | X_SIGNAL_ACTION),
            X_STRUCT_OFFSET(XObjectClass, notify),
            NULL, NULL,
            NULL,
            X_TYPE_NONE,
            1, X_TYPE_PARAM);

    x_type_add_interface_check(NULL, object_interface_check_properties);

#if HAVE_PRIVATE
    x_type_class_adjust_private_offset(class, &XObject_private_offset);
#endif
}

static inline xboolean install_property_internal(XType x_type, xuint property_id, XParamSpec *pspec)
{
    x_param_spec_ref_sink(pspec);
    x_object_init_pspec_pool();

    if (x_param_spec_pool_lookup(pspec_pool, pspec->name, x_type, FALSE)) {
        x_critical("When installing property: type '%s' already has a property named '%s'", x_type_name(x_type), pspec->name);
        x_param_spec_unref(pspec);
        return FALSE;
    }

    PARAM_SPEC_SET_PARAM_ID(pspec, property_id);
    x_param_spec_pool_insert(pspec_pool, x_steal_pointer(&pspec), x_type);

    return TRUE;
}

static xboolean validate_pspec_to_install(XParamSpec *pspec)
{
    x_return_val_if_fail(X_IS_PARAM_SPEC(pspec), FALSE);
    x_return_val_if_fail(PARAM_SPEC_PARAM_ID(pspec) == 0, FALSE);

    x_return_val_if_fail(pspec->flags & (X_PARAM_READABLE | X_PARAM_WRITABLE), FALSE);

    if (pspec->flags & X_PARAM_CONSTRUCT) {
        x_return_val_if_fail((pspec->flags & X_PARAM_CONSTRUCT_ONLY) == 0, FALSE);
    }

    if (pspec->flags & (X_PARAM_CONSTRUCT | X_PARAM_CONSTRUCT_ONLY)) {
        x_return_val_if_fail(pspec->flags & X_PARAM_WRITABLE, FALSE);
    }

    return TRUE;
}

static xboolean validate_and_install_class_property(XObjectClass *classt, XType oclass_type, XType parent_type, xuint property_id, XParamSpec *pspec)
{
    if (!validate_pspec_to_install(pspec)) {
        x_param_spec_ref_sink(pspec);
        x_param_spec_unref(pspec);
        return FALSE;
    }

    if (pspec->flags & X_PARAM_WRITABLE) {
        x_return_val_if_fail(classt->set_property != NULL, FALSE);
    }

    if (pspec->flags & X_PARAM_READABLE) {
        x_return_val_if_fail(classt->get_property != NULL, FALSE);
    }

    classt->flags |= CLASS_HAS_PROPS_FLAG;
    if (install_property_internal(oclass_type, property_id, pspec)) {
        if (pspec->flags & (X_PARAM_CONSTRUCT | X_PARAM_CONSTRUCT_ONLY)) {
            classt->construct_properties = x_slist_append(classt->construct_properties, pspec);
            classt->n_construct_properties += 1;
        }

        pspec = x_param_spec_pool_lookup(pspec_pool, pspec->name, parent_type, TRUE);
        if (pspec && pspec->flags & (X_PARAM_CONSTRUCT | X_PARAM_CONSTRUCT_ONLY)) {
            classt->construct_properties = x_slist_remove(classt->construct_properties, pspec);
            classt->n_construct_properties -= 1;
        }

        return TRUE;
    } else {
        return FALSE;
    }
}

void x_object_class_install_property(XObjectClass *classt, xuint property_id, XParamSpec *pspec)
{
    XType oclass_type, parent_type;

    x_return_if_fail(X_IS_OBJECT_CLASS(classt));
    x_return_if_fail(property_id > 0);

    oclass_type = X_OBJECT_CLASS_TYPE(classt);
    parent_type = x_type_parent(oclass_type);

    if (CLASS_HAS_DERIVED_CLASS(classt)) {
        x_error("Attempt to add property %s::%s to class after it was derived", X_OBJECT_CLASS_NAME(classt), pspec->name);
    }

    (void)validate_and_install_class_property(classt, oclass_type, parent_type, property_id, pspec);
}

typedef struct {
    const char *name;
    XParamSpec *pspec;
} PspecEntry;

static int compare_pspec_entry(const void *a, const void *b)
{
    const PspecEntry *ae = (const PspecEntry *)a;
    const PspecEntry *be = (const PspecEntry *)b;

    return ae->name < be->name ? -1 : (ae->name > be->name ? 1 : 0);
}

static inline XParamSpec *find_pspec(XObjectClass *classt, const char *property_name)
{
    xsize n_pspecs = classt->n_pspecs;
    const PspecEntry *pspecs = (const PspecEntry *)classt->pspecs;

    x_assert(n_pspecs <= X_MAXSSIZE);

    if (n_pspecs < 10) {
        for (xsize i = 0; i < n_pspecs; i++){
            if (pspecs[i].name == property_name) {
                return pspecs[i].pspec;
            }
        }
    } else {
        xssize mid;
        xssize lower = 0;
        xssize upper = (int)classt->n_pspecs - 1;

        while (lower <= upper) {
            mid = (lower + upper) / 2;
            if (property_name < pspecs[mid].name) {
                upper = mid - 1;
            } else if (property_name > pspecs[mid].name) {
                lower = mid + 1;
            } else {
                return pspecs[mid].pspec;
            }
        }
    }

    return x_param_spec_pool_lookup(pspec_pool, property_name, ((XTypeClass *)classt)->x_type, TRUE);
}

void x_object_class_install_properties(XObjectClass *oclass, xuint n_pspecs, XParamSpec **pspecs)
{
    xuint i;
    XType oclass_type, parent_type;

    x_return_if_fail(X_IS_OBJECT_CLASS(oclass));
    x_return_if_fail(n_pspecs > 1);
    x_return_if_fail(pspecs[0] == NULL);

    if (CLASS_HAS_DERIVED_CLASS(oclass)) {
        x_error("Attempt to add properties to %s after it was derived", X_OBJECT_CLASS_NAME(oclass));
    }

    oclass_type = X_OBJECT_CLASS_TYPE(oclass);
    parent_type = x_type_parent(oclass_type);

    for (i = 1; i < n_pspecs; i++) {
        XParamSpec *pspec = pspecs[i];

        if (!validate_and_install_class_property(oclass, oclass_type, parent_type, i, pspec)) {
            break;
        }
    }

    if (oclass->pspecs == NULL) {
        PspecEntry *entries;

        entries = x_new(PspecEntry, n_pspecs - 1);
        for (i = 1; i < n_pspecs; i++) {
            entries[i - 1].name = pspecs[i]->name;
            entries[i - 1].pspec = pspecs[i];
        }

        qsort(entries, n_pspecs - 1, sizeof(PspecEntry), compare_pspec_entry);

        oclass->pspecs = entries;
        oclass->n_pspecs = n_pspecs - 1;
    }
}

void x_object_interface_install_property(xpointer x_iface, XParamSpec *pspec)
{
    XTypeInterface *iface_class = (XTypeInterface *)x_iface;

    x_return_if_fail(X_TYPE_IS_INTERFACE(iface_class->x_type));
    x_return_if_fail(!X_IS_PARAM_SPEC_OVERRIDE(pspec));

    if (!validate_pspec_to_install(pspec)) {
        x_param_spec_ref_sink(pspec);
        x_param_spec_unref(pspec);
        return;
    }

    (void)install_property_internal(iface_class->x_type, 0, pspec);
}

static inline void param_spec_follow_override(XParamSpec **pspec)
{
    if (((XTypeInstance *)(*pspec))->x_class->x_type == X_TYPE_PARAM_OVERRIDE) {
        *pspec = ((XParamSpecOverride *)(*pspec))->overridden;
    }
}

XParamSpec *x_object_class_find_property(XObjectClass *classt, const xchar *property_name)
{
    XParamSpec *pspec;

    x_return_val_if_fail(X_IS_OBJECT_CLASS(classt), NULL);
    x_return_val_if_fail(property_name != NULL, NULL);

    pspec = find_pspec(classt, property_name);
    if (pspec) {
        param_spec_follow_override(&pspec);
    }

    return pspec;
}

XParamSpec *x_object_interface_find_property(xpointer x_iface, const xchar *property_name)
{
    XTypeInterface *iface_class = (XTypeInterface *)x_iface;

    x_return_val_if_fail(X_TYPE_IS_INTERFACE(iface_class->x_type), NULL);
    x_return_val_if_fail(property_name != NULL, NULL);

    x_object_init_pspec_pool();

    return x_param_spec_pool_lookup(pspec_pool, property_name, iface_class->x_type, FALSE);
}

void x_object_class_override_property(XObjectClass *oclass, xuint property_id, const xchar *name)
{
    XParamSpec *newt;
    XType parent_type;
    XParamSpec *overridden = NULL;

    x_return_if_fail(X_IS_OBJECT_CLASS(oclass));
    x_return_if_fail(property_id > 0);
    x_return_if_fail(name != NULL);

    parent_type = x_type_parent(X_OBJECT_CLASS_TYPE(oclass));
    if (parent_type != X_TYPE_NONE) {
        overridden = x_param_spec_pool_lookup(pspec_pool, name, parent_type, TRUE);
    }

    if (!overridden) {
        XType *ifaces;
        xuint n_ifaces;

        ifaces = x_type_interfaces(X_OBJECT_CLASS_TYPE(oclass), &n_ifaces);
        while (n_ifaces-- && !overridden) {
            overridden = x_param_spec_pool_lookup(pspec_pool, name, ifaces[n_ifaces], FALSE);
        }

        x_free(ifaces);
    }

    if (!overridden) {
        x_critical("%s: Can't find property to override for '%s::%s'", X_STRFUNC, X_OBJECT_CLASS_NAME(oclass), name);
        return;
    }

    newt = x_param_spec_override(name, overridden);
    x_object_class_install_property(oclass, property_id, newt);
}

XParamSpec **x_object_class_list_properties(XObjectClass *classt, xuint *n_properties_p)
{
    xuint n;
    XParamSpec **pspecs;

    x_return_val_if_fail(X_IS_OBJECT_CLASS(classt), NULL);

    pspecs = x_param_spec_pool_list(pspec_pool, X_OBJECT_CLASS_TYPE(classt), &n);
    if (n_properties_p) {
        *n_properties_p = n;
    }

    return pspecs;
}

XParamSpec **x_object_interface_list_properties(xpointer x_iface, xuint *n_properties_p)
{
    xuint n;
    XParamSpec **pspecs;
    XTypeInterface *iface_class = (XTypeInterface *)x_iface;

    x_return_val_if_fail(X_TYPE_IS_INTERFACE(iface_class->x_type), NULL);

    x_object_init_pspec_pool();

    pspecs = x_param_spec_pool_list(pspec_pool, iface_class->x_type, &n);
    if (n_properties_p) {
        *n_properties_p = n;
    }

    return pspecs;
}

static inline xuint object_get_optional_flags(XObject *object)
{
    return x_atomic_int_get(object_get_optional_flags_p(object));
}

static inline void object_set_optional_flags(XObject *object, xuint flags)
{
    x_atomic_int_or(object_get_optional_flags_p(object), flags);
}

static inline void object_unset_optional_flags(XObject *object, xuint flags)
{
    x_atomic_int_and(object_get_optional_flags_p(object), ~flags);
}

xboolean _x_object_has_signal_handler(XObject *object)
{
    return (object_get_optional_flags(object) & OPTIONAL_FLAG_HAS_SIGNAL_HANDLER) != 0;
}

static inline xboolean _x_object_has_notify_handler(XObject *object)
{
    return CLASS_NEEDS_NOTIFY(X_OBJECT_GET_CLASS(object)) || (object_get_optional_flags (object) & OPTIONAL_FLAG_HAS_NOTIFY_HANDLER) != 0;
}

void _x_object_set_has_signal_handler(XObject *object, xuint signal_id)
{
    xuint flags = OPTIONAL_FLAG_HAS_SIGNAL_HANDLER;
    if (signal_id == xobject_signals[NOTIFY]) {
        flags |= OPTIONAL_FLAG_HAS_NOTIFY_HANDLER;
    }

    object_set_optional_flags(object, flags);
}

static inline xboolean object_in_construction(XObject *object)
{
    return (object_get_optional_flags(object) & OPTIONAL_FLAG_IN_CONSTRUCTION) != 0;
}

static inline void set_object_in_construction(XObject *object)
{
    object_set_optional_flags(object, OPTIONAL_FLAG_IN_CONSTRUCTION);
}

static inline void unset_object_in_construction(XObject *object)
{
    object_unset_optional_flags(object, OPTIONAL_FLAG_IN_CONSTRUCTION);
}

static void x_object_init(XObject *object, XObjectClass *classt)
{
    object->ref_count = 1;
    object->qdata = NULL;

    if (CLASS_HAS_PROPS(classt) && CLASS_NEEDS_NOTIFY(classt)) {
        x_object_notify_queue_freeze(object);
    }

    set_object_in_construction(object);

    XOBJECT_IF_DEBUG(OBJECTS, {
        X_LOCK(debug_objects);
        debug_objects_count++;
        x_hash_table_add(debug_objects_ht, object);
        X_UNLOCK(debug_objects);
    });
}

static void x_object_do_set_property(XObject *object, xuint property_id, const XValue *value, XParamSpec *pspec)
{
    switch (property_id) {
        default:
            X_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void x_object_do_get_property(XObject *object, xuint property_id, XValue *value, XParamSpec *pspec)
{
    switch (property_id) {
        default:
            X_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void x_object_real_dispose(XObject *object)
{
    x_signal_handlers_destroy(object);

    x_datalist_id_set_data(&object->qdata, quark_weak_notifies, NULL);
    x_datalist_id_set_data(&object->qdata, quark_closure_array, NULL);
}

static void x_object_finalize(XObject *object)
{
    x_datalist_clear(&object->qdata);
    
    XOBJECT_IF_DEBUG(OBJECTS, {
        X_LOCK(debug_objects);
        x_assert(x_hash_table_contains(debug_objects_ht, object));
        x_hash_table_remove(debug_objects_ht, object);
        debug_objects_count--;
        X_UNLOCK(debug_objects);
    });
}

static void x_object_dispatch_properties_changed(XObject *object, xuint n_pspecs, XParamSpec **pspecs)
{
    xuint i;

    for (i = 0; i < n_pspecs; i++) {
        x_signal_emit(object, xobject_signals[NOTIFY], x_param_spec_get_name_quark(pspecs[i]), pspecs[i]);
    }
}

void x_object_run_dispose(XObject *object)
{
    x_return_if_fail(X_IS_OBJECT(object));
    x_return_if_fail(x_atomic_int_get(&object->ref_count) > 0);

    x_object_ref(object);
    TRACE(XOBJECT_OBJECT_DISPOSE(object, X_TYPE_FROM_INSTANCE(object), 0));
    X_OBJECT_GET_CLASS(object)->dispose(object);
    TRACE(XOBJECT_OBJECT_DISPOSE_END(object, X_TYPE_FROM_INSTANCE(object), 0));
    x_datalist_id_remove_data(&object->qdata, quark_weak_locations);
    x_object_unref(object);
}

void x_object_freeze_notify(XObject *object)
{
    x_return_if_fail(X_IS_OBJECT(object));

#ifndef X_DISABLE_CHECKS
    if (X_UNLIKELY(x_atomic_int_get(&object->ref_count) <= 0)) {
        x_critical("Attempting to freeze the notification queue for object %s[%p]; "
                   "Property notification does not work during instance finalization.",
                    X_OBJECT_TYPE_NAME(object), object);
        return;
    }
#endif

    x_object_notify_queue_freeze(object);
}

static inline void x_object_notify_by_spec_internal(XObject *object, XParamSpec *pspec)
{
    xboolean in_init;
    xuint object_flags;
    xboolean needs_notify;

    if (X_UNLIKELY(~pspec->flags & X_PARAM_READABLE)) {
        return;
    }

    param_spec_follow_override(&pspec);

    object_flags = object_get_optional_flags(object);
    needs_notify = ((object_flags & OPTIONAL_FLAG_HAS_NOTIFY_HANDLER) != 0) || CLASS_NEEDS_NOTIFY(X_OBJECT_GET_CLASS(object));
        in_init = (object_flags & OPTIONAL_FLAG_IN_CONSTRUCTION) != 0;

    if (pspec != NULL && needs_notify) {
        if (!x_object_notify_queue_add(object, NULL, pspec, in_init)) {
#ifndef __COVERITY__
            x_object_ref(object);
#endif
            X_OBJECT_GET_CLASS(object)->dispatch_properties_changed (object, 1, &pspec);

#ifndef __COVERITY__
            x_object_unref(object);
#endif
        }
    }
}

void x_object_notify(XObject *object, const xchar *property_name)
{
    XParamSpec *pspec;

    x_return_if_fail(X_IS_OBJECT(object));
    x_return_if_fail(property_name != NULL);

    pspec = x_param_spec_pool_lookup(pspec_pool, property_name, X_OBJECT_TYPE(object), TRUE);
    if (!pspec) {
        x_critical("%s: object class '%s' has no property named '%s'", X_STRFUNC, X_OBJECT_TYPE_NAME(object), property_name);
    } else {
        x_object_notify_by_spec_internal(object, pspec);
    }
}

void x_object_notify_by_pspec(XObject *object, XParamSpec *pspec)
{
    x_return_if_fail(X_IS_OBJECT(object));
    x_return_if_fail(X_IS_PARAM_SPEC(pspec));

    x_object_notify_by_spec_internal(object, pspec);
}

void x_object_thaw_notify(XObject *object)
{
    x_return_if_fail(X_IS_OBJECT(object));

#ifndef X_DISABLE_CHECKS
    if (X_UNLIKELY(x_atomic_int_get(&object->ref_count) <= 0)) {
        x_critical("Attempting to thaw the notification queue for object %s[%p]; "
                   "Property notification does not work during instance finalization.",
                   X_OBJECT_TYPE_NAME(object), object);
        return;
    }
#endif

    x_object_notify_queue_thaw(object, NULL, TRUE);
}

static void maybe_issue_property_deprecation_warning(const XParamSpec *pspec)
{
    xboolean already;
    static XMutex already_warned_lock;
    static const xchar *enable_diagnostic;
    static XHashTable *already_warned_table;

    if (x_once_init_enter_pointer(&enable_diagnostic)) {
        const xchar *value = x_getenv("X_ENABLE_DIAGNOSTIC");
        if (!value) {
            value = "0";
        }

        x_once_init_leave_pointer(&enable_diagnostic, value);
    }

    if (enable_diagnostic[0] == '0') {
        return;
    }

    x_mutex_lock(&already_warned_lock);

    if (already_warned_table == NULL) {
        already_warned_table = x_hash_table_new(NULL, NULL);
    }

    already = x_hash_table_contains(already_warned_table, (xpointer)pspec->name);
    if (!already) {
        x_hash_table_add(already_warned_table, (xpointer)pspec->name);
    }
    x_mutex_unlock(&already_warned_lock);

    if (!already) {
        x_warning("The property %s:%s is deprecated and shouldn't be used anymore. It will be removed in a future version.", x_type_name(pspec->owner_type), pspec->name);
    }
}

static inline void consider_issuing_property_deprecation_warning(const XParamSpec *pspec)
{
    if (X_UNLIKELY(pspec->flags & X_PARAM_DEPRECATED)) {
        maybe_issue_property_deprecation_warning(pspec);
    }
}

static inline void object_get_property(XObject *object, XParamSpec *pspec, XValue *value)
{
    XObjectClass *classt;
    XTypeInstance *inst = (XTypeInstance *)object;
    xuint param_id = PARAM_SPEC_PARAM_ID(pspec);

    if (X_LIKELY(inst->x_class->x_type == pspec->owner_type)) {
        classt = (XObjectClass *)inst->x_class;
    } else {
        classt = (XObjectClass *)x_type_class_peek(pspec->owner_type);
    }

    x_assert(classt != NULL);

    param_spec_follow_override(&pspec);
    consider_issuing_property_deprecation_warning(pspec);

    classt->get_property(object, param_id, value, pspec);
}

static inline void object_set_property(XObject *object, XParamSpec *pspec, const XValue *value, XObjectNotifyQueue *nqueue, xboolean user_specified)
{
    XObjectClass *classt;
    XParamSpecClass *pclass;
    XTypeInstance *inst = (XTypeInstance *)object;
    xuint param_id = PARAM_SPEC_PARAM_ID(pspec);

    if (X_LIKELY(inst->x_class->x_type == pspec->owner_type)) {
        classt = (XObjectClass *)inst->x_class;
    } else {
        classt = (XObjectClass *)x_type_class_peek(pspec->owner_type);
    }

    x_assert(classt != NULL);

    param_spec_follow_override(&pspec);
    if (user_specified) {
        consider_issuing_property_deprecation_warning(pspec);
    }

    pclass = X_PARAM_SPEC_GET_CLASS(pspec);
    if (x_value_type_compatible(X_VALUE_TYPE(value), pspec->value_type) && (pclass->value_validate == NULL || (pclass->value_is_valid != NULL && pclass->value_is_valid (pspec, value)))) {
        classt->set_property(object, param_id, value, pspec);
    } else {
        XValue tmp_value = X_VALUE_INIT;

        x_value_init(&tmp_value, pspec->value_type);

        if (!x_value_transform(value, &tmp_value)) {
            x_critical("unable to set property '%s' of type '%s' from value of type '%s'", pspec->name, x_type_name(pspec->value_type), X_VALUE_TYPE_NAME(value));
        } else if (x_param_value_validate(pspec, &tmp_value) && !(pspec->flags & X_PARAM_LAX_VALIDATION)) {
            xchar *contents = x_strdup_value_contents(value);

            x_critical("value \"%s\" of type '%s' is invalid or out of range for property '%s' of type '%s'", contents, X_VALUE_TYPE_NAME(value), pspec->name, x_type_name(pspec->value_type));
            x_free(contents);
        } else {
            classt->set_property (object, param_id, &tmp_value, pspec);
        }

        x_value_unset(&tmp_value);
    }

    if ((pspec->flags & (X_PARAM_EXPLICIT_NOTIFY | X_PARAM_READABLE)) == X_PARAM_READABLE && nqueue != NULL) {
        x_object_notify_queue_add(object, nqueue, pspec, FALSE);
    }
}

static void object_interface_check_properties(xpointer check_data, xpointer x_iface)
{
    xuint n;
    XParamSpec **pspecs;
    XObjectClass *classt;
    XTypeInterface *iface_class = (XTypeInterface *)x_iface;
    XType iface_type = iface_class->x_type;

    classt = (XObjectClass *)x_type_class_ref(iface_class->x_instance_type);
    if (classt == NULL) {
        return;
    }

    if (!X_IS_OBJECT_CLASS(classt)) {
        goto out;
    }

    pspecs = x_param_spec_pool_list(pspec_pool, iface_type, &n);
    while (n--) {
        XParamSpec *class_pspec = x_param_spec_pool_lookup(pspec_pool, pspecs[n]->name, X_OBJECT_CLASS_TYPE(classt), TRUE);
        if (!class_pspec) {
            x_critical("Object class %s doesn't implement property '%s' from interface '%s'", x_type_name(X_OBJECT_CLASS_TYPE(classt)), pspecs[n]->name, x_type_name(iface_type));
            continue;
        }

#define SUBSET(a, b, mask)    (((a) & ~(b) & (mask)) == 0)
        if (!SUBSET(pspecs[n]->flags, class_pspec->flags, X_PARAM_READABLE | X_PARAM_WRITABLE)) {
            x_critical("Flags for property '%s' on class '%s' remove functionality compared with the property on interface '%s'\n", pspecs[n]->name, x_type_name(X_OBJECT_CLASS_TYPE(classt)), x_type_name(iface_type));
            continue;
        }

        if (pspecs[n]->flags & X_PARAM_WRITABLE) {
            if (!SUBSET (class_pspec->flags, pspecs[n]->flags, X_PARAM_CONSTRUCT_ONLY)) {
                x_critical("Flags for property '%s' on class '%s' introduce additional restrictions on writability compared with the property on interface '%s'\n", pspecs[n]->name, x_type_name(X_OBJECT_CLASS_TYPE(classt)), x_type_name(iface_type));
                continue;
            }
        }
#undef SUBSET

        switch (pspecs[n]->flags & (X_PARAM_READABLE | X_PARAM_WRITABLE)) {
            case X_PARAM_READABLE | X_PARAM_WRITABLE:
                if (pspecs[n]->value_type != class_pspec->value_type) {
                    x_critical("Read/writable property '%s' on class '%s' has type '%s' which is not exactly equal to the "
                                "type '%s' of the property on the interface '%s'\n", pspecs[n]->name,
                                x_type_name(X_OBJECT_CLASS_TYPE(classt)), x_type_name(X_PARAM_SPEC_VALUE_TYPE(class_pspec)),
                                x_type_name(X_PARAM_SPEC_VALUE_TYPE(pspecs[n])), x_type_name(iface_type));
                }
                break;

            case X_PARAM_READABLE:
                if (!x_type_is_a(class_pspec->value_type, pspecs[n]->value_type)) {
                    x_critical("Read-only property '%s' on class '%s' has type '%s' which is not equal to or more "
                                "restrictive than the type '%s' of the property on the interface '%s'\n", pspecs[n]->name,
                                x_type_name(X_OBJECT_CLASS_TYPE(classt)), x_type_name(X_PARAM_SPEC_VALUE_TYPE(class_pspec)),
                                x_type_name(X_PARAM_SPEC_VALUE_TYPE(pspecs[n])), x_type_name(iface_type));
                }
                break;

            case X_PARAM_WRITABLE:
                if (!x_type_is_a(pspecs[n]->value_type, class_pspec->value_type)) {
                    x_critical("Write-only property '%s' on class '%s' has type '%s' which is not equal to or less "
                                "restrictive than the type '%s' of the property on the interface '%s' \n", pspecs[n]->name,
                                x_type_name(X_OBJECT_CLASS_TYPE(classt)), x_type_name(X_PARAM_SPEC_VALUE_TYPE(class_pspec)),
                                x_type_name(X_PARAM_SPEC_VALUE_TYPE(pspecs[n])), x_type_name(iface_type));
                }
                break;

            default:
                x_assert_not_reached();
        }
    }

    x_free(pspecs);

out:
    x_type_class_unref(classt);
}

XType x_object_get_type (void)
{
    return X_TYPE_OBJECT;
}

xpointer x_object_new(XType object_type, const xchar *first_property_name, ...)
{
    XObject *object;
    va_list var_args;

    if (!first_property_name) {
        return x_object_new_with_properties(object_type, 0, NULL, NULL);
    }

    va_start(var_args, first_property_name);
    object = x_object_new_valist(object_type, first_property_name, var_args);
    va_end(var_args);
    
    return object;
}

static inline xboolean x_object_is_aligned (XObject *object)
{
    typedef struct { char a; xint b; } xintAlign;
    typedef struct { char a; xlong b; } xlongAlign;
    typedef struct { char a; xdouble b; } xdoubleAlign;
    typedef struct { char a; xuint64 b; } xuint64Align;

    //return ((((xuintptr)(void *)object) % MAX(X_ALIGNOF(xdouble), MAX(X_ALIGNOF(xuint64), MAX(X_ALIGNOF(xint), X_ALIGNOF(xlong))))) == 0);
    return ((((xuintptr)(void *)object) % MAX(X_STRUCT_OFFSET(xdoubleAlign, b), MAX(X_STRUCT_OFFSET(xuint64Align, b), MAX(X_STRUCT_OFFSET(xintAlign, b), X_STRUCT_OFFSET(xlongAlign, b))))) == 0);
}

static xpointer x_object_new_with_custom_constructor(XObjectClass *classt, XObjectConstructParam *params, xuint n_params)
{
    xuint i;
    XSList *node;
    XObject *object;
    XValue *cvalues;
    xint cvals_used;
    xboolean newly_constructed;
    xboolean free_cparams = FALSE;
    XObjectConstructParam *cparams;
    XObjectNotifyQueue *nqueue = NULL;

    if (X_LIKELY(classt->n_construct_properties < 1024)) {
        cparams = x_newa0(XObjectConstructParam, classt->n_construct_properties);
        cvalues = x_newa0(XValue, classt->n_construct_properties);
    } else {
        cparams = x_new0(XObjectConstructParam, classt->n_construct_properties);
        cvalues = x_new0(XValue, classt->n_construct_properties);
        free_cparams = TRUE;
    }

    cvals_used = 0;
    i = 0;

    for (node = classt->construct_properties; node; node = node->next) {
        xuint j;
        XValue *value;
        XParamSpec *pspec;

        pspec = (XParamSpec *)node->data;
        value = NULL;

        for (j = 0; j < n_params; j++) {
            if (params[j].pspec == pspec) {
                consider_issuing_property_deprecation_warning(pspec);
                value = params[j].value;
                break;
            }
        }

        if (value == NULL) {
            value = &cvalues[cvals_used++];
            x_value_init(value, pspec->value_type);
            x_param_value_set_default(pspec, value);
        }

        cparams[i].pspec = pspec;
        cparams[i].value = value;
        i++;
    }

    object = classt->constructor(classt->x_type_class.x_type, classt->n_construct_properties, cparams);

    while (cvals_used--) {
        x_value_unset(&cvalues[cvals_used]);
    }

    if (free_cparams) {
        x_free(cparams);
        x_free(cvalues);
    }

    if (object == NULL) {
        x_critical("Custom constructor for class %s returned NULL (which is invalid). Please use GInitable instead.", X_OBJECT_CLASS_NAME(classt));
        return NULL;
    }

    if (!x_object_is_aligned(object)) {
        x_critical("Custom constructor for class %s returned a non-aligned "
                    "XObject (which is invalid since GLib 2.72). Assuming any "
                    "code using this object doesn’t require it to be aligned. "
                    "Please fix your constructor to align to the largest GLib "
                    "basic type (typically xdouble or xuint64).",
                    X_OBJECT_CLASS_NAME(classt));
    }

    newly_constructed = object_in_construction(object);
    if (newly_constructed) {
        unset_object_in_construction(object);
    }

    if (CLASS_HAS_PROPS(classt)) {
        if ((newly_constructed && _x_object_has_notify_handler(object)) || _x_object_has_notify_handler(object)) {
            nqueue = (XObjectNotifyQueue *)x_datalist_id_get_data(&object->qdata, quark_notify_queue);
            if (!nqueue) {
                nqueue = x_object_notify_queue_freeze(object);
            }
        }
    }

    if (newly_constructed && CLASS_HAS_CUSTOM_CONSTRUCTED(classt)) {
        classt->constructed(object);
    }

    for (i = 0; i < n_params; i++) {
        if (!(params[i].pspec->flags & (X_PARAM_CONSTRUCT | X_PARAM_CONSTRUCT_ONLY))) {
            object_set_property(object, params[i].pspec, params[i].value, nqueue, TRUE);
        }
    }

    if (nqueue) {
        x_object_notify_queue_thaw(object, nqueue, FALSE);
    }

    return object;
}

static xpointer x_object_new_internal(XObjectClass *classt, XObjectConstructParam *params, xuint n_params)
{
    xuint i;
    XObject *object;
    XObjectNotifyQueue *nqueue = NULL;

    if X_UNLIKELY(CLASS_HAS_CUSTOM_CONSTRUCTOR(classt)) {
        return x_object_new_with_custom_constructor(classt, params, n_params);
    }

    object = (XObject *)x_type_create_instance(classt->x_type_class.x_type);
    x_assert(x_object_is_aligned(object));

    unset_object_in_construction(object);

    if (CLASS_HAS_PROPS(classt)) {
        XSList *node;

        if (_x_object_has_notify_handler(object)) {
            nqueue = (XObjectNotifyQueue *)x_datalist_id_get_data(&object->qdata, quark_notify_queue);
            if (!nqueue) {
                nqueue = x_object_notify_queue_freeze(object);
            }
        }

        for (node = classt->construct_properties; node; node = node->next) {
            xuint j;
            XParamSpec *pspec;
            const XValue *value;
            xboolean user_specified = FALSE;

            pspec = (XParamSpec *)node->data;
            value = NULL;

            for (j = 0; j < n_params; j++) {
                if (params[j].pspec == pspec) {
                    value = params[j].value;
                    user_specified = TRUE;
                    break;
                }
            }

            if (value == NULL) {
                value = x_param_spec_get_default_value(pspec);
            }

            object_set_property(object, pspec, value, nqueue, user_specified);
        }
    }

    if (CLASS_HAS_CUSTOM_CONSTRUCTED(classt)) {
        classt->constructed(object);
    }

    for (i = 0; i < n_params; i++) {
        if (!(params[i].pspec->flags & (X_PARAM_CONSTRUCT | X_PARAM_CONSTRUCT_ONLY))) {
            object_set_property(object, params[i].pspec, params[i].value, nqueue, TRUE);
        }
    }

    if (nqueue) {
        x_object_notify_queue_thaw(object, nqueue, FALSE);
    }

    return object;
}

static inline xboolean x_object_new_is_valid_property(XType object_type, XParamSpec *pspec, const char *name, XObjectConstructParam *params, xuint n_params)
{
    xuint i;

    if (X_UNLIKELY(pspec == NULL)) {
        x_critical("%s: object class '%s' has no property named '%s'", X_STRFUNC, x_type_name(object_type), name);
        return FALSE;
    }

    if (X_UNLIKELY(~pspec->flags & X_PARAM_WRITABLE)) {
        x_critical("%s: property '%s' of object class '%s' is not writable", X_STRFUNC, pspec->name, x_type_name(object_type));
        return FALSE;
    }

    if (X_UNLIKELY(pspec->flags & (X_PARAM_CONSTRUCT | X_PARAM_CONSTRUCT_ONLY))) {
        for (i = 0; i < n_params; i++) {
            if (params[i].pspec == pspec) {
                break;
            }
        }

        if (X_UNLIKELY(i != n_params)) {
            x_critical("%s: property '%s' for type '%s' cannot be set twice", X_STRFUNC, name, x_type_name(object_type));
            return FALSE;
        }
    }

    return TRUE;
}

XObject *x_object_new_with_properties(XType object_type, xuint n_properties, const char *names[], const XValue values[])
{
    XObject *object;
    XObjectClass *classt, *unref_class = NULL;

    x_return_val_if_fail(X_TYPE_IS_OBJECT(object_type), NULL);

    classt = (XObjectClass *)x_type_class_peek_static(object_type);
    if (classt == NULL) {
        classt = unref_class = (XObjectClass *)x_type_class_ref(object_type);
    }

    if (n_properties > 0) {
        xuint i, count = 0;
        XObjectConstructParam *params;

        params = x_newa(XObjectConstructParam, n_properties);
        for (i = 0; i < n_properties; i++) {
            XParamSpec *pspec = find_pspec(classt, names[i]);
            if (!x_object_new_is_valid_property(object_type, pspec, names[i], params, count)) {
                continue;
            }

            params[count].pspec = pspec;
            params[count].value = (XValue *) &values[i];
            count++;
        }

        object = (XObject *)x_object_new_internal(classt, params, count);
    } else {
        object = (XObject *)x_object_new_internal(classt, NULL, 0);
    }

    if (unref_class != NULL) {
        x_type_class_unref(unref_class);
    }

    return object;
}

X_GNUC_BEGIN_IGNORE_DEPRECATIONS
xpointer x_object_newv(XType object_type, xuint n_parameters, XParameter *parameters)
{
    XObject *object;
    XObjectClass *classt, *unref_class = NULL;

    x_return_val_if_fail(X_TYPE_IS_OBJECT(object_type), NULL);
    x_return_val_if_fail(n_parameters == 0 || parameters != NULL, NULL);

    classt = (XObjectClass *)x_type_class_peek_static(object_type);
    if (!classt) {
        classt = unref_class = (XObjectClass *)x_type_class_ref(object_type);
    }

    if (n_parameters) {
        xuint i, j;
        XObjectConstructParam *cparams;

        cparams = x_newa(XObjectConstructParam, n_parameters);
        j = 0;

        for (i = 0; i < n_parameters; i++) {
            XParamSpec *pspec = find_pspec(classt, parameters[i].name);
            if (!x_object_new_is_valid_property(object_type, pspec, parameters[i].name, cparams, j)) {
                continue;
            }

            cparams[j].pspec = pspec;
            cparams[j].value = &parameters[i].value;
            j++;
        }

        object = (XObject *)x_object_new_internal(classt, cparams, j);
    } else {
        object = (XObject *)x_object_new_internal(classt, NULL, 0);
    }

    if (unref_class) {
        x_type_class_unref(unref_class);
    }

    return object;
}
X_GNUC_END_IGNORE_DEPRECATIONS

XObject *x_object_new_valist(XType object_type, const xchar *first_property_name, va_list var_args)
{
    XObject *object;
    XObjectClass *classt, *unref_class = NULL;

    x_return_val_if_fail(X_TYPE_IS_OBJECT(object_type), NULL);

    classt = (XObjectClass *)x_type_class_peek_static(object_type);
    if (!classt) {
        classt = unref_class = (XObjectClass *)x_type_class_ref(object_type);
    }

    if (first_property_name) {
        const xchar *name;
        xuint n_params = 0;
        XObjectConstructParam params_stack[16];
        XObjectConstructParam *params = params_stack;
        XValue values_stack[X_N_ELEMENTS(params_stack)];
        xuint n_params_alloc = X_N_ELEMENTS(params_stack);
        XTypeValueTable *vtabs_stack[X_N_ELEMENTS(params_stack)];

        XValue *values = values_stack;
        XTypeValueTable **vtabs = vtabs_stack;

        name = first_property_name;

        do {
            xchar *error = NULL;
            XParamSpec *pspec = find_pspec(classt, name);

            if (!x_object_new_is_valid_property(object_type, pspec, name, params, n_params)) {
                break;
            }

            if (X_UNLIKELY(n_params == n_params_alloc)) {
                xuint i;

                if (n_params_alloc == X_N_ELEMENTS(params_stack)) {
                    n_params_alloc = X_N_ELEMENTS(params_stack) * 2u;
                    params = x_new(XObjectConstructParam, n_params_alloc);
                    values = x_new(XValue, n_params_alloc);
                    vtabs = x_new(XTypeValueTable *, n_params_alloc);
                    memcpy(params, params_stack, sizeof(XObjectConstructParam) * n_params);
                    memcpy(values, values_stack, sizeof(XValue) * n_params);
                    memcpy(vtabs, vtabs_stack, sizeof(XTypeValueTable *) * n_params);
                } else {
                    n_params_alloc *= 2u;
                    params = (XObjectConstructParam *)x_realloc(params, sizeof(XObjectConstructParam) * n_params_alloc);
                    values = (XValue *)x_realloc(values, sizeof(XValue) * n_params_alloc);
                    vtabs = (XTypeValueTable **)x_realloc(vtabs, sizeof(XTypeValueTable *) * n_params_alloc);
                }

                for (i = 0; i < n_params; i++) {
                    params[i].value = &values[i];
                }
            }

            params[n_params].pspec = pspec;
            params[n_params].value = &values[n_params];
            memset(&values[n_params], 0, sizeof(XValue));

            X_VALUE_COLLECT_INIT2(&values[n_params], vtabs[n_params], pspec->value_type, var_args, X_VALUE_NOCOPY_CONTENTS, &error);

            if (error) {
                x_critical("%s: %s", X_STRFUNC, error);
                x_value_unset(&values[n_params]);
                x_free(error);
                break;
            }

            n_params++;
        } while ((name = va_arg(var_args, const xchar *)));

        object = (XObject *)x_object_new_internal(classt, params, n_params);
        while (n_params--) {
            if (vtabs[n_params]->value_free) {
                vtabs[n_params]->value_free(params[n_params].value);
            }
        }

        if (X_UNLIKELY(n_params_alloc != X_N_ELEMENTS(params_stack))) {
            x_free(params);
            x_free(values);
            x_free(vtabs);
        }
    } else {
        object = (XObject *)x_object_new_internal(classt, NULL, 0);
    }

    if (unref_class) {
        x_type_class_unref(unref_class);
    }

    return object;
}

static XObject *x_object_constructor(XType type, xuint n_construct_properties, XObjectConstructParam *construct_params)
{
    XObject *object;

    object = (XObject *)x_type_create_instance(type);

    if (n_construct_properties) {
        XObjectNotifyQueue *nqueue = x_object_notify_queue_freeze(object);

        while (n_construct_properties--) {
            XValue *value = construct_params->value;
            XParamSpec *pspec = construct_params->pspec;

            construct_params++;
            object_set_property(object, pspec, value, nqueue, TRUE);
        }

        x_object_notify_queue_thaw(object, nqueue, FALSE);
    }

    return object;
}

static void x_object_constructed(XObject *object)
{

}

static inline xboolean x_object_set_is_valid_property(XObject *object, XParamSpec *pspec, const char *property_name)
{
    if (X_UNLIKELY(pspec == NULL)) {
        x_critical("%s: object class '%s' has no property named '%s'", X_STRFUNC, X_OBJECT_TYPE_NAME(object), property_name);
        return FALSE;
    }

    if (X_UNLIKELY(!(pspec->flags & X_PARAM_WRITABLE))) {
        x_critical("%s: property '%s' of object class '%s' is not writable", X_STRFUNC, pspec->name, X_OBJECT_TYPE_NAME(object));
        return FALSE;
    }

    if (X_UNLIKELY(((pspec->flags & X_PARAM_CONSTRUCT_ONLY) && !object_in_construction(object)))) {
        x_critical("%s: construct property \"%s\" for object '%s' can't be set after construction", X_STRFUNC, pspec->name, X_OBJECT_TYPE_NAME(object));
        return FALSE;
    }

    return TRUE;
}

void x_object_setv(XObject *object, xuint n_properties, const xchar *names[], const XValue values[])
{
    xuint i;
    XParamSpec *pspec;
    XObjectClass *classt;
    XObjectNotifyQueue *nqueue = NULL;

    x_return_if_fail(X_IS_OBJECT(object));

    if (n_properties == 0) {
        return;
    }

    x_object_ref(object);

    classt = X_OBJECT_GET_CLASS(object);

    if (_x_object_has_notify_handler(object)) {
        nqueue = x_object_notify_queue_freeze(object);
    }

    for (i = 0; i < n_properties; i++) {
        pspec = find_pspec(classt, names[i]);
        if (!x_object_set_is_valid_property(object, pspec, names[i])) {
            break;
        }

        object_set_property(object, pspec, &values[i], nqueue, TRUE);
    }

    if (nqueue) {
        x_object_notify_queue_thaw(object, nqueue, FALSE);
    }

    x_object_unref(object);
}

void x_object_set_valist(XObject *object, const xchar *first_property_name, va_list var_args)
{
    const xchar *name;
    XObjectClass *classt;
    XObjectNotifyQueue *nqueue = NULL;

    x_return_if_fail(X_IS_OBJECT(object));
    x_object_ref(object);

    if (_x_object_has_notify_handler(object)) {
        nqueue = x_object_notify_queue_freeze(object);
    }

    classt = X_OBJECT_GET_CLASS(object);

    name = first_property_name;
    while (name) {
        XParamSpec *pspec;
        xchar *error = NULL;
        XTypeValueTable *vtab;
        XValue value = X_VALUE_INIT;

        pspec = find_pspec(classt, name);

        if (!x_object_set_is_valid_property(object, pspec, name)) {
            break;
        }

        X_VALUE_COLLECT_INIT2(&value, vtab, pspec->value_type, var_args, X_VALUE_NOCOPY_CONTENTS, &error);
        if (error) {
            x_critical("%s: %s", X_STRFUNC, error);
            x_free(error);
            x_value_unset(&value);
            break;
        }

        object_set_property(object, pspec, &value, nqueue, TRUE);

        if (vtab->value_free) {
            vtab->value_free(&value);
        }

        name = va_arg(var_args, xchar*);
    }

    if (nqueue) {
        x_object_notify_queue_thaw(object, nqueue, FALSE);
    }

    x_object_unref(object);
}

static inline xboolean x_object_get_is_valid_property(XObject *object, XParamSpec *pspec, const char *property_name)
{
    if (X_UNLIKELY(pspec == NULL)) {
        x_critical("%s: object class '%s' has no property named '%s'", X_STRFUNC, X_OBJECT_TYPE_NAME(object), property_name);
        return FALSE;
    }

    if (X_UNLIKELY(!(pspec->flags & X_PARAM_READABLE))) {
        x_critical("%s: property '%s' of object class '%s' is not readable", X_STRFUNC, pspec->name, X_OBJECT_TYPE_NAME(object));
        return FALSE;
    }

    return TRUE;
}

void x_object_getv(XObject *object, xuint n_properties, const xchar *names[], XValue values[])
{
    xuint i;
    XParamSpec *pspec;
    XObjectClass *classt;

    x_return_if_fail(X_IS_OBJECT(object));

    if (n_properties == 0) {
        return;
    }

    x_object_ref(object);
    classt = X_OBJECT_GET_CLASS(object);

    memset(values, 0, n_properties * sizeof(XValue));

    for (i = 0; i < n_properties; i++) {
        pspec = find_pspec(classt, names[i]);
        if (!x_object_get_is_valid_property(object, pspec, names[i])) {
            break;
        }

        x_value_init(&values[i], pspec->value_type);
        object_get_property(object, pspec, &values[i]);
    }

    x_object_unref(object);
}

void x_object_get_valist(XObject *object, const xchar *first_property_name, va_list var_args)
{
    const xchar *name;
    XObjectClass *classt;

    x_return_if_fail(X_IS_OBJECT(object));
    x_object_ref(object);

    classt = X_OBJECT_GET_CLASS(object);
    name = first_property_name;

    while (name) {
        xchar *error;
        XParamSpec *pspec;
        XValue value = X_VALUE_INIT;

        pspec = find_pspec(classt, name);

        if (!x_object_get_is_valid_property(object, pspec, name)) {
            break;
        }

        x_value_init(&value, pspec->value_type);
        object_get_property (object, pspec, &value);

        X_VALUE_LCOPY(&value, var_args, 0, &error);
        if (error) {
            x_critical("%s: %s", X_STRFUNC, error);
            x_free(error);
            x_value_unset(&value);
            break;
        }

        x_value_unset(&value);
        name = va_arg(var_args, xchar*);
    }

    x_object_unref(object);
}

void x_object_set (xpointer _object, const xchar *first_property_name, ...)
{
    va_list var_args;
    XObject *object = (XObject *)_object;

    x_return_if_fail(X_IS_OBJECT(object));
    
    va_start(var_args, first_property_name);
    x_object_set_valist(object, first_property_name, var_args);
    va_end(var_args);
}

void x_object_get(xpointer _object, const xchar *first_property_name, ...)
{
    va_list var_args;
    XObject *object = (XObject *)_object;

    x_return_if_fail(X_IS_OBJECT(object));

    va_start(var_args, first_property_name);
    x_object_get_valist(object, first_property_name, var_args);
    va_end(var_args);
}

void x_object_set_property(XObject *object, const xchar *property_name, const XValue *value)
{
    x_object_setv(object, 1, &property_name, value);
}

void x_object_get_property(XObject *object, const xchar *property_name, XValue *value)
{
    XParamSpec *pspec;

    x_return_if_fail(X_IS_OBJECT(object));
    x_return_if_fail(property_name != NULL);
    x_return_if_fail(value != NULL);

    x_object_ref(object);
    pspec = find_pspec(X_OBJECT_GET_CLASS(object), property_name);

    if (x_object_get_is_valid_property(object, pspec, property_name)) {
        XValue *prop_value, tmp_value = X_VALUE_INIT;

        if (X_VALUE_TYPE(value) == X_TYPE_INVALID) {
            x_value_init(value, pspec->value_type);
            prop_value = value;
        } else if (X_VALUE_TYPE(value) == pspec->value_type) {
            x_value_reset(value);
            prop_value = value;
        } else if (!x_value_type_transformable(pspec->value_type, X_VALUE_TYPE(value))) {
            x_critical("%s: can't retrieve property '%s' of type '%s' as value of type '%s'", X_STRFUNC, pspec->name, x_type_name(pspec->value_type), X_VALUE_TYPE_NAME(value));
            x_object_unref(object);
            return;
        } else {
            x_value_init(&tmp_value, pspec->value_type);
            prop_value = &tmp_value;
        }

        object_get_property(object, pspec, prop_value);
        if (prop_value != value) {
            x_value_transform(prop_value, value);
            x_value_unset(&tmp_value);
        }
    }

    x_object_unref(object);
}

xpointer x_object_connect(xpointer _object, const xchar *signal_spec, ...)
{
    va_list var_args;
    XObject *object = (XObject *)_object;

    x_return_val_if_fail(X_IS_OBJECT(object), NULL);
    x_return_val_if_fail(object->ref_count > 0, object);

    va_start(var_args, signal_spec);
    while (signal_spec) {
        XCallback callback = va_arg(var_args, XCallback);
        xpointer data = va_arg(var_args, xpointer);

        if (strncmp(signal_spec, "signal::", 8) == 0) {
            x_signal_connect_data(object, signal_spec + 8, callback, data, NULL, X_CONNECT_DEFAULT);
        } else if (strncmp(signal_spec, "object_signal::", 15) == 0 || strncmp(signal_spec, "object-signal::", 15) == 0) {
            x_signal_connect_object (object, signal_spec + 15, callback, data, X_CONNECT_DEFAULT);
        } else if (strncmp(signal_spec, "swapped_signal::", 16) == 0 || strncmp(signal_spec, "swapped-signal::", 16) == 0) {
            x_signal_connect_data(object, signal_spec + 16, callback, data, NULL, X_CONNECT_SWAPPED);
        } else if (strncmp(signal_spec, "swapped_object_signal::", 23) == 0 || strncmp(signal_spec, "swapped-object-signal::", 23) == 0) {
            x_signal_connect_object (object, signal_spec + 23, callback, data, X_CONNECT_SWAPPED);
        } else if (strncmp(signal_spec, "signal_after::", 14) == 0 || strncmp(signal_spec, "signal-after::", 14) == 0) {
            x_signal_connect_data(object, signal_spec + 14, callback, data, NULL, X_CONNECT_AFTER);
        } else if (strncmp(signal_spec, "object_signal_after::", 21) == 0 || strncmp(signal_spec, "object-signal-after::", 21) == 0) {
            x_signal_connect_object (object, signal_spec + 21, callback, data, X_CONNECT_AFTER);
        } else if (strncmp(signal_spec, "swapped_signal_after::", 22) == 0 || strncmp(signal_spec, "swapped-signal-after::", 22) == 0) {
            x_signal_connect_data(object, signal_spec + 22, callback, data, NULL, (XConnectFlags)(X_CONNECT_SWAPPED | X_CONNECT_AFTER));
        } else if (strncmp(signal_spec, "swapped_object_signal_after::", 29) == 0 || strncmp(signal_spec, "swapped-object-signal-after::", 29) == 0) {
            x_signal_connect_object(object, signal_spec + 29, callback, data, (XConnectFlags)(X_CONNECT_SWAPPED | X_CONNECT_AFTER));
        } else {
            x_critical("%s: invalid signal spec \"%s\"", X_STRFUNC, signal_spec);
            break;
        }

        signal_spec = va_arg(var_args, xchar*);
    }
    va_end(var_args);

    return object;
}

void x_object_disconnect(xpointer _object, const xchar *signal_spec, ...)
{
    va_list var_args;
    XObject *object = (XObject *)_object;

    x_return_if_fail(X_IS_OBJECT(object));
    x_return_if_fail(object->ref_count > 0);

    va_start(var_args, signal_spec);
    while (signal_spec) {
        XCallback callback = va_arg(var_args, XCallback);
        xpointer data = va_arg(var_args, xpointer);
        xuint sid = 0, detail = 0, mask = 0;

        if (strncmp(signal_spec, "any_signal::", 12) == 0 || strncmp(signal_spec, "any-signal::", 12) == 0) {
            signal_spec += 12;
            mask = X_SIGNAL_MATCH_ID | X_SIGNAL_MATCH_FUNC | X_SIGNAL_MATCH_DATA;
        } else if (strcmp (signal_spec, "any_signal") == 0 || strcmp (signal_spec, "any-signal") == 0) {
            signal_spec += 10;
            mask = X_SIGNAL_MATCH_FUNC | X_SIGNAL_MATCH_DATA;
        } else {
            x_critical("%s: invalid signal spec \"%s\"", X_STRFUNC, signal_spec);
            break;
        }

        if ((mask & X_SIGNAL_MATCH_ID) && !x_signal_parse_name(signal_spec, X_OBJECT_TYPE(object), &sid, &detail, FALSE)) {
            x_critical("%s: invalid signal name \"%s\"", X_STRFUNC, signal_spec);
        } else if (!x_signal_handlers_disconnect_matched(object, (XSignalMatchType)(mask | (detail ? X_SIGNAL_MATCH_DETAIL : 0)), sid, detail, NULL, (xpointer)callback, data)) {
            x_critical("%s: signal handler %p(%p) is not connected", X_STRFUNC, callback, data);
        }

        signal_spec = va_arg(var_args, xchar*);
    }
    va_end(var_args);
}

typedef struct {
    XObject         *object;
    xuint           n_weak_refs;
    struct {
        XWeakNotify notify;
        xpointer    data;
    } weak_refs[1];
} WeakRefStack;

static void weak_refs_notify(xpointer data)
{
    xuint i;
    WeakRefStack *wstack = (WeakRefStack *)data;

    for (i = 0; i < wstack->n_weak_refs; i++) {
        wstack->weak_refs[i].notify(wstack->weak_refs[i].data, wstack->object);
    }

    x_free(wstack);
}

void x_object_weak_ref(XObject *object, XWeakNotify notify, xpointer data)
{
    xuint i;
    WeakRefStack *wstack;

    x_return_if_fail(X_IS_OBJECT(object));
    x_return_if_fail(notify != NULL);
    x_return_if_fail(x_atomic_int_get(&object->ref_count) >= 1);

    object_bit_lock(object, OPTIONAL_BIT_LOCK_WEAK_REFS);
    wstack = (WeakRefStack *)x_datalist_id_remove_no_notify(&object->qdata, quark_weak_notifies);
    if (wstack) {
        i = wstack->n_weak_refs++;
        wstack = (WeakRefStack *)x_realloc(wstack, sizeof(*wstack) + sizeof(wstack->weak_refs[0]) * i);
    } else {
        wstack = x_renew(WeakRefStack, NULL, 1);
        wstack->object = object;
        wstack->n_weak_refs = 1;
        i = 0;
    }

    wstack->weak_refs[i].notify = notify;
    wstack->weak_refs[i].data = data;
    x_datalist_id_set_data_full(&object->qdata, quark_weak_notifies, wstack, weak_refs_notify);
    object_bit_unlock(object, OPTIONAL_BIT_LOCK_WEAK_REFS);
}

void x_object_weak_unref(XObject *object, XWeakNotify notify, xpointer data)
{
    WeakRefStack *wstack;
    xboolean found_one = FALSE;

    x_return_if_fail(X_IS_OBJECT(object));
    x_return_if_fail(notify != NULL);

    object_bit_lock(object, OPTIONAL_BIT_LOCK_WEAK_REFS);
    wstack = (WeakRefStack *)x_datalist_id_get_data(&object->qdata, quark_weak_notifies);
    if (wstack) {
        xuint i;

        for (i = 0; i < wstack->n_weak_refs; i++) {
            if (wstack->weak_refs[i].notify == notify && wstack->weak_refs[i].data == data) {
                found_one = TRUE;
                wstack->n_weak_refs -= 1;
                if (i != wstack->n_weak_refs) {
                    wstack->weak_refs[i] = wstack->weak_refs[wstack->n_weak_refs];
                }

                break;
            }
        }
    }
    object_bit_unlock(object, OPTIONAL_BIT_LOCK_WEAK_REFS);

    if (!found_one) {
        x_critical("%s: couldn't find weak ref %p(%p)", X_STRFUNC, notify, data);
    }
}

void x_object_add_weak_pointer(XObject *object,  xpointer *weak_pointer_location)
{
    x_return_if_fail(X_IS_OBJECT(object));
    x_return_if_fail(weak_pointer_location != NULL);

    x_object_weak_ref(object, (XWeakNotify)x_nullify_pointer, weak_pointer_location);
}

void x_object_remove_weak_pointer(XObject *object,  xpointer *weak_pointer_location)
{
    x_return_if_fail(X_IS_OBJECT(object));
    x_return_if_fail(weak_pointer_location != NULL);

    x_object_weak_unref(object, (XWeakNotify)x_nullify_pointer, weak_pointer_location);
}

static xuint object_floating_flag_handler(XObject *object, xint job)
{
    switch (job) {
        xpointer oldvalue;
        case +1:
            oldvalue = x_atomic_pointer_get(&object->qdata);
            while (!x_atomic_pointer_compare_and_exchange_full((void **)&object->qdata, oldvalue, (void *)((xuintptr)oldvalue | OBJECT_FLOATING_FLAG), &oldvalue));
            return (xsize)oldvalue & OBJECT_FLOATING_FLAG;

        case -1:
            oldvalue = x_atomic_pointer_get(&object->qdata);
            while (!x_atomic_pointer_compare_and_exchange_full((void **)&object->qdata, oldvalue, (void *)((xuintptr)oldvalue & ~(xsize)OBJECT_FLOATING_FLAG), &oldvalue));
            return (xsize) oldvalue & OBJECT_FLOATING_FLAG;

        default:
            return 0 != ((xsize)x_atomic_pointer_get(&object->qdata) & OBJECT_FLOATING_FLAG);
    }
}

xboolean x_object_is_floating(xpointer _object)
{
    XObject *object = (XObject *)_object;
    x_return_val_if_fail(X_IS_OBJECT(object), FALSE);

    return floating_flag_handler(object, 0);
}

xpointer (x_object_ref_sink)(xpointer _object)
{
    xboolean was_floating;
    XObject *object = (XObject *)_object;

    x_return_val_if_fail(X_IS_OBJECT(object), object);
    x_return_val_if_fail(x_atomic_int_get(&object->ref_count) >= 1, object);

    x_object_ref(object);
    was_floating = floating_flag_handler(object, -1);
    if (was_floating) {
        x_object_unref(object);
    }
    
    return object;
}

xpointer x_object_take_ref(xpointer _object)
{
    XObject *object = (XObject *)_object;

    x_return_val_if_fail(X_IS_OBJECT(object), object);
    x_return_val_if_fail(x_atomic_int_get(&object->ref_count) >= 1, object);

    floating_flag_handler(object, -1);
    return object;
}

void x_object_force_floating(XObject *object)
{
    x_return_if_fail(X_IS_OBJECT(object));
    x_return_if_fail(x_atomic_int_get(&object->ref_count) >= 1);

    floating_flag_handler(object, +1);
}

typedef struct {
    xuint             n_toggle_refs;
    struct {
        XToggleNotify notify;
        xpointer      data;
    } toggle_refs[1];
} ToggleRefStack;

static XToggleNotify toggle_refs_get_notify_unlocked(XObject *object, xpointer *out_data)
{
    ToggleRefStack *tstackptr;

    if (!OBJECT_HAS_TOGGLE_REF(object)) {
        return NULL;
    }

    tstackptr = (ToggleRefStack *)x_datalist_id_get_data(&object->qdata, quark_toggle_refs);

    if (tstackptr->n_toggle_refs != 1) {
        x_critical("Unexpected number of toggle-refs. x_object_add_toggle_ref() must be paired with x_object_remove_toggle_ref()");
        return NULL;
    }

    *out_data = tstackptr->toggle_refs[0].data;
    return tstackptr->toggle_refs[0].notify;
}

void x_object_add_toggle_ref(XObject *object, XToggleNotify notify, xpointer data)
{
    xuint i;
    ToggleRefStack *tstack;

    x_return_if_fail(X_IS_OBJECT(object));
    x_return_if_fail(notify != NULL);
    x_return_if_fail(x_atomic_int_get(&object->ref_count) >= 1);

    x_object_ref(object);

    object_bit_lock(object, OPTIONAL_BIT_LOCK_TOGGLE_REFS);
    tstack = (ToggleRefStack *)x_datalist_id_remove_no_notify(&object->qdata, quark_toggle_refs);
    if (tstack) {
        i = tstack->n_toggle_refs++;
        tstack = (ToggleRefStack *)x_realloc(tstack, sizeof(*tstack) + sizeof(tstack->toggle_refs[0]) * i);
    } else {
        tstack = x_renew(ToggleRefStack, NULL, 1);
        tstack->n_toggle_refs = 1;
        i = 0;
    }

    if (tstack->n_toggle_refs == 1) {
        x_datalist_set_flags(&object->qdata, OBJECT_HAS_TOGGLE_REF_FLAG);
    }

    tstack->toggle_refs[i].notify = notify;
    tstack->toggle_refs[i].data = data;
    x_datalist_id_set_data_full(&object->qdata, quark_toggle_refs, tstack, (XDestroyNotify)x_free);
    object_bit_unlock(object, OPTIONAL_BIT_LOCK_TOGGLE_REFS);
}

void x_object_remove_toggle_ref(XObject *object, XToggleNotify notify, xpointer data)
{
    ToggleRefStack *tstack;
    xboolean found_one = FALSE;

    x_return_if_fail(X_IS_OBJECT(object));
    x_return_if_fail(notify != NULL);

    object_bit_lock(object, OPTIONAL_BIT_LOCK_TOGGLE_REFS);
    tstack = (ToggleRefStack *)x_datalist_id_get_data(&object->qdata, quark_toggle_refs);
    if (tstack) {
        xuint i;

        for (i = 0; i < tstack->n_toggle_refs; i++) {
            if (tstack->toggle_refs[i].notify == notify && (tstack->toggle_refs[i].data == data || data == NULL)) {
                found_one = TRUE;
                tstack->n_toggle_refs -= 1;
                if (i != tstack->n_toggle_refs) {
                    tstack->toggle_refs[i] = tstack->toggle_refs[tstack->n_toggle_refs];
                }

                if (tstack->n_toggle_refs == 0) {
                    x_datalist_unset_flags(&object->qdata, OBJECT_HAS_TOGGLE_REF_FLAG);
                    x_datalist_id_set_data_full(&object->qdata, quark_toggle_refs, NULL, NULL);
                }

                break;
            }
        }
    }
    object_bit_unlock(object, OPTIONAL_BIT_LOCK_TOGGLE_REFS);

    if (found_one) {
        x_object_unref(object);
    } else {
        x_critical("%s: couldn't find toggle ref %p(%p)", X_STRFUNC, notify, data);
    }
}

static xpointer object_ref(XObject *object, XToggleNotify *out_toggle_notify, xpointer *out_toggle_data)
{
    xint old_ref;
    xpointer toggle_data;
    XToggleNotify toggle_notify;

    old_ref = x_atomic_int_get(&object->ref_count);

retry:
    toggle_notify = NULL;
    toggle_data = NULL;

    if (old_ref > 1 && old_ref < X_MAXINT) {
        if (!x_atomic_int_compare_and_exchange_full((int *)&object->ref_count, old_ref, old_ref + 1, &old_ref)) {
            goto retry;
        }
    } else if (old_ref == 1) {
        xboolean do_retry;

        object_bit_lock(object, OPTIONAL_BIT_LOCK_TOGGLE_REFS);
        toggle_notify = toggle_refs_get_notify_unlocked(object, &toggle_data);
        do_retry = !x_atomic_int_compare_and_exchange_full((int *)&object->ref_count, old_ref, old_ref + 1, &old_ref);
        object_bit_unlock(object, OPTIONAL_BIT_LOCK_TOGGLE_REFS);

        if (do_retry) {
            goto retry;
        }
    } else {
        xboolean object_already_finalized = TRUE;

        *out_toggle_notify = NULL;
        *out_toggle_data = NULL;
        x_return_val_if_fail(!object_already_finalized, NULL);
        return NULL;
    }

    TRACE(XOBJECT_OBJECT_REF(object, X_TYPE_FROM_INSTANCE(object), old_ref));

    *out_toggle_notify = toggle_notify;
    *out_toggle_data = toggle_data;
    return object;
}

xpointer (x_object_ref)(xpointer _object)
{
    xpointer toggle_data;
    XObject *object = _object;
    XToggleNotify toggle_notify;

    x_return_val_if_fail(X_IS_OBJECT(object), NULL);

    object = object_ref(object, &toggle_notify, &toggle_data);

    if (toggle_notify) {
        toggle_notify(toggle_data, object, FALSE);
    }

    return object;
}

static xboolean _object_unref_clear_weak_locations(XObject *object, xint *p_old_ref, xboolean do_unref)
{
    XSList **weak_locations;

    if (do_unref) {
        xboolean unreffed = FALSE;

        x_rw_lock_reader_lock(&weak_locations_lock);
        weak_locations = x_datalist_id_get_data(&object->qdata, quark_weak_locations);
        if (!weak_locations) {
            unreffed = x_atomic_int_compare_and_exchange_full((int *)&object->ref_count, 1, 0, p_old_ref);
            x_rw_lock_reader_unlock(&weak_locations_lock);
            return unreffed;
        }
        x_rw_lock_reader_unlock(&weak_locations_lock);

        x_rw_lock_writer_lock(&weak_locations_lock);
        if (!x_atomic_int_compare_and_exchange_full((int *)&object->ref_count, 1, 0, p_old_ref)) {
            x_rw_lock_writer_unlock(&weak_locations_lock);
            return FALSE;
        }

        weak_locations = x_datalist_id_remove_no_notify(&object->qdata, quark_weak_locations);
        x_clear_pointer(&weak_locations, weak_locations_free_unlocked);

        x_rw_lock_writer_unlock(&weak_locations_lock);
        return TRUE;
    }

    weak_locations = x_datalist_id_get_data(&object->qdata, quark_weak_locations);
    if (weak_locations != NULL) {
        x_rw_lock_writer_lock(&weak_locations_lock);

        *p_old_ref = x_atomic_int_get(&object->ref_count);
        if (*p_old_ref != 1) {
            x_rw_lock_writer_unlock(&weak_locations_lock);
            return FALSE;
        }

        weak_locations = x_datalist_id_remove_no_notify(&object->qdata, quark_weak_locations);
        x_clear_pointer(&weak_locations, weak_locations_free_unlocked);

        x_rw_lock_writer_unlock(&weak_locations_lock);
        return TRUE;
    }

    return TRUE;
}

void x_object_unref(xpointer _object)
{
    xint old_ref;
    XType obj_gtype;
    xboolean do_retry;
    xpointer toggle_data;
    XObjectNotifyQueue *nqueue;
    XToggleNotify toggle_notify;
    XObject *object = (XObject *)_object;

    x_return_if_fail(X_IS_OBJECT(object));

    obj_gtype = X_TYPE_FROM_INSTANCE(object);
    (void)obj_gtype;

    old_ref = x_atomic_int_get(&object->ref_count);

retry_beginning:
    if (old_ref > 2) {
        if (!x_atomic_int_compare_and_exchange_full((int *)&object->ref_count, old_ref, old_ref - 1, &old_ref)) {
            goto retry_beginning;
        }

        TRACE(XOBJECT_OBJECT_UNREF(object, obj_gtype, old_ref));
        return;
    }

    if (old_ref == 2) {
        object_bit_lock(object, OPTIONAL_BIT_LOCK_TOGGLE_REFS);
        toggle_notify = toggle_refs_get_notify_unlocked(object, &toggle_data);
        if (!x_atomic_int_compare_and_exchange_full((int *)&object->ref_count, old_ref, old_ref - 1, &old_ref)) {
            object_bit_unlock(object, OPTIONAL_BIT_LOCK_TOGGLE_REFS);
            goto retry_beginning;
        }
        object_bit_unlock(object, OPTIONAL_BIT_LOCK_TOGGLE_REFS);

        TRACE(XOBJECT_OBJECT_UNREF(object, obj_gtype, old_ref));
        if (toggle_notify) {
            toggle_notify(toggle_data, object, TRUE);
        }
        return;
    }

    if (X_UNLIKELY(old_ref != 1)) {
        xboolean object_already_finalized = TRUE;
        x_return_if_fail(!object_already_finalized);
        return;
    }

    if (!_object_unref_clear_weak_locations(object, &old_ref, FALSE)) {
        goto retry_beginning;
    }

    nqueue = x_object_notify_queue_freeze(object);

    TRACE(XOBJECT_OBJECT_DISPOSE(object, X_TYPE_FROM_INSTANCE(object), 1));
    X_OBJECT_GET_CLASS(object)->dispose(object);
    TRACE(XOBJECT_OBJECT_DISPOSE_END(object, X_TYPE_FROM_INSTANCE(object), 1));

retry_decrement:
    if (old_ref > 1 && nqueue) {
        x_object_notify_queue_thaw(object, nqueue, FALSE);
        nqueue = NULL;
    }

    if (old_ref > 2) {
        if (!x_atomic_int_compare_and_exchange_full((int *)&object->ref_count, old_ref, old_ref - 1, &old_ref)) {
            goto retry_decrement;
        }

        TRACE(XOBJECT_OBJECT_UNREF(object, obj_gtype, old_ref));
        return;
    }

    if (old_ref == 2) {
        object_bit_lock(object, OPTIONAL_BIT_LOCK_TOGGLE_REFS);
        toggle_notify = toggle_refs_get_notify_unlocked(object, &toggle_data);
        do_retry = !x_atomic_int_compare_and_exchange_full((int *)&object->ref_count, old_ref, old_ref - 1, &old_ref);
        object_bit_unlock(object, OPTIONAL_BIT_LOCK_TOGGLE_REFS);

        if (do_retry) {
            goto retry_decrement;
        }

        TRACE(XOBJECT_OBJECT_UNREF(object, obj_gtype, old_ref));
        if (toggle_notify) {
            toggle_notify(toggle_data, object, TRUE);
        }
        return;
    }

    if (!_object_unref_clear_weak_locations(object, &old_ref, TRUE)) {
        goto retry_decrement;
    }

    TRACE(XOBJECT_OBJECT_UNREF(object, obj_gtype, old_ref));

    x_datalist_id_set_data(&object->qdata, quark_closure_array, NULL);
    x_signal_handlers_destroy(object);
    x_datalist_id_set_data(&object->qdata, quark_weak_notifies, NULL);

    TRACE(XOBJECT_OBJECT_FINALIZE(object, X_TYPE_FROM_INSTANCE(object)));
    X_OBJECT_GET_CLASS(object)->finalize(object);
    TRACE(XOBJECT_OBJECT_FINALIZE_END(object, X_TYPE_FROM_INSTANCE(object)));

    XOBJECT_IF_DEBUG(OBJECTS, {
        xboolean was_present;
        X_LOCK(debug_objects);
        was_present = x_hash_table_remove(debug_objects_ht, object);
        X_UNLOCK(debug_objects);

        if (was_present) {
            x_critical("Object %p of type %s not finalized correctly.", object, X_OBJECT_TYPE_NAME(object));
        }
    });

    x_type_free_instance((XTypeInstance *)object);
}

#undef x_clear_object
void x_clear_object(XObject **object_ptr)
{
    x_clear_pointer(object_ptr, x_object_unref);
}

xpointer x_object_get_qdata(XObject *object, XQuark quark)
{
    x_return_val_if_fail(X_IS_OBJECT(object), NULL);
    return quark ? x_datalist_id_get_data(&object->qdata, quark) : NULL;
}

void x_object_set_qdata(XObject *object, XQuark quark, xpointer data)
{
    x_return_if_fail(X_IS_OBJECT(object));
    x_return_if_fail(quark > 0);

    x_datalist_id_set_data(&object->qdata, quark, data);
}

xpointer x_object_dup_qdata(XObject *object, XQuark quark, XDuplicateFunc dup_func, xpointer user_data)
{
    x_return_val_if_fail(X_IS_OBJECT(object), NULL);
    x_return_val_if_fail(quark > 0, NULL);

    return x_datalist_id_dup_data(&object->qdata, quark, dup_func, user_data);
}

xboolean x_object_replace_qdata(XObject *object, XQuark quark, xpointer oldval, xpointer newval, XDestroyNotify  destroy, XDestroyNotify *old_destroy)
{
    x_return_val_if_fail(X_IS_OBJECT(object), FALSE);
    x_return_val_if_fail(quark > 0, FALSE);

    return x_datalist_id_replace_data(&object->qdata, quark, oldval, newval, destroy, old_destroy);
}

void x_object_set_qdata_full(XObject *object, XQuark quark, xpointer data, XDestroyNotify destroy)
{
    x_return_if_fail(X_IS_OBJECT(object));
    x_return_if_fail(quark > 0);

    x_datalist_id_set_data_full(&object->qdata, quark, data, data ? destroy : (XDestroyNotify) NULL);
}

xpointer x_object_steal_qdata(XObject *object, XQuark quark)
{
    x_return_val_if_fail(X_IS_OBJECT(object), NULL);
    x_return_val_if_fail(quark > 0, NULL);

    return x_datalist_id_remove_no_notify(&object->qdata, quark);
}

xpointer x_object_get_data(XObject *object, const xchar *key)
{
    x_return_val_if_fail(X_IS_OBJECT(object), NULL);
    x_return_val_if_fail(key != NULL, NULL);

    return x_datalist_get_data (&object->qdata, key);
}

void x_object_set_data(XObject *object, const xchar *key, xpointer data)
{
    x_return_if_fail(X_IS_OBJECT(object));
    x_return_if_fail(key != NULL);

    x_datalist_id_set_data(&object->qdata, x_quark_from_string(key), data);
}

xpointer x_object_dup_data(XObject *object, const xchar *key, XDuplicateFunc dup_func, xpointer user_data)
{
    x_return_val_if_fail(X_IS_OBJECT(object), NULL);
    x_return_val_if_fail(key != NULL, NULL);

    return x_datalist_id_dup_data(&object->qdata, x_quark_from_string(key), dup_func, user_data);
}

xboolean x_object_replace_data(XObject *object, const xchar *key, xpointer oldval, xpointer newval, XDestroyNotify destroy, XDestroyNotify *old_destroy)
{
    x_return_val_if_fail(X_IS_OBJECT(object), FALSE);
    x_return_val_if_fail(key != NULL, FALSE);

    return x_datalist_id_replace_data(&object->qdata, x_quark_from_string(key), oldval, newval, destroy, old_destroy);
}

void x_object_set_data_full(XObject *object, const xchar *key, xpointer data, XDestroyNotify destroy)
{
    x_return_if_fail(X_IS_OBJECT(object));
    x_return_if_fail(key != NULL);

    x_datalist_id_set_data_full(&object->qdata, x_quark_from_string(key), data, data ? destroy : (XDestroyNotify) NULL);
}

xpointer x_object_steal_data(XObject *object, const xchar *key)
{
    XQuark quark;

    x_return_val_if_fail(X_IS_OBJECT(object), NULL);
    x_return_val_if_fail(key != NULL, NULL);

    quark = x_quark_try_string(key);
    return quark ? x_datalist_id_remove_no_notify(&object->qdata, quark) : NULL;
}

static void x_value_object_init(XValue *value)
{
    value->data[0].v_pointer = NULL;
}

static void x_value_object_free_value(XValue *value)
{
    x_clear_object((XObject **)&value->data[0].v_pointer);
}

static void x_value_object_copy_value(const XValue *src_value, XValue *dest_value)
{
    x_set_object((XObject **)&dest_value->data[0].v_pointer, src_value->data[0].v_pointer);
}

static void x_value_object_transform_value(const XValue *src_value, XValue *dest_value)
{
    if (src_value->data[0].v_pointer && x_type_is_a(X_OBJECT_TYPE(src_value->data[0].v_pointer), X_VALUE_TYPE(dest_value))) {
        dest_value->data[0].v_pointer = x_object_ref(src_value->data[0].v_pointer);
    } else {
        dest_value->data[0].v_pointer = NULL;
    }
}

static xpointer x_value_object_peek_pointer(const XValue *value)
{
    return value->data[0].v_pointer;
}

static xchar *x_value_object_collect_value(XValue *value, xuint n_collect_values, XTypeCValue *collect_values, xuint collect_flags)
{
    if (collect_values[0].v_pointer) {
        XObject *object = (XObject *)collect_values[0].v_pointer;

        if (object->x_type_instance.x_class == NULL) {
            return x_strconcat("invalid unclassed object pointer for value type '", X_VALUE_TYPE_NAME(value), "'", NULL);
        } else if (!x_value_type_compatible(X_OBJECT_TYPE(object), X_VALUE_TYPE(value))) {
            return x_strconcat("invalid object type '", X_OBJECT_TYPE_NAME(object), "' for value type '", X_VALUE_TYPE_NAME(value), "'", NULL);
        }

        value->data[0].v_pointer = x_object_ref(object);
    } else {
        value->data[0].v_pointer = NULL;
    }

    return NULL;
}

static xchar *x_value_object_lcopy_value(const XValue *value, xuint n_collect_values, XTypeCValue *collect_values, xuint collect_flags)
{
    XObject **object_p = (XObject **)collect_values[0].v_pointer;

    x_return_val_if_fail(object_p != NULL, x_strdup_printf("value location for '%s' passed as NULL", X_VALUE_TYPE_NAME(value)));

    if (!value->data[0].v_pointer) {
        *object_p = NULL;
    } else if (collect_flags & X_VALUE_NOCOPY_CONTENTS) {
        *object_p = (XObject *)value->data[0].v_pointer;
    } else {
        *object_p = x_object_ref((XObject *)value->data[0].v_pointer);
    }

    return NULL;
}

void x_value_set_object(XValue *value, xpointer v_object)
{
    XObject *old;

    x_return_if_fail(X_VALUE_HOLDS_OBJECT(value));

    if X_UNLIKELY(value->data[0].v_pointer == v_object) {
        return;
    }

    old = x_steal_pointer(&value->data[0].v_pointer);
    if (v_object) {
        x_return_if_fail(X_IS_OBJECT(v_object));
        x_return_if_fail(x_value_type_compatible(X_OBJECT_TYPE(v_object), X_VALUE_TYPE(value)));

        value->data[0].v_pointer = x_object_ref(v_object);
    }

    x_clear_object(&old);
}

void x_value_set_object_take_ownership(XValue  *value, xpointer v_object)
{
    x_value_take_object(value, v_object);
}

void x_value_take_object(XValue *value, xpointer v_object)
{
    x_return_if_fail(X_VALUE_HOLDS_OBJECT(value));

    x_clear_object((XObject **)&value->data[0].v_pointer);

    if (v_object) {
        x_return_if_fail(X_IS_OBJECT(v_object));
        x_return_if_fail(x_value_type_compatible(X_OBJECT_TYPE(v_object), X_VALUE_TYPE(value)));

        value->data[0].v_pointer = x_steal_pointer(&v_object);
    }
}

xpointer x_value_get_object(const XValue *value)
{
    x_return_val_if_fail(X_VALUE_HOLDS_OBJECT(value), NULL);
    return value->data[0].v_pointer;
}

xpointer x_value_dup_object(const XValue *value)
{
    x_return_val_if_fail(X_VALUE_HOLDS_OBJECT(value), NULL);
    return value->data[0].v_pointer ? x_object_ref(value->data[0].v_pointer) : NULL;
}

xulong x_signal_connect_object(xpointer instance, const xchar *detailed_signal, XCallback c_handler, xpointer gobject, XConnectFlags connect_flags)
{
    x_return_val_if_fail(X_TYPE_CHECK_INSTANCE(instance), 0);
    x_return_val_if_fail(detailed_signal != NULL, 0);
    x_return_val_if_fail(c_handler != NULL, 0);

    if (gobject) {
        XClosure *closure;

        x_return_val_if_fail(X_IS_OBJECT(gobject), 0);

        closure = ((connect_flags & X_CONNECT_SWAPPED) ? x_cclosure_new_object_swap : x_cclosure_new_object)(c_handler, (XObject *)gobject);
        return x_signal_connect_closure(instance, detailed_signal, closure, connect_flags & X_CONNECT_AFTER);
    } else {
        return x_signal_connect_data(instance, detailed_signal, c_handler, NULL, NULL, connect_flags);
    }
}

typedef struct {
    XObject  *object;
    xuint    n_closures;
    XClosure *closures[1];
} CArray;

static void object_remove_closure(xpointer data, XClosure *closure)
{
    xuint i;
    CArray *carray;
    XObject *object = (XObject *)data;

    object_bit_lock(object, OPTIONAL_BIT_LOCK_CLOSURE_ARRAY);
    carray = (CArray *)x_object_get_qdata(object, quark_closure_array);
    for (i = 0; i < carray->n_closures; i++) {
        if (carray->closures[i] == closure) {
            carray->n_closures--;
            if (i < carray->n_closures) {
                carray->closures[i] = carray->closures[carray->n_closures];
            }

            object_bit_unlock(object, OPTIONAL_BIT_LOCK_CLOSURE_ARRAY);
            return;
        }
    }
    object_bit_unlock(object, OPTIONAL_BIT_LOCK_CLOSURE_ARRAY);

    x_assert_not_reached();
}

static void destroy_closure_array(xpointer data)
{
    CArray *carray = (CArray *)data;
    xuint i, n = carray->n_closures;
    XObject *object = carray->object;

    for (i = 0; i < n; i++) {
        XClosure *closure = carray->closures[i];

        x_closure_remove_invalidate_notifier(closure, object, object_remove_closure);
        x_closure_invalidate(closure);
    }

    x_free(carray);
}

void x_object_watch_closure(XObject *object, XClosure *closure)
{
    xuint i;
    CArray *carray;

    x_return_if_fail(X_IS_OBJECT(object));
    x_return_if_fail(closure != NULL);
    x_return_if_fail(closure->is_invalid == FALSE);
    x_return_if_fail(closure->in_marshal == FALSE);
    x_return_if_fail(x_atomic_int_get(&object->ref_count) > 0);

    x_closure_add_invalidate_notifier(closure, object, object_remove_closure);
    x_closure_add_marshal_guards(closure, object, (XClosureNotify)x_object_ref, object, (XClosureNotify)x_object_unref);

    object_bit_lock(object, OPTIONAL_BIT_LOCK_CLOSURE_ARRAY);
    carray = (CArray *)x_datalist_id_remove_no_notify(&object->qdata, quark_closure_array);
    if (!carray) {
        carray = x_renew(CArray, NULL, 1);
        carray->object = object;
        carray->n_closures = 1;
        i = 0;
    } else {
        i = carray->n_closures++;
        carray = (CArray *)x_realloc(carray, sizeof(*carray) + sizeof(carray->closures[0]) * i);
    }

    carray->closures[i] = closure;
    x_datalist_id_set_data_full(&object->qdata, quark_closure_array, carray, destroy_closure_array);
    object_bit_unlock(object, OPTIONAL_BIT_LOCK_CLOSURE_ARRAY);
}

XClosure *x_closure_new_object(xuint sizeof_closure, XObject *object)
{
    XClosure *closure;

    x_return_val_if_fail(X_IS_OBJECT(object), NULL);
    x_return_val_if_fail(x_atomic_int_get(&object->ref_count) > 0, NULL);

    closure = x_closure_new_simple(sizeof_closure, object);
    x_object_watch_closure(object, closure);

    return closure;
}

XClosure *x_cclosure_new_object(XCallback callback_func, XObject *object)
{
    XClosure *closure;

    x_return_val_if_fail(X_IS_OBJECT(object), NULL);
    x_return_val_if_fail(x_atomic_int_get(&object->ref_count) > 0, NULL);
    x_return_val_if_fail(callback_func != NULL, NULL);

    closure = x_cclosure_new(callback_func, object, NULL);
    x_object_watch_closure(object, closure);

    return closure;
}

XClosure *x_cclosure_new_object_swap(XCallback callback_func, XObject *object)
{
    XClosure *closure;

    x_return_val_if_fail(X_IS_OBJECT(object), NULL);
    x_return_val_if_fail(x_atomic_int_get(&object->ref_count) > 0, NULL);
    x_return_val_if_fail(callback_func != NULL, NULL);

    closure = x_cclosure_new_swap(callback_func, object, NULL);
    x_object_watch_closure(object, closure);

    return closure;
}

xsize x_object_compat_control(xsize what, xpointer data)
{
    xpointer *pp;

    switch (what) {
        case 1:
            return (xsize)X_TYPE_INITIALLY_UNOWNED;

        case 2:
            floating_flag_handler = (xuint(*)(XObject *, xint))data;
            return 1;

        case 3:
            pp = (xpointer *)data;
            *pp = (xpointer)floating_flag_handler;
            return 1;

        default:
            return 0;
    }
}

X_DEFINE_TYPE(XInitiallyUnowned, x_initially_unowned, X_TYPE_OBJECT)

static void x_initially_unowned_init(XInitiallyUnowned *object)
{
    x_object_force_floating(object);
}

static void x_initially_unowned_class_init(XInitiallyUnownedClass *klass)
{

}

void x_weak_ref_init(XWeakRef *weak_ref, xpointer object)
{
    weak_ref->priv.p = NULL;
    if (object) {
        x_weak_ref_set(weak_ref, object);
    }
}

void x_weak_ref_clear(XWeakRef *weak_ref)
{
    x_weak_ref_set(weak_ref, NULL);
    weak_ref->priv.p = (void *)0xccccccccu;
}

xpointer x_weak_ref_get(XWeakRef *weak_ref)
{
    XObject *object;
    xpointer toggle_data = NULL;
    XToggleNotify toggle_notify = NULL;

    x_return_val_if_fail(weak_ref, NULL);

    x_rw_lock_reader_lock(&weak_locations_lock);
    object = weak_ref->priv.p;

    if (object) {
        object = object_ref(object, &toggle_notify, &toggle_data);
    }
    x_rw_lock_reader_unlock(&weak_locations_lock);

    if (toggle_notify) {
        toggle_notify(toggle_data, object, FALSE);
    }

    return object;
}

static void weak_locations_free_unlocked(XSList **weak_locations)
{
    if (*weak_locations) {
        XSList *weak_location;

        for (weak_location = *weak_locations; weak_location;) {
            XWeakRef *weak_ref_location = (XWeakRef *)weak_location->data;

            weak_ref_location->priv.p = NULL;
            weak_location = x_slist_delete_link(weak_location, weak_location);
        }
    }

    x_free(weak_locations);
}

static void weak_locations_free(xpointer data)
{
    XSList **weak_locations = (XSList **)data;

    x_rw_lock_writer_lock(&weak_locations_lock);
    weak_locations_free_unlocked(weak_locations);
    x_rw_lock_writer_unlock(&weak_locations_lock);
}

void x_weak_ref_set(XWeakRef *weak_ref, xpointer object)
{
    XObject *new_object;
    XObject *old_object;
    XSList **weak_locations;

    x_return_if_fail(weak_ref != NULL);
    x_return_if_fail(object == NULL || X_IS_OBJECT(object));

    new_object = (XObject *)object;

    x_rw_lock_writer_lock(&weak_locations_lock);

    old_object = (XObject *)weak_ref->priv.p;
    if (new_object != old_object) {
        weak_ref->priv.p = new_object;

        if (old_object != NULL) {
            weak_locations = (XSList **)x_datalist_id_get_data(&old_object->qdata, quark_weak_locations);
            if (weak_locations == NULL) {
                x_critical("unexpected missing XWeakRef");
            } else {
                *weak_locations = x_slist_remove(*weak_locations, weak_ref);

                if (!*weak_locations) {
                    weak_locations_free_unlocked(weak_locations);
                    x_datalist_id_remove_no_notify(&old_object->qdata, quark_weak_locations);
                }
            }
        }

        if (new_object != NULL) {
            if (x_atomic_int_get(&new_object->ref_count) < 1) {
                weak_ref->priv.p = NULL;
                x_rw_lock_writer_unlock(&weak_locations_lock);
                x_critical("calling x_weak_ref_set() with already destroyed object");
                return;
            }

            weak_locations = (XSList **)x_datalist_id_get_data(&new_object->qdata, quark_weak_locations);
            if (weak_locations == NULL) {
                weak_locations = x_new0(XSList *, 1);
                x_datalist_id_set_data_full(&new_object->qdata, quark_weak_locations, weak_locations, weak_locations_free);
            }

            *weak_locations = x_slist_prepend(*weak_locations, weak_ref);
        }
    }

    x_rw_lock_writer_unlock(&weak_locations_lock);
}
