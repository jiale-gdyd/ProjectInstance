#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <signal.h>
#include <locale.h>
#include <syslog.h>
#include <sys/un.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <xlib/xlib/config.h>

#include <xlib/xlib/xmem.h>
#include <xlib/xlib/xmain.h>
#include <xlib/xlib/xstring.h>
#include <xlib/xlib/xthread.h>
#include <xlib/xlib/xalloca.h>
#include <xlib/xlib/xcharset.h>
#include <xlib/xlib/xenviron.h>
#include <xlib/xlib/xpattern.h>
#include <xlib/xlib/xconvert.h>
#include <xlib/xlib/xlib-init.h>
#include <xlib/xlib/xstrfuncs.h>
#include <xlib/xlib/xmessages.h>
#include <xlib/xlib/xbacktrace.h>
#include <xlib/xlib/xtestutils.h>
#include <xlib/xlib/xthreadprivate.h>
#include <xlib/xlib/xutilsprivate.h>

typedef struct _XLogDomain XLogDomain;
typedef struct _XLogHandler XLogHandler;

struct _XLogDomain {
    xchar          *log_domain;
    XLogLevelFlags fatal_mask;
    XLogHandler    *handlers;
    XLogDomain     *next;
};

struct _XLogHandler {
    xuint          id;
    XLogLevelFlags log_level;
    XLogFunc       log_func;
    xpointer       data;
    XDestroyNotify destroy;
    XLogHandler    *next;
};

static void x_default_print_func(const xchar *string);
static void x_default_printerr_func(const xchar *string);

static XPrivate x_log_depth;
static XMutex x_messages_lock;
static xpointer fatal_log_data;
static XPrivate x_log_structured_depth;
static XLogDomain *x_log_domains = NULL;
static xpointer default_log_data = NULL;
static xpointer log_writer_user_data = NULL;
static xboolean x_log_debug_enabled = FALSE;
static XTestLogFatalFunc fatal_log_func = NULL;
static XDestroyNotify log_writer_user_data_free = NULL;
static XLogFunc default_log_func = x_log_default_handler;
static XLogWriterFunc log_writer_func = x_log_writer_default;
static XPrintFunc xlib_print_func = x_default_print_func;
static XPrintFunc xlib_printerr_func = x_default_printerr_func;

extern int mkostemp(char *temp, int flags);
static inline const char *format_string(const char *format, va_list args, char **out_allocated_string) X_GNUC_PRINTF(1, 0);
static inline FILE *log_level_to_file(XLogLevelFlags log_level);

static void _x_log_abort(xboolean breakpoint)
{
    xboolean debugger_present;

    if (x_test_subprocess()) {
        _exit (1);
    }

    debugger_present = TRUE;
    if (debugger_present && breakpoint) {
        X_BREAKPOINT();
    } else {
        x_abort();
    }
}

static void write_string(FILE *stream, const xchar *string)
{
    if (fputs(string, stream) == EOF) {

    }
}

static void write_string_sized(FILE *stream, const xchar *string, xssize length)
{
    if (length < 0) {
        write_string(stream, string);
    } else if (fwrite(string, 1, length, stream) < (size_t)length) {

    }
}

static XLogDomain *x_log_find_domain_L(const xchar *log_domain)
{
    XLogDomain *domain;

    domain = x_log_domains;
    while (domain) {
        if (strcmp(domain->log_domain, log_domain) == 0) {
            return domain;
        }

        domain = domain->next;
    }

    return NULL;
}

static XLogDomain *x_log_domain_new_L(const xchar *log_domain)
{
    XLogDomain *domain;

    domain = x_new(XLogDomain, 1);
    domain->log_domain = x_strdup(log_domain);
    domain->fatal_mask = (XLogLevelFlags)X_LOG_FATAL_MASK;
    domain->handlers = NULL;
    
    domain->next = x_log_domains;
    x_log_domains = domain;

    return domain;
}

static void x_log_domain_check_free_L(XLogDomain *domain)
{
    if ((domain->fatal_mask == X_LOG_FATAL_MASK) && (domain->handlers == NULL)) {
        XLogDomain *last, *work;

        last = NULL;
        work = x_log_domains;

        while (work) {
            if (work == domain) {
                if (last) {
                    last->next = domain->next;
                } else {
                    x_log_domains = domain->next;
                }

                x_free(domain->log_domain);
                x_free(domain);
                break;
            }

            last = work;
            work = last->next;
        }
    }
}

static XLogFunc x_log_domain_get_handler_L(XLogDomain *domain, XLogLevelFlags log_level, xpointer *data)
{
    if (domain && log_level) {
        XLogHandler *handler;

        handler = domain->handlers;
        while (handler) {
            if ((handler->log_level & log_level) == log_level) {
                *data = handler->data;
                return handler->log_func;
            }

            handler = handler->next;
        }
    }

    *data = default_log_data;
    return default_log_func;
}

XLogLevelFlags x_log_set_always_fatal(XLogLevelFlags fatal_mask)
{
    XLogLevelFlags old_mask;

    fatal_mask = (XLogLevelFlags)(fatal_mask & ((1 << X_LOG_LEVEL_USER_SHIFT) - 1));
    fatal_mask = (XLogLevelFlags)(fatal_mask | X_LOG_LEVEL_ERROR);
    fatal_mask = (XLogLevelFlags)(fatal_mask & ~X_LOG_FLAG_FATAL);

    x_mutex_lock(&x_messages_lock);
    old_mask = x_log_always_fatal;
    x_log_always_fatal = fatal_mask;
    x_mutex_unlock(&x_messages_lock);

    return old_mask;
}

XLogLevelFlags x_log_set_fatal_mask (const xchar *log_domain, XLogLevelFlags fatal_mask)
{
    XLogDomain *domain;
    XLogLevelFlags old_flags;

    if (!log_domain) {
        log_domain = "";
    }

    fatal_mask = (XLogLevelFlags)(fatal_mask | X_LOG_LEVEL_ERROR);
    fatal_mask = (XLogLevelFlags)(fatal_mask & ~X_LOG_FLAG_FATAL);

    x_mutex_lock(&x_messages_lock);
    domain = x_log_find_domain_L(log_domain);
    if (!domain) {
        domain = x_log_domain_new_L(log_domain);
    }
    old_flags = domain->fatal_mask;
    
    domain->fatal_mask = fatal_mask;
    x_log_domain_check_free_L(domain);
    x_mutex_unlock(&x_messages_lock);

    return old_flags;
}

xuint x_log_set_handler(const xchar *log_domain, XLogLevelFlags log_levels, XLogFunc log_func, xpointer user_data)
{
    return x_log_set_handler_full(log_domain, log_levels, log_func, user_data, NULL);
}

xuint x_log_set_handler_full(const xchar *log_domain, XLogLevelFlags log_levels, XLogFunc log_func, xpointer user_data, XDestroyNotify destroy)
{
    XLogDomain *domain;
    XLogHandler *handler;
    static xuint handler_id = 0;

    x_return_val_if_fail((log_levels & X_LOG_LEVEL_MASK) != 0, 0);
    x_return_val_if_fail(log_func != NULL, 0);

    if (!log_domain) {
        log_domain = "";
    }

    handler = x_new(XLogHandler, 1);

    x_mutex_lock(&x_messages_lock);
    domain = x_log_find_domain_L(log_domain);
    if (!domain) {
        domain = x_log_domain_new_L(log_domain);
    }

    handler->id = ++handler_id;
    handler->log_level = log_levels;
    handler->log_func = log_func;
    handler->data = user_data;
    handler->destroy = destroy;
    handler->next = domain->handlers;
    domain->handlers = handler;
    x_mutex_unlock(&x_messages_lock);

    return handler_id;
}

XLogFunc x_log_set_default_handler(XLogFunc log_func, xpointer user_data)
{
    XLogFunc old_log_func;

    x_mutex_lock(&x_messages_lock);
    old_log_func = default_log_func;
    default_log_func = log_func;
    default_log_data = user_data;
    x_mutex_unlock(&x_messages_lock);

    return old_log_func;
}

void x_test_log_set_fatal_handler(XTestLogFatalFunc log_func, xpointer user_data)
{
    x_mutex_lock(&x_messages_lock);
    fatal_log_func = log_func;
    fatal_log_data = user_data;
    x_mutex_unlock(&x_messages_lock);
}

void x_log_remove_handler(const xchar *log_domain, xuint handler_id)
{
    XLogDomain *domain;

    x_return_if_fail(handler_id > 0);

    if (!log_domain) {
        log_domain = "";
    }

    x_mutex_lock(&x_messages_lock);
    domain = x_log_find_domain_L(log_domain);
    if (domain) {
        XLogHandler *work, *last;

        last = NULL;
        work = domain->handlers;

        while (work) {
            if (work->id == handler_id) {
                if (last) {
                    last->next = work->next;
                } else {
                    domain->handlers = work->next;
                }

                x_log_domain_check_free_L(domain);
                x_mutex_unlock(&x_messages_lock);

                if (work->destroy) {
                    work->destroy (work->data);
                }

                x_free(work);
                return;
            }

            last = work;
            work = last->next;
        }
    }
    x_mutex_unlock(&x_messages_lock);

    x_warning("%s: could not find handler with id '%d' for domain \"%s\"", X_STRLOC, handler_id, log_domain);
}

#define CHAR_IS_SAFE(wc)        (!((wc < 0x20 && wc != '\t' && wc != '\n' && wc != '\r') || (wc == 0x7f) || (wc >= 0x80 && wc < 0xa0)))

static xchar *strdup_convert(const xchar *string, const xchar *charset)
{
    if (!x_utf8_validate (string, -1, NULL)) {
        xuchar *p;
        XString *gstring = x_string_new("[Invalid UTF-8] ");

        for (p = (xuchar *)string; *p; p++) {
            if (CHAR_IS_SAFE(*p) && !(*p == '\r' && *(p + 1) != '\n') && *p < 0x80) {
                x_string_append_c(gstring, *p);
            } else {
                x_string_append_printf(gstring, "\\x%02x", (xuint)(xuchar)*p);
            }
        }

        return x_string_free(gstring, FALSE);
    } else {
        XError *err = NULL;

        xchar *result = x_convert_with_fallback(string, -1, charset, "UTF-8", "?", NULL, NULL, &err);
        if (result) {
            return result;
        } else {
            static xboolean warned = FALSE; 
            if (!warned) {
                warned = TRUE;
                fprintf(stderr, "XLib: Cannot convert message: %s\n", err->message);
            }

            x_error_free(err);
            return x_strdup(string);
        }
    }
}

#define FORMAT_UNSIGNED_BUFSIZE         ((XLIB_SIZEOF_LONG * 3) + 3)

static void format_unsigned(xchar *buf, xulong num, xuint radix)
{
    xchar c;
    xint i, n;
    xulong tmp;

    if ((radix != 8) && (radix != 10) && (radix != 16)) {
        *buf = '\000';
        return;
    }

    if (!num) {
        *buf++ = '0';
        *buf = '\000';
        return;
    }

    if (radix == 16) {
        *buf++ = '0';
        *buf++ = 'x';
    } else if (radix == 8) {
        *buf++ = '0';
    }

    n = 0;
    tmp = num;
    while (tmp) {
        tmp /= radix;
        n++;
    }

    i = n;
    if (n > FORMAT_UNSIGNED_BUFSIZE - 3) {
        *buf = '\000';
        return;
    }

    while (num) {
        i--;
        c = (num % radix);
        if (c < 10) {
            buf[i] = c + '0';
        } else {
            buf[i] = c + 'a' - 10;
        }

        num /= radix;
    }

    buf[n] = '\000';
}

#define STRING_BUFFER_SIZE              (FORMAT_UNSIGNED_BUFSIZE + 32)

#define ALERT_LEVELS                    (X_LOG_LEVEL_ERROR | X_LOG_LEVEL_CRITICAL | X_LOG_LEVEL_WARNING)

#define DEFAULT_LEVELS                  (X_LOG_LEVEL_ERROR | X_LOG_LEVEL_CRITICAL | X_LOG_LEVEL_WARNING | X_LOG_LEVEL_MESSAGE)
#define INFO_LEVELS                     (X_LOG_LEVEL_INFO | X_LOG_LEVEL_DEBUG)

static const xchar *color_reset(xboolean use_color);
static const xchar *log_level_to_color(XLogLevelFlags log_level, xboolean use_color);

static xboolean xmessages_use_stderr = FALSE;

void x_log_writer_default_set_use_stderr(xboolean use_stderr)
{
    x_return_if_fail(x_thread_n_created() == 0);
    xmessages_use_stderr = use_stderr;
}

static FILE *mklevel_prefix(xchar level_prefix[STRING_BUFFER_SIZE], XLogLevelFlags log_level, xboolean use_color)
{
    strcpy(level_prefix, log_level_to_color(log_level, use_color));

    switch (log_level & X_LOG_LEVEL_MASK) {
        case X_LOG_LEVEL_ERROR:
            strcat(level_prefix, "ERROR");
            break;

        case X_LOG_LEVEL_CRITICAL:
            strcat(level_prefix, "CRITICAL");
            break;

        case X_LOG_LEVEL_WARNING:
            strcat(level_prefix, "WARNING");
            break;

        case X_LOG_LEVEL_MESSAGE:
            strcat(level_prefix, "Message");
            break;

        case X_LOG_LEVEL_INFO:
            strcat(level_prefix, "INFO");
            break;

        case X_LOG_LEVEL_DEBUG:
            strcat(level_prefix, "DEBUG");
            break;

        default:
            if (log_level) {
                strcat(level_prefix, "LOG-");
                format_unsigned(level_prefix + 4, log_level & X_LOG_LEVEL_MASK, 16);
            } else {
                strcat(level_prefix, "LOG");
            }
            break;
    }

    strcat(level_prefix, color_reset(use_color));

    if (log_level & X_LOG_FLAG_RECURSION) {
        strcat(level_prefix, " (recursed)");
    }

    if (log_level & ALERT_LEVELS) {
        strcat(level_prefix, " **");
    }

    return log_level_to_file(log_level);
}

typedef struct {
    xchar          *log_domain;
    XLogLevelFlags log_level;
    xchar          *pattern;
} XTestExpectedMessage;

static XSList *expected_messages = NULL;

void x_logv(const xchar *log_domain, XLogLevelFlags log_level, const xchar *format, va_list args)
{
    xint i;
    const char *msg;
    char buffer[1025], *msg_alloc = NULL;
    xboolean was_fatal = (log_level & X_LOG_FLAG_FATAL) != 0;
    xboolean was_recursion = (log_level & X_LOG_FLAG_RECURSION) != 0;

    log_level = (XLogLevelFlags)(log_level & X_LOG_LEVEL_MASK);
    if (!log_level) {
        return;
    }

    if (log_level & X_LOG_FLAG_RECURSION) {
        xsize size X_GNUC_UNUSED;

        size = vsnprintf(buffer, 1024, format, args);
        msg = buffer;
    } else {
        msg = format_string(format, args, &msg_alloc);
    }

    if (expected_messages) {
        XTestExpectedMessage *expected = (XTestExpectedMessage *)expected_messages->data;

        if ((x_strcmp0(expected->log_domain, log_domain) == 0) && ((log_level & expected->log_level) == expected->log_level) && x_pattern_match_simple(expected->pattern, msg)) {
            expected_messages = x_slist_delete_link(expected_messages, expected_messages);
            x_free(expected->log_domain);
            x_free(expected->pattern);
            x_free(expected);
            x_free(msg_alloc);

            return;
        } else if ((log_level & X_LOG_LEVEL_DEBUG) != X_LOG_LEVEL_DEBUG) {
            xchar *expected_message;
            xchar level_prefix[STRING_BUFFER_SIZE];

            mklevel_prefix(level_prefix, expected->log_level, FALSE);
            expected_message = x_strdup_printf("Did not see expected message %s-%s: %s", expected->log_domain ? expected->log_domain : "**", level_prefix, expected->pattern);
            x_log_default_handler(X_LOG_DOMAIN, X_LOG_LEVEL_CRITICAL, expected_message, NULL);
            x_free(expected_message);

            log_level = (XLogLevelFlags)(log_level | X_LOG_FLAG_FATAL);
        }
    }

    for (i = x_bit_nth_msf(log_level, -1); i >= 0; i = x_bit_nth_msf(log_level, i)) {
        XLogLevelFlags test_level;

        test_level = (XLogLevelFlags)(1L << i);
        if (log_level & test_level) {
            xuint depth;
            XLogFunc log_func;
            XLogDomain *domain;
            xpointer data = NULL;
            XLogLevelFlags domain_fatal_mask;
            xboolean masquerade_fatal = FALSE;

            if (was_fatal) {
                test_level = (XLogLevelFlags)(test_level | X_LOG_FLAG_FATAL);
            }

            if (was_recursion) {
                test_level = (XLogLevelFlags)(test_level | X_LOG_FLAG_RECURSION);
            }

            x_mutex_lock(&x_messages_lock);
            depth = XPOINTER_TO_UINT(x_private_get(&x_log_depth));
            domain = x_log_find_domain_L(log_domain ? log_domain : "");
            if (depth) {
                test_level = (XLogLevelFlags)(test_level | X_LOG_FLAG_RECURSION);
            }

            depth++;
            domain_fatal_mask = domain ? (XLogLevelFlags)domain->fatal_mask : (XLogLevelFlags)X_LOG_FATAL_MASK;
            if ((domain_fatal_mask | x_log_always_fatal) & test_level) {
                test_level = (XLogLevelFlags)(test_level | X_LOG_FLAG_FATAL);
            }

            if (test_level & X_LOG_FLAG_RECURSION) {
                log_func = _x_log_fallback_handler;
            } else {
                log_func = x_log_domain_get_handler_L(domain, test_level, &data);
            }

            domain = NULL;
            x_mutex_unlock(&x_messages_lock);

            x_private_set(&x_log_depth, XUINT_TO_POINTER(depth));

            log_func(log_domain, test_level, msg, data);

            if ((test_level & X_LOG_FLAG_FATAL) && !(test_level & X_LOG_LEVEL_ERROR)) {
                masquerade_fatal = fatal_log_func && !fatal_log_func(log_domain, test_level, msg, fatal_log_data);
            }

            if ((test_level & X_LOG_FLAG_FATAL) && !masquerade_fatal) {
                _x_log_abort(!(test_level & X_LOG_FLAG_RECURSION));
            }

            depth--;
            x_private_set(&x_log_depth, XUINT_TO_POINTER(depth));
        }
    }

    x_free(msg_alloc);
}

void x_log(const xchar *log_domain, XLogLevelFlags log_level, const xchar *format, ...)
{
    va_list args;

    va_start(args, format);
    x_logv(log_domain, log_level, format, args);
    va_end(args);
}

static const xchar *log_level_to_priority(XLogLevelFlags log_level)
{
    if (log_level & X_LOG_LEVEL_ERROR) {
        return "3";
    } else if (log_level & X_LOG_LEVEL_CRITICAL) {
        return "4";
    } else if (log_level & X_LOG_LEVEL_WARNING) {
        return "4";
    } else if (log_level & X_LOG_LEVEL_MESSAGE) {
        return "5";
    } else if (log_level & X_LOG_LEVEL_INFO) {
        return "6";
    } else if (log_level & X_LOG_LEVEL_DEBUG) {
        return "7";
    }

    return "5";
}

static int str_to_syslog_facility(const xchar *syslog_facility_str)
{
    int syslog_facility = LOG_USER;

    if (x_strcmp0(syslog_facility_str, "auth") == 0) {
        syslog_facility = LOG_AUTH;
    } else if (x_strcmp0(syslog_facility_str, "daemon") == 0) {
        syslog_facility = LOG_DAEMON;
    }

    return syslog_facility;
}

static inline FILE *log_level_to_file(XLogLevelFlags log_level)
{
    if (xmessages_use_stderr) {
        return stderr;
    }

    if (log_level & (X_LOG_LEVEL_ERROR | X_LOG_LEVEL_CRITICAL | X_LOG_LEVEL_WARNING | X_LOG_LEVEL_MESSAGE)) {
        return stderr;
    } else {
        return stdout;
    }
}

static const xchar *log_level_to_color(XLogLevelFlags log_level, xboolean use_color)
{
    if (!use_color) {
        return "";
    }

    if (log_level & X_LOG_LEVEL_ERROR) {
        return "\033[1;31m";                        /* red */
    } else if (log_level & X_LOG_LEVEL_CRITICAL) {
        return "\033[1;35m";                        /* magenta */
    } else if (log_level & X_LOG_LEVEL_WARNING) {
        return "\033[1;33m";                        /* yellow */
    } else if (log_level & X_LOG_LEVEL_MESSAGE) {
        return "\033[1;32m";                        /* green */
    } else if (log_level & X_LOG_LEVEL_INFO) {
        return "\033[1;32m";                        /* green */
    } else if (log_level & X_LOG_LEVEL_DEBUG) {
        return "\033[1;32m";                        /* green */
    }

    return "";
}

static const xchar *color_reset(xboolean use_color)
{
    if (!use_color) {
        return "";
    }

    return "\033[0m";
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"

void x_log_structured(const xchar *log_domain, XLogLevelFlags log_level, ...)
{
    xpointer p;
    va_list args;
    xsize n_fields, i;
    const char *format;
    const xchar *message;
    XArray *array = NULL;
    XLogField stack_fields[16];
    XLogField *fields = stack_fields;
    XLogField *fields_allocated = NULL;
    xchar buffer[1025], *message_allocated = NULL;

    va_start(args, log_level);

    n_fields = 2;
    if (log_domain) {
        n_fields++;
    }

    for (p = va_arg(args, xchar *), i = n_fields; strcmp((const char *)p, "MESSAGE") != 0; p = va_arg(args, xchar *), i++) {
        XLogField field;
        const xchar *key = (const xchar *)p;
        xconstpointer value = va_arg(args, xpointer);

        field.key = key;
        field.value = value;
        field.length = -1;

        if (i < 16) {
            stack_fields[i] = field;
        } else {
            if (log_level & X_LOG_FLAG_RECURSION) {
                continue;
            }

            if (i == 16) {
                array = x_array_sized_new(FALSE, FALSE, sizeof(XLogField), 32);
                x_array_append_vals(array, stack_fields, 16);
            }

            x_array_append_val(array, field);
        }
    }

    n_fields = i;
    if (array) {
        fields = fields_allocated = (XLogField *)x_array_free(array, FALSE);
    }
    format = va_arg(args, xchar *);

    if (log_level & X_LOG_FLAG_RECURSION) {
        xsize size X_GNUC_UNUSED;

        size = vsnprintf(buffer, sizeof(buffer), format, args);
        message = buffer;
    } else {
        message = format_string(format, args, &message_allocated);
    }

    fields[0].key = "MESSAGE";
    fields[0].value = message;
    fields[0].length = -1;

    fields[1].key = "PRIORITY";
    fields[1].value = log_level_to_priority(log_level);
    fields[1].length = -1;

    if (log_domain) {
        fields[2].key = "GLIB_DOMAIN";
        fields[2].value = log_domain;
        fields[2].length = -1;
    }

    x_log_structured_array(log_level, fields, n_fields);

    x_free(fields_allocated);
    x_free(message_allocated);

    va_end(args);
}

void x_log_variant(const xchar *log_domain, XLogLevelFlags log_level, XVariant *fields)
{
    xchar *key;
    XVariant *value;
    XLogField field;
    XVariantIter iter;
    XArray *fields_array;
    XSList *values_list, *print_list;

    x_return_if_fail(x_variant_is_of_type(fields, X_VARIANT_TYPE_VARDICT));

    values_list = print_list = NULL;
    fields_array = x_array_new(FALSE, FALSE, sizeof(XLogField));

    field.key = "PRIORITY";
    field.value = log_level_to_priority(log_level);
    field.length = -1;
    x_array_append_val(fields_array, field);

    if (log_domain) {
        field.key = "XLIB_DOMAIN";
        field.value = log_domain;
        field.length = -1;
        x_array_append_val(fields_array, field);
    }

    x_variant_iter_init(&iter, fields);
    while (x_variant_iter_next(&iter, "{&sv}", &key, &value)) {
        xboolean defer_unref = TRUE;

        field.key = key;
        field.length = -1;

        if (x_variant_is_of_type(value, X_VARIANT_TYPE_STRING)) {
            field.value = x_variant_get_string(value, NULL);
        } else if (x_variant_is_of_type(value, X_VARIANT_TYPE_BYTESTRING)) {
            xsize s;

            field.value = x_variant_get_fixed_array (value, &s, sizeof (xuchar));
            if (X_LIKELY(s <= X_MAXSSIZE)) {
                field.length = s;
            } else {
                fprintf(stderr, "Byte array too large (%" X_XSIZE_FORMAT " bytes) passed to x_log_variant(). Truncating to " X_STRINGIFY(X_MAXSSIZE) " bytes.", s);
                field.length = X_MAXSSIZE;
            }
        } else {
            char *s = x_variant_print(value, FALSE);
            field.value = s;
            print_list = x_slist_prepend(print_list, s);
            defer_unref = FALSE;
        }

        x_array_append_val(fields_array, field);

        if (X_LIKELY(defer_unref)) {
            values_list = x_slist_prepend(values_list, value);
        } else {
            x_variant_unref(value);
        }
    }

    x_log_structured_array(log_level, (XLogField *)fields_array->data, fields_array->len);

    x_array_free(fields_array, TRUE);
    x_slist_free_full(values_list, (XDestroyNotify)x_variant_unref);
    x_slist_free_full(print_list, x_free);
}

#pragma GCC diagnostic pop

static XLogWriterOutput _x_log_writer_fallback(XLogLevelFlags log_level, const XLogField *fields, xsize n_fields, xpointer user_data);

void x_log_structured_array(XLogLevelFlags log_level, const XLogField *fields, xsize n_fields)
{
    xuint depth;
    xboolean recursion;
    xpointer writer_user_data;
    XLogWriterFunc writer_func;

    if (n_fields == 0) {
        return;
    }

    depth = XPOINTER_TO_UINT(x_private_get(&x_log_structured_depth));
    recursion = (depth > 0);

    x_mutex_lock(&x_messages_lock);
    writer_func = recursion ? _x_log_writer_fallback : log_writer_func;
    writer_user_data = log_writer_user_data;
    x_mutex_unlock(&x_messages_lock);

    x_private_set(&x_log_structured_depth, XUINT_TO_POINTER(++depth));

    x_assert(writer_func != NULL);
    writer_func (log_level, fields, n_fields, writer_user_data);

    x_private_set(&x_log_structured_depth, XUINT_TO_POINTER(--depth));

    if (log_level & X_LOG_FATAL_MASK) {
        _x_log_abort(!(log_level & X_LOG_FLAG_RECURSION));
    }
}

void x_log_structured_standard(const xchar *log_domain, XLogLevelFlags log_level, const xchar *file, const xchar *line, const xchar *func, const xchar *message_format, ...)
{
    XLogField fields[] = {
        { "PRIORITY",    log_level_to_priority(log_level), -1 },
        { "CODE_FILE",   file, -1 },
        { "CODE_LINE",   line, -1 },
        { "CODE_FUNC",   func, -1 },
        { "MESSAGE",     NULL, -1 },
        { "XLIB_DOMAIN", log_domain, -1 },
    };

    va_list args;
    xsize n_fields;
    xchar buffer[1025];
    xchar *message_allocated = NULL;

    va_start(args, message_format);
    if (log_level & X_LOG_FLAG_RECURSION) {
        xsize size X_GNUC_UNUSED;

        size = vsnprintf(buffer, sizeof(buffer), message_format, args);
        fields[4].value = buffer;
    } else {
        fields[4].value = format_string(message_format, args, &message_allocated);
    }
    va_end(args);

    n_fields = X_N_ELEMENTS(fields) - ((log_domain == NULL) ? 1 : 0);
    x_log_structured_array(log_level, fields, n_fields);

    x_free(message_allocated);
}

void x_log_set_writer_func(XLogWriterFunc func, xpointer user_data, XDestroyNotify user_data_free)
{
    x_return_if_fail(func != NULL);

    x_mutex_lock(&x_messages_lock);

    if (log_writer_func != x_log_writer_default) {
        x_mutex_unlock(&x_messages_lock);
        x_error("x_log_set_writer_func() called multiple times");
        return;
    }

    log_writer_func = func;
    log_writer_user_data = user_data;
    log_writer_user_data_free = user_data_free;

    x_mutex_unlock(&x_messages_lock);
}

xboolean x_log_writer_supports_color(xint output_fd)
{
    x_return_val_if_fail(output_fd >= 0, FALSE);
    return isatty(output_fd);
}

static int journal_fd = -1;
static xboolean syslog_opened = FALSE;

#ifndef SOCK_CLOEXEC
#define SOCK_CLOEXEC            0
#else
#define HAVE_SOCK_CLOEXEC       1
#endif

static void open_journal (void)
{
    if ((journal_fd = socket(AF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC, 0)) < 0) {
        return;
    }

#ifndef HAVE_SOCK_CLOEXEC
    if (fcntl(journal_fd, F_SETFD, FD_CLOEXEC) < 0) {
        close(journal_fd);
        journal_fd = -1;
    }
#endif
}

#include <string.h>
#include <sys/un.h>
#include <sys/socket.h>

static int journal_str_has_prefix(const char *str, const char *prefix)
{
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

static int _x_fd_is_journal(int output_fd)
{
    union {
        struct sockaddr_storage storage;
        struct sockaddr         sa;
        struct sockaddr_un      un;
    } addr;

    int err;
    socklen_t addr_len;

    if (output_fd < 0) {
        return 0;
    }

    memset (&addr, 0, sizeof (addr));
    addr_len = sizeof(addr);
    err = getpeername(output_fd, &addr.sa, &addr_len);
    if ((err == 0) && (addr.storage.ss_family == AF_UNIX)) {
        return (journal_str_has_prefix(addr.un.sun_path, "/run/systemd/journal/") || journal_str_has_prefix(addr.un.sun_path, "/run/systemd/journal."));
    }

    return 0;
}

xboolean x_log_writer_is_journald(xint output_fd)
{
    return _x_fd_is_journal(output_fd);
}

static void escape_string(XString *string);

xchar *x_log_writer_format_fields(XLogLevelFlags log_level, const XLogField *fields, xsize n_fields, xboolean use_color)
{
    xsize i;
    xint64 now;
    time_t now_secs;
    XString *gstring;
    struct tm now_tm;
    xchar time_buf[128];
    xssize message_length = -1;
    const xchar *message = NULL;
    xssize log_domain_length = -1;
    const xchar *log_domain = NULL;
    xchar level_prefix[STRING_BUFFER_SIZE];

    for (i = 0; (message == NULL || log_domain == NULL) && i < n_fields; i++) {
        const XLogField *field = &fields[i];

        if (x_strcmp0(field->key, "MESSAGE") == 0) {
            message = (const xchar *)field->value;
            message_length = field->length;
        } else if (x_strcmp0(field->key, "XLIB_DOMAIN") == 0) {
            log_domain = (const xchar *)field->value;
            log_domain_length = field->length;
        }
    }

    mklevel_prefix(level_prefix, log_level, use_color);

    gstring = x_string_new(NULL);
    if (log_level & ALERT_LEVELS) {
        x_string_append(gstring, "\n");
    }

    if (!log_domain) {
        x_string_append(gstring, "** ");
    }

    if ((x_log_msg_prefix & (log_level & X_LOG_LEVEL_MASK)) == (log_level & X_LOG_LEVEL_MASK)) {
        xulong pid = getpid();
        const xchar *prg_name = x_get_prgname();

        if (prg_name == NULL) {
            x_string_append_printf(gstring, "(process:%lu): ", pid);
        } else {
            x_string_append_printf(gstring, "(%s:%lu): ", prg_name, pid);
        }
    }

    if (log_domain != NULL) {
        x_string_append_len(gstring, log_domain, log_domain_length);
        x_string_append_c(gstring, '-');
    }

    x_string_append(gstring, level_prefix);
    x_string_append(gstring, ": ");

    now = x_get_real_time();
    now_secs = (time_t)(now / 1000000);
    if (_x_localtime(now_secs, &now_tm)) {
        strftime(time_buf, sizeof(time_buf), "%H:%M:%S", &now_tm);
    } else {
        strcpy(time_buf, "(error)");
    }

    x_string_append_printf(gstring, "%s%s.%03d%s: ", use_color ? "\033[34m" : "", time_buf, (xint)((now / 1000) % 1000), color_reset(use_color));

    if (message == NULL) {
        x_string_append(gstring, "(NULL) message");
    } else {
        XString *msg;
        const xchar *charset;

        msg = x_string_new_len(message, message_length);
        escape_string(msg);

        if (x_get_console_charset(&charset)) {
            x_string_append(gstring, msg->str);
        } else {
            xchar *lstring = strdup_convert(msg->str, charset);
            x_string_append(gstring, lstring);
            x_free(lstring);
        }

        x_string_free(msg, TRUE);
    }

    return x_string_free(gstring, FALSE);
}

XLogWriterOutput x_log_writer_syslog(XLogLevelFlags log_level, const XLogField *fields, xsize n_fields, xpointer user_data)
{
    xsize i;
    int syslog_level;
    XString *gstring;
    int syslog_facility = 0;
    const char *message = NULL;
    xssize message_length = -1;
    const char *log_domain = NULL;
    xssize log_domain_length = -1;

    x_return_val_if_fail(fields != NULL, X_LOG_WRITER_UNHANDLED);
    x_return_val_if_fail(n_fields > 0, X_LOG_WRITER_UNHANDLED);

    if (!syslog_opened) {
        openlog(NULL, 0, 0);
        syslog_opened = TRUE;
    }

    for (i = 0; i < n_fields; i++) {
        const XLogField *field = &fields[i];

        if (x_strcmp0(field->key, "MESSAGE") == 0) {
            message = field->value;
            message_length = field->length;
        } else if (x_strcmp0(field->key, "XLIB_DOMAIN") == 0) {
            log_domain = field->value;
            log_domain_length = field->length;
        } else if (x_strcmp0(field->key, "SYSLOG_FACILITY") == 0) {
            syslog_facility = str_to_syslog_facility(field->value);
        }
    }

    gstring = x_string_new(NULL);

    if (log_domain != NULL) {
        x_string_append_len(gstring, log_domain, log_domain_length);
        x_string_append(gstring, ": ");
    }

    x_string_append_len(gstring, message, message_length);

    syslog_level = atoi(log_level_to_priority(log_level));
    syslog(syslog_level | syslog_facility, "%s", gstring->str);

    x_string_free(gstring, TRUE);

    return X_LOG_WRITER_HANDLED;
}

#if defined(__linux__) && !defined(__BIONIC__) && defined(HAVE_MKOSTEMP) && defined(O_CLOEXEC)
#define ENABLE_JOURNAL_SENDV
#endif

#ifdef ENABLE_JOURNAL_SENDV
static int journal_sendv(struct iovec *iov, xsize iovlen)
{
    int buf_fd = -1;
    struct msghdr mh;
    struct sockaddr_un sa;
    union {
        struct cmsghdr cmsghdr;
        xuint8 buf[CMSG_SPACE(sizeof(int))];
    } control;
    struct cmsghdr *cmsg;
    char path[] = "/dev/shm/journal.XXXXXX";

    if (journal_fd < 0) {
        open_journal();
    }

    if (journal_fd < 0) {
        return -1;
    }

    memset(&sa, 0, sizeof (sa));
    sa.sun_family = AF_UNIX;
    if (x_strlcpy(sa.sun_path, "/run/systemd/journal/socket", sizeof(sa.sun_path)) >= sizeof(sa.sun_path)) {
        return -1;
    }

    memset(&mh, 0, sizeof (mh));
    mh.msg_name = &sa;
    mh.msg_namelen = offsetof(struct sockaddr_un, sun_path) + strlen(sa.sun_path);
    mh.msg_iov = iov;
    mh.msg_iovlen = iovlen;

retry:
    if (sendmsg(journal_fd, &mh, MSG_NOSIGNAL) >= 0) {
        return 0;
    }

    if (errno == EINTR) {
        goto retry;
    }

    if (errno != EMSGSIZE && errno != ENOBUFS) {
        return -1;
    }

    if ((buf_fd = mkostemp(path, O_CLOEXEC | O_RDWR)) < 0) {
        return -1;
    }

    if (unlink(path) < 0) {
        close(buf_fd);
        return -1;
    }

    if (writev(buf_fd, iov, iovlen) < 0) {
        close(buf_fd);
        return -1;
    }

    mh.msg_iov = NULL;
    mh.msg_iovlen = 0;

    memset(&control, 0, sizeof(control));
    mh.msg_control = &control;
    mh.msg_controllen = sizeof(control);

    cmsg = CMSG_FIRSTHDR (&mh);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));
    memcpy(CMSG_DATA(cmsg), &buf_fd, sizeof(int));

    mh.msg_controllen = cmsg->cmsg_len;

retry2:
    if (sendmsg(journal_fd, &mh, MSG_NOSIGNAL) >= 0) {
        return 0;
    }

    if (errno == EINTR) {
        goto retry2;
    }

    return -1;
}
#endif

XLogWriterOutput x_log_writer_journald(XLogLevelFlags log_level, const XLogField *fields, xsize n_fields, xpointer user_data)
{
#ifdef ENABLE_JOURNAL_SENDV
    char *buf;
    xsize i, k;
    xint retval;
    struct iovec *iov, *v;
    const char equals = '=';
    const char newline = '\n';

    x_return_val_if_fail(fields != NULL, X_LOG_WRITER_UNHANDLED);
    x_return_val_if_fail(n_fields > 0, X_LOG_WRITER_UNHANDLED);

    iov = (struct iovec *)x_alloca(sizeof(struct iovec) * 5 * n_fields);
    buf = (char *)x_alloca(32 * n_fields);

    k = 0;
    v = iov;

    for (i = 0; i < n_fields; i++) {
        xuint64 length;
        xboolean binary;

        if (fields[i].length < 0) {
            length = strlen((const char *)fields[i].value);
            binary = strchr((const char *)fields[i].value, '\n') != NULL;
        } else {
            length = fields[i].length;
            binary = TRUE;
        }

        if (binary) {
            xuint64 nstr;

            v[0].iov_base = (xpointer)fields[i].key;
            v[0].iov_len = strlen(fields[i].key);

            v[1].iov_base = (xpointer)&newline;
            v[1].iov_len = 1;

            nstr = XUINT64_TO_LE(length);
            memcpy(&buf[k], &nstr, sizeof (nstr));

            v[2].iov_base = &buf[k];
            v[2].iov_len = sizeof (nstr);
            v += 3;
            k += sizeof (nstr);
        } else {
            v[0].iov_base = (xpointer)fields[i].key;
            v[0].iov_len = strlen(fields[i].key);

            v[1].iov_base = (xpointer)&equals;
            v[1].iov_len = 1;
            v += 2;
        }

        v[0].iov_base = (xpointer)fields[i].value;
        v[0].iov_len = length;

        v[1].iov_base = (xpointer)&newline;
        v[1].iov_len = 1;
        v += 2;
    }

    retval = journal_sendv(iov, v - iov);
    return retval == 0 ? X_LOG_WRITER_HANDLED : X_LOG_WRITER_UNHANDLED;
#else
    return X_LOG_WRITER_UNHANDLED;
#endif
}

XLogWriterOutput x_log_writer_standard_streams(XLogLevelFlags log_level, const XLogField *fields, xsize n_fields, xpointer user_data)
{
    FILE *stream;
    xchar *out = NULL;

    x_return_val_if_fail(fields != NULL, X_LOG_WRITER_UNHANDLED);
    x_return_val_if_fail(n_fields > 0, X_LOG_WRITER_UNHANDLED);

    stream = log_level_to_file(log_level);
    if (!stream || fileno(stream) < 0) {
        return X_LOG_WRITER_UNHANDLED;
    }

    out = x_log_writer_format_fields(log_level, fields, n_fields, x_log_writer_supports_color(fileno(stream)));
    fprintf(stream, "%s\n", out);
    fflush(stream);
    x_free(out);

    return X_LOG_WRITER_HANDLED;
}

static xboolean log_is_old_api(const XLogField *fields, xsize n_fields)
{
    return (n_fields >= 1 && x_strcmp0(fields[0].key, "XLIB_OLD_LOG_API") == 0 && x_strcmp0((const char *)fields[0].value, "1") == 0);
}

static xboolean domain_found(const xchar *domains, const char *log_domain)
{
    xuint len;
    const xchar *found;

    len = strlen(log_domain);
    for (found = strstr(domains, log_domain); found; found = strstr(found + 1, log_domain)) {
        if ((found == domains || found[-1] == ' ') && (found[len] == 0 || found[len] == ' ')) {
            return TRUE;
        }
    }

    return FALSE;
}

static struct {
    XRWLock  lock;
    xchar    *domains;
    xboolean domains_set;
} x_log_global;

void x_log_writer_default_set_debug_domains(const xchar *const *domains)
{
    x_rw_lock_writer_lock(&x_log_global.lock);

    x_free(x_log_global.domains);
    x_log_global.domains = domains ? x_strjoinv(" ", (xchar **)domains) : NULL;
    x_log_global.domains_set = TRUE;

    x_rw_lock_writer_unlock(&x_log_global.lock);
}

static xboolean should_drop_message(XLogLevelFlags log_level, const char *log_domain, const XLogField *fields, xsize n_fields)
{
    if (!(log_level & DEFAULT_LEVELS) && !(log_level >> X_LOG_LEVEL_USER_SHIFT) && !x_log_get_debug_enabled()) {
        xsize i;

        x_rw_lock_reader_lock(&x_log_global.lock);
        if (X_UNLIKELY(!x_log_global.domains_set)) {
            x_log_global.domains = x_strdup(x_getenv("X_MESSAGES_DEBUG"));
            x_log_global.domains_set = TRUE;
        }

        if ((log_level & INFO_LEVELS) == 0 || x_log_global.domains == NULL) {
            x_rw_lock_reader_unlock(&x_log_global.lock);
            return TRUE;
        }

        if (log_domain == NULL) {
            for (i = 0; i < n_fields; i++) {
                if (x_strcmp0(fields[i].key, "XLIB_DOMAIN") == 0) {
                    log_domain = (const char *)fields[i].value;
                    break;
                }
            }
        }

        if (strcmp(x_log_global.domains, "all") != 0 && (log_domain == NULL || !domain_found(x_log_global.domains, log_domain))) {
            x_rw_lock_reader_unlock(&x_log_global.lock);
            return TRUE;
        }

        x_rw_lock_reader_unlock(&x_log_global.lock);
    }

    return FALSE;
}

xboolean x_log_writer_default_would_drop(XLogLevelFlags log_level, const char *log_domain)
{
    return should_drop_message(log_level, log_domain, NULL, 0);
}

XLogWriterOutput x_log_writer_default(XLogLevelFlags log_level, const XLogField *fields, xsize n_fields, xpointer user_data)
{
    static xsize initialized = 0;
    static xboolean stderr_is_journal = FALSE;

    x_return_val_if_fail(fields != NULL, X_LOG_WRITER_UNHANDLED);
    x_return_val_if_fail(n_fields > 0, X_LOG_WRITER_UNHANDLED);

    if (should_drop_message(log_level, NULL, fields, n_fields)) {
        return X_LOG_WRITER_HANDLED;
    }

    if ((log_level & x_log_always_fatal) && !log_is_old_api(fields, n_fields)) {
        log_level = (XLogLevelFlags)(log_level | X_LOG_FLAG_FATAL);
    }

    if (x_once_init_enter(&initialized)) {
        stderr_is_journal = x_log_writer_is_journald(fileno(stderr));
        x_once_init_leave(&initialized, TRUE);
    }

    if (stderr_is_journal && x_log_writer_journald(log_level, fields, n_fields, user_data) == X_LOG_WRITER_HANDLED) {
        goto handled;
    }

    if (x_log_writer_standard_streams(log_level, fields, n_fields, user_data) == X_LOG_WRITER_HANDLED) {
        goto handled;
    }

    return X_LOG_WRITER_UNHANDLED;

handled:
    if (log_level & X_LOG_FLAG_FATAL) {
        _x_log_abort(!(log_level & X_LOG_FLAG_RECURSION));
    }

    return X_LOG_WRITER_HANDLED;
}

static XLogWriterOutput _x_log_writer_fallback (XLogLevelFlags log_level, const XLogField *fields, xsize n_fields, xpointer user_data)
{
    xsize i;
    FILE *stream;

    stream = log_level_to_file(log_level);

    for (i = 0; i < n_fields; i++) {
        const XLogField *field = &fields[i];

        if (strcmp(field->key, "MESSAGE") != 0
            && strcmp(field->key, "MESSAGE_ID") != 0
            && strcmp(field->key, "PRIORITY") != 0
            && strcmp(field->key, "CODE_FILE") != 0
            && strcmp(field->key, "CODE_LINE") != 0
            && strcmp(field->key, "CODE_FUNC") != 0
            && strcmp(field->key, "ERRNO") != 0
            && strcmp(field->key, "SYSLOG_FACILITY") != 0
            && strcmp(field->key, "SYSLOG_IDENTIFIER") != 0
            && strcmp(field->key, "SYSLOG_PID") != 0
            && strcmp(field->key, "XLIB_DOMAIN") != 0)
        {
            continue;
        }

        write_string(stream, field->key);
        write_string(stream, "=");
        write_string_sized(stream, (const xchar *)field->value, field->length);
    }

    {
        xchar pid_string[FORMAT_UNSIGNED_BUFSIZE];

        format_unsigned(pid_string, getpid(), 10);
        write_string(stream, "_PID=");
        write_string(stream, pid_string);
    }

    return X_LOG_WRITER_HANDLED;
}

xboolean x_log_get_debug_enabled(void)
{
    return x_atomic_int_get(&x_log_debug_enabled);
}

void x_log_set_debug_enabled(xboolean enabled)
{
    x_atomic_int_set(&x_log_debug_enabled, enabled);
}

void x_return_if_fail_warning(const char *log_domain, const char *pretty_function, const char *expression)
{
    x_log(log_domain, X_LOG_LEVEL_CRITICAL, "%s: assertion '%s' failed", pretty_function, expression);
}

void x_warn_message(const char *domain, const char *file, int line, const char *func, const char *warnexpr)
{
    char *s, lstr[32];

    x_snprintf(lstr, 32, "%d", line);
    if (warnexpr) {
        s = x_strconcat("(", file, ":", lstr, "):", func, func[0] ? ":" : "", " runtime check failed: (", warnexpr, ")", NULL);
    } else {
        s = x_strconcat("(", file, ":", lstr, "):", func, func[0] ? ":" : "", " ", "code should not be reached", NULL);
    }

    x_log(domain, X_LOG_LEVEL_WARNING, "%s", s);
    x_free(s);
}

void x_assert_warning(const char *log_domain, const char *file, const int line, const char *pretty_function, const char *expression)
{
    if (expression) {
        x_log(log_domain, X_LOG_LEVEL_ERROR, "file %s: line %d (%s): assertion failed: (%s)", file, line, pretty_function, expression);
    } else {
        x_log(log_domain, X_LOG_LEVEL_ERROR, "file %s: line %d (%s): should not be reached", file, line, pretty_function);
    }

    _x_log_abort(FALSE);
    x_abort();
}

void x_test_expect_message(const xchar *log_domain, XLogLevelFlags log_level, const xchar *pattern)
{
    XTestExpectedMessage *expected;

    x_return_if_fail(log_level != 0);
    x_return_if_fail(pattern != NULL);
    x_return_if_fail(~log_level & X_LOG_LEVEL_ERROR);

    expected = x_new(XTestExpectedMessage, 1);
    expected->log_domain = x_strdup(log_domain);
    expected->log_level = log_level;
    expected->pattern = x_strdup(pattern);

    expected_messages = x_slist_append(expected_messages, expected);
}

void x_test_assert_expected_messages_internal(const char *domain, const char *file, int line, const char *func)
{
    if (expected_messages) {
        xchar *message;
        XTestExpectedMessage *expected;
        xchar level_prefix[STRING_BUFFER_SIZE];

        expected = (XTestExpectedMessage *)expected_messages->data;

        mklevel_prefix(level_prefix, expected->log_level, FALSE);
        message = x_strdup_printf("Did not see expected message %s-%s: %s", expected->log_domain ? expected->log_domain : "**", level_prefix, expected->pattern);
        x_assertion_message(X_LOG_DOMAIN, file, line, func, message);
        x_free(message);
    }
}

void _x_log_fallback_handler(const xchar *log_domain, XLogLevelFlags log_level, const xchar *message, xpointer unused_data)
{
    FILE *stream;
    xchar level_prefix[STRING_BUFFER_SIZE];
    xchar pid_string[FORMAT_UNSIGNED_BUFSIZE];

    stream = mklevel_prefix(level_prefix, log_level, FALSE);
    if (!message) {
        message = "(NULL) message";
    }

    format_unsigned(pid_string, getpid(), 10);

    if (log_domain) {
        write_string(stream, "\n");
    } else {
        write_string(stream, "\n** ");
    }

    write_string(stream, "(process:");
    write_string(stream, pid_string);
    write_string(stream, "): ");

    if (log_domain) {
        write_string(stream, log_domain);
        write_string(stream, "-");
    }

    write_string(stream, level_prefix);
    write_string(stream, ": ");
    write_string(stream, message);
    write_string(stream, "\n");
}

static void escape_string(XString *string)
{
    xunichar wc;
    const char *p = string->str;

    while (p < string->str + string->len) {
        xboolean safe;

        wc = x_utf8_get_char_validated(p, -1);
        if (wc == (xunichar)-1 || wc == (xunichar)-2) {
            xuint pos;
            xchar *tmp;

            pos = p - string->str;

            tmp = x_strdup_printf("\\x%02x", (xuint)(xuchar)*p);
            x_string_erase(string, pos, 1);
            x_string_insert(string, pos, tmp);

            p = string->str + (pos + 4);

            x_free(tmp);
            continue;
        }

        if (wc == '\r') {
            safe = *(p + 1) == '\n';
        } else {
            safe = CHAR_IS_SAFE(wc);
        }

        if (!safe) {
            xuint pos;
            xchar *tmp;

            pos = p - string->str;

            tmp = x_strdup_printf("\\u%04x", wc); 
            x_string_erase(string, pos, x_utf8_next_char(p) - p);
            x_string_insert(string, pos, tmp);
            x_free(tmp);

            p = string->str + (pos + 6);
        } else {
            p = x_utf8_next_char(p);
        }
    }
}

void x_log_default_handler(const xchar *log_domain, XLogLevelFlags log_level, const xchar *message, xpointer unused_data)
{
    int n_fields = 0;
    XLogField fields[4];

    if (log_level & X_LOG_FLAG_RECURSION) {
        _x_log_fallback_handler(log_domain, log_level, message, unused_data);
        return;
    }

    fields[0].key = "XLIB_OLD_LOG_API";
    fields[0].value = "1";
    fields[0].length = -1;
    n_fields++;

    fields[1].key = "MESSAGE";
    fields[1].value = message;
    fields[1].length = -1;
    n_fields++;

    fields[2].key = "PRIORITY";
    fields[2].value = log_level_to_priority(log_level);
    fields[2].length = -1;
    n_fields++;

    if (log_domain) {
        fields[3].key = "XLIB_DOMAIN";
        fields[3].value = log_domain;
        fields[3].length = -1;
        n_fields++;
    }

    x_log_structured_array((XLogLevelFlags)(log_level & ~X_LOG_FLAG_FATAL), fields, n_fields);
}

XPrintFunc x_set_print_handler(XPrintFunc func)
{
    return x_atomic_pointer_exchange(&xlib_print_func, func ? func : x_default_print_func);
}

static void print_string(FILE *stream, const xchar *string)
{
    int ret;
    const xchar *charset;

    if (x_get_console_charset(&charset)) {
        ret = fputs(string, stream);
    } else {
        xchar *converted_string = strdup_convert(string, charset);

        ret = fputs(converted_string, stream);
        x_free(converted_string);
    }

    if (ret == EOF) {
        return;
    }

    fflush(stream);
}

X_ALWAYS_INLINE static inline const char *format_string(const char *format, va_list args, char **out_allocated_string)
{
#ifdef X_ENABLE_DEBUG
    x_assert(out_allocated_string != NULL);
#endif

    if (strchr (format, '%') == NULL) {
        *out_allocated_string = NULL;
        return format;
    } else {
        *out_allocated_string = x_strdup_vprintf(format, args);
        return *out_allocated_string;
    }
}

static void x_default_print_func(const xchar *string)
{
    print_string(stdout, string);
}

static void x_default_printerr_func(const xchar *string)
{
    print_string(stderr, string);
}

void x_print(const xchar *format, ...)
{
    va_list args;
    const xchar *string;
    xchar *free_me = NULL;
    XPrintFunc local_xlib_print_func;

    x_return_if_fail(format != NULL);

    va_start(args, format);
    string = format_string(format, args, &free_me);
    va_end(args);

    local_xlib_print_func = x_atomic_pointer_get(&xlib_print_func);
    local_xlib_print_func(string);

    x_free(free_me);
}

XPrintFunc x_set_printerr_handler(XPrintFunc func)
{
    return x_atomic_pointer_exchange(&xlib_printerr_func, func ? func : x_default_printerr_func);
}

void x_printerr(const xchar *format, ...)
{
    va_list args;
    const char *string;
    char *free_me = NULL;
    XPrintFunc local_xlib_printerr_func;

    x_return_if_fail(format != NULL);

    va_start(args, format);
    string = format_string(format, args, &free_me);
    va_end(args);

    local_xlib_printerr_func = x_atomic_pointer_get(&xlib_printerr_func);
    local_xlib_printerr_func(string);

    x_free(free_me);
}

xsize x_printf_string_upper_bound(const xchar *format, va_list args)
{
    xchar c;
    int count = x_vsnprintf(&c, 1, format, args);

    if (count < 0) {
        return 0;
    }

    return count + 1;
}
