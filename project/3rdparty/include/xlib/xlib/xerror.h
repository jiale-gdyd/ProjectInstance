#ifndef __X_ERROR_H__
#define __X_ERROR_H__

#include <stdarg.h>
#include "xquark.h"

X_BEGIN_DECLS

typedef struct _XError XError;

struct _XError {
    XQuark domain;
    xint   code;
    xchar  *message;
};

#define X_DEFINE_EXTENDED_ERROR(ErrorType, error_type)                                                      \
    static inline ErrorType ## Private *error_type ## _get_private(const XError *error)                     \
    {                                                                                                       \
        const xsize sa = 2 * sizeof(xsize);                                                                 \
        const xsize as = (sizeof(ErrorType ## Private) + (sa - 1)) & -sa;                                   \
        x_return_val_if_fail(error != NULL, NULL);                                                          \
        x_return_val_if_fail(error->domain == error_type ## _quark (), NULL);                               \
        return (ErrorType ## Private *)(((xuint8 *)error) - as);                                            \
    }                                                                                                       \
                                                                                                            \
    static void x_error_with_ ## error_type ## _private_init(XError *error)                                 \
    {                                                                                                       \
        ErrorType ## Private *priv = error_type ## _get_private(error);                                     \
        error_type ## _private_init(priv);                                                                  \
    }                                                                                                       \
                                                                                                            \
    static void x_error_with_ ## error_type ## _private_copy(const XError *src_error, XError *dest_error)   \
    {                                                                                                       \
        const ErrorType ## Private *src_priv = error_type ## _get_private(src_error);                       \
        ErrorType ## Private *dest_priv = error_type ## _get_private(dest_error);                           \
        error_type ## _private_copy(src_priv, dest_priv);                                                   \
    }                                                                                                       \
                                                                                                            \
    static void x_error_with_ ## error_type ## _private_clear(XError *error)                                \
    {                                                                                                       \
        ErrorType ## Private *priv = error_type ## _get_private(error);                                     \
        error_type ## _private_clear(priv);                                                                 \
    }                                                                                                       \
                                                                                                            \
    XQuark error_type ## _quark (void)                                                                      \
    {                                                                                                       \
        static XQuark q;                                                                                    \
        static xsize initialized = 0;                                                                       \
                                                                                                            \
        if (x_once_init_enter(&initialized)) {                                                              \
            q = x_error_domain_register_static(#ErrorType, sizeof(ErrorType ## Private), x_error_with_ ## error_type ## _private_init, x_error_with_ ## error_type ## _private_copy, x_error_with_ ## error_type ## _private_clear); \
            x_once_init_leave &initialized, 1);                                                             \
        }                                                                                                   \
                                                                                                            \
        return q;                                                                                           \
    }

typedef void (*XErrorInitFunc)(XError *error);
typedef void (*XErrorClearFunc)(XError *error);
typedef void (*XErrorCopyFunc)(const XError *src_error, XError *dest_error);

XLIB_AVAILABLE_IN_2_68
XQuark x_error_domain_register_static(const char *error_type_name, xsize error_type_private_size, XErrorInitFunc error_type_init, XErrorCopyFunc error_type_copy, XErrorClearFunc error_type_clear);

XLIB_AVAILABLE_IN_2_68
XQuark x_error_domain_register(const char *error_type_name, xsize error_type_private_size, XErrorInitFunc error_type_init, XErrorCopyFunc error_type_copy, XErrorClearFunc error_type_clear);

XLIB_AVAILABLE_IN_ALL
XError *x_error_new(XQuark domain, xint code, const xchar *format, ...) X_GNUC_PRINTF(3, 4);

XLIB_AVAILABLE_IN_ALL
XError *x_error_new_literal(XQuark domain, xint code, const xchar *message);

XLIB_AVAILABLE_IN_ALL
XError *x_error_new_valist(XQuark domain, xint code, const xchar *format, va_list args) X_GNUC_PRINTF(3, 0);

XLIB_AVAILABLE_IN_ALL
void x_error_free(XError *error);

XLIB_AVAILABLE_IN_ALL
XError *x_error_copy(const XError *error);

XLIB_AVAILABLE_IN_ALL
xboolean x_error_matches(const XError *error, XQuark domain, xint code);

XLIB_AVAILABLE_IN_ALL
void x_set_error(XError **err, XQuark domain, xint code, const xchar *format, ...) X_GNUC_PRINTF(4, 5);

XLIB_AVAILABLE_IN_ALL
void x_set_error_literal(XError **err, XQuark domain, xint code, const xchar *message);

XLIB_AVAILABLE_IN_ALL
void x_propagate_error(XError **dest, XError *src);

XLIB_AVAILABLE_IN_ALL
void x_clear_error(XError **err);

XLIB_AVAILABLE_IN_ALL
void x_prefix_error(XError **err, const xchar *format, ...) X_GNUC_PRINTF(2, 3);

XLIB_AVAILABLE_IN_2_70
void x_prefix_error_literal(XError **err, const xchar *prefix);

XLIB_AVAILABLE_IN_ALL
void x_propagate_prefixed_error(XError **dest, XError *src, const xchar *format, ...) X_GNUC_PRINTF(3, 4);

X_END_DECLS

#endif
