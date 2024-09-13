#ifndef __X_REFSTRING_H__
#define __X_REFSTRING_H__

#include "xmem.h"
#include "xmacros.h"

X_BEGIN_DECLS

XLIB_AVAILABLE_IN_2_58
char *x_ref_string_new(const char *str);

XLIB_AVAILABLE_IN_2_58
char *x_ref_string_new_len(const char *str, xssize len);

XLIB_AVAILABLE_IN_2_58
char *x_ref_string_new_intern(const char *str);

XLIB_AVAILABLE_IN_2_58
char *x_ref_string_acquire(char *str);

XLIB_AVAILABLE_IN_2_58
void x_ref_string_release(char *str);

XLIB_AVAILABLE_IN_2_58
xsize x_ref_string_length(char *str);

typedef char XRefString;

XLIB_AVAILABLE_IN_2_84
xboolean x_ref_string_equal(const char *str1, const char *str2);

X_END_DECLS

#endif
