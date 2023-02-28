#ifndef __X_VERSION_H__
#define __X_VERSION_H__

#include "xtypes.h"

X_BEGIN_DECLS

XLIB_VAR const xuint xlib_major_version;
XLIB_VAR const xuint xlib_minor_version;
XLIB_VAR const xuint xlib_micro_version;

XLIB_VAR const xuint xlib_binary_age;
XLIB_VAR const xuint xlib_interface_age;

XLIB_AVAILABLE_IN_ALL
const xchar *xlib_check_version(xuint required_major, xuint required_minor, xuint required_micro);

#define XLIB_CHECK_VERSION(major, minor, micro)     (XLIB_MAJOR_VERSION > (major) || (XLIB_MAJOR_VERSION == (major) && XLIB_MINOR_VERSION > (minor)) || (XLIB_MAJOR_VERSION == (major) && XLIB_MINOR_VERSION == (minor) && XLIB_MICRO_VERSION >= (micro)))

X_END_DECLS

#endif
