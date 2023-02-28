#ifndef __X_OBJECT_H__
#define __X_OBJECT_H__

#include "xtype.h"
#include "xvalue.h"
#include "xparam.h"
#include "xboxed.h"
#include "xsignal.h"
#include "xclosure.h"

X_BEGIN_DECLS

#define X_TYPE_IS_OBJECT(type)                  (X_TYPE_FUNDAMENTAL(type) == X_TYPE_OBJECT)
#define X_OBJECT(object)                        (X_TYPE_CHECK_INSTANCE_CAST((object), X_TYPE_OBJECT, XObject))
#define X_OBJECT_CLASS(classt)                  (X_TYPE_CHECK_CLASS_CAST((classt), X_TYPE_OBJECT, XObjectClass))

#if XLIB_VERSION_MAX_ALLOWED >= XLIB_VERSION_2_42
#define X_IS_OBJECT(object)                     (X_TYPE_CHECK_INSTANCE_FUNDAMENTAL_TYPE((object), X_TYPE_OBJECT))
#else
#define X_IS_OBJECT(object)                     (X_TYPE_CHECK_INSTANCE_TYPE((object), X_TYPE_OBJECT))
#endif

#define X_IS_OBJECT_CLASS(classt)               (X_TYPE_CHECK_CLASS_TYPE((classt), X_TYPE_OBJECT))
#define X_OBJECT_GET_CLASS(object)              (X_TYPE_INSTANCE_GET_CLASS((object), X_TYPE_OBJECT, XObjectClass))

#define X_OBJECT_TYPE(object)                   (X_TYPE_FROM_INSTANCE(object))
#define X_OBJECT_TYPE_NAME(object)              (x_type_name(X_OBJECT_TYPE(object)))

#define X_OBJECT_CLASS_TYPE(classt)             (X_TYPE_FROM_CLASS(classt))
#define X_OBJECT_CLASS_NAME(classt)             (x_type_name(X_OBJECT_CLASS_TYPE(classt)))

#define X_VALUE_HOLDS_OBJECT(value)             (X_TYPE_CHECK_VALUE_TYPE((value), X_TYPE_OBJECT))

#define X_TYPE_INITIALLY_UNOWNED                (x_initially_unowned_get_type())
#define X_INITIALLY_UNOWNED(object)             (X_TYPE_CHECK_INSTANCE_CAST((object), X_TYPE_INITIALLY_UNOWNED, XInitiallyUnowned))
#define X_INITIALLY_UNOWNED_CLASS(classt)       (X_TYPE_CHECK_CLASS_CAST((classt), X_TYPE_INITIALLY_UNOWNED, XInitiallyUnownedClass))
#define X_IS_INITIALLY_UNOWNED(object)          (X_TYPE_CHECK_INSTANCE_TYPE((object), X_TYPE_INITIALLY_UNOWNED))
#define X_IS_INITIALLY_UNOWNED_CLASS(classt)    (X_TYPE_CHECK_CLASS_TYPE((classt), X_TYPE_INITIALLY_UNOWNED))
#define X_INITIALLY_UNOWNED_GET_CLASS(object)   (X_TYPE_INSTANCE_GET_CLASS((object), X_TYPE_INITIALLY_UNOWNED, XInitiallyUnownedClass))

typedef struct _XObject XObject;
typedef struct _XObjectClass XObjectClass;
typedef struct _XObject XInitiallyUnowned;
typedef struct _XObjectClass XInitiallyUnownedClass;
typedef struct _XObjectConstructParam XObjectConstructParam;

typedef void (*XObjectFinalizeFunc)(XObject *object);
typedef void (*XWeakNotify)(xpointer data, XObject *where_the_object_was);
typedef void (*XObjectGetPropertyFunc)(XObject *object, xuint property_id, XValue *value, XParamSpec *pspec);
typedef void (*XObjectSetPropertyFunc)(XObject *object, xuint property_id, const XValue *value, XParamSpec *pspec);

struct _XObject {
    XTypeInstance x_type_instance;
    xuint         ref_count;
    XData         *qdata;
};

struct _XObjectClass {
    XTypeClass x_type_class;
    XSList     *construct_properties;

    XObject *(*constructor)(XType type, xuint n_construct_properties, XObjectConstructParam *construct_properties);
    void (*set_property)(XObject *object, xuint property_id, const XValue *value, XParamSpec *pspec);
    void (*get_property)(XObject *object, xuint property_id, XValue *value, XParamSpec *pspec);
    void (*dispose)(XObject *object);
    void (*finalize)(XObject *object);
    void (*dispatch_properties_changed)(XObject *object, xuint n_pspecs, XParamSpec **pspecs);
    void (*notify)(XObject *object, XParamSpec *pspec);
    void (*constructed)(XObject *object);

    xsize    flags;
    xsize    n_construct_properties;
    xpointer pspecs;
    xsize    n_pspecs;
    xpointer pdummy[3];
};

struct _XObjectConstructParam {
    XParamSpec *pspec;
    XValue     *value;
};

XLIB_AVAILABLE_IN_ALL
XType x_initially_unowned_get_type(void);

XLIB_AVAILABLE_IN_ALL
void x_object_class_install_property(XObjectClass *oclass, xuint property_id, XParamSpec *pspec);

XLIB_AVAILABLE_IN_ALL
XParamSpec *x_object_class_find_property(XObjectClass *oclass, const xchar *property_name);

XLIB_AVAILABLE_IN_ALL
XParamSpec **x_object_class_list_properties(XObjectClass *oclass, xuint *n_properties);

XLIB_AVAILABLE_IN_ALL
void x_object_class_override_property(XObjectClass *oclass, xuint property_id, const xchar *name);

XLIB_AVAILABLE_IN_ALL
void x_object_class_install_properties(XObjectClass *oclass, xuint n_pspecs, XParamSpec **pspecs);

XLIB_AVAILABLE_IN_ALL
void x_object_interface_install_property(xpointer x_iface, XParamSpec *pspec);

XLIB_AVAILABLE_IN_ALL
XParamSpec *x_object_interface_find_property(xpointer x_iface, const xchar *property_name);

XLIB_AVAILABLE_IN_ALL
XParamSpec **x_object_interface_list_properties(xpointer x_iface, xuint *n_properties_p);

XLIB_AVAILABLE_IN_ALL
XType x_object_get_type(void) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
xpointer x_object_new(XType object_type, const xchar *first_property_name, ...);

XLIB_AVAILABLE_IN_2_54
XObject *x_object_new_with_properties(XType object_type, xuint n_properties, const char *names[], const XValue values[]);

X_GNUC_BEGIN_IGNORE_DEPRECATIONS

XLIB_DEPRECATED_IN_2_54_FOR(x_object_new_with_properties)
xpointer x_object_newv(XType object_type, xuint n_parameters, XParameter *parameters);

X_GNUC_END_IGNORE_DEPRECATIONS

XLIB_AVAILABLE_IN_ALL
XObject *x_object_new_valist(XType object_type, const xchar *first_property_name, va_list var_args);

XLIB_AVAILABLE_IN_ALL
void x_object_set(xpointer object, const xchar *first_property_name, ...) X_GNUC_NULL_TERMINATED;

XLIB_AVAILABLE_IN_ALL
void x_object_get(xpointer object, const xchar *first_property_name, ...) X_GNUC_NULL_TERMINATED;

XLIB_AVAILABLE_IN_ALL
xpointer x_object_connect(xpointer object, const xchar *signal_spec, ...) X_GNUC_NULL_TERMINATED;

XLIB_AVAILABLE_IN_ALL
void x_object_disconnect(xpointer object, const xchar *signal_spec, ...) X_GNUC_NULL_TERMINATED;

XLIB_AVAILABLE_IN_2_54
void x_object_setv(XObject *object, xuint n_properties, const xchar *names[], const XValue values[]);

XLIB_AVAILABLE_IN_ALL
void x_object_set_valist(XObject *object, const xchar *first_property_name, va_list var_args);

XLIB_AVAILABLE_IN_2_54
void x_object_getv(XObject *object, xuint n_properties, const xchar *names[], XValue values[]);

XLIB_AVAILABLE_IN_ALL
void x_object_get_valist(XObject *object, const xchar *first_property_name, va_list var_args);

XLIB_AVAILABLE_IN_ALL
void x_object_set_property(XObject *object, const xchar *property_name, const XValue *value);

XLIB_AVAILABLE_IN_ALL
void x_object_get_property(XObject *object, const xchar *property_name, XValue *value);

XLIB_AVAILABLE_IN_ALL
void x_object_freeze_notify(XObject *object);

XLIB_AVAILABLE_IN_ALL
void x_object_notify(XObject *object, const xchar *property_name);

XLIB_AVAILABLE_IN_ALL
void x_object_notify_by_pspec(XObject *object, XParamSpec *pspec);

XLIB_AVAILABLE_IN_ALL
void x_object_thaw_notify(XObject *object);

XLIB_AVAILABLE_IN_ALL
xboolean x_object_is_floating(xpointer object);

XLIB_AVAILABLE_IN_ALL
xpointer x_object_ref_sink(xpointer object);

XLIB_AVAILABLE_IN_2_70
xpointer x_object_take_ref(xpointer object);

XLIB_AVAILABLE_IN_ALL
xpointer x_object_ref(xpointer object);

XLIB_AVAILABLE_IN_ALL
void x_object_unref(xpointer object);

XLIB_AVAILABLE_IN_ALL
void x_object_weak_ref(XObject *object, XWeakNotify notify, xpointer data);

XLIB_AVAILABLE_IN_ALL
void x_object_weak_unref(XObject *object, XWeakNotify notify, xpointer data);

XLIB_AVAILABLE_IN_ALL
void x_object_add_weak_pointer(XObject *object, xpointer *weak_pointer_location);

XLIB_AVAILABLE_IN_ALL
void x_object_remove_weak_pointer(XObject *object,  xpointer *weak_pointer_location);

#if defined(xlib_typeof) && XLIB_VERSION_MAX_ALLOWED >= XLIB_VERSION_2_56
#define x_object_ref(Obj)               ((xlib_typeof(Obj))(x_object_ref)(Obj))
#define x_object_ref_sink(Obj)          ((xlib_typeof(Obj))(x_object_ref_sink)(Obj))
#endif

typedef void (*XToggleNotify)(xpointer data, XObject *object, xboolean is_last_ref);

XLIB_AVAILABLE_IN_ALL
void x_object_add_toggle_ref(XObject *object, XToggleNotify notify, xpointer data);

XLIB_AVAILABLE_IN_ALL
void x_object_remove_toggle_ref(XObject *object, XToggleNotify notify, xpointer data);

XLIB_AVAILABLE_IN_ALL
xpointer x_object_get_qdata(XObject *object, XQuark quark);

XLIB_AVAILABLE_IN_ALL
void x_object_set_qdata(XObject *object, XQuark quark, xpointer data);

XLIB_AVAILABLE_IN_ALL
void x_object_set_qdata_full(XObject *object, XQuark quark, xpointer data, XDestroyNotify destroy);

XLIB_AVAILABLE_IN_ALL
xpointer x_object_steal_qdata(XObject *object, XQuark quark);

XLIB_AVAILABLE_IN_2_34
xpointer x_object_dup_qdata(XObject *object, XQuark quark, XDuplicateFunc dup_func, xpointer user_data);

XLIB_AVAILABLE_IN_2_34
xboolean x_object_replace_qdata(XObject *object, XQuark quark, xpointer oldval, xpointer newval, XDestroyNotify destroy, XDestroyNotify *old_destroy);

XLIB_AVAILABLE_IN_ALL
xpointer x_object_get_data(XObject *object, const xchar *key);

XLIB_AVAILABLE_IN_ALL
void x_object_set_data(XObject *object, const xchar *key, xpointer data);

XLIB_AVAILABLE_IN_ALL
void x_object_set_data_full(XObject *object, const xchar *key, xpointer data, XDestroyNotify destroy);

XLIB_AVAILABLE_IN_ALL
xpointer x_object_steal_data(XObject *object, const xchar *key);

XLIB_AVAILABLE_IN_2_34
xpointer x_object_dup_data(XObject *object, const xchar *key, XDuplicateFunc dup_func, xpointer user_data);

XLIB_AVAILABLE_IN_2_34
xboolean x_object_replace_data(XObject *object, const xchar *key, xpointer oldval, xpointer newval, XDestroyNotify destroy, XDestroyNotify *old_destroy);

XLIB_AVAILABLE_IN_ALL
void x_object_watch_closure(XObject *object, XClosure *closure);

XLIB_AVAILABLE_IN_ALL
XClosure *x_cclosure_new_object(XCallback callback_func, XObject *object);

XLIB_AVAILABLE_IN_ALL
XClosure *x_cclosure_new_object_swap(XCallback callback_func, XObject *object);

XLIB_AVAILABLE_IN_ALL
XClosure *x_closure_new_object(xuint sizeof_closure, XObject *object);

XLIB_AVAILABLE_IN_ALL
void x_value_set_object(XValue *value, xpointer v_object);

XLIB_AVAILABLE_IN_ALL
xpointer x_value_get_object(const XValue *value);

XLIB_AVAILABLE_IN_ALL
xpointer x_value_dup_object(const XValue *value);

XLIB_AVAILABLE_IN_ALL
xulong x_signal_connect_object(xpointer instance, const xchar *detailed_signal, XCallback c_handler, xpointer gobject, XConnectFlags connect_flags);

XLIB_AVAILABLE_IN_ALL
void x_object_force_floating(XObject *object);

XLIB_AVAILABLE_IN_ALL
void x_object_run_dispose(XObject *object);

XLIB_AVAILABLE_IN_ALL
void x_value_take_object(XValue *value, xpointer v_object);

XLIB_DEPRECATED_FOR(x_value_take_object)
void x_value_set_object_take_ownership(XValue *value, xpointer v_object);

XLIB_DEPRECATED
xsize x_object_compat_control(xsize what, xpointer data);

#define X_OBJECT_WARN_INVALID_PSPEC(object, pname, property_id, pspec)          \
    X_STMT_START {                                                              \
        XObject *_xlib__object = (XObject *)(object);                           \
        XParamSpec *_xlib__pspec = (XParamSpec *)(pspec);                       \
        xuint _xlib__property_id = (property_id);                               \
        x_warning("%s:%d: invalid %s id %u for \"%s\" of type '%s' in '%s'",    \
                    __FILE__, __LINE__,                                         \
                    (pname),                                                    \
                    _xlib__property_id,                                         \
                    _xlib__pspec->name,                                         \
                    x_type_name(X_PARAM_SPEC_TYPE(_xlib__pspec)),               \
                    X_OBJECT_TYPE_NAME(_xlib__object));                         \
    } X_STMT_END

#define X_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec) \
    X_OBJECT_WARN_INVALID_PSPEC((object), "property", (property_id), (pspec))

XLIB_AVAILABLE_IN_ALL
void x_clear_object(XObject **object_ptr);

#define x_clear_object(object_ptr)          x_clear_pointer((object_ptr), x_object_unref)

static inline xboolean (x_set_object)(XObject **object_ptr, XObject *new_object)
{
    XObject *old_object = *object_ptr;

    if (old_object == new_object) {
        return FALSE;
    }

    if (new_object != NULL) {
        x_object_ref(new_object);
    }

    *object_ptr = new_object;
    if (old_object != NULL) {
        x_object_unref(old_object);
    }

    return TRUE;
}

#define x_set_object(object_ptr, new_object) \
    (X_GNUC_EXTENSION ({ \
        X_STATIC_ASSERT(sizeof *(object_ptr) == sizeof(new_object)); \
        union { char *in; XObject **out; } _object_ptr; \
        _object_ptr.in = (char *)(object_ptr); \
        (void)(0 ? *(object_ptr) = (new_object), FALSE : FALSE); \
        (x_set_object)(_object_ptr.out, (XObject *)new_object); \
    })) XLIB_AVAILABLE_MACRO_IN_2_44

static inline void (x_assert_finalize_object)(XObject *object)
{
    xpointer weak_pointer = object;

    x_assert_true(X_IS_OBJECT(weak_pointer));
    x_object_add_weak_pointer(object, &weak_pointer);
    x_object_unref(weak_pointer);
    x_assert_null(weak_pointer);
}

#define x_assert_finalize_object(object)        (x_assert_finalize_object((XObject *)object))

static inline void (x_clear_weak_pointer)(xpointer *weak_pointer_location)
{
    XObject *object = (XObject *)*weak_pointer_location;

    if (object != NULL) {
        x_object_remove_weak_pointer(object, weak_pointer_location);
        *weak_pointer_location = NULL;
    }
}

#define x_clear_weak_pointer(weak_pointer_location) \
    ((x_clear_weak_pointer)((xpointer *)(weak_pointer_location)))

static inline xboolean (x_set_weak_pointer)(xpointer *weak_pointer_location, XObject  *new_object)
{
    XObject *old_object = (XObject *)*weak_pointer_location;

    if (old_object == new_object) {
        return FALSE;
    }

    if (old_object != NULL) {
        x_object_remove_weak_pointer(old_object, weak_pointer_location);
    }
    *weak_pointer_location = new_object;

    if (new_object != NULL) {
        x_object_add_weak_pointer(new_object, weak_pointer_location);
    }

    return TRUE;
}

#define x_set_weak_pointer(weak_pointer_location, new_object) \
    (0 ? *(weak_pointer_location) = (new_object), FALSE : (x_set_weak_pointer)((xpointer *)(weak_pointer_location), (XObject *)(new_object)))

typedef struct {
    union { xpointer p; } priv;
} XWeakRef;

XLIB_AVAILABLE_IN_ALL
void x_weak_ref_init(XWeakRef *weak_ref, xpointer object);

XLIB_AVAILABLE_IN_ALL
void x_weak_ref_clear(XWeakRef *weak_ref);

XLIB_AVAILABLE_IN_ALL
xpointer x_weak_ref_get(XWeakRef *weak_ref);

XLIB_AVAILABLE_IN_ALL
void x_weak_ref_set(XWeakRef *weak_ref, xpointer object);

X_END_DECLS

#endif
