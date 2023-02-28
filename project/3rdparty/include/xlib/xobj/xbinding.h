#ifndef __X_BINDING_H__
#define __X_BINDING_H__

#include "../xlib.h"
#include "xobject.h"

X_BEGIN_DECLS

#define X_TYPE_BINDING_FLAGS    (x_binding_flags_get_type())

#define X_TYPE_BINDING          (x_binding_get_type())
#define X_BINDING(obj)          (X_TYPE_CHECK_INSTANCE_CAST((obj), X_TYPE_BINDING, XBinding))
#define X_IS_BINDING(obj)       (X_TYPE_CHECK_INSTANCE_TYPE((obj), X_TYPE_BINDING))

typedef struct _XBinding XBinding;

typedef xboolean (*XBindingTransformFunc)(XBinding *binding, const XValue *from_value, XValue *to_value, xpointer user_data);

typedef enum {
    X_BINDING_DEFAULT        = 0,
    X_BINDING_BIDIRECTIONAL  = 1 << 0,
    X_BINDING_SYNC_CREATE    = 1 << 1,
    X_BINDING_INVERT_BOOLEAN = 1 << 2
} XBindingFlags;

XLIB_AVAILABLE_IN_ALL
XType x_binding_flags_get_type(void) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
XType x_binding_get_type(void) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
XBindingFlags x_binding_get_flags(XBinding *binding);

XLIB_DEPRECATED_IN_2_68_FOR(x_binding_dup_source)
XObject *x_binding_get_source(XBinding *binding);

XLIB_AVAILABLE_IN_2_68
XObject *x_binding_dup_source(XBinding *binding);

XLIB_DEPRECATED_IN_2_68_FOR(x_binding_dup_target)
XObject *x_binding_get_target(XBinding *binding);

XLIB_AVAILABLE_IN_2_68
XObject *x_binding_dup_target(XBinding *binding);

XLIB_AVAILABLE_IN_ALL
const xchar *x_binding_get_source_property(XBinding *binding);

XLIB_AVAILABLE_IN_ALL
const xchar *x_binding_get_target_property(XBinding *binding);

XLIB_AVAILABLE_IN_2_38
void x_binding_unbind(XBinding *binding);

XLIB_AVAILABLE_IN_ALL
XBinding *x_object_bind_property(xpointer source, const xchar *source_property, xpointer target, const xchar *target_property, XBindingFlags flags);

XLIB_AVAILABLE_IN_ALL
XBinding *x_object_bind_property_full(xpointer source, const xchar *source_property, xpointer target, const xchar *target_property, XBindingFlags flags, XBindingTransformFunc transform_to, XBindingTransformFunc transform_from, xpointer user_data, XDestroyNotify notify);

XLIB_AVAILABLE_IN_ALL
XBinding *x_object_bind_property_with_closures(xpointer source, const xchar *source_property, xpointer target, const xchar *target_property, XBindingFlags flags, XClosure *transform_to, XClosure *transform_from);

X_END_DECLS

#endif
