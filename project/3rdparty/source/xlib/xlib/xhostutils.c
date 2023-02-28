#include <unistd.h>
#include <string.h>

#include <xlib/xlib/config.h>
#include <xlib/xlib/xmem.h>
#include <xlib/xlib/xarray.h>
#include <xlib/xlib/xstring.h>
#include <xlib/xlib/xlibintl.h>
#include <xlib/xlib/xstrfuncs.h>
#include <xlib/xlib/xlibconfig.h>
#include <xlib/xlib/xhostutils.h>

#define IDNA_ACE_PREFIX                     "xn--"
#define IDNA_ACE_PREFIX_LEN                 4

#define PUNYCODE_BASE                       36
#define PUNYCODE_TMIN                       1
#define PUNYCODE_TMAX                       26
#define PUNYCODE_SKEW                       38
#define PUNYCODE_DAMP                       700
#define PUNYCODE_INITIAL_BIAS               72
#define PUNYCODE_INITIAL_N                  0x80

#define PUNYCODE_IS_BASIC(cp)               ((xuint)(cp) < 0x80)

static inline xchar encode_digit(xuint dig)
{
    if (dig < 26) {
        return dig + 'a';
    } else {
        return dig - 26 + '0';
    }
}

static inline xuint decode_digit(xchar dig)
{
    if (dig >= 'A' && dig <= 'Z') {
        return dig - 'A';
    } else if (dig >= 'a' && dig <= 'z') {
        return dig - 'a';
    } else if (dig >= '0' && dig <= '9') {
        return dig - '0' + 26;
    } else {
        return X_MAXUINT;
    }
}

static xuint adapt(xuint delta, xuint numpoints, xboolean firsttime)
{
    xuint k;

    delta = firsttime ? delta / PUNYCODE_DAMP : delta / 2;
    delta += delta / numpoints;

    k = 0;
    while (delta > ((PUNYCODE_BASE - PUNYCODE_TMIN) * PUNYCODE_TMAX) / 2) {
        delta /= PUNYCODE_BASE - PUNYCODE_TMIN;
        k += PUNYCODE_BASE;
    }

    return k + ((PUNYCODE_BASE - PUNYCODE_TMIN + 1) * delta / (delta + PUNYCODE_SKEW));
}

static xboolean punycode_encode(const xchar *input_utf8, xsize input_utf8_length, XString *output)
{
    xsize input_length;
    xlong written_chars;
    xunichar n, m, *input;
    xboolean success = FALSE;
    xuint delta, handled_chars, num_basic_chars, bias, j, q, k, t, digit;

    input = x_utf8_to_ucs4(input_utf8, input_utf8_length, NULL, &written_chars, NULL);
    if (!input) {
        return FALSE;
    }

    input_length = (xsize)(written_chars > 0 ? written_chars : 0);

    for (j = num_basic_chars = 0; j < input_length; j++) {
        if (PUNYCODE_IS_BASIC(input[j])) {
            x_string_append_c(output, x_ascii_tolower(input[j]));
            num_basic_chars++;
        }
    }

    if (num_basic_chars) {
        x_string_append_c(output, '-');
    }
    handled_chars = num_basic_chars;

    delta = 0;
    bias = PUNYCODE_INITIAL_BIAS;
    n = PUNYCODE_INITIAL_N;

    while (handled_chars < input_length) {
        for (m = X_MAXUINT, j = 0; j < input_length; j++) {
            if (input[j] >= n && input[j] < m) {
                m = input[j];
            }
        }

        if (m - n > (X_MAXUINT - delta) / (handled_chars + 1)) {
            goto fail;
        }

        delta += (m - n) * (handled_chars + 1);
        n = m;

        for (j = 0; j < input_length; j++) {
            if (input[j] < n) {
                if (++delta == 0) {
                    goto fail;
                }
            } else if (input[j] == n) {
                q = delta;
                for (k = PUNYCODE_BASE; ; k += PUNYCODE_BASE) {
                    if (k <= bias) {
                        t = PUNYCODE_TMIN;
                    } else if (k >= bias + PUNYCODE_TMAX) {
                        t = PUNYCODE_TMAX;
                    } else {
                        t = k - bias;
                    }

                    if (q < t) {
                        break;
                    }

                    digit = t + (q - t) % (PUNYCODE_BASE - t);
                    x_string_append_c(output, encode_digit(digit));
                    q = (q - t) / (PUNYCODE_BASE - t);
                }

                x_string_append_c(output, encode_digit(q));
                bias = adapt (delta, handled_chars + 1, handled_chars == num_basic_chars);
                delta = 0;
                handled_chars++;
            }
        }

        delta++;
        n++;
    }

    success = TRUE;

 fail:
    x_free(input);
    return success;
}

#define idna_is_junk(ch)    ((ch) == 0x00AD || (ch) == 0x1806 || (ch) == 0x200B || (ch) == 0x2060 || (ch) == 0xFEFF || (ch) == 0x034F || (ch) == 0x180B || (ch) == 0x180C || (ch) == 0x180D || (ch) == 0x200C || (ch) == 0x200D || ((ch) >= 0xFE00 && (ch) <= 0xFE0F))

static xchar *remove_junk(const xchar *str, xint len)
{
    xunichar ch;
    const xchar *p;
    XString *cleaned = NULL;

    for (p = str; len == -1 ? *p : p < str + len; p = x_utf8_next_char(p)) {
        ch = x_utf8_get_char(p);
        if (idna_is_junk(ch)) {
            if (!cleaned) {
                cleaned = x_string_new(NULL);
                x_string_append_len(cleaned, str, p - str);
            }
        } else if (cleaned) {
            x_string_append_unichar(cleaned, ch);
        }
    }

    if (cleaned) {
        return x_string_free(cleaned, FALSE);
    } else {
        return NULL;
    }
}

static inline xboolean contains_uppercase_letters(const xchar *str, xint len)
{
    const xchar *p;

    for (p = str; len == -1 ? *p : p < str + len; p = x_utf8_next_char(p)) {
        if (x_unichar_isupper(x_utf8_get_char(p))) {
            return TRUE;
        }
    }

    return FALSE;
}

static inline xboolean contains_non_ascii(const xchar *str, xint len)
{
    const xchar *p;

    for (p = str; len == -1 ? *p : p < str + len; p++) {
        if ((xuchar)*p > 0x80) {
            return TRUE;
        }
    }

    return FALSE;
}

static inline xboolean idna_is_prohibited(xunichar ch)
{
    switch (x_unichar_type(ch)) {
        case X_UNICODE_CONTROL:
        case X_UNICODE_FORMAT:
        case X_UNICODE_UNASSIGNED:
        case X_UNICODE_PRIVATE_USE:
        case X_UNICODE_SURROGATE:
        case X_UNICODE_LINE_SEPARATOR:
        case X_UNICODE_PARAGRAPH_SEPARATOR:
        case X_UNICODE_SPACE_SEPARATOR:
            return TRUE;

        case X_UNICODE_OTHER_SYMBOL:
            if (ch == 0xFFFC || ch == 0xFFFD || (ch >= 0x2FF0 && ch <= 0x2FFB)) {
                return TRUE;
            }
            return FALSE;

        case X_UNICODE_NON_SPACING_MARK:
            if (ch == 0x0340 || ch == 0x0341) {
                return TRUE;
            }
            return FALSE;

        default:
            return FALSE;
    }
}

static xchar *nameprep(const xchar *hostname, xint len, xboolean *is_unicode)
{
    xchar *name, *tmp = NULL, *p;

    name = remove_junk (hostname, len);
    if (name) {
        tmp = name;
        len = -1;
    } else {
        name = (xchar *)hostname;
    }

    if (contains_uppercase_letters(name, len)) {
        name = x_utf8_strdown(name, len);
        x_free(tmp);
        tmp = name;
        len = -1;
    }

    if (!contains_non_ascii(name, len)) {
        *is_unicode = FALSE;
        if (name == (xchar *)hostname) {
            return len == -1 ? x_strdup(hostname) : x_strndup(hostname, len);
        } else {
            return name;
        }
    }

    *is_unicode = TRUE;

    name = x_utf8_normalize(name, len, X_NORMALIZE_NFKC);
    x_free(tmp);
    tmp = name;

    if (!name) {
        return NULL;
    }

    if (contains_uppercase_letters(name, -1)) {
        name = x_utf8_strdown(name, -1);
        x_free(tmp);
        tmp = name;
    }

    for (p = name; *p; p = x_utf8_next_char(p)) {
        if (idna_is_prohibited(x_utf8_get_char(p))) {
            name = NULL;
            x_free(tmp);
            goto done;
        }
    }

done:
    return name;
}

#define idna_is_dot(str)                                                                    \
    (((xuchar)(str)[0] == '.') ||                                                           \
    ((xuchar)(str)[0] == 0xE3 && (xuchar)(str)[1] == 0x80 && (xuchar)(str)[2] == 0x82) ||   \
    ((xuchar)(str)[0] == 0xEF && (xuchar)(str)[1] == 0xBC && (xuchar)(str)[2] == 0x8E) ||   \
    ((xuchar)(str)[0] == 0xEF && (xuchar)(str)[1] == 0xBD && (xuchar)(str)[2] == 0xA1))

static const xchar *idna_end_of_label(const xchar *str)
{
    for (; *str; str = x_utf8_next_char(str)) {
        if (idna_is_dot(str)) {
            return str;
        }
    }

    return str;
}

static xsize get_hostname_max_length_bytes(void)
{
#if defined(_SC_HOST_NAME_MAX)
    xlong max = sysconf(_SC_HOST_NAME_MAX);
    if (max > 0) {
        return (xsize) max;
    }
#ifdef HOST_NAME_MAX
    return HOST_NAME_MAX;
#else
    return _POSIX_HOST_NAME_MAX;
#endif
#else
    return 255;
#endif
}

static xboolean strlen_greater_than(const xchar *str, xsize comparison_length)
{
    xsize i;

    for (i = 0; str[i] != '\0'; i++) {
        if (i > comparison_length) {
            return TRUE;
        }
    }

    return FALSE;
}

xchar *x_hostname_to_ascii(const xchar *hostname)
{
    XString *out;
    xboolean unicode;
    xssize llen, oldlen;
    xchar *name, *label, *p;
    xsize hostname_max_length_bytes = get_hostname_max_length_bytes();

    if (hostname_max_length_bytes <= X_MAXSIZE / 4 && strlen_greater_than(hostname, 4 * MAX (255, hostname_max_length_bytes))) {
        return NULL;
    }

    label = name = nameprep(hostname, -1, &unicode);
    if (!name || !unicode) {
        return name;
    }

    out = x_string_new(NULL);

    do {
        unicode = FALSE;
        for (p = label; *p && !idna_is_dot(p); p++) {
            if ((xuchar)*p > 0x80) {
                unicode = TRUE;
            }
        }

        oldlen = out->len;
        llen = p - label;
        if (unicode) {
            if (!strncmp (label, IDNA_ACE_PREFIX, IDNA_ACE_PREFIX_LEN)) {
                goto fail;
            }

            x_string_append(out, IDNA_ACE_PREFIX);
            if (!punycode_encode(label, llen, out)) {
                goto fail;
            }
        } else {
            x_string_append_len(out, label, llen);
        }

        if (out->len - oldlen > 63) {
            goto fail;
        }

        label += llen;
        if (*label) {
            label = x_utf8_next_char(label);
        }

        if (*label) {
            x_string_append_c(out, '.');
        }
    } while (*label);

    x_free(name);
    return x_string_free(out, FALSE);

fail:
    x_free(name);
    x_string_free(out, TRUE);
    return NULL;
}

xboolean x_hostname_is_non_ascii(const xchar *hostname)
{
    return contains_non_ascii(hostname, -1);
}

static xboolean punycode_decode(const xchar *input, xsize input_length, XString *output)
{
    xunichar n;
    xuint i, bias;
    const xchar *split;
    XArray *output_chars;
    xuint oldi, w, k, digit, t;

    n = PUNYCODE_INITIAL_N;
    i = 0;
    bias = PUNYCODE_INITIAL_BIAS;

    split = input + input_length - 1;
    while (split > input && *split != '-') {
        split--;
    }

    if (split > input) {
        output_chars = x_array_sized_new(FALSE, FALSE, sizeof (xunichar), split - input);
        input_length -= (split - input) + 1;
        while (input < split) {
            xunichar ch = (xunichar)*input++;
            if (!PUNYCODE_IS_BASIC(ch)) {
                goto fail;
            }

            x_array_append_val(output_chars, ch);
        }

        input++;
    } else {
        output_chars = x_array_new(FALSE, FALSE, sizeof (xunichar));
    }

    while (input_length) {
        oldi = i;
        w = 1;

        for (k = PUNYCODE_BASE; ; k += PUNYCODE_BASE) {
            if (!input_length--) {
                goto fail;
            }

            digit = decode_digit(*input++);
            if (digit >= PUNYCODE_BASE) {
                goto fail;
            }

            if (digit > (X_MAXUINT - i) / w) {
                goto fail;
            }

            i += digit * w;
            if (k <= bias) {
                t = PUNYCODE_TMIN;
            } else if (k >= bias + PUNYCODE_TMAX) {
                t = PUNYCODE_TMAX;
            } else {
                t = k - bias;
            }

            if (digit < t) {
                break;
            }

            if (w > X_MAXUINT / (PUNYCODE_BASE - t)) {
                goto fail;
            }

            w *= (PUNYCODE_BASE - t);
        }

        bias = adapt(i - oldi, output_chars->len + 1, oldi == 0);

        if (i / (output_chars->len + 1) > X_MAXUINT - n) {
            goto fail;
        }

        n += i / (output_chars->len + 1);
        i %= (output_chars->len + 1);

        x_array_insert_val(output_chars, i++, n);
    }

    for (i = 0; i < output_chars->len; i++) {
        x_string_append_unichar(output, x_array_index(output_chars, xunichar, i));
    }

    x_array_free(output_chars, TRUE);
    return TRUE;

fail:
    x_array_free(output_chars, TRUE);
    return FALSE;
}

xchar *x_hostname_to_unicode(const xchar *hostname)
{
    xssize llen;
    XString *out;
    xsize hostname_max_length_bytes = get_hostname_max_length_bytes ();

    if (hostname_max_length_bytes <= X_MAXSIZE / 4 && strlen_greater_than(hostname, 4 * MAX (255, hostname_max_length_bytes))) {
        return NULL;
    }

    out = x_string_new(NULL);

    do {
        llen = idna_end_of_label(hostname) - hostname;
        if (!x_ascii_strncasecmp(hostname, IDNA_ACE_PREFIX, IDNA_ACE_PREFIX_LEN)) {
            hostname += IDNA_ACE_PREFIX_LEN;
            llen -= IDNA_ACE_PREFIX_LEN;
            if (!punycode_decode(hostname, llen, out)) {
                x_string_free(out, TRUE);
                return NULL;
            }
        } else {
            xboolean unicode;
            xchar *canonicalized = nameprep(hostname, llen, &unicode);

            if (!canonicalized) {
                x_string_free(out, TRUE);
                return NULL;
            }

            x_string_append(out, canonicalized);
            x_free(canonicalized);
        }

        hostname += llen;
        if (*hostname) {
            hostname = x_utf8_next_char(hostname);
        }

        if (*hostname) {
            x_string_append_c(out, '.');
        }
    } while (*hostname);

    return x_string_free(out, FALSE);
}

xboolean x_hostname_is_ascii_encoded(const xchar *hostname)
{
    while (1) {
        if (!x_ascii_strncasecmp(hostname, IDNA_ACE_PREFIX, IDNA_ACE_PREFIX_LEN)) {
            return TRUE;
        }

        hostname = idna_end_of_label(hostname);
        if (*hostname) {
            hostname = x_utf8_next_char(hostname);
        }

        if (!*hostname) {
            return FALSE;
        }
    }
}

xboolean x_hostname_is_ip_address(const xchar *hostname)
{
    xchar *p, *end;
    xint nsegments, octet;

    p = (char *)hostname;
    if (strchr (p, ':')) {
        xboolean skipped;

        nsegments = 0;
        skipped = FALSE;

        while (*p && *p != '%' && nsegments < 8) {
            if (p != (char *)hostname || (p[0] == ':' && p[1] == ':')) {
                if (*p != ':') {
                    return FALSE;
                }

                p++;
            }

            if (*p == ':' && !skipped) {
                skipped = TRUE;
                nsegments++;

                if (!p[1]) {
                    p++;
                }

                continue;
            }

            for (end = p; x_ascii_isxdigit(*end); end++);
            if (end == p || end > p + 4) {
                return FALSE;
            }

            if (*end == '.') {
                if ((nsegments == 6 && !skipped) || (nsegments <= 6 && skipped)) {
                    goto parse_ipv4;
                } else {
                    return FALSE;
                }
            }

            nsegments++;
            p = end;
        }

        return (!*p || (p[0] == '%' && p[1])) && (nsegments == 8 || skipped);
    }

parse_ipv4:
    for (nsegments = 0; nsegments < 4; nsegments++) {
        if (nsegments != 0) {
            if (*p != '.') {
                return FALSE;
            }

            p++;
        }

        octet = 0;
        if (*p == '0') {
            end = p + 1;
        } else {
            for (end = p; x_ascii_isdigit(*end); end++) {
                octet = 10 * octet + (*end - '0');

                if (octet > 255) {
                    break;
                }
            }
        }

        if (end == p || end > p + 3 || octet > 255) {
            return FALSE;
        }

        p = end;
    }

    return !*p;
}
