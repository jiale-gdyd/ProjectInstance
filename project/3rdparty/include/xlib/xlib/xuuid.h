#ifndef __X_UUID_H__
#define __X_UUID_H__

#include "xtypes.h"

X_BEGIN_DECLS

XLIB_AVAILABLE_IN_2_52
xboolean x_uuid_string_is_valid(const xchar *str);

XLIB_AVAILABLE_IN_2_52
xchar *x_uuid_string_random(void);

X_END_DECLS

#endif
