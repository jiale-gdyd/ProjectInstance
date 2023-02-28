#include <string.h>
#include <xlib/xlib/config.h>

#include <xlib/xlib/xhash.h>
#include <xlib/xlib/xerror.h>
#include <xlib/xlib/xslice.h>
#include <xlib/xlib/xthread.h>
#include <xlib/xlib/xlib-init.h>
#include <xlib/xlib/xstrfuncs.h>
#include <xlib/xlib/xtestutils.h>

static XRWLock error_domain_global;
static XHashTable *error_domain_ht = NULL;

void x_error_init(void)
{
    error_domain_ht = x_hash_table_new(NULL, NULL);
}

typedef struct {
    xsize           private_size;
    XErrorInitFunc  init;
    XErrorCopyFunc  copy;
    XErrorClearFunc clear;
} ErrorDomainInfo;

static inline ErrorDomainInfo *error_domain_lookup(XQuark domain)
{
    return (ErrorDomainInfo *)x_hash_table_lookup(error_domain_ht, XUINT_TO_POINTER(domain));
}

#define STRUCT_ALIGNMENT        (2 * sizeof (xsize))
#define ALIGN_STRUCT(offset)    ((offset + (STRUCT_ALIGNMENT - 1)) & -STRUCT_ALIGNMENT)

static void error_domain_register(XQuark error_quark, xsize error_type_private_size, XErrorInitFunc error_type_init, XErrorCopyFunc error_type_copy, XErrorClearFunc error_type_clear)
{
    x_rw_lock_writer_lock(&error_domain_global);
    if (error_domain_lookup(error_quark) == NULL) {
        ErrorDomainInfo *info = x_new(ErrorDomainInfo, 1);
        info->private_size = ALIGN_STRUCT(error_type_private_size);
        info->init = error_type_init;
        info->copy = error_type_copy;
        info->clear = error_type_clear;

        x_hash_table_insert(error_domain_ht, XUINT_TO_POINTER(error_quark), info);
    } else {
        const char *name = x_quark_to_string(error_quark);
        x_critical("Attempted to register an extended error domain for %s more than once", name);
    }
    x_rw_lock_writer_unlock (&error_domain_global);
}

XQuark x_error_domain_register_static(const char *error_type_name, xsize error_type_private_size, XErrorInitFunc error_type_init, XErrorCopyFunc error_type_copy, XErrorClearFunc error_type_clear)
{
    XQuark error_quark;

    x_return_val_if_fail(error_type_name != NULL, 0);
    x_return_val_if_fail(error_type_private_size > 0, 0);
    x_return_val_if_fail(error_type_init != NULL, 0);
    x_return_val_if_fail(error_type_copy != NULL, 0);
    x_return_val_if_fail(error_type_clear != NULL, 0);

    error_quark = x_quark_from_static_string(error_type_name);
    error_domain_register(error_quark, error_type_private_size, error_type_init, error_type_copy, error_type_clear);

    return error_quark;
}

XQuark x_error_domain_register(const char *error_type_name, xsize error_type_private_size, XErrorInitFunc error_type_init, XErrorCopyFunc error_type_copy, XErrorClearFunc error_type_clear)
{
    XQuark error_quark;

    x_return_val_if_fail(error_type_name != NULL, 0);
    x_return_val_if_fail(error_type_private_size > 0, 0);
    x_return_val_if_fail(error_type_init != NULL, 0);
    x_return_val_if_fail(error_type_copy != NULL, 0);
    x_return_val_if_fail(error_type_clear != NULL, 0);

    error_quark = x_quark_from_string(error_type_name);
    error_domain_register(error_quark, error_type_private_size, error_type_init, error_type_copy, error_type_clear);

    return error_quark;
}

static XError *x_error_allocate(XQuark domain, ErrorDomainInfo *out_info)
{
    XError *error;
    xuint8 *allocated;
    xsize private_size;
    ErrorDomainInfo *info;

    x_rw_lock_reader_lock(&error_domain_global);
    info = error_domain_lookup(domain);
    if (info != NULL) {
        if (out_info != NULL) {
            *out_info = *info;
        }

        private_size = info->private_size;
        x_rw_lock_reader_unlock(&error_domain_global);
    } else {
        x_rw_lock_reader_unlock(&error_domain_global);
        if (out_info != NULL) {
            memset(out_info, 0, sizeof(*out_info));
        }

        private_size = 0;
    }

    allocated = (xuint8 *)x_slice_alloc0(private_size + sizeof(XError));

    error = (XError *)(allocated + private_size);
    return error;
}

static XError *x_error_new_steal(XQuark domain, xint code, xchar *message, ErrorDomainInfo *out_info)
{
    ErrorDomainInfo info;
    XError *error = x_error_allocate(domain, &info);

    error->domain = domain;
    error->code = code;
    error->message = message;

    if (info.init != NULL) {
        info.init(error);
    }

    if (out_info != NULL) {
        *out_info = info;
    }

    return error;
}

XError *x_error_new_valist(XQuark domain, xint code, const xchar *format, va_list args)
{
    x_return_val_if_fail(format != NULL, NULL);
    x_warn_if_fail(domain != 0);

    return x_error_new_steal(domain, code, x_strdup_vprintf(format, args), NULL);
}

XError *x_error_new(XQuark domain, xint code, const xchar *format, ...)
{
    va_list args;
    XError *error;

    x_return_val_if_fail(format != NULL, NULL);
    x_return_val_if_fail(domain != 0, NULL);

    va_start(args, format);
    error = x_error_new_valist(domain, code, format, args);
    va_end(args);

    return error;
}

XError *x_error_new_literal(XQuark domain, xint code, const xchar *message)
{
    x_return_val_if_fail(message != NULL, NULL);
    x_return_val_if_fail(domain != 0, NULL);

    return x_error_new_steal(domain, code, x_strdup(message), NULL);
}

void x_error_free(XError *error)
{
    xuint8 *allocated;
    xsize private_size;
    ErrorDomainInfo *info;

    x_return_if_fail(error != NULL);

    x_rw_lock_reader_lock(&error_domain_global);
    info = error_domain_lookup(error->domain);
    if (info != NULL) {
        XErrorClearFunc clear = info->clear;

        private_size = info->private_size;
        x_rw_lock_reader_unlock(&error_domain_global);
        clear (error);
    } else {
        x_rw_lock_reader_unlock(&error_domain_global);
        private_size = 0;
    }

    x_free(error->message);
    allocated = ((xuint8 *)error) - private_size;

    x_slice_free1(private_size + sizeof(XError), allocated);
}

XError *x_error_copy(const XError *error)
{
    XError *copy;
    ErrorDomainInfo info;

    x_return_val_if_fail(error != NULL, NULL);
    x_return_val_if_fail(error->message != NULL, NULL);
    x_warn_if_fail(error->domain != 0);

    copy = x_error_new_steal(error->domain, error->code, x_strdup(error->message), &info);
    if (info.copy != NULL) {
        info.copy(error, copy);
    }

    return copy;
}

xboolean x_error_matches(const XError *error, XQuark domain, xint code)
{
    return error && error->domain == domain && error->code == code;
}

#define ERROR_OVERWRITTEN_WARNING "XError set over the top of a previous XError or uninitialized memory.\n" \
               "This indicates a bug in someone's code. You must ensure an error is NULL before it's set.\n" \
               "The overwriting error message was: %s"

void x_set_error(XError **err, XQuark domain, xint code, const xchar *format, ...)
{
    XError *newt;
    va_list args;

    if (err == NULL) {
        return;
    }

    va_start(args, format);
    newt = x_error_new_valist(domain, code, format, args);
    va_end(args);

    if (*err == NULL) {
        *err = newt;
    } else {
        x_warning(ERROR_OVERWRITTEN_WARNING, newt->message);
        x_error_free(newt);
    }
}

void x_set_error_literal(XError **err, XQuark domain, xint code, const xchar *message)
{
    if (err == NULL) {
        return;
    }

    if (*err == NULL) {
        *err = x_error_new_literal(domain, code, message);
    } else {
        x_warning(ERROR_OVERWRITTEN_WARNING, message);
    }
}

void x_propagate_error(XError **dest, XError *src)
{
    x_return_if_fail(src != NULL);

    if (dest == NULL) {
        x_error_free(src);
        return;
    } else {
        if (*dest != NULL) {
            x_warning(ERROR_OVERWRITTEN_WARNING, src->message);
            x_error_free(src);
        } else {
            *dest = src;
        }
    }
}

void x_clear_error(XError **err)
{
    if (err && *err) {
        x_error_free(*err);
        *err = NULL;
    }
}

X_GNUC_PRINTF(2, 0)
static void x_error_add_prefix(xchar **string, const xchar *format, va_list ap)
{
    xchar *prefix;
    xchar *oldstring;

    prefix = x_strdup_vprintf(format, ap);
    oldstring = *string;
    *string = x_strconcat(prefix, oldstring, NULL);
    x_free(oldstring);
    x_free(prefix);
}

void x_prefix_error(XError **err, const xchar *format, ...)
{
    if (err && *err) {
        va_list ap;

        va_start(ap, format);
        x_error_add_prefix(&(*err)->message, format, ap);
        va_end(ap);
    }
}

void x_prefix_error_literal(XError **err, const xchar *prefix)
{
    if (err && *err) {
        xchar *oldstring;

        oldstring = (*err)->message;
        (*err)->message = x_strconcat(prefix, oldstring, NULL);
        x_free(oldstring);
    }
}

void x_propagate_prefixed_error(XError **dest, XError *src, const xchar *format, ...)
{
    x_propagate_error (dest, src);

    if (dest) {
        va_list ap;

        x_assert(*dest != NULL);

        va_start(ap, format);
        x_error_add_prefix(&(*dest)->message, format, ap);
        va_end(ap);
    }
}
