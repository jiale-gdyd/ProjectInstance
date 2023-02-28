#ifndef __XOBJECT_ENUM_TYPES_H__
#define __XOBJECT_ENUM_TYPES_H__

#include "../xlib/xlib-object.h"

X_BEGIN_DECLS

XLIB_AVAILABLE_IN_2_60
XType x_unicode_type_get_type(void) X_GNUC_CONST;

XLIB_AVAILABLE_IN_2_60
XType x_unicode_break_type_get_type(void) X_GNUC_CONST;

XLIB_AVAILABLE_IN_2_60
XType x_unicode_script_get_type(void) X_GNUC_CONST;

XLIB_AVAILABLE_IN_2_60
XType x_normalize_mode_get_type(void) X_GNUC_CONST;

#define X_TYPE_UNICODE_TYPE                 (x_unicode_type_get_type())
#define X_TYPE_UNICODE_BREAK_TYPE           (x_unicode_break_type_get_type())
#define X_TYPE_UNICODE_SCRIPT               (x_unicode_script_get_type())
#define X_TYPE_NORMALIZE_MODE               (x_normalize_mode_get_type())

X_END_DECLS

#endif
