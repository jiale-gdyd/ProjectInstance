#ifndef __X_STRFUNCS_H__
#define __X_STRFUNCS_H__

#include <stdarg.h>
#include <string.h>

#include "xmacros.h"
#include "xtypes.h"
#include "xerror.h"
#include "xmem.h"

X_BEGIN_DECLS

typedef enum {
    X_ASCII_ALNUM  = 1 << 0,
    X_ASCII_ALPHA  = 1 << 1,
    X_ASCII_CNTRL  = 1 << 2,
    X_ASCII_DIGIT  = 1 << 3,
    X_ASCII_GRAPH  = 1 << 4,
    X_ASCII_LOWER  = 1 << 5,
    X_ASCII_PRINT  = 1 << 6,
    X_ASCII_PUNCT  = 1 << 7,
    X_ASCII_SPACE  = 1 << 8,
    X_ASCII_UPPER  = 1 << 9,
    X_ASCII_XDIGIT = 1 << 10
} XAsciiType;

XLIB_VAR const xuint16 *const x_ascii_table;

#define x_ascii_isalnum(c)                  ((x_ascii_table[(xuchar)(c)] & X_ASCII_ALNUM) != 0)
#define x_ascii_isalpha(c)                  ((x_ascii_table[(xuchar)(c)] & X_ASCII_ALPHA) != 0)
#define x_ascii_iscntrl(c)                  ((x_ascii_table[(xuchar)(c)] & X_ASCII_CNTRL) != 0)
#define x_ascii_isdigit(c)                  ((x_ascii_table[(xuchar)(c)] & X_ASCII_DIGIT) != 0)
#define x_ascii_isgraph(c)                  ((x_ascii_table[(xuchar)(c)] & X_ASCII_GRAPH) != 0)
#define x_ascii_islower(c)                  ((x_ascii_table[(xuchar)(c)] & X_ASCII_LOWER) != 0)
#define x_ascii_isprint(c)                  ((x_ascii_table[(xuchar)(c)] & X_ASCII_PRINT) != 0)
#define x_ascii_ispunct(c)                  ((x_ascii_table[(xuchar)(c)] & X_ASCII_PUNCT) != 0)
#define x_ascii_isspace(c)                  ((x_ascii_table[(xuchar)(c)] & X_ASCII_SPACE) != 0)
#define x_ascii_isupper(c)                  ((x_ascii_table[(xuchar)(c)] & X_ASCII_UPPER) != 0)
#define x_ascii_isxdigit(c)                 ((x_ascii_table[(xuchar)(c)] & X_ASCII_XDIGIT) != 0)

#define X_STR_DELIMITERS                    "_-|> <."
#define X_ASCII_DTOSTR_BUF_SIZE             (29 + 10)

XLIB_AVAILABLE_IN_ALL
xchar x_ascii_tolower(xchar c) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
xchar x_ascii_toupper(xchar c) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
xint x_ascii_digit_value(xchar c) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
xint x_ascii_xdigit_value(xchar c) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
xchar *x_strdelimit(xchar *string, const xchar *delimiters, xchar new_delimiter);

XLIB_AVAILABLE_IN_ALL
xchar *x_strcanon(xchar *string, const xchar *valid_chars, xchar substitutor);

XLIB_AVAILABLE_IN_ALL
const xchar *x_strerror(xint errnum) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
const xchar *x_strsignal(xint signum) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
xchar *x_strreverse(xchar *string);

XLIB_AVAILABLE_IN_ALL
xsize x_strlcpy(xchar *dest, const xchar *src, xsize dest_size);

XLIB_AVAILABLE_IN_ALL
xsize x_strlcat(xchar *dest, const xchar *src, xsize dest_size);

XLIB_AVAILABLE_IN_ALL
xchar *x_strstr_len(const xchar *haystack, xssize haystack_len, const xchar *needle);

XLIB_AVAILABLE_IN_ALL
xchar *x_strrstr(const xchar *haystack, const xchar *needle);

XLIB_AVAILABLE_IN_ALL
xchar *x_strrstr_len(const xchar *haystack, xssize haystack_len, const xchar *needle);

XLIB_AVAILABLE_IN_ALL
xboolean (x_str_has_suffix)(const xchar *str, const xchar *suffix);

XLIB_AVAILABLE_IN_ALL
xboolean (x_str_has_prefix)(const xchar *str, const xchar *prefix);

#if X_GNUC_CHECK_VERSION(2, 0)

#define _X_STR_NONNULL(x)                   ((x) + !(x))

#define x_str_has_prefix(STR, PREFIX)                                                                                                       \
    (__builtin_constant_p(PREFIX) ?                                                                                                         \
        X_GNUC_EXTENSION ({                                                                                                                 \
            const char *const __str = (STR);                                                                                                \
            const char *const __prefix = (PREFIX);                                                                                          \
            xboolean __result = FALSE;                                                                                                      \
                                                                                                                                            \
            if X_UNLIKELY(__str == NULL || __prefix == NULL) {                                                                              \
                __result = (x_str_has_prefix)(__str, __prefix);                                                                             \
            } else {                                                                                                                        \
                const size_t __str_len = strlen(_X_STR_NONNULL(__str));                                                                     \
                const size_t __prefix_len = strlen(_X_STR_NONNULL(__prefix));                                                               \
                if (__str_len >= __prefix_len) {                                                                                            \
                    __result = memcmp(_X_STR_NONNULL(__str), _X_STR_NONNULL(__prefix), __prefix_len) == 0;                                  \
                }                                                                                                                           \
            }                                                                                                                               \
            __result;                                                                                                                       \
        })                                                                                                                                  \
    : (x_str_has_prefix)(STR, PREFIX))

#define x_str_has_suffix(STR, SUFFIX)                                                                                                       \
    (__builtin_constant_p(SUFFIX)?                                                                                                          \
        X_GNUC_EXTENSION ({                                                                                                                 \
            const char *const __str = (STR);                                                                                                \
            const char *const __suffix = (SUFFIX);                                                                                          \
            xboolean __result = FALSE;                                                                                                      \
                                                                                                                                            \
            if X_UNLIKELY(__str == NULL || __suffix == NULL) {                                                                              \
                __result = (x_str_has_suffix)(__str, __suffix);                                                                             \
            } else {                                                                                                                        \
                const size_t __str_len = strlen(_X_STR_NONNULL(__str));                                                                     \
                const size_t __suffix_len = strlen(_X_STR_NONNULL(__suffix));                                                               \
                if (__str_len >= __suffix_len) {                                                                                            \
                    __result = memcmp(__str + __str_len - __suffix_len, _X_STR_NONNULL(__suffix), __suffix_len) == 0;                       \
                }                                                                                                                           \
            }                                                                                                                               \
            __result;                                                                                                                       \
        })                                                                                                                                  \
    : (x_str_has_suffix)(STR, SUFFIX))

#define x_strdup(STR)                                                                                                                       \
    (__builtin_constant_p((STR)) ?                                                                                                          \
        (X_LIKELY((STR) != NULL) ?                                                                                                          \
        X_GNUC_EXTENSION ({                                                                                                                 \
            const char *const ___str = ((STR));                                                                                             \
            const char *const __str = _X_STR_NONNULL(___str);                                                                               \
            const size_t __str_len = strlen(__str) + 1;                                                                                     \
            char *__dup_str = (char *)x_malloc(__str_len);                                                                                  \
            (char *)memcpy(__dup_str, __str, __str_len);                                                                                    \
        })                                                                                                                                  \
        : (char *)(NULL))                                                                                                                   \
    : (x_strdup)((STR)))

#endif

XLIB_AVAILABLE_IN_ALL
xdouble x_strtod(const xchar *nptr, xchar **endptr);

XLIB_AVAILABLE_IN_ALL
xdouble x_ascii_strtod(const xchar *nptr, xchar **endptr);

XLIB_AVAILABLE_IN_ALL
xuint64 x_ascii_strtoull(const xchar *nptr, xchar **endptr, xuint base);

XLIB_AVAILABLE_IN_ALL
xint64 x_ascii_strtoll(const xchar *nptr, xchar **endptr, xuint base);

XLIB_AVAILABLE_IN_ALL
xchar *x_ascii_dtostr(xchar *buffer, xint buf_len, xdouble d);

XLIB_AVAILABLE_IN_ALL
xchar *x_ascii_formatd(xchar *buffer, xint buf_len, const xchar *format, xdouble d);

XLIB_AVAILABLE_IN_ALL
xchar *x_strchug(xchar *string);

XLIB_AVAILABLE_IN_ALL
xchar *x_strchomp(xchar *string);

#define x_strstrip(string)              x_strchomp(x_strchug(string))

XLIB_AVAILABLE_IN_ALL
xint x_ascii_strcasecmp(const xchar *s1, const xchar *s2);

XLIB_AVAILABLE_IN_ALL
xint x_ascii_strncasecmp(const xchar *s1, const xchar *s2, xsize n);

XLIB_AVAILABLE_IN_ALL
xchar *x_ascii_strdown(const xchar *str, xssize len) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_ALL
xchar *x_ascii_strup(const xchar *str, xssize len) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_2_40
xboolean x_str_is_ascii(const xchar *str);

XLIB_DEPRECATED
xint x_strcasecmp(const xchar *s1, const xchar *s2);

XLIB_DEPRECATED
xint x_strncasecmp(const xchar *s1, const xchar *s2, xuint n);

XLIB_DEPRECATED
xchar *x_strdown(xchar *string);

XLIB_DEPRECATED
xchar *x_strup(xchar *string);

XLIB_AVAILABLE_IN_ALL
xchar *(x_strdup)(const xchar *str) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_ALL
xchar *x_strdup_printf(const xchar *format, ...) X_GNUC_PRINTF(1, 2) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_ALL
xchar *x_strdup_vprintf(const xchar *format, va_list args) X_GNUC_PRINTF(1, 0) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_ALL
xchar *x_strndup(const xchar *str, xsize n) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_ALL
xchar *x_strnfill(xsize length, xchar fill_char) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_ALL
xchar *x_strconcat(const xchar *string1, ...) X_GNUC_MALLOC X_GNUC_NULL_TERMINATED;

XLIB_AVAILABLE_IN_ALL
xchar *x_strjoin(const xchar *separator, ...) X_GNUC_MALLOC X_GNUC_NULL_TERMINATED;

XLIB_AVAILABLE_IN_ALL
xchar *x_strcompress(const xchar *source) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_ALL
xchar *x_strescape(const xchar *source, const xchar *exceptions) X_GNUC_MALLOC;

XLIB_DEPRECATED_IN_2_68_FOR (x_memdup2)
xpointer x_memdup(xconstpointer mem, xuint byte_size) X_GNUC_ALLOC_SIZE(2);

XLIB_AVAILABLE_IN_2_68
xpointer x_memdup2(xconstpointer mem, xsize byte_size) X_GNUC_ALLOC_SIZE(2);

typedef xchar **XStrv;

XLIB_AVAILABLE_IN_ALL
xchar **x_strsplit(const xchar  *string, const xchar *delimiter, xint max_tokens);

XLIB_AVAILABLE_IN_ALL
xchar **x_strsplit_set(const xchar *string, const xchar *delimiters, xint max_tokens);

XLIB_AVAILABLE_IN_ALL
xchar *x_strjoinv(const xchar *separator, xchar **str_array) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_ALL
void x_strfreev(xchar **str_array);

XLIB_AVAILABLE_IN_ALL
xchar **x_strdupv(xchar **str_array);

XLIB_AVAILABLE_IN_ALL
xuint x_strv_length(xchar **str_array);

XLIB_AVAILABLE_IN_ALL
xchar *x_stpcpy(xchar *dest, const char *src);

XLIB_AVAILABLE_IN_2_40
xchar *x_str_to_ascii(const xchar *str, const xchar *from_locale);

XLIB_AVAILABLE_IN_2_40
xchar **x_str_tokenize_and_fold(const xchar *string, const xchar *translit_locale, xchar ***ascii_alternates);

XLIB_AVAILABLE_IN_2_40
xboolean x_str_match_string(const xchar *search_term, const xchar *potential_hit, xboolean accept_alternates);

XLIB_AVAILABLE_IN_2_44
xboolean x_strv_contains(const xchar *const *strv, const xchar *str);

XLIB_AVAILABLE_IN_2_60
xboolean x_strv_equal(const xchar *const *strv1, const xchar *const *strv2);

typedef enum {
    X_NUMBER_PARSER_ERROR_INVALID,
    X_NUMBER_PARSER_ERROR_OUT_OF_BOUNDS,
} XNumberParserError;

#define X_NUMBER_PARSER_ERROR               (x_number_parser_error_quark())

XLIB_AVAILABLE_IN_2_54
XQuark x_number_parser_error_quark(void);

XLIB_AVAILABLE_IN_2_54
xboolean x_ascii_string_to_signed(const xchar *str, xuint base, xint64 min, xint64 max, xint64 *out_num, XError **error);

XLIB_AVAILABLE_IN_2_54
xboolean x_ascii_string_to_unsigned(const xchar *str, xuint base, xuint64 min, xuint64 max, xuint64 *out_num, XError **error);

XLIB_AVAILABLE_STATIC_INLINE_IN_2_76
static inline xboolean x_set_str(char **str_pointer, const char *new_str)
{
    char *copy;

    if (*str_pointer == new_str || (*str_pointer && new_str && strcmp(*str_pointer, new_str) == 0)) {
        return FALSE;
    }

    copy = x_strdup(new_str);
    x_free(*str_pointer);
    *str_pointer = copy;

    return TRUE;
}

X_END_DECLS

#endif
