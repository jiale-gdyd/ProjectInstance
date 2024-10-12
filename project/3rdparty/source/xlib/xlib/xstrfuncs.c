#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>
#include <locale.h>
#include <limits.h>

#include <xlib/xlib/config.h>
#include <xlib/xlib/xarray.h>
#include <xlib/xlib/xprintf.h>
#include <xlib/xlib/xlibintl.h>
#include <xlib/xlib/xstrfuncs.h>

static const xuint16 ascii_table_data[256] = {
    0x004, 0x004, 0x004, 0x004, 0x004, 0x004, 0x004, 0x004,
    0x004, 0x104, 0x104, 0x004, 0x104, 0x104, 0x004, 0x004,
    0x004, 0x004, 0x004, 0x004, 0x004, 0x004, 0x004, 0x004,
    0x004, 0x004, 0x004, 0x004, 0x004, 0x004, 0x004, 0x004,
    0x140, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0,
    0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0,
    0x459, 0x459, 0x459, 0x459, 0x459, 0x459, 0x459, 0x459,
    0x459, 0x459, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0,
    0x0d0, 0x653, 0x653, 0x653, 0x653, 0x653, 0x653, 0x253,
    0x253, 0x253, 0x253, 0x253, 0x253, 0x253, 0x253, 0x253,
    0x253, 0x253, 0x253, 0x253, 0x253, 0x253, 0x253, 0x253,
    0x253, 0x253, 0x253, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0,
    0x0d0, 0x473, 0x473, 0x473, 0x473, 0x473, 0x473, 0x073,
    0x073, 0x073, 0x073, 0x073, 0x073, 0x073, 0x073, 0x073,
    0x073, 0x073, 0x073, 0x073, 0x073, 0x073, 0x073, 0x073,
    0x073, 0x073, 0x073, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x004
};

const xuint16 *const x_ascii_table = ascii_table_data;

#define ISSPACE(c)              ((c) == ' ' || (c) == '\f' || (c) == '\n' || (c) == '\r' || (c) == '\t' || (c) == '\v')
#define ISUPPER(c)              ((c) >= 'A' && (c) <= 'Z')
#define ISLOWER(c)              ((c) >= 'a' && (c) <= 'z')
#define ISALPHA(c)              (ISUPPER(c) || ISLOWER(c))
#define TOUPPER(c)              (ISLOWER(c) ? (c) - 'a' + 'A' : (c))
#define TOLOWER(c)              (ISUPPER(c) ? (c) - 'A' + 'a' : (c))

static locale_t get_C_locale(void)
{
    static xsize initialized = FALSE;
    static locale_t C_locale = NULL;

    if (x_once_init_enter(&initialized)) {
        C_locale = newlocale(LC_ALL_MASK, "C", NULL);
        x_once_init_leave(&initialized, TRUE);
    }

    return C_locale;
}

xchar *(x_strdup)(const xchar *str)
{
    xsize length;
    xchar *new_str;

    if X_LIKELY(str) {
        length = strlen(str) + 1;
        new_str = x_new(char, length);
        memcpy(new_str, str, length);
    } else {
        new_str = NULL;
    }

    return new_str;
}

xpointer x_memdup(xconstpointer mem, xuint byte_size)
{
    xpointer new_mem;

    if (mem && (byte_size != 0)) {
        new_mem = x_malloc(byte_size);
        memcpy(new_mem, mem, byte_size);
    } else {
        new_mem = NULL;
    }

    return new_mem;
}

xpointer x_memdup2(xconstpointer mem, xsize byte_size)
{
    xpointer new_mem;

    if (mem && (byte_size != 0)) {
        new_mem = x_malloc(byte_size);
        memcpy(new_mem, mem, byte_size);
    } else {
        new_mem = NULL;
    }

    return new_mem;
}

xchar *x_strndup(const xchar *str, xsize n)
{
    xchar *new_str;

    if (str) {
        new_str = x_new(xchar, n + 1);
        strncpy(new_str, str, n);
        new_str[n] = '\0';
    } else {
        new_str = NULL;
    }

    return new_str;
}

xchar *x_strnfill(xsize length, xchar fill_char)
{
    xchar *str;

    str = x_new(xchar, length + 1);
    memset(str, (xuchar)fill_char, length);
    str[length] = '\0';

    return str;
}

xchar *x_stpcpy(xchar *dest, const xchar *src)
{
    x_return_val_if_fail(dest != NULL, NULL);
    x_return_val_if_fail(src != NULL, NULL);
    return stpcpy(dest, src);
}

xchar *x_strdup_vprintf(const xchar *format, va_list args)
{
    xchar *string = NULL;

    x_vasprintf(&string, format, args);
    return string;
}

xchar *x_strdup_printf(const xchar *format, ...)
{
    va_list args;
    xchar *buffer;

    va_start(args, format);
    buffer = x_strdup_vprintf(format, args);
    va_end(args);

    return buffer;
}

xchar *x_strconcat(const xchar *string1, ...)
{
    xsize l;
    xchar *s;
    xchar *ptr;
    va_list args;
    xchar *concat;

    if (!string1) {
        return NULL;
    }

    l = 1 + strlen(string1);
    va_start(args, string1);
    s = va_arg(args, xchar *);

    while (s) {
        l += strlen(s);
        s = va_arg(args, xchar *);
    }
    va_end(args);

    concat = x_new(xchar, l);
    ptr = concat;

    ptr = x_stpcpy(ptr, string1);
    va_start(args, string1);
    s = va_arg(args, xchar *);

    while (s) {
        ptr = x_stpcpy(ptr, s);
        s = va_arg(args, xchar *);
    }
    va_end(args);

    return concat;
}

xdouble x_strtod(const xchar *nptr, xchar **endptr)
{
    xdouble val_1;
    xdouble val_2 = 0;
    xchar *fail_pos_1;
    xchar *fail_pos_2;

    x_return_val_if_fail(nptr != NULL, 0);

    fail_pos_1 = NULL;
    fail_pos_2 = NULL;

    val_1 = strtod(nptr, &fail_pos_1);

    if (fail_pos_1 && (fail_pos_1[0] != 0)) {
        val_2 = x_ascii_strtod(nptr, &fail_pos_2);
    }

    if (!fail_pos_1 || (fail_pos_1[0] == 0) || (fail_pos_1 >= fail_pos_2)) {
        if (endptr) {
            *endptr = fail_pos_1;
        }

        return val_1;
    } else {
        if (endptr) {
            *endptr = fail_pos_2;
        }

        return val_2;
    }
}

xdouble x_ascii_strtod(const xchar *nptr, xchar **endptr)
{
    x_return_val_if_fail(nptr != NULL, 0);
    errno = 0;
    return strtod_l(nptr, endptr, get_C_locale());
}

xchar *x_ascii_dtostr(xchar *buffer, xint buf_len, xdouble d)
{
    return x_ascii_formatd(buffer, buf_len, "%.17g", d);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
xchar *x_ascii_formatd(xchar *buffer, xint buf_len, const xchar *format, xdouble d)
{
    locale_t old_locale;

    x_return_val_if_fail(buffer != NULL, NULL);
    x_return_val_if_fail(format[0] == '%', NULL);
    x_return_val_if_fail(strpbrk(format + 1, "'l%") == NULL, NULL);

    old_locale = uselocale(get_C_locale());
    snprintf(buffer, buf_len, format, d);
    uselocale(old_locale);

    return buffer;
}
#pragma GCC diagnostic pop

xuint64 x_ascii_strtoull(const xchar *nptr, xchar **endptr, xuint base)
{
    return strtoull_l(nptr, endptr, base, get_C_locale());
}

xint64 x_ascii_strtoll(const xchar *nptr, xchar **endptr, xuint base)
{
    return strtoll_l(nptr, endptr, base, get_C_locale());
}

const xchar *x_strerror(xint errnum)
{
    const xchar *msg;
    xint saved_errno = errno;
    static XHashTable *errors;
    X_LOCK_DEFINE_STATIC(errors);

    X_LOCK(errors);
    if (errors) {
        msg = (const xchar *)x_hash_table_lookup(errors, XINT_TO_POINTER(errnum));
    } else {
        errors = x_hash_table_new(NULL, NULL);
        msg = NULL;
    }

    if (!msg) {
        xchar buf[1024];
        XError *error = NULL;
#if defined(HAVE_STRERROR_R) && !defined(STRERROR_R_CHAR_P)
        int ret;
#endif

#if defined(HAVE_STRERROR_R)
#if defined(STRERROR_R_CHAR_P)
        msg = (const xchar *)strerror_r(errnum, buf, sizeof(buf));
#else
        ret = strerror_r(errnum, buf, sizeof(buf));
        if ((ret == 0) || (ret == EINVAL)) {
            msg = buf;
        }
#endif
#else
        x_strlcpy(buf, strerror(errnum), sizeof(buf));
        msg = buf;
#endif

        if (!msg) {
            X_UNLOCK(errors);

            errno = saved_errno;
            return NULL;
        }

        if (!x_get_console_charset(NULL)) {
            msg = x_locale_to_utf8(msg, -1, NULL, NULL, &error);
            if (error) {
                x_print("%s\n", error->message);
                x_error_free(error);
            }
        } else if (msg == (const xchar *)buf) {
            msg = x_strdup(buf);
        }

        x_hash_table_insert(errors, XINT_TO_POINTER(errnum), (char *)msg);
    }
    X_UNLOCK(errors);

    errno = saved_errno;
    return msg;
}

const xchar *x_strsignal(xint signum)
{
    xchar *msg;
    xchar *tofree;
    const xchar *ret;

    msg = tofree = NULL;

    msg = strsignal(signum);
    if (!x_get_console_charset(NULL)) {
        msg = tofree = x_locale_to_utf8(msg, -1, NULL, NULL, NULL);
    }

    if (!msg) {
        msg = tofree = x_strdup_printf("unknown signal (%d)", signum);
    }

    ret = x_intern_string(msg);
    x_free(tofree);

    return ret;
}

xsize x_strlcpy(xchar *dest, const xchar *src, xsize dest_size)
{
    xchar *d = dest;
    xsize n = dest_size;
    const xchar *s = src;

    x_return_val_if_fail(dest != NULL, 0);
    x_return_val_if_fail(src  != NULL, 0);

    if ((n != 0) && (--n != 0)) {
        do {
            xchar c = *s++;

            *d++ = c;
            if (c == 0) {
                break;
            }
        } while (--n != 0);
    }

    if (n == 0) {
        if (dest_size != 0) {
            *d = 0;
        }

        while (*s++);
    }

    return s - src - 1;
}

xsize x_strlcat(xchar *dest, const xchar *src, xsize dest_size)
{
    xsize dlength;
    xchar *d = dest;
    const xchar *s = src;
    xsize bytes_left = dest_size;

    x_return_val_if_fail(dest != NULL, 0);
    x_return_val_if_fail(src  != NULL, 0);

    while ((*d != 0) && (bytes_left-- != 0)) {
        d++;
    }

    dlength = d - dest;
    bytes_left = dest_size - dlength;

    if (bytes_left == 0) {
        return dlength + strlen(s);
    }

    while (*s != 0) {
        if (bytes_left != 1) {
            *d++ = *s;
            bytes_left--;
        }

        s++;
    }
    *d = 0;

    return dlength + (s - src);
}

xchar *x_ascii_strdown(const xchar *str, xssize len)
{
    xchar *result, *s;

    x_return_val_if_fail(str != NULL, NULL);

    if (len < 0) {
        result = x_strdup(str);
    } else {
        result = x_strndup(str, (xsize)len);
    }

    for (s = result; *s; s++) {
        *s = x_ascii_tolower(*s);
    }

    return result;
}

xchar *x_ascii_strup(const xchar *str, xssize len)
{
    xchar *result, *s;

    x_return_val_if_fail(str != NULL, NULL);

    if (len < 0) {
        result = x_strdup(str);
    } else {
        result = x_strndup(str, (xsize)len);
    }

    for (s = result; *s; s++) {
        *s = x_ascii_toupper(*s);
    }

    return result;
}

xchar *x_strdown(xchar *string)
{
    xuchar *s;

    x_return_val_if_fail(string != NULL, NULL);

    s = (xuchar *)string;

    while (*s) {
        if (isupper(*s)) {
            *s = tolower(*s);
        }

        s++;
    }

    return (xchar *)string;
}

xchar *x_strup(xchar *string)
{
    xuchar *s;

    x_return_val_if_fail(string != NULL, NULL);

    s = (xuchar *)string;
    while (*s) {
        if (islower(*s)) {
            *s = toupper(*s);
        }

        s++;
    }

    return (xchar *)string;
}

xchar *x_strreverse(xchar *string)
{
    x_return_val_if_fail(string != NULL, NULL);

    if (*string) {
        xchar *h, *t;

        h = string;
        t = string + strlen(string) - 1;

        while (h < t) {
            xchar c;

            c = *h;
            *h = *t;
            h++;
            *t = c;
            t--;
        }
    }

    return string;
}

xchar x_ascii_tolower(xchar c)
{
    return x_ascii_isupper(c) ? c - 'A' + 'a' : c;
}

xchar x_ascii_toupper(xchar c)
{
    return x_ascii_islower(c) ? c - 'a' + 'A' : c;
}

int x_ascii_digit_value(xchar c)
{
    if (x_ascii_isdigit(c)) {
        return c - '0';
    }

    return -1;
}

int x_ascii_xdigit_value(xchar c)
{
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }

    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }

    return x_ascii_digit_value(c);
}

xint x_ascii_strcasecmp(const xchar *s1, const xchar *s2)
{
    xint c1, c2;

    x_return_val_if_fail(s1 != NULL, 0);
    x_return_val_if_fail(s2 != NULL, 0);

    while (*s1 && *s2) {
        c1 = (xint)(xuchar)TOLOWER(*s1);
        c2 = (xint)(xuchar)TOLOWER(*s2);
        if (c1 != c2) {
            return (c1 - c2);
        }

        s1++; s2++;
    }

    return (((xint)(xuchar)*s1) - ((xint)(xuchar)*s2));
}

xint x_ascii_strncasecmp(const xchar *s1, const xchar *s2, xsize n)
{
    xint c1, c2;

    x_return_val_if_fail(s1 != NULL, 0);
    x_return_val_if_fail(s2 != NULL, 0);

    while (n && *s1 && *s2) {
        n -= 1;
        c1 = (xint)(xuchar)TOLOWER(*s1);
        c2 = (xint)(xuchar)TOLOWER(*s2);
        if (c1 != c2) {
            return (c1 - c2);
        }

        s1++; s2++;
    }

    if (n) {
        return (((xint)(xuchar) *s1) - ((xint)(xuchar) *s2));
    } else {
        return 0;
    }
}

xint x_strcasecmp(const xchar *s1, const xchar *s2)
{
    x_return_val_if_fail(s1 != NULL, 0);
    x_return_val_if_fail(s2 != NULL, 0);

    return strcasecmp(s1, s2);
}

xint x_strncasecmp(const xchar *s1, const xchar *s2, xuint n)
{
    return strncasecmp(s1, s2, n);
}

xchar *x_strdelimit(xchar *string, const xchar *delimiters, xchar new_delim)
{
    xchar *c;

    x_return_val_if_fail(string != NULL, NULL);

    if (!delimiters) {
        delimiters = X_STR_DELIMITERS;
    }

    for (c = string; *c; c++) {
        if (strchr(delimiters, *c)) {
            *c = new_delim;
        }
    }

    return string;
}

xchar *x_strcanon(xchar *string, const xchar *valid_chars, xchar substitutor)
{
    xchar *c;

    x_return_val_if_fail(string != NULL, NULL);
    x_return_val_if_fail(valid_chars != NULL, NULL);

    for (c = string; *c; c++) {
        if (!strchr(valid_chars, *c)) {
            *c = substitutor;
        }
    }

    return string;
}

xchar *x_strcompress(const xchar *source)
{
    xchar *q;
    xchar *dest;
    const xchar *p = source, *octal;

    x_return_val_if_fail(source != NULL, NULL);

    dest = (xchar *)x_malloc(strlen(source) + 1);
    q = dest;

    while (*p) {
        if (*p == '\\') {
            p++;
            switch (*p) {
                case '\0':
                    x_warning ("x_strcompress: trailing \\");
                    goto out;
                    case '0':  case '1':  case '2':  case '3':  case '4':
                    case '5':  case '6':  case '7':
                    *q = 0;
                    octal = p;
                    while ((p < octal + 3) && (*p >= '0') && (*p <= '7')) {
                        *q = (*q * 8) + (*p - '0');
                        p++;
                    }
                    q++;
                    p--;
                    break;

                case 'b':
                    *q++ = '\b';
                    break;

                case 'f':
                    *q++ = '\f';
                    break;

                case 'n':
                    *q++ = '\n';
                    break;

                case 'r':
                    *q++ = '\r';
                    break;

                case 't':
                    *q++ = '\t';
                    break;

                case 'v':
                    *q++ = '\v';
                    break;

                default:
                    *q++ = *p;
                    break;
                }
        } else {
            *q++ = *p;
        }

        p++;
    }

out:
    *q = 0;
    return dest;
}

xchar *x_strescape(const xchar *source, const xchar *exceptions)
{
    xchar *q;
    xchar *dest;
    const xuchar *p;
    xuchar excmap[256];

    x_return_val_if_fail(source != NULL, NULL);

    p = (xuchar *)source;
    q = dest = (xchar *)x_malloc(strlen(source) * 4 + 1);

    memset(excmap, 0, 256);
    if (exceptions) {
        xuchar *e = (xuchar *)exceptions;

        while (*e) {
            excmap[*e] = 1;
            e++;
        }
    }

    while (*p) {
        if (excmap[*p]) {
            *q++ = *p;
        } else {
            switch (*p) {
                case '\b':
                    *q++ = '\\';
                    *q++ = 'b';
                    break;

                case '\f':
                    *q++ = '\\';
                    *q++ = 'f';
                    break;

                case '\n':
                    *q++ = '\\';
                    *q++ = 'n';
                    break;

                case '\r':
                    *q++ = '\\';
                    *q++ = 'r';
                    break;

                case '\t':
                    *q++ = '\\';
                    *q++ = 't';
                    break;

                case '\v':
                    *q++ = '\\';
                    *q++ = 'v';
                    break;

                case '\\':
                    *q++ = '\\';
                    *q++ = '\\';
                    break;

                case '"':
                    *q++ = '\\';
                    *q++ = '"';
                    break;

                default:
                    if ((*p < ' ') || (*p >= 0177)) {
                        *q++ = '\\';
                        *q++ = '0' + (((*p) >> 6) & 07);
                        *q++ = '0' + (((*p) >> 3) & 07);
                        *q++ = '0' + ((*p) & 07);
                    } else {
                        *q++ = *p;
                    }

                    break;
            }
        }

        p++;
    }

    *q = 0;
    return dest;
}

xchar *x_strchug(xchar *string)
{
    xuchar *start;

    x_return_val_if_fail(string != NULL, NULL);

    for (start = (xuchar *)string; *start && x_ascii_isspace(*start); start++);
    memmove(string, start, strlen((xchar *)start) + 1);

    return string;
}

xchar *x_strchomp(xchar *string)
{
    xsize len;

    x_return_val_if_fail(string != NULL, NULL);

    len = strlen(string);
    while (len--) {
        if (x_ascii_isspace((xuchar)string[len])) {
            string[len] = '\0';
        } else {
            break;
        }
    }

    return string;
}

xchar **x_strsplit(const xchar *string, const xchar *delimiter, xint max_tokens)
{
    char *s;
    const xchar *remainder;
    XPtrArray *string_list;

    x_return_val_if_fail(string != NULL, NULL);
    x_return_val_if_fail(delimiter != NULL, NULL);
    x_return_val_if_fail(delimiter[0] != '\0', NULL);

    if (max_tokens < 1) {
        max_tokens = X_MAXINT;
        string_list = x_ptr_array_new();
    } else {
        string_list = x_ptr_array_new_full(max_tokens + 1, NULL);
    }

    remainder = string;
    s = (char *)strstr(remainder, delimiter);
    if (s) {
        xsize delimiter_len = strlen(delimiter);
        while (--max_tokens && s) {
            xsize len;

            len = s - remainder;
            x_ptr_array_add(string_list, x_strndup(remainder, len));
            remainder = s + delimiter_len;
            s = (char *)strstr(remainder, delimiter);
        }
    }

    if (*string) {
        x_ptr_array_add(string_list, x_strdup(remainder));
    }
    x_ptr_array_add(string_list, NULL);

    return (char **)x_ptr_array_free(string_list, FALSE);
}

xchar **x_strsplit_set(const xchar *string, const xchar *delimiters, xint max_tokens)
{
    xchar *token;
    xint n_tokens;
    xchar **result;
    const xchar *s;
    const xchar *current;
    XSList *tokens, *list;
    xuint8 delim_table[256];

    x_return_val_if_fail(string != NULL, NULL);
    x_return_val_if_fail(delimiters != NULL, NULL);

    if (max_tokens < 1) {
        max_tokens = X_MAXINT;
    }

    if (*string == '\0') {
        result = x_new(char *, 1);
        result[0] = NULL;
        return result;
    }

    memset(delim_table, FALSE, sizeof (delim_table));
    for (s = delimiters; *s != '\0'; ++s) {
        delim_table[*(xuchar *)s] = TRUE;
    }

    tokens = NULL;
    n_tokens = 0;

    s = current = string;
    while (*s != '\0') {
        if (delim_table[*(xuchar *)s] && ((n_tokens + 1) < max_tokens)) {
            token = x_strndup(current, s - current);
            tokens = x_slist_prepend(tokens, token);
            ++n_tokens;

            current = s + 1;
        }

        ++s;
    }

    token = x_strndup(current, s - current);
    tokens = x_slist_prepend(tokens, token);
    ++n_tokens;

    result = x_new(xchar *, n_tokens + 1);

    result[n_tokens] = NULL;
    for (list = tokens; list != NULL; list = list->next) {
        result[--n_tokens] = (xchar *)list->data;
    }

    x_slist_free(tokens);
    return result;
}

void x_strfreev(xchar **str_array)
{
    if (str_array) {
        xsize i;

        for (i = 0; str_array[i] != NULL; i++) {
            x_free(str_array[i]);
        }

        x_free(str_array);
    }
}

xchar **x_strdupv(xchar **str_array)
{
    if (str_array) {
        xsize i;
        xchar **retval;

        i = 0;
        while (str_array[i]) {
            ++i;
        }

        retval = x_new(xchar *, i + 1);

        i = 0;
        while (str_array[i]) {
            retval[i] = x_strdup(str_array[i]);
            ++i;
        }

        retval[i] = NULL;
        return retval;
    } else {
        return NULL;
    }
}

xchar *x_strjoinv(const xchar *separator, xchar **str_array)
{
    xchar *ptr;
    xchar *string;

    x_return_val_if_fail(str_array != NULL, NULL);

    if (separator == NULL) {
        separator = "";
    }

    if (*str_array) {
        xsize i;
        xsize len;
        xsize separator_len;

        separator_len = strlen(separator);
        len = 1 + strlen(str_array[0]);
        for (i = 1; str_array[i] != NULL; i++) {
            len += strlen(str_array[i]);
        }
        len += separator_len * (i - 1);

        string = x_new(xchar, len);
        ptr = x_stpcpy(string, *str_array);
        for (i = 1; str_array[i] != NULL; i++) {
            ptr = x_stpcpy(ptr, separator);
            ptr = x_stpcpy(ptr, str_array[i]);
        }
    } else {
        string = x_strdup("");
    }

    return string;
}

xchar *x_strjoin(const xchar *separator, ...)
{
    xsize len;
    xchar *ptr;
    va_list args;
    xchar *string, *s;
    xsize separator_len;

    if (separator == NULL) {
        separator = "";
    }

    separator_len = strlen(separator);
    va_start(args, separator);
    s = va_arg(args, xchar*);

    if (s) {
        len = 1 + strlen(s);

        s = va_arg(args, xchar *);
        while (s) {
            len += separator_len + strlen(s);
            s = va_arg(args, xchar *);
        }
        va_end(args);

        string = x_new(xchar, len);
        va_start(args, separator);

        s = va_arg(args, xchar *);
        ptr = x_stpcpy(string, s);

        s = va_arg(args, xchar *);
        while (s) {
            ptr = x_stpcpy(ptr, separator);
            ptr = x_stpcpy(ptr, s);
            s = va_arg(args, xchar *);
        }
    } else {
        string = x_strdup("");
    }
    va_end(args);

    return string;
}

xchar *x_strstr_len(const xchar *haystack, xssize haystack_len, const xchar *needle)
{
    x_return_val_if_fail(haystack != NULL, NULL);
    x_return_val_if_fail(needle != NULL, NULL);

    if (haystack_len < 0) {
        return (xchar *)strstr(haystack, needle);
    } else {
        xsize i;
        const xchar *end;
        const xchar *p = haystack;
        xsize needle_len = strlen(needle);
        xsize haystack_len_unsigned = haystack_len;

        if (needle_len == 0) {
            return (xchar *)haystack;
        }

        if (haystack_len_unsigned < needle_len) {
            return NULL;
        }

        end = haystack + haystack_len - needle_len;
        while ((p <= end) && *p) {
            for (i = 0; i < needle_len; i++) {
                if (p[i] != needle[i]) {
                    goto next;
                }
            }

            return (xchar *)p;
next:
            p++;
        }

        return NULL;
    }
}

xchar *x_strrstr(const xchar *haystack, const xchar *needle)
{
    xsize i;
    const xchar *p;
    xsize needle_len;
    xsize haystack_len;

    x_return_val_if_fail(haystack != NULL, NULL);
    x_return_val_if_fail(needle != NULL, NULL);

    needle_len = strlen(needle);
    haystack_len = strlen(haystack);

    if (needle_len == 0) {
        return (xchar *)haystack;
    }

    if (haystack_len < needle_len) {
        return NULL;
    }

    p = haystack + haystack_len - needle_len;
    while (p >= haystack) {
        for (i = 0; i < needle_len; i++) {
            if (p[i] != needle[i]) {
                goto next;
            }
        }

        return (xchar *)p;

next:
        p--;
    }

    return NULL;
}

xchar *x_strrstr_len(const xchar *haystack, xssize haystack_len, const xchar *needle)
{
    x_return_val_if_fail(haystack != NULL, NULL);
    x_return_val_if_fail(needle != NULL, NULL);

    if (haystack_len < 0) {
        return x_strrstr(haystack, needle);
    } else {
        xsize i;
        xsize needle_len = strlen(needle);
        const xchar *haystack_max = haystack + haystack_len;
        const xchar *p = haystack;

        while ((p < haystack_max) && *p) {
            p++;
        }

        if (p < (haystack + needle_len)) {
            return NULL;
        }

        p -= needle_len;

        while (p >= haystack) {
            for (i = 0; i < needle_len; i++) {
                if (p[i] != needle[i]) {
                    goto next;
                }
            }

            return (xchar *)p;
next:
            p--;
        }

        return NULL;
    }
}

xboolean (x_str_has_suffix)(const xchar *str, const xchar *suffix)
{
    xsize str_len;
    xsize suffix_len;

    x_return_val_if_fail(str != NULL, FALSE);
    x_return_val_if_fail(suffix != NULL, FALSE);

    str_len = strlen(str);
    suffix_len = strlen(suffix);

    if (str_len < suffix_len) {
        return FALSE;
    }

    return strcmp(str + str_len - suffix_len, suffix) == 0;
}

xboolean (x_str_has_prefix)(const xchar *str, const xchar *prefix)
{
    x_return_val_if_fail(str != NULL, FALSE);
    x_return_val_if_fail(prefix != NULL, FALSE);

    return strncmp(str, prefix, strlen(prefix)) == 0;
}

xuint x_strv_length(xchar **str_array)
{
    xuint i = 0;
    x_return_val_if_fail(str_array != NULL, 0);

    while (str_array[i]) {
        ++i;
    }

    return i;
}

static void index_add_folded(XPtrArray *array, const xchar *start, const xchar *end)
{
    xchar *normal;

    normal = x_utf8_normalize(start, end - start, X_NORMALIZE_ALL_COMPOSE);

    if (strstr(normal, "ı") || strstr(normal, "İ")) {
        XString *tmp;
        xchar *s = normal;

        tmp = x_string_new(NULL);
        while (*s) {
            xchar *i, *I, *e;

            i = strstr(s, "ı");
            I = strstr(s, "İ");

            if (!i && !I) {
                break;
            } else if (i && !I) {
                e = i;
            } else if (I && !i) {
                e = I;
            } else if (i < I) {
                e = i;
            } else {
                e = I;
            }

            x_string_append_len(tmp, s, e - s);
            x_string_append_c(tmp, 'i');
            s = x_utf8_next_char(e);
        }

        x_string_append(tmp, s);
        x_free(normal);
        normal = x_string_free(tmp, FALSE);
    }

    x_ptr_array_add(array, x_utf8_casefold(normal, -1));
    x_free(normal);
}

static xchar **split_words(const xchar *value)
{
    const xchar *s;
    XPtrArray *result;
    const xchar *start = NULL;

    result = x_ptr_array_new();

    for (s = value; *s; s = x_utf8_next_char(s)) {
        xunichar c = x_utf8_get_char(s);

        if (start == NULL) {
            if (x_unichar_isalnum (c) || x_unichar_ismark(c))
                start = s;
        } else {
            if (!x_unichar_isalnum(c) && !x_unichar_ismark(c)) {
                index_add_folded(result, start, s);
                start = NULL;
            }
        }
    }

    if (start) {
        index_add_folded(result, start, s);
    }
    x_ptr_array_add(result, NULL);

    return (xchar **)x_ptr_array_free(result, FALSE);
}

xchar **x_str_tokenize_and_fold(const xchar *string, const xchar *translit_locale, xchar ***ascii_alternates)
{
    xchar **result;

    x_return_val_if_fail(string != NULL, NULL);

    if (ascii_alternates && x_str_is_ascii(string)) {
        *ascii_alternates = x_new0(xchar *, 0 + 1);
        ascii_alternates = NULL;
    }

    result = split_words(string);
    if (ascii_alternates) {
        xint i, j, n;

        n = x_strv_length(result);
        *ascii_alternates = x_new(xchar *, n + 1);
        j = 0;

        for (i = 0; i < n; i++) {
            if (!x_str_is_ascii(result[i])) {
                xint k;
                xchar *ascii;
                xchar *composed;

                composed = x_utf8_normalize(result[i], -1, X_NORMALIZE_ALL_COMPOSE);
                ascii = x_str_to_ascii(composed, translit_locale);

                for (k = 0; ascii[k]; k++) {
                    if (!x_ascii_isalnum(ascii[k])) {
                        break;
                    }
                }

                if (ascii[k] == '\0') {
                    (*ascii_alternates)[j++] = ascii;
                } else {
                    x_free(ascii);
                }

                x_free(composed);
            }
        }

        (*ascii_alternates)[j] = NULL;
    }

    return result;
}

xboolean x_str_match_string(const xchar *search_term, const xchar *potential_hit, xboolean accept_alternates)
{
    xint i, j;
    xboolean matched;
    xchar **hit_tokens;
    xchar **term_tokens;
    xchar **alternates = NULL;

    x_return_val_if_fail(search_term != NULL, FALSE);
    x_return_val_if_fail(potential_hit != NULL, FALSE);

    term_tokens = x_str_tokenize_and_fold(search_term, NULL, NULL);
    hit_tokens = x_str_tokenize_and_fold(potential_hit, NULL, accept_alternates ? &alternates : NULL);

    matched = TRUE;

    for (i = 0; term_tokens[i]; i++) {
        for (j = 0; hit_tokens[j]; j++) {
            if (x_str_has_prefix(hit_tokens[j], term_tokens[i])) {
                goto one_matched;
            }
        }

        if (accept_alternates) {
            for (j = 0; alternates[j]; j++) {
                if (x_str_has_prefix(alternates[j], term_tokens[i])) {
                    goto one_matched;
                }
            }
        }

        matched = FALSE;
        break;

one_matched:
        continue;
    }

    x_strfreev(term_tokens);
    x_strfreev(hit_tokens);
    x_strfreev(alternates);

    return matched;
}

xboolean x_strv_contains(const xchar *const *strv, const xchar *str)
{
    x_return_val_if_fail(strv != NULL, FALSE);
    x_return_val_if_fail(str != NULL, FALSE);

    for (; *strv != NULL; strv++) {
        if (x_str_equal(str, *strv)) {
            return TRUE;
        }
    }

    return FALSE;
}

xboolean x_strv_equal(const xchar *const *strv1, const xchar *const *strv2)
{
    x_return_val_if_fail(strv1 != NULL, FALSE);
    x_return_val_if_fail(strv2 != NULL, FALSE);

    if (strv1 == strv2) {
        return TRUE;
    }

    for (; *strv1 != NULL && *strv2 != NULL; strv1++, strv2++) {
        if (!x_str_equal(*strv1, *strv2)) {
            return FALSE;
        }
    }

    return (*strv1 == NULL && *strv2 == NULL);
}

static xboolean str_has_sign(const xchar *str)
{
    return str[0] == '-' || str[0] == '+';
}

static xboolean str_has_hex_prefix(const xchar *str)
{
    return str[0] == '0' && x_ascii_tolower(str[1]) == 'x';
}

xboolean x_ascii_string_to_signed(const xchar *str, xuint base, xint64 min, xint64 max, xint64 *out_num, XError **error)
{
    xint64 number;
    xint saved_errno = 0;
    const xchar *end_ptr = NULL;

    x_return_val_if_fail(str != NULL, FALSE);
    x_return_val_if_fail((base >= 2) && (base <= 36), FALSE);
    x_return_val_if_fail(min <= max, FALSE);
    x_return_val_if_fail((error == NULL) || (*error == NULL), FALSE);

    if (str[0] == '\0') {
        x_set_error_literal(error, X_NUMBER_PARSER_ERROR, X_NUMBER_PARSER_ERROR_INVALID, _("Empty string is not a number"));
        return FALSE;
    }

    errno = 0;
    number = x_ascii_strtoll(str, (xchar **)&end_ptr, base);
    saved_errno = errno;

    if (x_ascii_isspace(str[0])
        || (base == 16
        && (str_has_sign (str) ? str_has_hex_prefix (str + 1) : str_has_hex_prefix (str)))
        || (saved_errno != 0 && saved_errno != ERANGE)
        || end_ptr == NULL
        || *end_ptr != '\0')
    {
        x_set_error(error, X_NUMBER_PARSER_ERROR, X_NUMBER_PARSER_ERROR_INVALID, _("“%s” is not a signed number"), str);
        return FALSE;
    }

    if ((saved_errno == ERANGE) || (number < min) || (number > max)) {
        xchar *min_str = x_strdup_printf("%" X_XINT64_FORMAT, min);
        xchar *max_str = x_strdup_printf("%" X_XINT64_FORMAT, max);

        x_set_error(error, X_NUMBER_PARSER_ERROR, X_NUMBER_PARSER_ERROR_OUT_OF_BOUNDS, _("Number “%s” is out of bounds [%s, %s]"), str, min_str, max_str);
        x_free(min_str);
        x_free(max_str);

        return FALSE;
    }

    if (out_num != NULL) {
        *out_num = number;
    }

    return TRUE;
}

xboolean x_ascii_string_to_unsigned(const xchar *str, xuint base, xuint64 min, xuint64 max, xuint64 *out_num, XError **error)
{
    xuint64 number;
    xint saved_errno = 0;
    const xchar *end_ptr = NULL;

    x_return_val_if_fail(str != NULL, FALSE);
    x_return_val_if_fail((base >= 2) && (base <= 36), FALSE);
    x_return_val_if_fail(min <= max, FALSE);
    x_return_val_if_fail((error == NULL) || (*error == NULL), FALSE);

    if (str[0] == '\0') {
        x_set_error_literal(error, X_NUMBER_PARSER_ERROR, X_NUMBER_PARSER_ERROR_INVALID, _("Empty string is not a number"));
        return FALSE;
    }

    errno = 0;
    number = x_ascii_strtoull(str, (xchar **)&end_ptr, base);
    saved_errno = errno;

    if (x_ascii_isspace(str[0])
        || str_has_sign (str)
        || (base == 16 && str_has_hex_prefix (str))
        || (saved_errno != 0 && saved_errno != ERANGE)
        || end_ptr == NULL
        || *end_ptr != '\0')
    {
        x_set_error(error, X_NUMBER_PARSER_ERROR, X_NUMBER_PARSER_ERROR_INVALID, _("“%s” is not an unsigned number"), str);
        return FALSE;
    }

    if ((saved_errno == ERANGE) || (number < min) || (number > max)) {
        xchar *min_str = x_strdup_printf("%" X_XUINT64_FORMAT, min);
        xchar *max_str = x_strdup_printf("%" X_XUINT64_FORMAT, max);

        x_set_error(error, X_NUMBER_PARSER_ERROR, X_NUMBER_PARSER_ERROR_OUT_OF_BOUNDS, _("Number “%s” is out of bounds [%s, %s]"), str, min_str, max_str);
        x_free(min_str);
        x_free(max_str);

        return FALSE;
    }

    if (out_num != NULL) {
        *out_num = number;
    }

    return TRUE;
}

X_DEFINE_QUARK(x-number-parser-error-quark, x_number_parser_error)
