#ifndef __X_MESSAGES_H__
#define __X_MESSAGES_H__

#include <stdarg.h>

#include "xatomic.h"
#include "xtypes.h"
#include "xmacros.h"
#include "xvariant.h"

X_BEGIN_DECLS

#define X_LOG_LEVEL_USER_SHIFT      (8)

XLIB_AVAILABLE_IN_ALL
xsize x_printf_string_upper_bound(const xchar *format, va_list args) X_GNUC_PRINTF(1, 0);

typedef enum {
    X_LOG_FLAG_RECURSION = 1 << 0,
    X_LOG_FLAG_FATAL     = 1 << 1,
    X_LOG_LEVEL_ERROR    = 1 << 2,
    X_LOG_LEVEL_CRITICAL = 1 << 3,
    X_LOG_LEVEL_WARNING  = 1 << 4,
    X_LOG_LEVEL_MESSAGE  = 1 << 5,
    X_LOG_LEVEL_INFO     = 1 << 6,
    X_LOG_LEVEL_DEBUG    = 1 << 7,
    X_LOG_LEVEL_MASK     = ~(X_LOG_FLAG_RECURSION | X_LOG_FLAG_FATAL)
} XLogLevelFlags;

#define X_LOG_FATAL_MASK            (X_LOG_FLAG_RECURSION | X_LOG_LEVEL_ERROR)

typedef void (*XLogFunc)(const xchar *log_domain, XLogLevelFlags log_level, const xchar *message, xpointer user_data);

XLIB_AVAILABLE_IN_ALL
xuint x_log_set_handler(const xchar *log_domain, XLogLevelFlags log_levels, XLogFunc log_func, xpointer user_data);

XLIB_AVAILABLE_IN_2_46
xuint x_log_set_handler_full(const xchar *log_domain, XLogLevelFlags log_levels, XLogFunc log_func, xpointer user_data, XDestroyNotify destroy);

XLIB_AVAILABLE_IN_ALL
void x_log_remove_handler(const xchar *log_domain, xuint handler_id);

XLIB_AVAILABLE_IN_ALL
void x_log_default_handler(const xchar *log_domain, XLogLevelFlags log_level, const xchar *message, xpointer unused_data);

XLIB_AVAILABLE_IN_ALL
XLogFunc x_log_set_default_handler(XLogFunc log_func, xpointer user_data);

XLIB_AVAILABLE_IN_ALL
void x_log(const xchar *log_domain, XLogLevelFlags log_level, const xchar *format, ...) X_GNUC_PRINTF(3, 4);

XLIB_AVAILABLE_IN_ALL
void x_logv(const xchar *log_domain, XLogLevelFlags log_level, const xchar *format, va_list args) X_GNUC_PRINTF(3, 0);

XLIB_AVAILABLE_IN_ALL
XLogLevelFlags x_log_set_fatal_mask(const xchar *log_domain, XLogLevelFlags fatal_mask);

XLIB_AVAILABLE_IN_ALL
XLogLevelFlags x_log_set_always_fatal(XLogLevelFlags fatal_mask);

typedef enum {
    X_LOG_WRITER_HANDLED   = 1,
    X_LOG_WRITER_UNHANDLED = 0,
} XLogWriterOutput;

typedef struct _XLogField XLogField;

struct _XLogField {
    const xchar   *key;
    xconstpointer value;
    xssize        length;
};

typedef XLogWriterOutput(*XLogWriterFunc)(XLogLevelFlags log_level, const XLogField *fields, xsize n_fields, xpointer user_data);

XLIB_AVAILABLE_IN_2_50
void x_log_structured(const xchar *log_domain, XLogLevelFlags log_level, ...);

XLIB_AVAILABLE_IN_2_50
void x_log_structured_array(XLogLevelFlags log_level, const XLogField *fields, xsize n_fields);

XLIB_AVAILABLE_IN_2_50
void x_log_variant(const xchar *log_domain, XLogLevelFlags log_level, XVariant *fields);

XLIB_AVAILABLE_IN_2_50
void x_log_set_writer_func(XLogWriterFunc func, xpointer user_data, XDestroyNotify user_data_free);

XLIB_AVAILABLE_IN_2_50
xboolean x_log_writer_supports_color(xint output_fd);

XLIB_AVAILABLE_IN_2_50
xboolean x_log_writer_is_journald(xint output_fd);

XLIB_AVAILABLE_IN_2_50
xchar *x_log_writer_format_fields(XLogLevelFlags log_level, const XLogField *fields, xsize n_fields, xboolean use_color);

XLIB_AVAILABLE_IN_2_50
XLogWriterOutput x_log_writer_journald(XLogLevelFlags log_level, const XLogField *fields, xsize n_fields, xpointer user_data);

XLIB_AVAILABLE_IN_2_50
XLogWriterOutput x_log_writer_standard_streams(XLogLevelFlags log_level, const XLogField *fields, xsize n_fields, xpointer user_data);

XLIB_AVAILABLE_IN_2_50
XLogWriterOutput x_log_writer_default(XLogLevelFlags log_level, const XLogField *fields, xsize n_fields, xpointer user_data);

XLIB_AVAILABLE_IN_2_68
void x_log_writer_default_set_use_stderr(xboolean use_stderr);

XLIB_AVAILABLE_IN_2_68
xboolean x_log_writer_default_would_drop(XLogLevelFlags log_level, const char *log_domain);

XLIB_AVAILABLE_IN_2_72
xboolean x_log_get_debug_enabled(void);

XLIB_AVAILABLE_IN_2_72
void x_log_set_debug_enabled(xboolean enabled);

#define X_DEBUG_HERE()                                          \
    x_log_structured(X_LOG_DOMAIN, X_LOG_LEVEL_DEBUG, "CODE_FILE", __FILE__, "CODE_LINE", X_STRINGIFY (__LINE__), "CODE_FUNC", X_STRFUNC, "MESSAGE", "%" X_XINT64_FORMAT ": %s", x_get_monotonic_time(), X_STRLOC)

void _x_log_fallback_handler(const xchar *log_domain, XLogLevelFlags log_level, const xchar *message, xpointer unused_data);

XLIB_AVAILABLE_IN_ALL
void x_return_if_fail_warning(const char *log_domain, const char *pretty_function, const char *expression) X_ANALYZER_NORETURN;

XLIB_AVAILABLE_IN_ALL
void x_warn_message(const char *domain, const char *file, int line, const char *func, const char *warnexpr) X_ANALYZER_NORETURN;

X_NORETURN
XLIB_DEPRECATED
void x_assert_warning(const char *log_domain, const char *file, const int line, const char *pretty_function, const char *expression);

XLIB_AVAILABLE_IN_2_56
void x_log_structured_standard(const xchar *log_domain, XLogLevelFlags log_level, const xchar *file, const xchar *line, const xchar *func, const xchar *message_format, ...) X_GNUC_PRINTF(6, 7);

#ifndef X_LOG_DOMAIN
#define X_LOG_DOMAIN        ((xchar *)0)
#endif

#if defined(X_HAVE_ISO_VARARGS) && !X_ANALYZER_ANALYZING
#if defined(X_LOG_USE_STRUCTURED) && XLIB_VERSION_MAX_ALLOWED >= XLIB_VERSION_2_56
#define x_error(...)  \
    X_STMT_START {                                                                                                            \
        x_log_structured_standard(X_LOG_DOMAIN, X_LOG_LEVEL_ERROR, __FILE__, X_STRINGIFY(__LINE__), X_STRFUNC, __VA_ARGS__);  \
        for (;;);                                                                                                             \
    } X_STMT_END

#define x_message(...)          x_log_structured_standard(X_LOG_DOMAIN, X_LOG_LEVEL_MESSAGE, __FILE__, X_STRINGIFY(__LINE__), X_STRFUNC, __VA_ARGS__)
#define x_critical(...)         x_log_structured_standard(X_LOG_DOMAIN, X_LOG_LEVEL_CRITICAL, __FILE__, X_STRINGIFY(__LINE__), X_STRFUNC, __VA_ARGS__)
#define x_warning(...)          x_log_structured_standard(X_LOG_DOMAIN, X_LOG_LEVEL_WARNING, __FILE__, X_STRINGIFY(__LINE__), X_STRFUNC, __VA_ARGS__)
#define x_info(...)             x_log_structured_standard(X_LOG_DOMAIN, X_LOG_LEVEL_INFO, __FILE__, X_STRINGIFY(__LINE__), X_STRFUNC, __VA_ARGS__)
#define x_debug(...)            x_log_structured_standard(X_LOG_DOMAIN, X_LOG_LEVEL_DEBUG, __FILE__, X_STRINGIFY(__LINE__), X_STRFUNC, __VA_ARGS__)

#else

#define x_error(...)                                            \
    X_STMT_START {                                              \
        x_log(X_LOG_DOMAIN, X_LOG_LEVEL_ERROR, __VA_ARGS__);    \
        for (;;);                                               \
    } X_STMT_END

#define x_message(...)          x_log(X_LOG_DOMAIN, X_LOG_LEVEL_MESSAGE, __VA_ARGS__)
#define x_critical(...)         x_log(X_LOG_DOMAIN, X_LOG_LEVEL_CRITICAL, __VA_ARGS__)
#define x_warning(...)          x_log(X_LOG_DOMAIN, X_LOG_LEVEL_WARNING, __VA_ARGS__)
#define x_info(...)             x_log(X_LOG_DOMAIN, X_LOG_LEVEL_INFO, __VA_ARGS__)
#define x_debug(...)            x_log(X_LOG_DOMAIN, X_LOG_LEVEL_DEBUG, __VA_ARGS__)
#endif
#elif defined(X_HAVE_GNUC_VARARGS)  && !X_ANALYZER_ANALYZING
#if defined(X_LOG_USE_STRUCTURED) && XLIB_VERSION_MAX_ALLOWED >= XLIB_VERSION_2_56
#define x_error(format...)                                                                                              \
    X_STMT_START {                                                                                                      \
        x_log_structured_standard(X_LOG_DOMAIN, X_LOG_LEVEL_ERROR, __FILE__, X_STRINGIFY(__LINE__), X_STRFUNC, format); \
        for (;;);                                                                                                       \
    } X_STMT_END

#define x_message(format...)    x_log_structured_standard(X_LOG_DOMAIN, X_LOG_LEVEL_MESSAGE, __FILE__, X_STRINGIFY(__LINE__), X_STRFUNC, format)
#define x_critical(format...)   x_log_structured_standard(X_LOG_DOMAIN, X_LOG_LEVEL_CRITICAL, __FILE__, X_STRINGIFY(__LINE__), X_STRFUNC, format)
#define x_warning(format...)    x_log_structured_standard(X_LOG_DOMAIN, X_LOG_LEVEL_WARNING, __FILE__, X_STRINGIFY(__LINE__), X_STRFUNC, format)
#define x_info(format...)       x_log_structured_standard(X_LOG_DOMAIN, X_LOG_LEVEL_INFO, __FILE__, X_STRINGIFY(__LINE__), X_STRFUNC, format)
#define x_debug(format...)      x_log_structured_standard(X_LOG_DOMAIN, X_LOG_LEVEL_DEBUG, __FILE__, X_STRINGIFY(__LINE__), X_STRFUNC, format)
#else
#define x_error(format...)                              \
    X_STMT_START {                                      \
        x_log(X_LOG_DOMAIN, X_LOG_LEVEL_ERROR, format); \
        for (;;);                                       \
    } X_STMT_END

#define x_message(format...)    x_log(X_LOG_DOMAIN, X_LOG_LEVEL_MESSAGE, format)
#define x_critical(format...)   x_log(X_LOG_DOMAIN, X_LOG_LEVEL_CRITICAL, format)
#define x_warning(format...)    x_log(X_LOG_DOMAIN, X_LOG_LEVEL_WARNING, format)
#define x_info(format...)       x_log(X_LOG_DOMAIN, X_LOG_LEVEL_INFO, format)
#define x_debug(format...)      x_log(X_LOG_DOMAIN, X_LOG_LEVEL_DEBUG, format)
#endif

#else

X_NORETURN static void x_error(const xchar *format, ...) X_ANALYZER_NORETURN;
static void x_critical(const xchar *format, ...) X_ANALYZER_NORETURN;

static inline void x_error (const xchar *format, ...)
{
    va_list args;

    va_start(args, format);
    x_logv(X_LOG_DOMAIN, X_LOG_LEVEL_ERROR, format, args);
    va_end(args);

    for(;;);
}

static inline void x_message(const xchar *format, ...)
{
    va_list args;

    va_start(args, format);
    x_logv(X_LOG_DOMAIN, X_LOG_LEVEL_MESSAGE, format, args);
    va_end(args);
}

static inline void x_critical(const xchar *format, ...)
{
    va_list args;

    va_start(args, format);
    x_logv(X_LOG_DOMAIN, X_LOG_LEVEL_CRITICAL, format, args);
    va_end(args);
}

static inline void x_warning(const xchar *format, ...)
{
    va_list args;

    va_start(args, format);
    x_logv(X_LOG_DOMAIN, X_LOG_LEVEL_WARNING, format, args);
    va_end(args);
}

static inline void x_info(const xchar *format, ...)
{
    va_list args;

    va_start(args, format);
    x_logv(X_LOG_DOMAIN, X_LOG_LEVEL_INFO, format, args);
    va_end(args);
}

static inline void x_debug(const xchar *format, ...)
{
  va_list args;

    va_start(args, format);
    x_logv(X_LOG_DOMAIN, X_LOG_LEVEL_DEBUG, format, args);
    va_end(args);
}
#endif

#if defined(X_HAVE_ISO_VARARGS) && !X_ANALYZER_ANALYZING
#define x_warning_once(...)                                                                         \
    X_STMT_START {                                                                                  \
        static int X_PASTE(_XWarningOnceBoolean, __LINE__) = 0;                                     \
        if (x_atomic_int_compare_and_exchange(&X_PASTE(_XWarningOnceBoolean, __LINE__), 0, 1)) {    \
            x_warning(__VA_ARGS__);                                                                 \
        }                                                                                           \
    } X_STMT_END                                                                                    \
  XLIB_AVAILABLE_MACRO_IN_2_64
#elif defined(X_HAVE_GNUC_VARARGS)  && !X_ANALYZER_ANALYZING
#define x_warning_once(format...)                                                                   \
    X_STMT_START {                                                                                  \
        static int X_PASTE(_XWarningOnceBoolean, __LINE__) = 0;                                     \
        if (x_atomic_int_compare_and_exchange(&X_PASTE(_XWarningOnceBoolean, __LINE__), 0, 1)) {    \
            x_warning(format);                                                                      \
        }                                                                                           \
    } X_STMT_END                                                                                    \
    XLIB_AVAILABLE_MACRO_IN_2_64
#else
#define x_warning_once x_warning
#endif

typedef void (*XPrintFunc)(const xchar *string);

XLIB_AVAILABLE_IN_ALL
void x_print(const xchar *format, ...) X_GNUC_PRINTF(1, 2);

XLIB_AVAILABLE_IN_ALL
XPrintFunc x_set_print_handler(XPrintFunc func);

XLIB_AVAILABLE_IN_ALL
void x_printerr(const xchar *format, ...) X_GNUC_PRINTF (1, 2);

XLIB_AVAILABLE_IN_ALL
XPrintFunc x_set_printerr_handler(XPrintFunc func);

#define x_warn_if_reached()                                                     \
    do {                                                                        \
        x_warn_message(X_LOG_DOMAIN, __FILE__, __LINE__, X_STRFUNC, NULL);      \
    } while (0)

#define x_warn_if_fail(expr)                                                    \
    do {                                                                        \
        if X_LIKELY(expr) {                                                     \
            ;                                                                   \
        } else {                                                                \
            x_warn_message(X_LOG_DOMAIN, __FILE__, __LINE__, X_STRFUNC, #expr); \
        }                                                                       \
    } while (0)

#define x_return_if_fail(expr)                                                  \
    X_STMT_START {                                                              \
        if (X_LIKELY(expr)) {                                                   \
        } else {                                                                \
            x_return_if_fail_warning(X_LOG_DOMAIN, X_STRFUNC, #expr);           \
            return;                                                             \
        }                                                                       \
    } X_STMT_END

#define x_return_val_if_fail(expr, val)                                         \
    X_STMT_START {                                                              \
        if (X_LIKELY(expr)) {                                                   \
        } else {                                                                \
            x_return_if_fail_warning(X_LOG_DOMAIN, X_STRFUNC, #expr);           \
            return (val);                                                       \
        }                                                                       \
    } X_STMT_END

#define x_return_if_reached()                                                                                                       \
    X_STMT_START {                                                                                                                  \
        x_log(X_LOG_DOMAIN, X_LOG_LEVEL_CRITICAL, "file %s: line %d (%s): should not be reached", __FILE__, __LINE__, X_STRFUNC);   \
        return;                                                                                                                     \
    } X_STMT_END

#define x_return_val_if_reached(val)                                                                                                \
    X_STMT_START {                                                                                                                  \
        x_log(X_LOG_DOMAIN, X_LOG_LEVEL_CRITICAL, "file %s: line %d (%s): should not be reached", __FILE__, __LINE__, X_STRFUNC);   \
        return (val);                                                                                                               \
    } X_STMT_END

X_END_DECLS

#endif
