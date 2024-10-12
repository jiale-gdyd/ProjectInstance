#include <string.h>
#include <stdlib.h>
#include <langinfo.h>

#include <xlib/xlib/config.h>
#include <xlib/xlib/xhash.h>
#include <xlib/xlib/xtypes.h>
#include <xlib/xlib/xthread.h>
#include <xlib/xlib/xlibintl.h>
#include <xlib/xlib/xconvert.h>
#include <xlib/xlib/xstrfuncs.h>
#include <xlib/xlib/xtestutils.h>

#define UTF8_COMPUTE(Char, Mask, Len)                           \
    if (Char < 128) {                                           \
        Len = 1;                                                \
        Mask = 0x7f;                                            \
    } else if ((Char & 0xe0) == 0xc0) {                         \
        Len = 2;                                                \
        Mask = 0x1f;                                            \
    } else if ((Char & 0xf0) == 0xe0) {                         \
        Len = 3;                                                \
        Mask = 0x0f;                                            \
    } else if ((Char & 0xf8) == 0xf0) {                         \
        Len = 4;                                                \
        Mask = 0x07;                                            \
    } else if ((Char & 0xfc) == 0xf8) {                         \
        Len = 5;                                                \
        Mask = 0x03;                                            \
    } else if ((Char & 0xfe) == 0xfc) {                         \
        Len = 6;                                                \
        Mask = 0x01;                                            \
    } else {                                                    \
        Len = -1;                                               \
    }

#define UTF8_LENGTH(Char)           ((Char) < 0x80 ? 1 : ((Char) < 0x800 ? 2 : ((Char) < 0x10000 ? 3 : ((Char) < 0x200000 ? 4 : ((Char) < 0x4000000 ? 5 : 6)))))

#define UTF8_GET(Result, Chars, Count, Mask, Len)               \
    (Result) = (Chars)[0] & (Mask);                             \
    for ((Count) = 1; (Count) < (Len); ++(Count)) {             \
        if (((Chars)[(Count)] & 0xc0) != 0x80) {                \
            (Result) = -1;                                      \
            break;                                              \
        }                                                       \
        (Result) <<= 6;                                         \
        (Result) |= ((Chars)[(Count)] & 0x3f);                  \
    }

#define UNICODE_VALID(Char)                                     \
    ((Char) < 0x110000 && (((Char) & 0xFFFFF800) != 0xD800))

static const xchar utf8_skip_data[256] = {
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,6,6,1,1
};

const xchar *const x_utf8_skip = utf8_skip_data;

xchar *x_utf8_find_prev_char(const xchar *str, const xchar *p)
{
    while (p > str) {
        --p;
        if ((*p & 0xc0) != 0x80) {
            return (xchar *)p;
        }
    }

    return NULL;
}

xchar *x_utf8_find_next_char(const xchar *p, const xchar *end)
{
    if (end) {
        for (++p; p < end && (*p & 0xc0) == 0x80; ++p);
        return (p >= end) ? NULL : (xchar *)p;
    } else {
        for (++p; (*p & 0xc0) == 0x80; ++p);
        return (xchar *)p;
    }
}

xchar *x_utf8_prev_char(const xchar *p)
{
    while (TRUE) {
        p--;
        if ((*p & 0xc0) != 0x80) {
            return (xchar *)p;
        }
    }
}

xlong x_utf8_strlen(const xchar *p, xssize max)
{
    xlong len = 0;
    const xchar *start = p;

    x_return_val_if_fail(p != NULL || max == 0, 0);

    if (max < 0) {
        while (*p) {
            p = x_utf8_next_char(p);
            ++len;
        }
    } else {
        if (max == 0 || !*p) {
            return 0;
        }

        p = x_utf8_next_char(p);
        while (p - start < max && *p) {
            ++len;
            p = x_utf8_next_char(p);
        }

        if (p - start <= max) {
            ++len;
        }
    }

    return len;
}

xchar *x_utf8_substring(const xchar *str, xlong start_pos, xlong end_pos)
{
    xchar *start, *end, *out;

    x_return_val_if_fail(end_pos >= start_pos || end_pos == -1, NULL);

    start = x_utf8_offset_to_pointer(str, start_pos);
    if (end_pos == -1) {
        xlong length = x_utf8_strlen(start, -1);
        end = x_utf8_offset_to_pointer(start, length);
    } else {
        end = x_utf8_offset_to_pointer(start, end_pos - start_pos);
    }

    out = (xchar *)x_malloc(end - start + 1);
    memcpy(out, start, end - start);
    out[end - start] = 0;

    return out;
}

xunichar x_utf8_get_char(const xchar *p)
{
    xunichar result;
    int i, mask = 0, len;
    unsigned char c = (unsigned char)*p;

    UTF8_COMPUTE(c, mask, len);
    if (len == -1) {
        return (xunichar)-1;
    }
    UTF8_GET(result, p, i, mask, len);

    return result;
}

xchar *x_utf8_offset_to_pointer(const xchar *str, xlong offset)
{
    const xchar *s = str;

    if (offset > 0) {
        while (offset--) {
            s = x_utf8_next_char(s);
        }
    } else {
        const char *s1;

        while (offset) {
            s1 = s;
            s += offset;
            while ((*s & 0xc0) == 0x80) {
                s--;
            }

            offset += x_utf8_pointer_to_offset(s, s1);
        }
    }

    return (xchar *)s;
}

xlong x_utf8_pointer_to_offset(const xchar *str, const xchar *pos)
{
    xlong offset = 0;
    const xchar *s = str;

    if (pos < str) {
        offset = - x_utf8_pointer_to_offset(pos, str);
    } else {
        while (s < pos) {
            s = x_utf8_next_char(s);
            offset++;
        }
    }

    return offset;
}

xchar *x_utf8_strncpy(xchar *dest, const xchar *src, xsize n)
{
    const xchar *s = src;
    while (n && *s) {
        s = x_utf8_next_char(s);
        n--;
    }

    strncpy(dest, src, s - src);
    dest[s - src] = 0;

    return dest;
}

xchar *x_utf8_truncate_middle(const xchar *string, xsize truncate_length)
{
    const xchar *ellipsis = "…";
    const xsize ellipsis_bytes = strlen(ellipsis);

    xsize length;
    xchar *result;
    xsize left_bytes;
    xsize right_bytes;
    xchar *left_substring_end;
    xchar *right_substring_end;
    xsize left_substring_length;
    xchar *right_substring_begin;

    x_return_val_if_fail(string != NULL, NULL);

    length = x_utf8_strlen(string, -1);
    if (length <= truncate_length) {
        return x_strdup(string);
    }

    if (truncate_length == 0) {
        return x_strdup("");
    }

    truncate_length -= 1;
    left_substring_length = truncate_length / 2;

    left_substring_end = x_utf8_offset_to_pointer(string, left_substring_length);
    right_substring_begin = x_utf8_offset_to_pointer(left_substring_end, length - truncate_length);
    right_substring_end = x_utf8_offset_to_pointer(right_substring_begin, truncate_length - left_substring_length);

    x_assert(*right_substring_end == '\0');

    left_bytes = left_substring_end - string;
    right_bytes = right_substring_end - right_substring_begin;

    result = x_malloc(left_bytes + ellipsis_bytes + right_bytes + 1);

    strncpy(result, string, left_bytes);
    memcpy(result + left_bytes, ellipsis, ellipsis_bytes);
    strncpy(result + left_bytes + ellipsis_bytes, right_substring_begin, right_bytes);
    result[left_bytes + ellipsis_bytes + right_bytes] = '\0';

    return result;
}

int x_unichar_to_utf8(xunichar c, xchar *outbuf)
{
    int i;
    int first;
    xuint len = 0;

    if (c < 0x80) {
        first = 0;
        len = 1;
    } else if (c < 0x800) {
        first = 0xc0;
        len = 2;
    } else if (c < 0x10000) {
        first = 0xe0;
        len = 3;
    } else if (c < 0x200000) {
        first = 0xf0;
        len = 4;
    } else if (c < 0x4000000) {
        first = 0xf8;
        len = 5;
    } else {
        first = 0xfc;
        len = 6;
    }

    if (outbuf) {
        for (i = len - 1; i > 0; --i) {
            outbuf[i] = (c & 0x3f) | 0x80;
            c >>= 6;
        }

        outbuf[0] = c | first;
    }

    return len;
}

xchar *x_utf8_strchr(const char *p, xssize len, xunichar c)
{
    xchar ch[10];

    xint charlen = x_unichar_to_utf8(c, ch);
    ch[charlen] = '\0';

    return x_strstr_len(p, len, ch);
}

xchar *x_utf8_strrchr(const char *p, xssize len, xunichar c)
{
    xchar ch[10];

    xint charlen = x_unichar_to_utf8(c, ch);
    ch[charlen] = '\0';

    return x_strrstr_len(p, len, ch);
}

static inline xunichar x_utf8_get_char_extended(const xchar *p, xssize max_len)
{
    xsize i, len;
    xunichar min_code;
    xunichar wc = (xuchar)*p;
    const xunichar partial_sequence = (xunichar) -2;
    const xunichar malformed_sequence = (xunichar) -1;

    if (wc < 0x80) {
        return wc;
    } else if (X_UNLIKELY(wc < 0xc0)) {
        return malformed_sequence;
    } else if (wc < 0xe0) {
        len = 2;
        wc &= 0x1f;
        min_code = 1 << 7;
    } else if (wc < 0xf0) {
        len = 3;
        wc &= 0x0f;
        min_code = 1 << 11;
    } else if (wc < 0xf8) {
        len = 4;
        wc &= 0x07;
        min_code = 1 << 16;
    } else if (wc < 0xfc) {
        len = 5;
        wc &= 0x03;
        min_code = 1 << 21;
    } else if (wc < 0xfe) {
        len = 6;
        wc &= 0x01;
        min_code = 1 << 26;
    } else {
        return malformed_sequence;
    }

    if (X_UNLIKELY(max_len >= 0 && len > (xsize) max_len)) {
        for (i = 1; i < (xsize) max_len; i++) {
            if ((((xuchar *)p)[i] & 0xc0) != 0x80) {
                return malformed_sequence;
            }
        }

        return partial_sequence;
    }

    for (i = 1; i < len; ++i) {
        xunichar ch = ((xuchar *)p)[i];

        if (X_UNLIKELY((ch & 0xc0) != 0x80)) {
            if (ch) {
                return malformed_sequence;
            } else {
                return partial_sequence;
            }
        }

        wc <<= 6;
        wc |= (ch & 0x3f);
    }

    if (X_UNLIKELY(wc < min_code)) {
        return malformed_sequence;
    }

    return wc;
}

xunichar x_utf8_get_char_validated(const xchar *p, xssize max_len)
{
    xunichar result;

    if (max_len == 0) {
        return (xunichar)-2;
    }

    result = x_utf8_get_char_extended(p, max_len);
    if (result == 0 && max_len > 0) {
        return (xunichar) -2;
    }

    if (result & 0x80000000) {
        return result;
    } else if (!UNICODE_VALID(result)) {
        return (xunichar)-1;
    } else {
        return result;
    }
}

#define CONT_BYTE_FAST(p)       ((xuchar)*p++ & 0x3f)

xunichar *x_utf8_to_ucs4_fast(const xchar *str, xlong len, xlong *items_written)
{
    const xchar *p;
    xint n_chars, i;
    xunichar *result;

    x_return_val_if_fail(str != NULL, NULL);

    p = str;
    n_chars = 0;

    if (len < 0) {
        while (*p) {
            p = x_utf8_next_char(p);
            ++n_chars;
        }
    } else {
        while (p < str + len && *p) {
            p = x_utf8_next_char(p);
            ++n_chars;
        }
    }

    result = x_new(xunichar, n_chars + 1);
    p = str;

    for (i = 0; i < n_chars; i++) {
        xunichar wc;
        xuchar first = (xuchar)*p++;

        if (first < 0xc0) {
            wc = first;
        } else {
            xunichar c1 = CONT_BYTE_FAST(p);
            if (first < 0xe0) {
                wc = ((first & 0x1f) << 6) | c1;
            } else {
                xunichar c2 = CONT_BYTE_FAST(p);
                if (first < 0xf0) {
                    wc = ((first & 0x0f) << 12) | (c1 << 6) | c2;
                } else {
                    xunichar c3 = CONT_BYTE_FAST(p);
                    wc = ((first & 0x07) << 18) | (c1 << 12) | (c2 << 6) | c3;

                    if (X_UNLIKELY(first >= 0xf8)) {
                        xunichar mask = 1 << 20;
                        while ((wc & mask) != 0) {
                            wc <<= 6;
                            wc |= CONT_BYTE_FAST(p);
                            mask <<= 5;
                        }

                        wc &= mask - 1;
                    }
                }
            }
        }

        result[i] = wc;
    }

    result[i] = 0;
    if (items_written) {
        *items_written = i;
    }

    return result;
}

static xpointer try_malloc_n(xsize n_blocks, xsize n_block_bytes, XError **error)
{
    xpointer ptr = x_try_malloc_n(n_blocks, n_block_bytes);
    if (ptr == NULL) {
      x_set_error_literal(error, X_CONVERT_ERROR, X_CONVERT_ERROR_NO_MEMORY, _("Failed to allocate memory"));
    }

    return ptr;
}

xunichar *x_utf8_to_ucs4(const xchar *str, xlong len, xlong *items_read, xlong *items_written, XError **error)
{
    xint n_chars, i;
    const xchar *in;
    xunichar *result = NULL;

    in = str;
    n_chars = 0;

    while ((len < 0 || str + len - in > 0) && *in) {
        xunichar wc = x_utf8_get_char_extended(in, len < 0 ? 6 : str + len - in);
        if (wc & 0x80000000) {
            if (wc == (xunichar)-2) {
                if (items_read) {
                    break;
                } else {
                    x_set_error_literal(error, X_CONVERT_ERROR, X_CONVERT_ERROR_PARTIAL_INPUT, _("Partial character sequence at end of input"));
                }
            } else {
                x_set_error_literal(error, X_CONVERT_ERROR, X_CONVERT_ERROR_ILLEGAL_SEQUENCE, _("Invalid byte sequence in conversion input"));
            }

            goto err_out;
        }

        n_chars++;
        in = x_utf8_next_char(in);
    }

    result = (xunichar *)try_malloc_n(n_chars + 1, sizeof(xunichar), error);
    if (result == NULL) {
        goto err_out;
    }

    in = str;
    for (i=0; i < n_chars; i++) {
        result[i] = x_utf8_get_char(in);
        in = x_utf8_next_char(in);
    }
    result[i] = 0;

    if (items_written) {
        *items_written = n_chars;
    }

err_out:
    if (items_read) {
        *items_read = in - str;
    }

    return result;
}

xchar *x_ucs4_to_utf8(const xunichar *str, xlong len, xlong *items_read, xlong *items_written, XError **error)
{
    xint i;
    xchar *p;
    xint result_length;
    xchar *result = NULL;

    result_length = 0;
    for (i = 0; len < 0 || i < len ; i++) {
        if (!str[i]) {
            break;
        }

        if (str[i] >= 0x80000000) {
            x_set_error_literal(error, X_CONVERT_ERROR, X_CONVERT_ERROR_ILLEGAL_SEQUENCE, _("Character out of range for UTF-8"));
            goto err_out;
        }

        result_length += UTF8_LENGTH(str[i]);
    }

    result = (xchar *)try_malloc_n(result_length + 1, 1, error);
    if (result == NULL) {
        goto err_out;
    }
    p = result;

    i = 0;
    while (p < result + result_length) {
        p += x_unichar_to_utf8(str[i++], p);
    }
    *p = '\0';

    if (items_written) {
        *items_written = p - result;
    }

err_out:
    if (items_read) {
        *items_read = i;
    }

    return result;
}

#define SURROGATE_VALUE(h, l)           (((h) - 0xd800) * 0x400 + (l) - 0xdc00 + 0x10000)

xchar *x_utf16_to_utf8(const xunichar2  *str, xlong len, xlong *items_read, xlong *items_written, XError **error)
{
    xchar *out;
    xint n_bytes;
    const xunichar2 *in;
    xchar *result = NULL;
    xunichar high_surrogate;

    x_return_val_if_fail(str != NULL, NULL);

    n_bytes = 0;
    in = str;
    high_surrogate = 0;

    while ((len < 0 || in - str < len) && *in) {
        xunichar wc;
        xunichar2 c = *in;

        if (c >= 0xdc00 && c < 0xe000) {
            if (high_surrogate) {
                wc = SURROGATE_VALUE(high_surrogate, c);
                high_surrogate = 0;
            } else {
                x_set_error_literal(error, X_CONVERT_ERROR, X_CONVERT_ERROR_ILLEGAL_SEQUENCE, _("Invalid sequence in conversion input"));
                goto err_out;
            }
        } else {
            if (high_surrogate) {
                x_set_error_literal(error, X_CONVERT_ERROR, X_CONVERT_ERROR_ILLEGAL_SEQUENCE, _("Invalid sequence in conversion input"));
                goto err_out;
            }

            if (c >= 0xd800 && c < 0xdc00) {
                high_surrogate = c;
                goto next1;
            } else {
                wc = c;
            }
        }

        n_bytes += UTF8_LENGTH (wc);
next1:
        in++;
    }

    if (high_surrogate && !items_read) {
        x_set_error_literal(error, X_CONVERT_ERROR, X_CONVERT_ERROR_PARTIAL_INPUT, _("Partial character sequence at end of input"));
        goto err_out;
    }

    result = (xchar *)try_malloc_n(n_bytes + 1, 1, error);
    if (result == NULL) {
        goto err_out;
    }

    high_surrogate = 0;
    out = result;
    in = str;

    while (out < result + n_bytes) {
        xunichar wc;
        xunichar2 c = *in;

        if (c >= 0xdc00 && c < 0xe000) {
            wc = SURROGATE_VALUE(high_surrogate, c);
            high_surrogate = 0;
        } else if (c >= 0xd800 && c < 0xdc00) {
            high_surrogate = c;
            goto next2;
        } else {
            wc = c;
        }

        out += x_unichar_to_utf8(wc, out);
next2:
        in++;
    }

    *out = '\0';

    if (items_written) {
        *items_written = out - result;
    }

err_out:
    if (items_read) {
        *items_read = in - str;
    }

    return result;
}

xunichar *x_utf16_to_ucs4(const xunichar2 *str, xlong len, xlong *items_read, xlong *items_written, XError **error)
{
    xchar *out;
    size_t n_bytes;
    const xunichar2 *in;
    xchar *result = NULL;
    xunichar high_surrogate;

    x_return_val_if_fail(str != NULL, NULL);

    n_bytes = 0;
    in = str;
    high_surrogate = 0;

    while ((len < 0 || in - str < len) && *in) {
        xunichar2 c = *in;

        if (c >= 0xdc00 && c < 0xe000) {
            if (high_surrogate) {
                high_surrogate = 0;
            } else {
                x_set_error_literal(error, X_CONVERT_ERROR, X_CONVERT_ERROR_ILLEGAL_SEQUENCE, _("Invalid sequence in conversion input"));
                goto err_out;
            }
        } else {
            if (high_surrogate) {
                x_set_error_literal(error, X_CONVERT_ERROR, X_CONVERT_ERROR_ILLEGAL_SEQUENCE, _("Invalid sequence in conversion input"));
                goto err_out;
            }

            if (c >= 0xd800 && c < 0xdc00) {
                high_surrogate = c;
                goto next1;
            }
        }

        n_bytes += sizeof(xunichar);

next1:
        in++;
    }

    if (high_surrogate && !items_read) {
        x_set_error_literal(error, X_CONVERT_ERROR, X_CONVERT_ERROR_PARTIAL_INPUT, _("Partial character sequence at end of input"));
        goto err_out;
    }

    result = (xchar *)try_malloc_n(n_bytes + 4, 1, error);
    if (result == NULL) {
        goto err_out;
    }

    high_surrogate = 0;
    out = result;
    in = str;

    while (out < result + n_bytes) {
        xunichar wc;
        xunichar2 c = *in;

        if (c >= 0xdc00 && c < 0xe000) {
            wc = SURROGATE_VALUE(high_surrogate, c);
            high_surrogate = 0;
        } else if (c >= 0xd800 && c < 0xdc00) {
            high_surrogate = c;
            goto next2;
        } else {
            wc = c;
        }

        *(xunichar *)out = wc;
        out += sizeof(xunichar);

next2:
        in++;
    }

    *(xunichar *)out = 0;

    if (items_written) {
        *items_written = (out - result) / sizeof(xunichar);
    }

err_out:
    if (items_read) {
        *items_read = in - str;
    }

    return (xunichar *)result;
}

xunichar2 *x_utf8_to_utf16(const xchar *str, xlong len, xlong *items_read, xlong *items_written, XError **error)
{
    xint i;
    xint n16;
    const xchar *in;
    xunichar2 *result = NULL;

    x_return_val_if_fail(str != NULL, NULL);

    in = str;
    n16 = 0;

    while ((len < 0 || str + len - in > 0) && *in) {
        xunichar wc = x_utf8_get_char_extended(in, len < 0 ? 6 : str + len - in);
        if (wc & 0x80000000) {
            if (wc == (xunichar)-2) {
                if (items_read) {
                    break;
                } else {
                    x_set_error_literal(error, X_CONVERT_ERROR, X_CONVERT_ERROR_PARTIAL_INPUT, _("Partial character sequence at end of input"));
                }
            } else {
                x_set_error_literal(error, X_CONVERT_ERROR, X_CONVERT_ERROR_ILLEGAL_SEQUENCE, _("Invalid byte sequence in conversion input"));
            }

            goto err_out;
        }

        if (wc < 0xd800) {
            n16 += 1;
        } else if (wc < 0xe000) {
        x_set_error_literal(error, X_CONVERT_ERROR, X_CONVERT_ERROR_ILLEGAL_SEQUENCE, _("Invalid sequence in conversion input"));
            goto err_out;
        } else if (wc < 0x10000) {
            n16 += 1;
        } else if (wc < 0x110000) {
            n16 += 2;
        } else {
            x_set_error_literal(error, X_CONVERT_ERROR, X_CONVERT_ERROR_ILLEGAL_SEQUENCE, _("Character out of range for UTF-16"));
            goto err_out;
        }
        
        in = x_utf8_next_char(in);
    }

    result = (xunichar2 *)try_malloc_n(n16 + 1, sizeof(xunichar2), error);
    if (result == NULL) {
        goto err_out;
    }

    in = str;
    for (i = 0; i < n16;) {
        xunichar wc = x_utf8_get_char(in);

        if (wc < 0x10000) {
            result[i++] = wc;
        } else {
            result[i++] = (wc - 0x10000) / 0x400 + 0xd800;
            result[i++] = (wc - 0x10000) % 0x400 + 0xdc00;
        }

        in = x_utf8_next_char(in);
    }

    result[i] = 0;
    if (items_written) {
        *items_written = n16;
    }

err_out:
    if (items_read) {
        *items_read = in - str;
    }

    return result;
}

xunichar2 *x_ucs4_to_utf16(const xunichar *str, xlong len, xlong *items_read, xlong *items_written, XError **error)
{
    xint n16;
    xint i, j;
    xunichar2 *result = NULL;

    n16 = 0;
    i = 0;

    while ((len < 0 || i < len) && str[i]) {
        xunichar wc = str[i];

        if (wc < 0xd800) {
            n16 += 1;
        } else if (wc < 0xe000) {
            x_set_error_literal(error, X_CONVERT_ERROR, X_CONVERT_ERROR_ILLEGAL_SEQUENCE, _("Invalid sequence in conversion input"));
            goto err_out;
        } else if (wc < 0x10000) {
            n16 += 1;
        } else if (wc < 0x110000) {
            n16 += 2;
        } else {
            x_set_error_literal(error, X_CONVERT_ERROR, X_CONVERT_ERROR_ILLEGAL_SEQUENCE, _("Character out of range for UTF-16"));
            goto err_out;
        }

        i++;
    }

    result = (xunichar2 *)try_malloc_n(n16 + 1, sizeof(xunichar2), error);
    if (result == NULL) {
        goto err_out;
    }

    for (i = 0, j = 0; j < n16; i++) {
        xunichar wc = str[i];

        if (wc < 0x10000) {
            result[j++] = wc;
        } else {
            result[j++] = (wc - 0x10000) / 0x400 + 0xd800;
            result[j++] = (wc - 0x10000) % 0x400 + 0xdc00;
        }
    }
    result[j] = 0;

    if (items_written) {
        *items_written = n16;
    }

err_out:
    if (items_read) {
        *items_read = i;
    }

    return result;
}

#define align_to(_val, _to)         (((_val) + (_to) - 1) & ~((_to) - 1))

static inline xuint8 load_u8(xconstpointer memory, xsize offset)
{
    return ((const xuint8 *)memory)[offset];
}

#if X_GNUC_CHECK_VERSION(4,8) || defined(__clang__)
# define _attribute_aligned(n)      __attribute__((aligned(n)))
#elif defined(_MSC_VER)
#define _attribute_aligned(n)       __declspec(align(n))
#else
#define _attribute_aligned(n)
#endif

static inline xsize load_word(xconstpointer memory, xsize offset)
{
#if XLIB_SIZEOF_VOID_P == 8
    _attribute_aligned(8) const xuint8 *m = ((const xuint8 *)memory) + offset;
    return ((xuint64)m[0] <<  0) | ((xuint64)m[1] <<  8) |
         ((xuint64)m[2] << 16) | ((xuint64)m[3] << 24) |
         ((xuint64)m[4] << 32) | ((xuint64)m[5] << 40) |
         ((xuint64)m[6] << 48) | ((xuint64)m[7] << 56);
#else
    _attribute_aligned(4) const xuint8 *m = ((const xuint8 *)memory) + offset;
    return ((xuint)m[0] <<  0) | ((xuint)m[1] <<  8) | ((xuint)m[2] << 16) | ((xuint)m[3] << 24);
#endif
}

#define UTF8_ASCII_MASK         ((xsize)0x8080808080808080L)
#define UTF8_ASCII_SUB          ((xsize)0x0101010101010101L)

static inline int utf8_word_is_ascii(xsize word)
{
    return ((((word - UTF8_ASCII_SUB) | word) & UTF8_ASCII_MASK) == 0);
}

static void utf8_verify_ascii(const char **strp, xsize *lenp)
{
    const char *str = *strp;
    xsize len = lenp ? *lenp : (xsize)-1;

    while (len > 0 && load_u8(str, 0) < 128) {
        if ((xpointer)align_to((xuintptr)str, sizeof(xsize)) == str) {
            while (len >= 2 * sizeof(xsize)) {
                if (!utf8_word_is_ascii(load_word (str, 0)) || !utf8_word_is_ascii(load_word (str, sizeof(xsize)))) {
                    break;
                }

                str += 2 * sizeof(xsize);
                len -= 2 * sizeof(xsize);
            }

            while (len > 0 && load_u8(str, 0) < 128) {
                if X_UNLIKELY(load_u8(str, 0) == 0x00) {
                    goto out;
                }

                ++str;
                --len;
            }
        } else {
            if X_UNLIKELY(load_u8 (str, 0) == 0x00) {
                goto out;
            }
        }
    }

out:
    *strp = str;
    if (lenp) {
        *lenp = len;
    }
}

#define UTF8_CHAR_IS_TAIL(_x)       (((_x) & 0xC0) == 0x80)

static void utf8_verify(const char **strp, xsize *lenp)
{
    const char *str = *strp;
    xsize len = lenp ? *lenp : (xsize)-1;

    while (len > 0) {
        xuint8 b = load_u8(str, 0);
        if (b == 0x00) {
            goto out;
        } else if (b <= 0x7F) {
            utf8_verify_ascii((const char **)&str, &len);
        } else if (b >= 0xC2 && b <= 0xDF) {
            if X_UNLIKELY(len < 2) {
                goto out;
            }

            if X_UNLIKELY(!UTF8_CHAR_IS_TAIL(load_u8(str, 1))) {
                goto out;
            }

            str += 2;
            len -= 2;
        } else if (b == 0xE0) {
            if X_UNLIKELY(len < 3) {
                goto out;
            }

            if X_UNLIKELY(load_u8(str, 1) < 0xA0 || load_u8(str, 1) > 0xBF) {
                goto out;
            }

            if X_UNLIKELY(!UTF8_CHAR_IS_TAIL(load_u8(str, 2))) {
                goto out;
            }

            str += 3;
            len -= 3;
        } else if (b >= 0xE1 && b <= 0xEC) {
            if X_UNLIKELY(len < 3) {
                goto out;
            }

            if X_UNLIKELY(!UTF8_CHAR_IS_TAIL(load_u8(str, 1))) {
                goto out;
            }

            if X_UNLIKELY(!UTF8_CHAR_IS_TAIL(load_u8(str, 2))) {
                goto out;
            }

            str += 3;
            len -= 3;
        } else if (b == 0xED) {
            if X_UNLIKELY(len < 3) {
                goto out;
            }

            if X_UNLIKELY(load_u8(str, 1) < 0x80 || load_u8(str, 1) > 0x9F) {
                goto out;
            }

            if X_UNLIKELY(!UTF8_CHAR_IS_TAIL(load_u8(str, 2))) {
                goto out;
            }

            str += 3;
            len -= 3;
        } else if (b >= 0xEE && b <= 0xEF) {
            if X_UNLIKELY(len < 3) {
                goto out;
            }

            if X_UNLIKELY(!UTF8_CHAR_IS_TAIL(load_u8(str, 1))) {
                goto out;
            }

            if X_UNLIKELY(!UTF8_CHAR_IS_TAIL(load_u8(str, 2))) {
                goto out;
            }

            str += 3;
            len -= 3;
        } else if (b == 0xF0) {
            if X_UNLIKELY(len < 4) {
                goto out;
            }

            if X_UNLIKELY(load_u8(str, 1) < 0x90 || load_u8(str, 1) > 0xBF) {
                goto out;
            }

            if X_UNLIKELY(!UTF8_CHAR_IS_TAIL(load_u8(str, 2))) {
                goto out;
            }

            if X_UNLIKELY(!UTF8_CHAR_IS_TAIL(load_u8(str, 3))) {
                goto out;
            }

            str += 4;
            len -= 4;
        } else if (b >= 0xF1 && b <= 0xF3) {
            if X_UNLIKELY(len < 4) {
                goto out;
            }

            if X_UNLIKELY(!UTF8_CHAR_IS_TAIL(load_u8(str, 1))) {
                goto out;
            }

            if X_UNLIKELY(!UTF8_CHAR_IS_TAIL(load_u8(str, 2))) {
                goto out;
            }

            if X_UNLIKELY(!UTF8_CHAR_IS_TAIL(load_u8(str, 3))) {
                goto out;
            }

            str += 4;
            len -= 4;
        } else if (b == 0xF4) {
            if X_UNLIKELY(len < 4) {
                goto out;
            }

            if X_UNLIKELY(load_u8(str, 1) < 0x80 || load_u8(str, 1) > 0x8F) {
                goto out;
            }

            if X_UNLIKELY(!UTF8_CHAR_IS_TAIL(load_u8(str, 2))) {
                goto out;
            }

            if X_UNLIKELY(!UTF8_CHAR_IS_TAIL(load_u8(str, 3))) {
                goto out;
            }

            str += 4;
            len -= 4;
        } else {
            goto out;
        }
    }

out:
    *strp = str;
    if (lenp) {
        *lenp = len;
    }
}

xboolean x_utf8_validate(const char *str, xssize max_len, const xchar **end)
{
    if (max_len >= 0) {
        return x_utf8_validate_len(str, max_len, end);
    }

    utf8_verify(&str, NULL);
    if (end != NULL) {
        *end = str;
    }

    return *str == 0;
}

xboolean x_utf8_validate_len(const char *str, xsize max_len, const xchar **end)
{
    utf8_verify(&str, &max_len);

    if (end != NULL) {
        *end = str;
    }

    return max_len == 0;
}

xboolean x_str_is_ascii(const xchar *str)
{
    utf8_verify_ascii(&str, NULL);
    return *str == 0;
}

xboolean x_unichar_validate(xunichar ch)
{
    return UNICODE_VALID(ch);
}

xchar *x_utf8_strreverse(const xchar *str, xssize len)
{
    const xchar *p;
    xchar *r, *result;

    if (len < 0) {
        len = strlen(str);
    }

    result = x_new(xchar, len + 1);
    r = result + len;
    p = str;

    while (r > result) {
        xchar *m, skip = x_utf8_skip[*(xuchar *)p];
        r -= skip;
        x_assert(r >= result);
        for (m = r; skip; skip--) {
            *m++ = *p++;
        }
    }
    result[len] = 0;

    return result;
}

xchar *x_utf8_make_valid(const xchar *str, xssize len)
{
    XString *string;
    const xchar *remainder, *invalid;
    xsize remaining_bytes, valid_bytes;

    x_return_val_if_fail(str != NULL, NULL);

    if (len < 0) {
        len = strlen(str);
    }

    string = NULL;
    remainder = str;
    remaining_bytes = len;

    while (remaining_bytes != 0)  {
        if (x_utf8_validate(remainder, remaining_bytes, &invalid)) {
            break;
        }
        valid_bytes = invalid - remainder;
        
        if (string == NULL) {
            string = x_string_sized_new(remaining_bytes);
        }

        x_string_append_len(string, remainder, valid_bytes);
        x_string_append(string, "\357\277\275");

        remaining_bytes -= valid_bytes + 1;
        remainder = invalid + 1;
    }

    if (string == NULL) {
        return x_strndup(str, len);
    }

    x_string_append_len(string, remainder, remaining_bytes);
    x_string_append_c(string, '\0');

    x_assert(x_utf8_validate(string->str, -1, NULL));

    return x_string_free(string, FALSE);
}
