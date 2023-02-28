#ifndef __X_BOXED_H__
#define __X_BOXED_H__

#include "xtype.h"

#ifndef __XI_SCANNER__
#include "xlib-types.h"
#endif

X_BEGIN_DECLS

#define X_TYPE_IS_BOXED(type)       (X_TYPE_FUNDAMENTAL(type) == X_TYPE_BOXED)
#define X_VALUE_HOLDS_BOXED(value)  (X_TYPE_CHECK_VALUE_TYPE((value), X_TYPE_BOXED))

#define X_TYPE_CLOSURE              (x_closure_get_type())
#define X_TYPE_VALUE                (x_value_get_type())

typedef void (*XBoxedFreeFunc)(xpointer boxed);
typedef xpointer (*XBoxedCopyFunc)(xpointer boxed);

XLIB_AVAILABLE_IN_ALL
xpointer x_boxed_copy(XType boxed_type, xconstpointer src_boxed);

XLIB_AVAILABLE_IN_ALL
void x_boxed_free(XType boxed_type, xpointer boxed);

XLIB_AVAILABLE_IN_ALL
void x_value_set_boxed(XValue *value, xconstpointer v_boxed);

XLIB_AVAILABLE_IN_ALL
void x_value_set_static_boxed(XValue *value, xconstpointer v_boxed);

XLIB_AVAILABLE_IN_ALL
void x_value_take_boxed(XValue *value, xconstpointer v_boxed);

XLIB_DEPRECATED_FOR(x_value_take_boxed)
void x_value_set_boxed_take_ownership(XValue *value, xconstpointer v_boxed);

XLIB_AVAILABLE_IN_ALL
xpointer x_value_get_boxed(const XValue *value);

XLIB_AVAILABLE_IN_ALL
xpointer x_value_dup_boxed(const XValue *value);

XLIB_AVAILABLE_IN_ALL
XType x_boxed_type_register_static(const xchar *name, XBoxedCopyFunc boxed_copy, XBoxedFreeFunc boxed_free);

XLIB_AVAILABLE_IN_ALL
XType x_closure_get_type(void) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
XType x_value_get_type(void) X_GNUC_CONST;

X_END_DECLS

#endif
