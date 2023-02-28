#ifndef __X_BINDING_GROUP_H__
#define __X_BINDING_GROUP_H__

#include "../xlib.h"
#include "xobject.h"
#include "xbinding.h"

X_BEGIN_DECLS

#define X_BINDING_GROUP(obj)            (X_TYPE_CHECK_INSTANCE_CAST((obj), X_TYPE_BINDING_GROUP, XBindingGroup))
#define X_IS_BINDING_GROUP(obj)         (X_TYPE_CHECK_INSTANCE_TYPE((obj), X_TYPE_BINDING_GROUP))
#define X_TYPE_BINDING_GROUP            (x_binding_group_get_type())

typedef struct _XBindingGroup XBindingGroup;

XLIB_AVAILABLE_IN_2_72
XType x_binding_group_get_type(void) X_GNUC_CONST;

XLIB_AVAILABLE_IN_2_72
XBindingGroup *x_binding_group_new(void);

XLIB_AVAILABLE_IN_2_72
xpointer x_binding_group_dup_source(XBindingGroup *self);

XLIB_AVAILABLE_IN_2_72
void x_binding_group_set_source(XBindingGroup *self, xpointer source);

XLIB_AVAILABLE_IN_2_72
void x_binding_group_bind(XBindingGroup *self, const xchar *source_property, xpointer target, const xchar *target_property, XBindingFlags flags);

XLIB_AVAILABLE_IN_2_72
void x_binding_group_bind_full(XBindingGroup *self, const xchar *source_property, xpointer target, const xchar *target_property, XBindingFlags flags, XBindingTransformFunc transform_to, XBindingTransformFunc transform_from, xpointer user_data, XDestroyNotify user_data_destroy);

XLIB_AVAILABLE_IN_2_72
void x_binding_group_bind_with_closures(XBindingGroup *self, const xchar *source_property, xpointer target, const xchar *target_property, XBindingFlags flags, XClosure *transform_to, XClosure *transform_from);

X_END_DECLS

#endif
