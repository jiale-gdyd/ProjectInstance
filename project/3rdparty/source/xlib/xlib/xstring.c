#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <xlib/xlib/config.h>
#include <xlib/xlib/xstring.h>
#include <xlib/xlib/xprintf.h>
#include <xlib/xlib/xuriprivate.h>
#include <xlib/xlib/xutilsprivate.h>

static void x_string_expand(XString *string, xsize len)
{
    if X_UNLIKELY((X_MAXSIZE - string->len - 1) < len) {
        x_error("adding %" X_XSIZE_FORMAT " to string would overflow", len);
    }

    string->allocated_len = x_nearest_pow(string->len + len + 1);
    if (string->allocated_len == 0) {
        string->allocated_len = string->len + len + 1;
    }
    string->str = x_realloc(string->str, string->allocated_len);
}

static inline void x_string_maybe_expand(XString *string, xsize len)
{
    if (X_UNLIKELY((string->len + len) >= string->allocated_len)) {
        x_string_expand(string, len);
    }
}

XString *x_string_sized_new(xsize dfl_size)
{
    XString *string = x_slice_new(XString);

    string->allocated_len = 0;
    string->len = 0;
    string->str = NULL;

    x_string_expand(string, MAX(dfl_size, 64));
    string->str[0] = 0;

    return string;
}

XString *x_string_new(const xchar *init)
{
    XString *string;

    if (init == NULL || *init == '\0') {
        string = x_string_sized_new(2);
    } else {
        xint len;

        len = strlen(init);
        string = x_string_sized_new(len + 2);

        x_string_append_len(string, init, len);
    }

    return string;
}

XString *x_string_new_take(xchar *init)
{
    XString *str;

    if (init == NULL) {
        return x_string_new(NULL);
    }

    str = x_slice_new(XString);

    str->str = init;
    str->len = strlen(str->str);
    str->allocated_len = str->len + 1;

    return str;
}

XString *x_string_new_len(const xchar *init, xssize len)
{
    XString *string;

    if (len < 0) {
        return x_string_new(init);
    } else {
        string = x_string_sized_new(len);
        if (init) {
            x_string_append_len(string, init, len);
        }

        return string;
    }
}

xchar *(x_string_free)(XString *string, xboolean free_segment)
{
    xchar *segment;

    x_return_val_if_fail(string != NULL, NULL);

    if (free_segment) {
        x_free(string->str);
        segment = NULL;
    } else {
        segment = string->str;
    }

    x_slice_free(XString, string);
    return segment;
}

xchar *x_string_free_and_steal(XString *string)
{
    return (x_string_free)(string, FALSE);
}

XBytes *x_string_free_to_bytes(XString *string)
{
    xsize len;
    xchar *buf;

    x_return_val_if_fail(string != NULL, NULL);

    len = string->len;
    buf = x_string_free(string, FALSE);

    return x_bytes_new_take(buf, len);
}

xboolean x_string_equal(const XString *v, const XString *v2)
{
    xchar *p, *q;
    XString *string1 = (XString *)v;
    XString *string2 = (XString *)v2;
    xsize i = string1->len;

    if (i != string2->len) {
        return FALSE;
    }

    p = string1->str;
    q = string2->str;

    while (i) {
        if (*p != *q) {
            return FALSE;
        }

        p++;
        q++;
        i--;
    }

    return TRUE;
}

xuint x_string_hash(const XString *str)
{
    xuint h = 0;
    xsize n = str->len;
    const xchar *p = str->str;

    while (n--) {
        h = (h << 5) - h + *p;
        p++;
    }

    return h;
}

XString *x_string_assign(XString *string, const xchar *rval)
{
    x_return_val_if_fail(string != NULL, NULL);
    x_return_val_if_fail(rval != NULL, string);

    if (string->str != rval) {
        x_string_truncate(string, 0);
        x_string_append(string, rval);
    }

    return string;
}

XString *(x_string_truncate)(XString *string, xsize len)
{
    x_return_val_if_fail(string != NULL, NULL);

    string->len = MIN(len, string->len);
    string->str[string->len] = 0;

    return string;
}

XString *x_string_set_size(XString *string, xsize len)
{
    x_return_val_if_fail(string != NULL, NULL);

    if (len >= string->allocated_len) {
        x_string_maybe_expand(string, len - string->len);
    }
    string->len = len;
    string->str[len] = 0;

    return string;
}

XString *x_string_insert_len(XString *string, xssize pos, const xchar *val, xssize len)
{
    xsize len_unsigned, pos_unsigned;

    x_return_val_if_fail(string != NULL, NULL);
    x_return_val_if_fail(len == 0 || val != NULL, string);

    if (len == 0) {
        return string;
    }

    if (len < 0) {
        len = strlen(val);
    }
    len_unsigned = len;

    if (pos < 0) {
        pos_unsigned = string->len;
    } else {
        pos_unsigned = pos;
        x_return_val_if_fail(pos_unsigned <= string->len, string);
    }

    if (X_UNLIKELY(val >= string->str && val <= string->str + string->len)) {
        xsize offset = val - string->str;
        xsize precount = 0;

        x_string_maybe_expand(string, len_unsigned);
        val = string->str + offset;

        if (pos_unsigned < string->len) {
            memmove(string->str + pos_unsigned + len_unsigned, string->str + pos_unsigned, string->len - pos_unsigned);
        }

        if (offset < pos_unsigned) {
            precount = MIN(len_unsigned, pos_unsigned - offset);
            memcpy(string->str + pos_unsigned, val, precount);
        }

        if (len_unsigned > precount) {
            memcpy(string->str + pos_unsigned + precount, val + precount + len_unsigned, len_unsigned - precount);
        }
    } else {
        x_string_maybe_expand(string, len_unsigned);

        if (pos_unsigned < string->len) {
            memmove(string->str + pos_unsigned + len_unsigned, string->str + pos_unsigned, string->len - pos_unsigned);
        }

        if (len_unsigned == 1) {
            string->str[pos_unsigned] = *val;
        } else {
            memcpy(string->str + pos_unsigned, val, len_unsigned);
        }
    }

    string->len += len_unsigned;
    string->str[string->len] = 0;

    return string;
}

XString *x_string_append_uri_escaped(XString *string, const xchar *unescaped, const xchar *reserved_chars_allowed, xboolean allow_utf8)
{
    _uri_encoder(string, (const xuchar *)unescaped, strlen(unescaped), reserved_chars_allowed, allow_utf8);
    return string;
}

XString *(x_string_append)(XString *string, const xchar *val)
{
    return x_string_insert_len(string, -1, val, -1);
}

XString *(x_string_append_len)(XString *string, const xchar *val, xssize len)
{
    return x_string_insert_len(string, -1, val, len);
}

XString *(x_string_append_c)(XString *string, xchar c)
{
    x_return_val_if_fail(string != NULL, NULL);
    return x_string_insert_c(string, -1, c);
}

XString *x_string_append_unichar(XString *string, xunichar wc)
{
    x_return_val_if_fail(string != NULL, NULL);
    return x_string_insert_unichar(string, -1, wc);
}

XString *x_string_prepend(XString *string, const xchar *val)
{
    return x_string_insert_len(string, 0, val, -1);
}

XString *x_string_prepend_len(XString *string, const xchar *val, xssize len)
{
    return x_string_insert_len(string, 0, val, len);
}

XString *x_string_prepend_c(XString *string, xchar c)
{
    x_return_val_if_fail(string != NULL, NULL);
    return x_string_insert_c(string, 0, c);
}

XString *x_string_prepend_unichar(XString *string, xunichar wc)
{
    x_return_val_if_fail(string != NULL, NULL);
    return x_string_insert_unichar(string, 0, wc);
}

XString *x_string_insert(XString *string, xssize pos, const xchar *val)
{
    return x_string_insert_len(string, pos, val, -1);
}

XString *x_string_insert_c(XString *string, xssize pos, xchar c)
{
    xsize pos_unsigned;

    x_return_val_if_fail(string != NULL, NULL);
    x_string_maybe_expand(string, 1);

    if (pos < 0) {
        pos = string->len;
    } else {
        x_return_val_if_fail((xsize)pos <= string->len, string);
    }
    pos_unsigned = pos;

    if (pos_unsigned < string->len) {
        memmove(string->str + pos_unsigned + 1, string->str + pos_unsigned, string->len - pos_unsigned);
    }

    string->str[pos_unsigned] = c;
    string->len += 1;
    string->str[string->len] = 0;

    return string;
}

XString *x_string_insert_unichar(XString *string, xssize pos, xunichar wc)
{
    xchar *dest;
    xint charlen, first, i;

    x_return_val_if_fail(string != NULL, NULL);

    if (wc < 0x80) {
        first = 0;
        charlen = 1;
    } else if (wc < 0x800) {
        first = 0xc0;
        charlen = 2;
    } else if (wc < 0x10000) {
        first = 0xe0;
        charlen = 3;
    } else if (wc < 0x200000) {
        first = 0xf0;
        charlen = 4;
    } else if (wc < 0x4000000) {
        first = 0xf8;
        charlen = 5;
     } else {
        first = 0xfc;
        charlen = 6;
    }

    x_string_maybe_expand(string, charlen);

    if (pos < 0) {
        pos = string->len;
    } else {
        x_return_val_if_fail((xsize)pos <= string->len, string);
    }

    if ((xsize)pos < string->len) {
        memmove(string->str + pos + charlen, string->str + pos, string->len - pos);
    }

    dest = string->str + pos;
    for (i = charlen - 1; i > 0; --i) {
        dest[i] = (wc & 0x3f) | 0x80;
        wc >>= 6;
    }

    dest[0] = wc | first;
    string->len += charlen;
    string->str[string->len] = 0;

    return string;
}

XString *x_string_overwrite(XString *string, xsize pos, const xchar *val)
{
    x_return_val_if_fail(val != NULL, string);
    return x_string_overwrite_len(string, pos, val, strlen(val));
}

XString *x_string_overwrite_len(XString *string, xsize pos, const xchar *val, xssize len)
{
    xsize end;

    x_return_val_if_fail(string != NULL, NULL);

    if (!len) {
        return string;
    }

    x_return_val_if_fail(val != NULL, string);
    x_return_val_if_fail(pos <= string->len, string);

    if (len < 0) {
        len = strlen(val);
    }

    end = pos + len;
    if (end > string->len) {
        x_string_maybe_expand(string, end - string->len);
    }
    memcpy(string->str + pos, val, len);

    if (end > string->len) {
        string->str[end] = '\0';
        string->len = end;
    }

    return string;
}

XString *x_string_erase(XString *string, xssize pos, xssize len)
{
    xsize len_unsigned, pos_unsigned;

    x_return_val_if_fail(string != NULL, NULL);
    x_return_val_if_fail(pos >= 0, string);
    pos_unsigned = pos;

    x_return_val_if_fail(pos_unsigned <= string->len, string);

    if (len < 0) {
        len_unsigned = string->len - pos_unsigned;
    } else {
        len_unsigned = len;
        x_return_val_if_fail(pos_unsigned + len_unsigned <= string->len, string);

        if (pos_unsigned + len_unsigned < string->len) {
            memmove(string->str + pos_unsigned, string->str + pos_unsigned + len_unsigned, string->len - (pos_unsigned + len_unsigned));
        }
    }

    string->len -= len_unsigned;
    string->str[string->len] = 0;

    return string;
}

xuint x_string_replace (XString *string, const xchar *find, const xchar *replace, xuint limit)
{
    xuint n = 0;
    xchar *cur, *next;
    xsize f_len, r_len, pos;

    x_return_val_if_fail(string != NULL, 0);
    x_return_val_if_fail(find != NULL, 0);
    x_return_val_if_fail(replace != NULL, 0);

    f_len = strlen(find);
    r_len = strlen(replace);
    cur = string->str;

    while ((next = strstr(cur, find)) != NULL) {
        pos = next - string->str;
        x_string_erase(string, pos, f_len);
        x_string_insert(string, pos, replace);
        cur = string->str + pos + r_len;
        n++;

        if (f_len == 0) {
            if (cur[0] == '\0') {
                break;
            } else {
                cur++;
            }
        }

        if (n == limit) {
            break;
        }
    }

    return n;
}

XString *x_string_ascii_down(XString *string)
{
    xint n;
    xchar *s;

    x_return_val_if_fail(string != NULL, NULL);

    n = string->len;
    s = string->str;

    while (n) {
        *s = x_ascii_tolower(*s);
        s++;
        n--;
    }

    return string;
}

XString *x_string_ascii_up(XString *string)
{
    xint n;
    xchar *s;

    x_return_val_if_fail(string != NULL, NULL);

    n = string->len;
    s = string->str;

    while (n) {
        *s = x_ascii_toupper(*s);
        s++;
        n--;
    }

    return string;
}

XString *x_string_down(XString *string)
{
    xlong n;
    xuchar *s;

    x_return_val_if_fail(string != NULL, NULL);

    n = string->len;
    s = (xuchar *)string->str;

    while (n) {
        if (isupper(*s)) {
            *s = tolower(*s);
        }

        s++;
        n--;
    }

    return string;
}

XString *x_string_up(XString *string)
{
    xlong n;
    xuchar *s;

    x_return_val_if_fail(string != NULL, NULL);

    n = string->len;
    s = (xuchar *)string->str;

    while (n) {
        if (islower(*s)) {
            *s = toupper(*s);
        }

        s++;
        n--;
    }

    return string;
}

void x_string_append_vprintf(XString *string, const xchar *format, va_list args)
{
    xint len;
    xchar *buf;

    x_return_if_fail(string != NULL);
    x_return_if_fail(format != NULL);

    len = x_vasprintf(&buf, format, args);

    if (len >= 0) {
        x_string_maybe_expand(string, len);
        memcpy(string->str + string->len, buf, len + 1);
        string->len += len;
        x_free(buf);
    } else {
        x_critical("Failed to append to string: invalid format/args passed to x_vasprintf()");
    }
}

void x_string_vprintf(XString *string, const xchar *format, va_list args)
{
    x_string_truncate(string, 0);
    x_string_append_vprintf(string, format, args);
}

void x_string_printf(XString *string, const xchar *format, ...)
{
    va_list args;

    x_string_truncate(string, 0);

    va_start(args, format);
    x_string_append_vprintf(string, format, args);
    va_end(args);
}

void x_string_append_printf(XString *string, const xchar *format, ...)
{
    va_list args;

    va_start(args, format);
    x_string_append_vprintf(string, format, args);
    va_end(args);
}
