#ifndef __X_PRINTF_H__
#define __X_PRINTF_H__

#include <stdio.h>
#include <stdarg.h>
#include "../xlib.h"

X_BEGIN_DECLS

XLIB_AVAILABLE_IN_ALL
xint x_printf(xchar const *format, ...) X_GNUC_PRINTF(1, 2);

XLIB_AVAILABLE_IN_ALL
xint x_fprintf(FILE *file, xchar const *format, ...) X_GNUC_PRINTF(2, 3);

XLIB_AVAILABLE_IN_ALL
xint x_sprintf(xchar *string, xchar const *format, ...) X_GNUC_PRINTF(2, 3);

XLIB_AVAILABLE_IN_ALL
xint x_vprintf(xchar const *format, va_list args) X_GNUC_PRINTF(1, 0);

XLIB_AVAILABLE_IN_ALL
xint x_vfprintf(FILE *file, xchar const *format, va_list args) X_GNUC_PRINTF(2, 0);

XLIB_AVAILABLE_IN_ALL
xint x_vsprintf(xchar *string, xchar const *format, va_list args) X_GNUC_PRINTF(2, 0);

XLIB_AVAILABLE_IN_ALL
xint x_vasprintf(xchar **string, xchar const *format, va_list args) X_GNUC_PRINTF(2, 0);

X_END_DECLS

#endif
