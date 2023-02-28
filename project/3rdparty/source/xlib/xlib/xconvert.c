#include <iconv.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <xlib/xlib/config.h>
#include <xlib/xlib/xlibconfig.h>

#include <xlib/xlib/xslist.h>
#include <xlib/xlib/xthread.h>
#include <xlib/xlib/xenviron.h>
#include <xlib/xlib/xlibintl.h>
#include <xlib/xlib/xconvert.h>
#include <xlib/xlib/xunicode.h>
#include <xlib/xlib/xstrfuncs.h>
#include <xlib/xlib/xtestutils.h>
#include <xlib/xlib/xfileutils.h>
#include <xlib/xlib/xthreadprivate.h>
#include <xlib/xlib/xconvertprivate.h>
#include <xlib/xlib/xcharsetprivate.h>

#define NUL_TERMINATOR_LENGTH       4

X_DEFINE_QUARK(x_convert_error, x_convert_error)

static xboolean try_conversion(const char *to_codeset, const char *from_codeset, iconv_t *cd)
{
    *cd = iconv_open(to_codeset, from_codeset);

    if (*cd == (iconv_t)-1 && errno == EINVAL) {
        return FALSE;
    } else {
        return TRUE;
    }
}

static xboolean try_to_aliases(const char **to_aliases, const char *from_codeset, iconv_t *cd)
{
    if (to_aliases) {
        const char **p = to_aliases;
        while (*p) {
            if (try_conversion(*p, from_codeset, cd)) {
                return TRUE;
            }

            p++;
        }
    }

    return FALSE;
}

XIConv x_iconv_open(const xchar *to_codeset, const xchar *from_codeset)
{
    iconv_t cd;

    if (!try_conversion(to_codeset, from_codeset, &cd)) {
        const char **to_aliases = _x_charset_get_aliases(to_codeset);
        const char **from_aliases = _x_charset_get_aliases(from_codeset);

        if (from_aliases) {
            const char **p = from_aliases;
            while (*p) {
                if (try_conversion(to_codeset, *p, &cd)) {
                    goto out;
                }

                if (try_to_aliases(to_aliases, *p, &cd)) {
                    goto out;
                }

                p++;
            }
        }

        if (try_to_aliases(to_aliases, from_codeset, &cd)) {
            goto out;
        }
    }

out:
    return (cd == (iconv_t)-1) ? (XIConv)-1 : (XIConv)cd;
}

xsize x_iconv(XIConv converter, xchar **inbuf, xsize *inbytes_left, xchar **outbuf, xsize *outbytes_left)
{
    iconv_t cd = (iconv_t)converter;
    return iconv(cd, inbuf, inbytes_left, outbuf, outbytes_left);
}

xint x_iconv_close(XIConv converter)
{
    iconv_t cd = (iconv_t)converter;
    return iconv_close(cd);
}

static XIConv open_converter(const xchar *to_codeset, const xchar *from_codeset, XError **error)
{
    XIConv cd;

    cd = x_iconv_open(to_codeset, from_codeset);

    if (cd == (XIConv) -1) {
        if (error) {
            if (errno == EINVAL) {
                x_set_error(error, X_CONVERT_ERROR, X_CONVERT_ERROR_NO_CONVERSION, _("Conversion from character set “%s” to “%s” is not supported"), from_codeset, to_codeset);
            } else {
                x_set_error(error, X_CONVERT_ERROR, X_CONVERT_ERROR_FAILED, _("Could not open converter from “%s” to “%s”"), from_codeset, to_codeset);
            }
        }
    }

    return cd;
}

static int close_converter(XIConv cd)
{
    if (cd == (XIConv) -1) {
        return 0;
    }

    return x_iconv_close(cd);
}

xchar *x_convert_with_iconv(const xchar *str, xssize len, XIConv converter, xsize *bytes_read,  xsize *bytes_written,  XError **error)
{
    xsize err;
    xchar *dest;
    xchar *outp;
    const xchar *p;
    xsize outbuf_size;
    xboolean done = FALSE;
    xboolean reset = FALSE;
    xsize inbytes_remaining;
    xsize outbytes_remaining;
    xboolean have_error = FALSE;

    x_return_val_if_fail(converter != (XIConv) -1, NULL);

    if (len < 0) {
        len = strlen(str);
    }

    p = str;
    inbytes_remaining = len;
    outbuf_size = len + NUL_TERMINATOR_LENGTH;

    outbytes_remaining = outbuf_size - NUL_TERMINATOR_LENGTH;
    outp = dest = (xchar *)x_malloc(outbuf_size);

    while (!done && !have_error) {
        if (reset) {
            err = x_iconv(converter, NULL, &inbytes_remaining, &outp, &outbytes_remaining);
        } else {
            err = x_iconv(converter, (char **)&p, &inbytes_remaining, &outp, &outbytes_remaining);
        }

        if (err == (xsize) -1) {
            switch (errno) {
                case EINVAL:
                    done = TRUE;
                    break;

                case E2BIG: {
                    xsize used = outp - dest;

                    outbuf_size *= 2;
                    dest = (xchar *)x_realloc(dest, outbuf_size);

                    outp = dest + used;
                    outbytes_remaining = outbuf_size - used - NUL_TERMINATOR_LENGTH;
                }
                break;

                case EILSEQ:
                    x_set_error_literal(error, X_CONVERT_ERROR, X_CONVERT_ERROR_ILLEGAL_SEQUENCE, _("Invalid byte sequence in conversion input"));
                    have_error = TRUE;
                    break;

                default: {
                    int errsv = errno;
                    x_set_error(error, X_CONVERT_ERROR, X_CONVERT_ERROR_FAILED, _("Error during conversion: %s"), x_strerror(errsv));
                }
                have_error = TRUE;
                break;
            }
        } else if (err > 0) {
            x_set_error_literal(error, X_CONVERT_ERROR, X_CONVERT_ERROR_ILLEGAL_SEQUENCE, _("Unrepresentable character in conversion input"));
            have_error = TRUE;
        } else {
            if (!reset) {
                reset = TRUE;
                inbytes_remaining = 0;
            } else {
                done = TRUE;
            }
        }
    }

    memset(outp, 0, NUL_TERMINATOR_LENGTH);

    if (bytes_read) {
        *bytes_read = p - str;
    } else {
        if ((p - str) != len) {
            if (!have_error) {
                x_set_error_literal(error, X_CONVERT_ERROR, X_CONVERT_ERROR_PARTIAL_INPUT, _("Partial character sequence at end of input"));
                have_error = TRUE;
            }
        }
    }

    if (bytes_written) {
        *bytes_written = outp - dest;
    }

    if (have_error) {
        x_free(dest);
        return NULL;
    } else {
        return dest;
    }
}

xchar *x_convert(const xchar *str, xssize len, const xchar *to_codeset, const xchar *from_codeset, xsize *bytes_read, xsize *bytes_written, XError **error)
{
    XIConv cd;
    xchar *res;

    x_return_val_if_fail(str != NULL, NULL);
    x_return_val_if_fail(to_codeset != NULL, NULL);
    x_return_val_if_fail(from_codeset != NULL, NULL);

    cd = open_converter(to_codeset, from_codeset, error);
    if (cd == (XIConv) -1) {
        if (bytes_read) {
            *bytes_read = 0;
        }

        if (bytes_written) {
            *bytes_written = 0;
        }

        return NULL;
    }

    res = x_convert_with_iconv(str, len, cd, bytes_read, bytes_written, error);
    close_converter(cd);

    return res;
}

xchar *x_convert_with_fallback(const xchar *str, xssize len, const xchar *to_codeset, const xchar *from_codeset, const xchar *fallback, xsize *bytes_read, xsize *bytes_written, XError **error)
{
    xsize err;
    XIConv cd;
    xchar *utf8;
    xchar *dest;
    xchar *outp;
    const xchar *p;
    xsize outbuf_size;
    xboolean done = FALSE;
    xsize save_inbytes = 0;
    xsize inbytes_remaining;
    xsize outbytes_remaining;
    XError *local_error = NULL;
    const xchar *save_p = NULL;
    xboolean have_error = FALSE;
    const xchar *insert_str = NULL;

    x_return_val_if_fail(str != NULL, NULL);
    x_return_val_if_fail(to_codeset != NULL, NULL);
    x_return_val_if_fail(from_codeset != NULL, NULL);

    if (len < 0) {
        len = strlen(str);
    }

    dest = x_convert(str, len, to_codeset, from_codeset, bytes_read, bytes_written, &local_error);
    if (!local_error) {
        return dest;
    }

    x_assert(dest == NULL);

    if (!x_error_matches(local_error, X_CONVERT_ERROR, X_CONVERT_ERROR_ILLEGAL_SEQUENCE)) {
        x_propagate_error(error, local_error);
        return NULL;
    } else {
        x_error_free(local_error);
    }
    local_error = NULL;

    cd = open_converter(to_codeset, "UTF-8", error);
    if (cd == (XIConv) -1) {
        if (bytes_read) {
            *bytes_read = 0;
        }

        if (bytes_written) {
            *bytes_written = 0;
        }

        return NULL;
    }

    utf8 = x_convert(str, len, "UTF-8", from_codeset, bytes_read, &inbytes_remaining, error);
    if (!utf8) {
        close_converter(cd);
        if (bytes_written) {
            *bytes_written = 0;
        }

        return NULL;
    }

    p = utf8;

    outbuf_size = len + NUL_TERMINATOR_LENGTH;
    outbytes_remaining = outbuf_size - NUL_TERMINATOR_LENGTH;
    outp = dest = (xchar *)x_malloc(outbuf_size);

    while (!done && !have_error) {
        xsize inbytes_tmp = inbytes_remaining;
        err = x_iconv(cd, (char **)&p, &inbytes_tmp, &outp, &outbytes_remaining);
        inbytes_remaining = inbytes_tmp;

        if (err == (xsize) -1) {
            switch (errno) {
                case EINVAL:
                    x_assert_not_reached();
                    break;

            case E2BIG: {
                xsize used = outp - dest;

                outbuf_size *= 2;
                dest = (xchar *)x_realloc(dest, outbuf_size);

                outp = dest + used;
                outbytes_remaining = outbuf_size - used - NUL_TERMINATOR_LENGTH;
                break;
            }

            case EILSEQ:
                if (save_p) {
                    x_set_error(error, X_CONVERT_ERROR, X_CONVERT_ERROR_ILLEGAL_SEQUENCE, _("Cannot convert fallback “%s” to codeset “%s”"), insert_str, to_codeset);
                    have_error = TRUE;
                    break;
                } else if (p) {
                    if (!fallback) {
                        xunichar ch = x_utf8_get_char(p);
                        insert_str = x_strdup_printf(ch < 0x10000 ? "\\u%04x" : "\\U%08x", ch);
                    } else {
                        insert_str = fallback;
                    }

                    save_p = x_utf8_next_char(p);
                    save_inbytes = inbytes_remaining - (save_p - p);
                    p = insert_str;
                    inbytes_remaining = strlen(p);
                    break;
                }
                X_GNUC_FALLTHROUGH;

                default: {
                    int errsv = errno;
                    x_set_error(error, X_CONVERT_ERROR, X_CONVERT_ERROR_FAILED, _("Error during conversion: %s"), x_strerror(errsv));
                }

                have_error = TRUE;
                break;
            }
        } else {
            if (save_p) {
                if (!fallback) {
                    x_free((xchar *)insert_str);
                }

                p = save_p;
                inbytes_remaining = save_inbytes;
                save_p = NULL;
            } else if (p) {
                p = NULL;
                inbytes_remaining = 0;
            } else {
                done = TRUE;
            }
        }
    }

    memset(outp, 0, NUL_TERMINATOR_LENGTH);
    close_converter(cd);

    if (bytes_written) {
        *bytes_written = outp - dest;
    }
    x_free(utf8);

    if (have_error) {
        if (save_p && !fallback) {
            x_free((xchar *)insert_str);
        }

        x_free(dest);
        return NULL;
    } else {
        return dest;
    }
}

static xchar *strdup_len(const xchar *string, xssize len, xsize *bytes_read, xsize *bytes_written, XError **error)
{
    xsize real_len;
    const xchar *end_valid;

    if (!x_utf8_validate(string, len, &end_valid)) {
        if (bytes_read) {
            *bytes_read = end_valid - string;
        }

        if (bytes_written) {
            *bytes_written = 0;
        }

        x_set_error_literal(error, X_CONVERT_ERROR, X_CONVERT_ERROR_ILLEGAL_SEQUENCE, _("Invalid byte sequence in conversion input"));
        return NULL;
    }

    real_len = end_valid - string;

    if (bytes_read) {
        *bytes_read = real_len;
    }

    if (bytes_written) {
        *bytes_written = real_len;
    }

    return x_strndup(string, real_len);
}

typedef enum {
    CONVERT_CHECK_NO_NULS_IN_INPUT  = 1 << 0,
    CONVERT_CHECK_NO_NULS_IN_OUTPUT = 1 << 1
} ConvertCheckFlags;

static xchar *convert_checked(const xchar *string, xssize len, const xchar *to_codeset, const xchar *from_codeset, ConvertCheckFlags flags, xsize *bytes_read, xsize *bytes_written, XError **error)
{
    xchar *out;
    xsize outbytes;

    if ((flags & CONVERT_CHECK_NO_NULS_IN_INPUT) && len > 0) {
        const xchar *early_nul = (const xchar *)memchr(string, '\0', len);
        if (early_nul != NULL) {
            if (bytes_read) {
                *bytes_read = early_nul - string;
            }

            if (bytes_written) {
                *bytes_written = 0;
            }

            x_set_error_literal(error, X_CONVERT_ERROR, X_CONVERT_ERROR_ILLEGAL_SEQUENCE, _("Embedded NUL byte in conversion input"));
            return NULL;
        }
    }

    out = x_convert(string, len, to_codeset, from_codeset, bytes_read, &outbytes, error);
    if (out == NULL) {
        if (bytes_written) {
            *bytes_written = 0;
        }

        return NULL;
    }

    if ((flags & CONVERT_CHECK_NO_NULS_IN_OUTPUT) && memchr(out, '\0', outbytes) != NULL) {
        x_free(out);
        if (bytes_written) {
            *bytes_written = 0;
        }

        x_set_error_literal(error, X_CONVERT_ERROR, X_CONVERT_ERROR_EMBEDDED_NUL, _("Embedded NUL byte in conversion output"));
        return NULL;
    }

    if (bytes_written) {
        *bytes_written = outbytes;
    }

    return out;
}

xchar *x_locale_to_utf8(const xchar *opsysstring, xssize len, xsize *bytes_read, xsize *bytes_written, XError **error)
{
    const char *charset;

    if (x_get_charset(&charset)) {
        return strdup_len(opsysstring, len, bytes_read, bytes_written, error);
    } else {
        return convert_checked(opsysstring, len, "UTF-8", charset, CONVERT_CHECK_NO_NULS_IN_OUTPUT, bytes_read, bytes_written, error);
    }
}

xchar *_x_time_locale_to_utf8(const xchar *opsysstring, xssize len, xsize *bytes_read, xsize *bytes_written, XError **error)
{
    const char *charset;

    if (_x_get_time_charset(&charset)) {
        return strdup_len(opsysstring, len, bytes_read, bytes_written, error);
    } else {
        return convert_checked(opsysstring, len, "UTF-8", charset, CONVERT_CHECK_NO_NULS_IN_OUTPUT, bytes_read, bytes_written, error);
    }
}

xchar *_x_ctype_locale_to_utf8(const xchar *opsysstring, xssize len, xsize *bytes_read, xsize *bytes_written, XError **error)
{
    const char *charset;

    if (_x_get_ctype_charset(&charset)) {
        return strdup_len(opsysstring, len, bytes_read, bytes_written, error);
    } else {
        return convert_checked(opsysstring, len, "UTF-8", charset, CONVERT_CHECK_NO_NULS_IN_OUTPUT, bytes_read, bytes_written, error);
    }
}

xchar *x_locale_from_utf8(const xchar *utf8string, xssize len, xsize *bytes_read, xsize *bytes_written, XError **error)
{
    const xchar *charset;

    if (x_get_charset(&charset)) {
        return strdup_len(utf8string, len, bytes_read, bytes_written, error);
    } else {
        return convert_checked(utf8string, len, charset, "UTF-8", CONVERT_CHECK_NO_NULS_IN_INPUT, bytes_read, bytes_written, error);
    }
}

typedef struct _XFilenameCharsetCache XFilenameCharsetCache;

struct _XFilenameCharsetCache {
    xboolean is_utf8;
    xchar    *charset;
    xchar    **filename_charsets;
};

static void filename_charset_cache_free(xpointer data)
{
    XFilenameCharsetCache *cache = (XFilenameCharsetCache *)data;

    x_free(cache->charset);
    x_strfreev(cache->filename_charsets);
    x_free(cache);
}

xboolean x_get_filename_charsets(const xchar ***filename_charsets)
{
    const xchar *charset;
    static XPrivate cache_private = X_PRIVATE_INIT(filename_charset_cache_free);
    XFilenameCharsetCache *cache = (XFilenameCharsetCache *)x_private_get(&cache_private);

    if (!cache) {
        cache = (XFilenameCharsetCache *)x_private_set_alloc0(&cache_private, sizeof(XFilenameCharsetCache));
    }
    x_get_charset(&charset);

    if (!(cache->charset && strcmp(cache->charset, charset) == 0)) {
        xint i;
        const xchar *p;
        const xchar *new_charset;

        x_free(cache->charset);
        x_strfreev(cache->filename_charsets);
        cache->charset = x_strdup(charset);

        p = x_getenv("X_FILENAME_ENCODING");
        if (p != NULL && p[0] != '\0')  {
            cache->filename_charsets = x_strsplit(p, ",", 0);
            cache->is_utf8 = (strcmp(cache->filename_charsets[0], "UTF-8") == 0);

            for (i = 0; cache->filename_charsets[i]; i++) {
                if (strcmp("@locale", cache->filename_charsets[i]) == 0) {
                    x_get_charset(&new_charset);
                    x_free(cache->filename_charsets[i]);
                    cache->filename_charsets[i] = x_strdup(new_charset);
                }
            }
        } else if (x_getenv("X_BROKEN_FILENAMES") != NULL) {
            cache->filename_charsets = x_new0(xchar *, 2);
            cache->is_utf8 = x_get_charset(&new_charset);
            cache->filename_charsets[0] = x_strdup(new_charset);
        } else {
            cache->filename_charsets = x_new0(xchar *, 3);
            cache->is_utf8 = TRUE;
            cache->filename_charsets[0] = x_strdup("UTF-8");
            if (!x_get_charset(&new_charset)) {
                cache->filename_charsets[1] = x_strdup(new_charset);
            }
        }
    }

    if (filename_charsets) {
        *filename_charsets = (const xchar **)cache->filename_charsets;
    }

    return cache->is_utf8;
}

static xboolean get_filename_charset(const xchar **filename_charset)
{
    xboolean is_utf8;
    const xchar **charsets;

    is_utf8 = x_get_filename_charsets(&charsets);
    if (filename_charset) {
        *filename_charset = charsets[0];
    }

    return is_utf8;
}

xchar *x_filename_to_utf8(const xchar *opsysstring, xssize len, xsize *bytes_read, xsize *bytes_written, XError **error)
{
    const xchar *charset;

    x_return_val_if_fail(opsysstring != NULL, NULL);

    if (get_filename_charset(&charset)) {
        return strdup_len(opsysstring, len, bytes_read, bytes_written, error);
    } else {
        return convert_checked(opsysstring, len, "UTF-8", charset, (ConvertCheckFlags)(CONVERT_CHECK_NO_NULS_IN_INPUT | CONVERT_CHECK_NO_NULS_IN_OUTPUT), bytes_read, bytes_written, error);
    }
}

xchar *x_filename_from_utf8(const xchar *utf8string, xssize len, xsize *bytes_read, xsize *bytes_written, XError **error)
{
    const xchar *charset;

    if (get_filename_charset(&charset)) {
        return strdup_len(utf8string, len, bytes_read, bytes_written, error);
    } else {
        return convert_checked(utf8string, len, charset, "UTF-8", (ConvertCheckFlags)(CONVERT_CHECK_NO_NULS_IN_INPUT | CONVERT_CHECK_NO_NULS_IN_OUTPUT), bytes_read, bytes_written, error);
    }
}

static xboolean has_case_prefix(const xchar *haystack, const xchar *needle)
{
    const xchar *h, *n;

    h = haystack;
    n = needle;

    while (*n && *h && x_ascii_tolower(*n) == x_ascii_tolower(*h)) {
        n++;
        h++;
    }

    return *n == '\0';
}

typedef enum {
    UNSAFE_ALL        = 0x1,
    UNSAFE_ALLOW_PLUS = 0x2,
    UNSAFE_PATH       = 0x8,
    UNSAFE_HOST       = 0x10,
    UNSAFE_SLASHES    = 0x20
} UnsafeCharacterSet;

static const xuchar acceptable[96] = {
    /* A table of the ASCII chars from space (32) to DEL (127) */
    /*      !    "    #    $    %    &    '    (    )    *    +    ,    -    .    / */ 
    0x00,0x3F,0x20,0x20,0x28,0x00,0x2C,0x3F,0x3F,0x3F,0x3F,0x2A,0x28,0x3F,0x3F,0x1C,
    /* 0    1    2    3    4    5    6    7    8    9    :    ;    <    =    >    ? */
    0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x38,0x20,0x20,0x2C,0x20,0x20,
    /* @    A    B    C    D    E    F    G    H    I    J    K    L    M    N    O */
    0x38,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
    /* P    Q    R    S    T    U    V    W    X    Y    Z    [    \    ]    ^    _ */
    0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x20,0x20,0x20,0x20,0x3F,
    /* `    a    b    c    d    e    f    g    h    i    j    k    l    m    n    o */
    0x20,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
    /* p    q    r    s    t    u    v    w    x    y    z    {    |    }    ~  DEL */
    0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x20,0x20,0x20,0x3F,0x20
};

static const xchar hex[] = "0123456789ABCDEF";

static xchar *x_escape_uri_string(const xchar *string, UnsafeCharacterSet mask)
{
#define ACCEPTABLE(a)   ((a)>=32 && (a)<128 && (acceptable[(a)-32] & use_mask))

    int c;
    xchar *q;
    xchar *result;
    const xchar *p;
    xint unacceptable;
    UnsafeCharacterSet use_mask;

    x_return_val_if_fail(mask == UNSAFE_ALL || mask == UNSAFE_ALLOW_PLUS || mask == UNSAFE_PATH || mask == UNSAFE_HOST || mask == UNSAFE_SLASHES, NULL);

    unacceptable = 0;
    use_mask = mask;
    for (p = string; *p != '\0'; p++) {
        c = (xuchar) *p;
        if (!ACCEPTABLE(c)) {
            unacceptable++;
        }
    }

    result = (xchar *)x_malloc(p - string + unacceptable * 2 + 1);

    use_mask = mask;
    for (q = result, p = string; *p != '\0'; p++) {
        c = (xuchar) *p;
        if (!ACCEPTABLE(c)) {
            *q++ = '%';
            *q++ = hex[c >> 4];
            *q++ = hex[c & 15];
        } else {
            *q++ = *p;
        }
    }
    *q = '\0';

    return result;
}

static xchar *x_escape_file_uri(const xchar *hostname, const xchar *pathname)
{
    char *res;
    char *escaped_path;
    char *escaped_hostname = NULL;

    if (hostname && *hostname != '\0') {
        escaped_hostname = x_escape_uri_string(hostname, UNSAFE_HOST);
    }

    escaped_path = x_escape_uri_string(pathname, UNSAFE_PATH);

    res = x_strconcat("file://", (escaped_hostname) ? escaped_hostname : "", (*escaped_path != '/') ? "/" : "", escaped_path, NULL);
    x_free(escaped_hostname);
    x_free(escaped_path);

    return res;
}

static int unescape_character(const char *scanner)
{
    int first_digit;
    int second_digit;

    first_digit = x_ascii_xdigit_value(scanner[0]);
    if (first_digit < 0) {
        return -1;
    }

    second_digit = x_ascii_xdigit_value(scanner[1]);
    if (second_digit < 0) {
        return -1;
    }

    return (first_digit << 4) | second_digit;
}

static xchar *x_unescape_uri_string(const char *escaped, int len, const char *illegal_escaped_characters, xboolean ascii_must_not_be_escaped)
{
    int c;
    xchar *out, *result;
    const xchar *in, *in_end;

    if (escaped == NULL) {
        return NULL;
    }

    if (len < 0) {
        len = strlen(escaped);
    }

    result = (xchar *)x_malloc(len + 1);
    out = result;

    for (in = escaped, in_end = escaped + len; in < in_end; in++) {
        c = *in;
        if (c == '%') {
            if (in + 3 > in_end) {
                break;
            }

            c = unescape_character(in + 1);
            if (c <= 0) {
                break;
            }

            if (ascii_must_not_be_escaped && c <= 0x7F) {
                break;
            }

            if (strchr(illegal_escaped_characters, c) != NULL) {
                break;
            }

            in += 2;
        }

        *out++ = c;
    }

    x_assert(out - result <= len);
    *out = '\0';

    if (in != in_end) {
        x_free(result);
        return NULL;
    }

    return result;
}

static xboolean is_asciialphanum(xunichar c)
{
    return c <= 0x7F && x_ascii_isalnum(c);
}

static xboolean is_asciialpha(xunichar c)
{
    return c <= 0x7F && x_ascii_isalpha(c);
}

static xboolean hostname_validate(const char *hostname)
{
    const char *p;
    xunichar c, first_char, last_char;

    p = hostname;
    if (*p == '\0') {
        return TRUE;
    }

    do {
        c = x_utf8_get_char(p);
        p = x_utf8_next_char(p);
        if (!is_asciialphanum(c)) {
            return FALSE;
        }

        first_char = c;
        do {
            last_char = c;
            c = x_utf8_get_char(p);
            p = x_utf8_next_char(p);
        } while (is_asciialphanum(c) || c == '-');

        if (last_char == '-') {
            return FALSE;
        }

        if (c == '\0' || (c == '.' && *p == '\0')) {
            return is_asciialpha(first_char);
        }
    } while (c == '.');

    return FALSE;
}

xchar *x_filename_from_uri(const xchar *uri, xchar **hostname, XError **error)
{
    int offs;
    char *result;
    char *filename;
    const char *path_part;
    const char *host_part;
    char *unescaped_hostname;

    if (hostname) {
        *hostname = NULL;
    }

    if (!has_case_prefix(uri, "file:/")) {
        x_set_error(error, X_CONVERT_ERROR, X_CONVERT_ERROR_BAD_URI, _("The URI “%s” is not an absolute URI using the “file” scheme"), uri);
        return NULL;
    }

    path_part = uri + strlen("file:");

    if (strchr (path_part, '#') != NULL) {
        x_set_error(error, X_CONVERT_ERROR, X_CONVERT_ERROR_BAD_URI, _("The local file URI “%s” may not include a “#”"), uri);
        return NULL;
    }

    if (has_case_prefix(path_part, "///")) {
        path_part += 2;
    } else if (has_case_prefix(path_part, "//")) {
        path_part += 2;
        host_part = path_part;

        path_part = strchr(path_part, '/');
        if (path_part == NULL) {
            x_set_error(error, X_CONVERT_ERROR, X_CONVERT_ERROR_BAD_URI, _("The URI “%s” is invalid"), uri);
            return NULL;
        }

        unescaped_hostname = x_unescape_uri_string(host_part, path_part - host_part, "", TRUE);

        if (unescaped_hostname == NULL || !hostname_validate(unescaped_hostname)) {
            x_free(unescaped_hostname);
            x_set_error(error, X_CONVERT_ERROR, X_CONVERT_ERROR_BAD_URI, _("The hostname of the URI “%s” is invalid"), uri);
            return NULL;
        }

        if (hostname) {
            *hostname = unescaped_hostname;
        } else {
            x_free(unescaped_hostname);
        }
    }

    filename = x_unescape_uri_string(path_part, -1, "/", FALSE);
    if (filename == NULL) {
        x_set_error(error, X_CONVERT_ERROR, X_CONVERT_ERROR_BAD_URI, _("The URI “%s” contains invalidly escaped characters"), uri);
        return NULL;
    }

    offs = 0;
    result = x_strdup(filename + offs);
    x_free(filename);

    return result;
}

xchar *x_filename_to_uri(const xchar *filename, const xchar *hostname, XError **error)
{
    char *escaped_uri;

    x_return_val_if_fail(filename != NULL, NULL);

    if (!x_path_is_absolute(filename)) {
        x_set_error(error, X_CONVERT_ERROR, X_CONVERT_ERROR_NOT_ABSOLUTE_PATH, _("The pathname “%s” is not an absolute path"), filename);
        return NULL;
    }

    if (hostname && !(x_utf8_validate(hostname, -1, NULL) && hostname_validate (hostname))) {
        x_set_error_literal(error, X_CONVERT_ERROR, X_CONVERT_ERROR_ILLEGAL_SEQUENCE, _("Invalid hostname"));
        return NULL;
    }

    escaped_uri = x_escape_file_uri(hostname, filename);
    return escaped_uri;
}

xchar **x_uri_list_extract_uris(const xchar *uri_list)
{
    XPtrArray *uris;
    const xchar *p, *q;

    uris = x_ptr_array_new();
    p = uri_list;

    while (p) {
        if (*p != '#') {
            while (x_ascii_isspace(*p)) {
                p++;
            }

            q = p;
            while (*q && (*q != '\n') && (*q != '\r')) {
                q++;
            }

            if (q > p) {
                q--;
                while (q > p && x_ascii_isspace(*q)) {
                    q--;
                }

                if (q > p) {
                    x_ptr_array_add(uris, x_strndup(p, q - p + 1));
                }
            }
        }

        p = strchr(p, '\n');
        if (p)
        p++;
    }

    x_ptr_array_add(uris, NULL);
    return (xchar **)x_ptr_array_free(uris, FALSE);
}

xchar *x_filename_display_basename(const xchar *filename)
{
    char *basename;
    char *display_name;

    x_return_val_if_fail(filename != NULL, NULL);
    
    basename = x_path_get_basename(filename);
    display_name = x_filename_display_name(basename);
    x_free(basename);

    return display_name;
}

xchar *x_filename_display_name(const xchar *filename)
{
    xint i;
    xboolean is_utf8;
    const xchar **charsets;
    xchar *display_name = NULL;

    is_utf8 = x_get_filename_charsets(&charsets);
    if (is_utf8) {
        if (x_utf8_validate(filename, -1, NULL)) {
            display_name = x_strdup(filename);
        }
    }

    if (!display_name) {
        for (i = is_utf8 ? 1 : 0; charsets[i]; i++) {
            display_name = x_convert(filename, -1, "UTF-8", charsets[i], NULL, NULL, NULL);
            if (display_name) {
                break;
            }
        }
    }

    if (!display_name) {
        display_name = x_utf8_make_valid(filename, -1);
    }

    return display_name;
}
