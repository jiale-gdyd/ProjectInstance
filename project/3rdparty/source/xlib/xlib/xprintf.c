#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>

#include <xlib/xlib/config.h>
#include <xlib/xlib/xprintf.h>

extern int vasprintf(char **strp, const char *fmt, va_list ap);

xint x_printf(xchar const *format, ...)
{
    xint retval;
    va_list args;

    va_start(args, format);
    retval = x_vprintf(format, args);
    va_end(args);

    return retval;
}

xint x_fprintf(FILE *file, xchar const *format, ...)
{
    xint retval;
    va_list args;

    va_start(args, format);
    retval = x_vfprintf(file, format, args);
    va_end(args);

    return retval;
}

xint x_sprintf(xchar *string, xchar const *format, ...)
{
    xint retval;
    va_list args;

    va_start(args, format);
    retval = x_vsprintf(string, format, args);
    va_end(args);

    return retval;
}

xint x_snprintf(xchar *string, xulong n, xchar const *format, ...)
{
    xint retval;
    va_list args;

    va_start(args, format);
    retval = x_vsnprintf(string, n, format, args);
    va_end(args);

    return retval;
}

xint x_vprintf(xchar const *format, va_list args)
{
    x_return_val_if_fail(format != NULL, -1);
    return vprintf(format, args);
}

xint x_vfprintf(FILE *file, xchar const *format, va_list args)
{
    x_return_val_if_fail(format != NULL, -1);
    return vfprintf(file, format, args);
}

xint x_vsprintf(xchar *string, xchar const *format, va_list args)
{
    x_return_val_if_fail(string != NULL, -1);
    x_return_val_if_fail(format != NULL, -1);

    return vsprintf(string, format, args);
}

xint x_vsnprintf(xchar *string, xulong n, xchar const *format, va_list args)
{
    x_return_val_if_fail((n == 0) || (string != NULL), -1);
    x_return_val_if_fail(format != NULL, -1);

    return vsnprintf(string, n, format, args);
}

xint x_vasprintf(xchar **string, xchar const *format, va_list args)
{
    xint len;
    int saved_errno;

    x_return_val_if_fail(string != NULL, -1);

    len = vasprintf(string, format, args);
    saved_errno = errno;
    if (len < 0) {
        if (saved_errno == ENOMEM) {
            fputs(X_STRLOC, stderr);
            fputs(": failed to allocate memory\n", stderr);
            x_abort();
        } else {
            *string = NULL;
        }
    }

    return len;
}
