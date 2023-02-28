#include <wchar.h>
#include <locale.h>
#include <string.h>

#include <xlib/xlib/config.h>
#include <xlib/xlib/xmem.h>
#include <xlib/xlib/xstring.h>
#include <xlib/xlib/xcharset.h>
#include <xlib/xlib/xconvert.h>
#include <xlib/xlib/xunicode.h>
#include <xlib/xlib/xstrfuncs.h>
#include <xlib/xlib/xtestutils.h>
#include <xlib/xlib/xunicodeprivate.h>

#if SIZEOF_WCHAR_T == 4 && defined(__STDC_ISO_10646__)
#define GUNICHAR_EQUALS_WCHAR_T             1
#endif

#define COLLATION_SENTINEL                  "\1\1\1"

xint x_utf8_collate(const xchar *str1, const xchar *str2)
{
    xint result;
    xunichar *str1_norm;
    xunichar *str2_norm;

    x_return_val_if_fail(str1 != NULL, 0);
    x_return_val_if_fail(str2 != NULL, 0);

    str1_norm = _x_utf8_normalize_wc(str1, -1, X_NORMALIZE_ALL_COMPOSE);
    str2_norm = _x_utf8_normalize_wc(str2, -1, X_NORMALIZE_ALL_COMPOSE);

    result = wcscoll((wchar_t *)str1_norm, (wchar_t *)str2_norm);

    x_free(str1_norm);
    x_free(str2_norm);

    return result;
}

static inline int utf8_encode (char *buf, wchar_t val)
{
    int retval;

    if (val < 0x80) {
        if (buf) {
            *buf++ = (char)val;
        }

        retval = 1;
    } else {
        int step;

        for (step = 2; step < 6; ++step) {
            if ((val & (~(xuint32)0 << (5 * step + 1))) == 0) {
                break;
            }
        }
        retval = step;

        if (buf) {
            *buf = (unsigned char)(~0xff >> step);
            --step;
            do {
                buf[step] = 0x80 | (val & 0x3f);
                val >>= 6;
            } while (--step > 0);
            *buf |= val;
        }
    }

    return retval;
}

xchar *x_utf8_collate_key(const xchar *str, xssize len)
{
    xsize i;
    xchar *result;
    xsize xfrm_len;
    xunichar *str_norm;
    wchar_t *result_wc;
    xsize result_len = 0;

    x_return_val_if_fail(str != NULL, NULL);

    str_norm = _x_utf8_normalize_wc(str, len, X_NORMALIZE_ALL_COMPOSE);

    xfrm_len = wcsxfrm(NULL, (wchar_t *)str_norm, 0);
    result_wc = x_new(wchar_t, xfrm_len + 1);
    wcsxfrm(result_wc, (wchar_t *)str_norm, xfrm_len + 1);

    for (i = 0; i < xfrm_len; i++) {
        result_len += utf8_encode(NULL, result_wc[i]);
    }

    result = (xchar *)x_malloc(result_len + 1);
    result_len = 0;
    for (i = 0; i < xfrm_len; i++) {
        result_len += utf8_encode(result + result_len, result_wc[i]);
    }
    result[result_len] = '\0';

    x_free(result_wc);
    x_free(str_norm);

    return result;
}

xchar *x_utf8_collate_key_for_filename(const xchar *str, xssize len)
{
    xint digits;
    const xchar *p;
    XString *result;
    XString *append;
    const xchar *end;
    const xchar *prev;
    xchar *collate_key;
    xint leading_zeros;

    if (len < 0) {
        len = strlen(str);
    }

    result = x_string_sized_new(len * 2);
    append = x_string_sized_new(0);

    end = str + len;

    for (prev = p = str; p < end; p++) {
        switch (*p) {
            case '.':
                if (prev != p) {
                    collate_key = x_utf8_collate_key(prev, p - prev);
                    x_string_append(result, collate_key);
                    x_free(collate_key);
                }

                x_string_append(result, COLLATION_SENTINEL "\1");
                prev = p + 1;
                break;

            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                if (prev != p)  {
                    collate_key = x_utf8_collate_key(prev, p - prev);
                    x_string_append(result, collate_key);
                    x_free(collate_key);
                }

                x_string_append(result, COLLATION_SENTINEL "\2");
                prev = p;

                if (*p == '0') {
                    leading_zeros = 1;
                    digits = 0;
                } else {
                    leading_zeros = 0;
                    digits = 1;
                }

                while (++p < end) {
                    if (*p == '0' && !digits) {
                        ++leading_zeros;
                    } else if (x_ascii_isdigit(*p)) {
                        ++digits;
                    } else {
                        if (!digits) {
                            ++digits;
                            --leading_zeros;
                        }

                        break;
                    }
                }

                while (digits > 1) {
                    x_string_append_c(result, ':');
                    --digits;
                }

                if (leading_zeros > 0) {
                    x_string_append_c(append, (char)leading_zeros);
                    prev += leading_zeros;
                }

                x_string_append_len(result, prev, p - prev);
                prev = p;
                --p;
                break;

            default:
                break;
        }
    }

    if (prev != p)  {
        collate_key = x_utf8_collate_key(prev, p - prev);
        x_string_append(result, collate_key);
        x_free(collate_key);
    }

    x_string_append(result, append->str);
    x_string_free(append, TRUE);

    return x_string_free(result, FALSE);
}
