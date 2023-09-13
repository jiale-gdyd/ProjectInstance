#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <locale.h>

#include <xlib/xlib/config.h>
#include <xlib/xlib/xmem.h>
#include <xlib/xlib/xtypes.h>
#include <xlib/xlib/xstring.h>
#include <xlib/xlib/xunicode.h>
#include <xlib/xlib/xtestutils.h>
#include <xlib/xlib/xscripttable.h>
#include <xlib/xlib/xunichartables.h>
#include <xlib/xlib/xmirroringtable.h>

#define X_UNICHAR_FULLWIDTH_A                   0xff21
#define X_UNICHAR_FULLWIDTH_I                   0xff29
#define X_UNICHAR_FULLWIDTH_J                   0xff2a
#define X_UNICHAR_FULLWIDTH_F                   0xff26
#define X_UNICHAR_FULLWIDTH_a                   0xff41
#define X_UNICHAR_FULLWIDTH_f                   0xff46

#define ATTR_TABLE(Page)                        (((Page) <= X_UNICODE_LAST_PAGE_PART1) ? attr_table_part1[Page] : attr_table_part2[(Page) - 0xe00])
#define ATTTABLE(Page, Char)                    ((ATTR_TABLE(Page) == X_UNICODE_MAX_TABLE_INDEX) ? 0 : (attr_data[ATTR_TABLE(Page)][Char]))

#define TTYPE_PART1(Page, Char)                 \
    ((type_table_part1[Page] >= X_UNICODE_MAX_TABLE_INDEX) ? (type_table_part1[Page] - X_UNICODE_MAX_TABLE_INDEX) : (type_data[type_table_part1[Page]][Char]))

#define TTYPE_PART2(Page, Char)                 \
    ((type_table_part2[Page] >= X_UNICODE_MAX_TABLE_INDEX) ? (type_table_part2[Page] - X_UNICODE_MAX_TABLE_INDEX) : (type_data[type_table_part2[Page]][Char]))

#define TYPE(Char)                              \
    (((Char) <= X_UNICODE_LAST_CHAR_PART1) ? TTYPE_PART1((Char) >> 8, (Char) & 0xff) : (((Char) >= 0xe0000 && (Char) <= X_UNICODE_LAST_CHAR) ? TTYPE_PART2(((Char) - 0xe0000) >> 8, (Char) & 0xff) : X_UNICODE_UNASSIGNED))


#define IS(Type, Class)                         (((xuint)1 << (Type)) & (Class))
#define OR(Type, Rest)                          (((xuint)1 << (Type)) | (Rest))

#define ISALPHA(Type)   IS((Type),             \
    OR(X_UNICODE_LOWERCASE_LETTER,             \
    OR(X_UNICODE_UPPERCASE_LETTER,             \
    OR(X_UNICODE_TITLECASE_LETTER,             \
    OR(X_UNICODE_MODIFIER_LETTER,              \
    OR(X_UNICODE_OTHER_LETTER, 0))))))

#define ISALDIGIT(Type) IS((Type),             \
    OR(X_UNICODE_DECIMAL_NUMBER,               \
    OR(X_UNICODE_LETTER_NUMBER,                \
    OR(X_UNICODE_OTHER_NUMBER,                 \
    OR(X_UNICODE_LOWERCASE_LETTER,             \
    OR(X_UNICODE_UPPERCASE_LETTER,             \
    OR(X_UNICODE_TITLECASE_LETTER,             \
    OR(X_UNICODE_MODIFIER_LETTER,              \
    OR(X_UNICODE_OTHER_LETTER, 0)))))))))

#define ISMARK(Type)    IS((Type),             \
    OR(X_UNICODE_NON_SPACING_MARK,             \
    OR(X_UNICODE_SPACING_MARK,                 \
    OR(X_UNICODE_ENCLOSING_MARK, 0))))

#define ISZEROWIDTHTYPE(Type)   IS((Type),     \
    OR(X_UNICODE_NON_SPACING_MARK,             \
    OR(X_UNICODE_ENCLOSING_MARK,               \
    OR(X_UNICODE_FORMAT, 0))))

xboolean x_unichar_isalnum(xunichar c)
{
    return ISALDIGIT(TYPE(c)) ? TRUE : FALSE;
}

xboolean x_unichar_isalpha(xunichar c)
{
    return ISALPHA(TYPE(c)) ? TRUE : FALSE;
}

xboolean x_unichar_iscntrl(xunichar c)
{
    return TYPE(c) == X_UNICODE_CONTROL;
}

xboolean x_unichar_isdigit(xunichar c)
{
    return TYPE(c) == X_UNICODE_DECIMAL_NUMBER;
}

xboolean x_unichar_isgraph(xunichar c)
{
    return !IS(TYPE(c), OR(X_UNICODE_CONTROL, OR(X_UNICODE_FORMAT, OR(X_UNICODE_UNASSIGNED, OR(X_UNICODE_SURROGATE, OR(X_UNICODE_SPACE_SEPARATOR, 0))))));
}

xboolean x_unichar_islower(xunichar c)
{
    return TYPE(c) == X_UNICODE_LOWERCASE_LETTER;
}

xboolean x_unichar_isprint (xunichar c)
{
    return !IS(TYPE(c), OR(X_UNICODE_CONTROL, OR(X_UNICODE_FORMAT, OR(X_UNICODE_UNASSIGNED, OR(X_UNICODE_SURROGATE, 0)))));
}

xboolean x_unichar_ispunct(xunichar c)
{
  return IS(TYPE(c),
        OR(X_UNICODE_CONNECT_PUNCTUATION,
        OR(X_UNICODE_DASH_PUNCTUATION,
        OR(X_UNICODE_CLOSE_PUNCTUATION,
        OR(X_UNICODE_FINAL_PUNCTUATION,
        OR(X_UNICODE_INITIAL_PUNCTUATION,
        OR(X_UNICODE_OTHER_PUNCTUATION,
        OR(X_UNICODE_OPEN_PUNCTUATION,
        OR(X_UNICODE_CURRENCY_SYMBOL,
        OR(X_UNICODE_MODIFIER_SYMBOL,
        OR(X_UNICODE_MATH_SYMBOL,
        OR(X_UNICODE_OTHER_SYMBOL,
        0)))))))))))) ? TRUE : FALSE;
}

xboolean x_unichar_isspace(xunichar c)
{
    switch (c) {
        case '\t':
        case '\n':
        case '\r':
        case '\f':
            return TRUE;
            break;

        default: {
            return IS(TYPE(c), OR(X_UNICODE_SPACE_SEPARATOR, OR(X_UNICODE_LINE_SEPARATOR, OR(X_UNICODE_PARAGRAPH_SEPARATOR, 0)))) ? TRUE : FALSE;
        }
        break;
    }
}

xboolean x_unichar_ismark(xunichar c)
{
    return ISMARK(TYPE(c));
}

xboolean x_unichar_isupper(xunichar c)
{
    return TYPE(c) == X_UNICODE_UPPERCASE_LETTER;
}

xboolean x_unichar_istitle(xunichar c)
{
    unsigned int i;
    for (i = 0; i < X_N_ELEMENTS(title_table); ++i) {
        if (title_table[i][0] == c) {
            return TRUE;
        }
    }

    return FALSE;
}

xboolean x_unichar_isxdigit(xunichar c)
{
    return ((c >= 'a' && c <= 'f') ||
            (c >= 'A' && c <= 'F') ||
            (c >= X_UNICHAR_FULLWIDTH_a && c <= X_UNICHAR_FULLWIDTH_f) ||
            (c >= X_UNICHAR_FULLWIDTH_A && c <= X_UNICHAR_FULLWIDTH_F) ||
            (TYPE(c) == X_UNICODE_DECIMAL_NUMBER));
}

xboolean x_unichar_isdefined(xunichar c)
{
    return !IS(TYPE(c), OR(X_UNICODE_UNASSIGNED, OR(X_UNICODE_SURROGATE, 0)));
}

xboolean x_unichar_iszerowidth(xunichar c)
{
    if (X_UNLIKELY(c == 0x00AD)) {
        return FALSE;
    }

    if (X_UNLIKELY(ISZEROWIDTHTYPE(TYPE(c)))) {
        return TRUE;
    }

    if (X_UNLIKELY((c >= 0x1160 && c < 0x1200) || (c >= 0xD7B0 && c < 0xD800) || c == 0x200B)) {
        return TRUE;
    }

    return FALSE;
}

static int interval_compare(const void *key, const void *elt)
{
    xunichar c = XPOINTER_TO_UINT(key);
    struct Interval *interval = (struct Interval *)elt;

    if (c < interval->start) {
        return -1;
    }

    if (c > interval->end) {
        return +1;
    }

    return 0;
}

#define X_WIDTH_TABLE_MIDPOINT          (X_N_ELEMENTS(x_unicode_width_table_wide) / 2)

static inline xboolean x_unichar_iswide_bsearch(xunichar ch)
{
    int lower = 0;
    static int saved_mid = X_WIDTH_TABLE_MIDPOINT;
    int mid = saved_mid;
    int upper = X_N_ELEMENTS(x_unicode_width_table_wide) - 1;

    do {
        if (ch < x_unicode_width_table_wide[mid].start) {
            upper = mid - 1;
        } else if (ch > x_unicode_width_table_wide[mid].end) {
            lower = mid + 1;
        } else {
            return TRUE;
        }

        mid = (lower + upper) / 2;
    } while (lower <= upper);

    return FALSE;
}

static const struct Interval default_wide_blocks[] = {
    { 0x3400, 0x4dbf },
    { 0x4e00, 0x9fff },
    { 0xf900, 0xfaff },
    { 0x20000, 0x2fffd },
    { 0x30000, 0x3fffd }
};

xboolean x_unichar_iswide(xunichar c)
{
    if (c < x_unicode_width_table_wide[0].start) {
        return FALSE;
    } else if (x_unichar_iswide_bsearch(c)) {
        return TRUE;
    } else if (x_unichar_type(c) == X_UNICODE_UNASSIGNED
        && bsearch(XUINT_TO_POINTER(c), default_wide_blocks, X_N_ELEMENTS(default_wide_blocks), sizeof default_wide_blocks[0], interval_compare))
    {
        return TRUE;
    }

    return FALSE;
}

xboolean x_unichar_iswide_cjk(xunichar c)
{
    if (x_unichar_iswide(c)) {
        return TRUE;
    }

    if (c == 0) {
        return FALSE;
    }

    if (bsearch(XUINT_TO_POINTER(c),  x_unicode_width_table_ambiguous, X_N_ELEMENTS(x_unicode_width_table_ambiguous), sizeof(x_unicode_width_table_ambiguous[0]), interval_compare)) {
        return TRUE;
    }

    return FALSE;
}

xunichar x_unichar_toupper(xunichar c)
{
    int t = TYPE(c);
    if (t == X_UNICODE_LOWERCASE_LETTER) {
        xunichar val = ATTTABLE(c >> 8, c & 0xff);
        if (val >= 0x1000000) {
            const xchar *p = special_case_table + (val - 0x1000000);
            val = x_utf8_get_char(p);
        }

        return val ? val : c;
    } else if (t == X_UNICODE_TITLECASE_LETTER) {
        unsigned int i;
        for (i = 0; i < X_N_ELEMENTS(title_table); ++i) {
            if (title_table[i][0] == c) {
                return title_table[i][1] ? title_table[i][1] : c;
            }
        }
    }

    return c;
}

xunichar x_unichar_tolower(xunichar c)
{
    int t = TYPE(c);
    if (t == X_UNICODE_UPPERCASE_LETTER) {
        xunichar val = ATTTABLE (c >> 8, c & 0xff);
        if (val >= 0x1000000) {
            const xchar *p = special_case_table + (val - 0x1000000);
            return x_utf8_get_char(p);
        } else {
            return val ? val : c;
        }
    } else if (t == X_UNICODE_TITLECASE_LETTER) {
        unsigned int i;
        for (i = 0; i < X_N_ELEMENTS(title_table); ++i) {
            if (title_table[i][0] == c) {
                return title_table[i][2];
            }
        }
    }

    return c;
}

xunichar x_unichar_totitle(xunichar c)
{
    unsigned int i;

    if (c == 0) {
        return c;
    }

    for (i = 0; i < X_N_ELEMENTS(title_table); ++i) {
        if (title_table[i][0] == c || title_table[i][1] == c || title_table[i][2] == c) {
            return title_table[i][0];
        }
    }

    if (TYPE(c) == X_UNICODE_LOWERCASE_LETTER) {
        return x_unichar_toupper(c);
    }

    return c;
}

int x_unichar_digit_value(xunichar c)
{
    if (TYPE(c) == X_UNICODE_DECIMAL_NUMBER) {
        return ATTTABLE(c >> 8, c & 0xff);
    }

    return -1;
}

int x_unichar_xdigit_value(xunichar c)
{
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }

    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }

    if (c >= X_UNICHAR_FULLWIDTH_A && c <= X_UNICHAR_FULLWIDTH_F) {
        return c - X_UNICHAR_FULLWIDTH_A + 10;
    }

    if (c >= X_UNICHAR_FULLWIDTH_a && c <= X_UNICHAR_FULLWIDTH_f) {
        return c - X_UNICHAR_FULLWIDTH_a + 10;
    }

    if (TYPE(c) == X_UNICODE_DECIMAL_NUMBER) {
        return ATTTABLE (c >> 8, c & 0xff);
    }

    return -1;
}

XUnicodeType x_unichar_type(xunichar c)
{
    return (XUnicodeType)TYPE(c);
}

typedef enum {
    LOCALE_NORMAL,
    LOCALE_TURKIC,
    LOCALE_LITHUANIAN
} LocaleType;

static LocaleType get_locale_type(void)
{
    const char *locale = setlocale(LC_CTYPE, NULL);

    if (locale == NULL) {
        return LOCALE_NORMAL;
    }

    switch (locale[0]) {
        case 'a':
            if (locale[1] == 'z') {
                return LOCALE_TURKIC;
            }
            break;

        case 'l':
            if (locale[1] == 't') {
                return LOCALE_LITHUANIAN;
            }
            break;

        case 't':
            if (locale[1] == 'r') {
                return LOCALE_TURKIC;
            }
            break;
    }

    return LOCALE_NORMAL;
}

static xint output_marks(const char **p_inout, char *out_buffer, xboolean remove_dot)
{
    xint len = 0;
    const char *p = *p_inout;

    while (*p) {
        xunichar c = x_utf8_get_char(p);

        if (ISMARK(TYPE(c))) {
            if (!remove_dot || c != 0x307) {
                len += x_unichar_to_utf8(c, out_buffer ? out_buffer + len : NULL);
            }

            p = x_utf8_next_char(p);
        } else {
            break;
        }
    }

    *p_inout = p;
    return len;
}

static xint output_special_case(xchar *out_buffer, int offset, int type, int which)
{
    xint len;
    const xchar *p = special_case_table + offset;

    if (type != X_UNICODE_TITLECASE_LETTER) {
        p = x_utf8_next_char(p);
    }

    if (which == 1) {
        p += strlen(p) + 1;
    }

    len = strlen(p);
    if (out_buffer) {
        memcpy(out_buffer, p, len);
    }

    return len;
}

static xsize real_toupper(const xchar *str, xssize max_len, xchar *out_buffer, LocaleType locale_type)
{
    xsize len = 0;
    const xchar *p = str;
    const char *last = NULL;
    xboolean last_was_i = FALSE;

    while ((max_len < 0 || p < str + max_len) && *p) {
        xunichar val;
        xunichar c = x_utf8_get_char(p);
        int t = TYPE(c);

        last = p;
        p = x_utf8_next_char(p);

        if (locale_type == LOCALE_LITHUANIAN) {
            if (c == 'i') {
                last_was_i = TRUE;
            } else {
                if (last_was_i) {
                    xsize decomp_len, i;
                    xunichar decomp[X_UNICHAR_MAX_DECOMPOSITION_LENGTH];

                    decomp_len = x_unichar_fully_decompose (c, FALSE, decomp, X_N_ELEMENTS(decomp));
                    for (i = 0; i < decomp_len; i++) {
                        if (decomp[i] != 0x307) {
                            len += x_unichar_to_utf8(x_unichar_toupper(decomp[i]), out_buffer ? out_buffer + len : NULL);
                        }
                    }

                    len += output_marks(&p, out_buffer ? out_buffer + len : NULL, TRUE);
                    continue;
                }

                if (!ISMARK(t)) {
                    last_was_i = FALSE;
                }
            }
        }

        if (locale_type == LOCALE_TURKIC && c == 'i') {
            len += x_unichar_to_utf8(0x130, out_buffer ? out_buffer + len : NULL); 
        } else if (c == 0x0345) {
            len += output_marks(&p, out_buffer ? out_buffer + len : NULL, FALSE);
            len += x_unichar_to_utf8(0x399, out_buffer ? out_buffer + len : NULL);
        } else if (IS(t, OR(X_UNICODE_LOWERCASE_LETTER, OR(X_UNICODE_TITLECASE_LETTER, 0)))) {
            val = ATTTABLE (c >> 8, c & 0xff);

            if (val >= 0x1000000) {
                len += output_special_case(out_buffer ? out_buffer + len : NULL, val - 0x1000000, t, t == X_UNICODE_LOWERCASE_LETTER ? 0 : 1);
            } else {
                if (t == X_UNICODE_TITLECASE_LETTER) {
                    unsigned int i;
                    for (i = 0; i < X_N_ELEMENTS(title_table); ++i) {
                        if (title_table[i][0] == c) {
                            val = title_table[i][1];
                            break;
                        }
                    }
                }

                len += x_unichar_to_utf8(val ? val : c, out_buffer ? out_buffer + len : NULL);
            }
        } else {
            xsize char_len = x_utf8_skip[*(xuchar *)last];
            if (out_buffer) {
                memcpy(out_buffer + len, last, char_len);
            }

            len += char_len;
        }
    }

    return len;
}

xchar *x_utf8_strup(const xchar *str, xssize len)
{
    xchar *result;
    xsize result_len;
    LocaleType locale_type;

    x_return_val_if_fail(str != NULL, NULL);

    locale_type = get_locale_type();

    result_len = real_toupper(str, len, NULL, locale_type);
    result = (xchar *)x_malloc(result_len + 1);
    real_toupper(str, len, result, locale_type);
    result[result_len] = '\0';

    return result;
}

static xboolean has_more_above(const xchar *str)
{
    const xchar *p = str;
    xint combining_class;

    while (*p) {
        combining_class = x_unichar_combining_class(x_utf8_get_char(p));
        if (combining_class == 230) {
            return TRUE;
        } else if (combining_class == 0) {
            break;
        }

        p = x_utf8_next_char(p);
    }

    return FALSE;
}

static xsize real_tolower(const xchar *str, xssize max_len, xchar *out_buffer, LocaleType locale_type)
{
    xsize len = 0;
    const xchar *p = str;
    const char *last = NULL;

    while ((max_len < 0 || p < str + max_len) && *p) {
        xunichar val;
        xunichar c = x_utf8_get_char(p);
        int t = TYPE(c);

        last = p;
        p = x_utf8_next_char(p);

        if (locale_type == LOCALE_TURKIC && (c == 'I' || c == 0x130 || c == X_UNICHAR_FULLWIDTH_I)) {
            xboolean combining_dot = (c == 'I' || c == X_UNICHAR_FULLWIDTH_I) && x_utf8_get_char(p) == 0x0307;
            if (combining_dot || c == 0x130) {
                len += x_unichar_to_utf8(0x0069, out_buffer ? out_buffer + len : NULL);
                if (combining_dot) {
                    p = x_utf8_next_char(p);
                }
            } else {
                len += x_unichar_to_utf8(0x131, out_buffer ? out_buffer + len : NULL); 
            }
        } else if (locale_type == LOCALE_LITHUANIAN && (c == 0x00cc || c == 0x00cd || c == 0x0128)) {
            len += x_unichar_to_utf8(0x0069, out_buffer ? out_buffer + len : NULL); 
            len += x_unichar_to_utf8(0x0307, out_buffer ? out_buffer + len : NULL); 

            switch (c) {
                case 0x00cc: 
                    len += x_unichar_to_utf8(0x0300, out_buffer ? out_buffer + len : NULL); 
                    break;

                case 0x00cd: 
                    len += x_unichar_to_utf8(0x0301, out_buffer ? out_buffer + len : NULL); 
                    break;

                case 0x0128: 
                    len += x_unichar_to_utf8(0x0303, out_buffer ? out_buffer + len : NULL); 
                    break;
            }
        } else if (locale_type == LOCALE_LITHUANIAN &&  (c == 'I' || c == X_UNICHAR_FULLWIDTH_I || c == 'J' || c == X_UNICHAR_FULLWIDTH_J || c == 0x012e) && has_more_above(p)) {
            len += x_unichar_to_utf8(x_unichar_tolower(c), out_buffer ? out_buffer + len : NULL); 
            len += x_unichar_to_utf8(0x0307, out_buffer ? out_buffer + len : NULL); 
        } else if (c == 0x03A3) {
            if ((max_len < 0 || p < str + max_len) && *p) {
                xunichar next_c = x_utf8_get_char(p);
                int next_type = TYPE(next_c);

                if (ISALPHA(next_type)) {
                    val = 0x3c3;
                } else {
                    val = 0x3c2;
                }
            } else {
                val = 0x3c2;
            }

            len += x_unichar_to_utf8(val, out_buffer ? out_buffer + len : NULL);
        } else if (IS(t, OR(X_UNICODE_UPPERCASE_LETTER, OR(X_UNICODE_TITLECASE_LETTER, 0)))) {
            val = ATTTABLE (c >> 8, c & 0xff);
            if (val >= 0x1000000) {
                len += output_special_case(out_buffer ? out_buffer + len : NULL, val - 0x1000000, t, 0);
            } else {
                if (t == X_UNICODE_TITLECASE_LETTER) {
                    unsigned int i;
                    for (i = 0; i < X_N_ELEMENTS(title_table); ++i) {
                        if (title_table[i][0] == c) {
                            val = title_table[i][2];
                            break;
                        }
                    }
                }

                len += x_unichar_to_utf8(val ? val : c, out_buffer ? out_buffer + len : NULL);
            }
        } else {
            xsize char_len = x_utf8_skip[*(xuchar *)last];
            if (out_buffer) {
                memcpy(out_buffer + len, last, char_len);
            }

            len += char_len;
        }
    }

    return len;
}

xchar *x_utf8_strdown(const xchar *str, xssize len)
{
    xchar *result;
    xsize result_len;
    LocaleType locale_type;

    x_return_val_if_fail(str != NULL, NULL);

    locale_type = get_locale_type();
    result_len = real_tolower(str, len, NULL, locale_type);
    result = (xchar *)x_malloc(result_len + 1);
    real_tolower(str, len, result, locale_type);
    result[result_len] = '\0';

    return result;
}

xchar *x_utf8_casefold(const xchar *str, xssize len)
{
    const char *p;
    XString *result;

    x_return_val_if_fail(str != NULL, NULL);

    result = x_string_new(NULL);
    p = str;

    while ((len < 0 || p < str + len) && *p) {
        xunichar ch = x_utf8_get_char(p);

        int start = 0;
        int end = X_N_ELEMENTS(casefold_table);

        if (ch >= casefold_table[start].ch && ch <= casefold_table[end - 1].ch) {
            while (TRUE) {
                int half = (start + end) / 2;
                if (ch == casefold_table[half].ch) {
                    x_string_append(result, casefold_table[half].data);
                    goto next;
                } else if (half == start) {
                    break;
                } else if (ch > casefold_table[half].ch) {
                    start = half;
                } else {
                    end = half;
                }
            }
        }

        x_string_append_unichar(result, x_unichar_tolower(ch));
next:
        p = x_utf8_next_char(p);
    }

    return x_string_free(result, FALSE); 
}

xboolean x_unichar_get_mirror_char(xunichar ch, xunichar *mirrored_ch)
{
    xboolean found;
    xunichar mirrored;

    mirrored = XLIB_GET_MIRRORING(ch);

    found = ch != mirrored;
    if (mirrored_ch) {
        *mirrored_ch = mirrored;
    }

    return found;
}

#define X_SCRIPT_TABLE_MIDPOINT         (X_N_ELEMENTS(x_script_table) / 2)

static inline XUnicodeScript x_unichar_get_script_bsearch(xunichar ch)
{
    int lower = 0;
    int upper = X_N_ELEMENTS(x_script_table) - 1;
    static int saved_mid = X_SCRIPT_TABLE_MIDPOINT;
    int mid = saved_mid;

    do {
        if (ch < x_script_table[mid].start) {
            upper = mid - 1;
        } else if (ch >= x_script_table[mid].start + x_script_table[mid].chars) {
            lower = mid + 1;
        } else {
            return (XUnicodeScript)x_script_table[saved_mid = mid].script;
        }

        mid = (lower + upper) / 2;
    } while (lower <= upper);

    return X_UNICODE_SCRIPT_UNKNOWN;
}

XUnicodeScript x_unichar_get_script(xunichar ch)
{
    if (ch < X_EASY_SCRIPTS_RANGE) {
        return (XUnicodeScript)x_script_easy_table[ch];
    } else  {
        return x_unichar_get_script_bsearch(ch);
    }
}

static const xuint32 iso15924_tags[] = {
#define PACK(a,b,c,d)   ((xuint32)((((xuint8)(a))<<24)|(((xuint8)(b))<<16)|(((xuint8)(c))<<8)|((xuint8)(d))))

    PACK ('Z','y','y','y'), /* X_UNICODE_SCRIPT_COMMON */
    PACK ('Z','i','n','h'), /* X_UNICODE_SCRIPT_INHERITED */
    PACK ('A','r','a','b'), /* X_UNICODE_SCRIPT_ARABIC */
    PACK ('A','r','m','n'), /* X_UNICODE_SCRIPT_ARMENIAN */
    PACK ('B','e','n','g'), /* X_UNICODE_SCRIPT_BENGALI */
    PACK ('B','o','p','o'), /* X_UNICODE_SCRIPT_BOPOMOFO */
    PACK ('C','h','e','r'), /* X_UNICODE_SCRIPT_CHEROKEE */
    PACK ('C','o','p','t'), /* X_UNICODE_SCRIPT_COPTIC */
    PACK ('C','y','r','l'), /* X_UNICODE_SCRIPT_CYRILLIC */
    PACK ('D','s','r','t'), /* X_UNICODE_SCRIPT_DESERET */
    PACK ('D','e','v','a'), /* X_UNICODE_SCRIPT_DEVANAGARI */
    PACK ('E','t','h','i'), /* X_UNICODE_SCRIPT_ETHIOPIC */
    PACK ('G','e','o','r'), /* X_UNICODE_SCRIPT_GEORGIAN */
    PACK ('G','o','t','h'), /* X_UNICODE_SCRIPT_GOTHIC */
    PACK ('G','r','e','k'), /* X_UNICODE_SCRIPT_GREEK */
    PACK ('G','u','j','r'), /* X_UNICODE_SCRIPT_GUJARATI */
    PACK ('G','u','r','u'), /* X_UNICODE_SCRIPT_GURMUKHI */
    PACK ('H','a','n','i'), /* X_UNICODE_SCRIPT_HAN */
    PACK ('H','a','n','g'), /* X_UNICODE_SCRIPT_HANGUL */
    PACK ('H','e','b','r'), /* X_UNICODE_SCRIPT_HEBREW */
    PACK ('H','i','r','a'), /* X_UNICODE_SCRIPT_HIRAGANA */
    PACK ('K','n','d','a'), /* X_UNICODE_SCRIPT_KANNADA */
    PACK ('K','a','n','a'), /* X_UNICODE_SCRIPT_KATAKANA */
    PACK ('K','h','m','r'), /* X_UNICODE_SCRIPT_KHMER */
    PACK ('L','a','o','o'), /* X_UNICODE_SCRIPT_LAO */
    PACK ('L','a','t','n'), /* X_UNICODE_SCRIPT_LATIN */
    PACK ('M','l','y','m'), /* X_UNICODE_SCRIPT_MALAYALAM */
    PACK ('M','o','n','g'), /* X_UNICODE_SCRIPT_MONGOLIAN */
    PACK ('M','y','m','r'), /* X_UNICODE_SCRIPT_MYANMAR */
    PACK ('O','g','a','m'), /* X_UNICODE_SCRIPT_OGHAM */
    PACK ('I','t','a','l'), /* X_UNICODE_SCRIPT_OLD_ITALIC */
    PACK ('O','r','y','a'), /* X_UNICODE_SCRIPT_ORIYA */
    PACK ('R','u','n','r'), /* X_UNICODE_SCRIPT_RUNIC */
    PACK ('S','i','n','h'), /* X_UNICODE_SCRIPT_SINHALA */
    PACK ('S','y','r','c'), /* X_UNICODE_SCRIPT_SYRIAC */
    PACK ('T','a','m','l'), /* X_UNICODE_SCRIPT_TAMIL */
    PACK ('T','e','l','u'), /* X_UNICODE_SCRIPT_TELUGU */
    PACK ('T','h','a','a'), /* X_UNICODE_SCRIPT_THAANA */
    PACK ('T','h','a','i'), /* X_UNICODE_SCRIPT_THAI */
    PACK ('T','i','b','t'), /* X_UNICODE_SCRIPT_TIBETAN */
    PACK ('C','a','n','s'), /* X_UNICODE_SCRIPT_CANADIAN_ABORIGINAL */
    PACK ('Y','i','i','i'), /* X_UNICODE_SCRIPT_YI */
    PACK ('T','g','l','g'), /* X_UNICODE_SCRIPT_TAGALOG */
    PACK ('H','a','n','o'), /* X_UNICODE_SCRIPT_HANUNOO */
    PACK ('B','u','h','d'), /* X_UNICODE_SCRIPT_BUHID */
    PACK ('T','a','g','b'), /* X_UNICODE_SCRIPT_TAGBANWA */

    /* Unicode-4.0 additions */
    PACK ('B','r','a','i'), /* X_UNICODE_SCRIPT_BRAILLE */
    PACK ('C','p','r','t'), /* X_UNICODE_SCRIPT_CYPRIOT */
    PACK ('L','i','m','b'), /* X_UNICODE_SCRIPT_LIMBU */
    PACK ('O','s','m','a'), /* X_UNICODE_SCRIPT_OSMANYA */
    PACK ('S','h','a','w'), /* X_UNICODE_SCRIPT_SHAVIAN */
    PACK ('L','i','n','b'), /* X_UNICODE_SCRIPT_LINEAR_B */
    PACK ('T','a','l','e'), /* X_UNICODE_SCRIPT_TAI_LE */
    PACK ('U','g','a','r'), /* X_UNICODE_SCRIPT_UGARITIC */

    /* Unicode-4.1 additions */
    PACK ('T','a','l','u'), /* X_UNICODE_SCRIPT_NEW_TAI_LUE */
    PACK ('B','u','g','i'), /* X_UNICODE_SCRIPT_BUGINESE */
    PACK ('G','l','a','g'), /* X_UNICODE_SCRIPT_GLAGOLITIC */
    PACK ('T','f','n','g'), /* X_UNICODE_SCRIPT_TIFINAGH */
    PACK ('S','y','l','o'), /* X_UNICODE_SCRIPT_SYLOTI_NAGRI */
    PACK ('X','p','e','o'), /* X_UNICODE_SCRIPT_OLD_PERSIAN */
    PACK ('K','h','a','r'), /* X_UNICODE_SCRIPT_KHAROSHTHI */

    /* Unicode-5.0 additions */
    PACK ('Z','z','z','z'), /* X_UNICODE_SCRIPT_UNKNOWN */
    PACK ('B','a','l','i'), /* X_UNICODE_SCRIPT_BALINESE */
    PACK ('X','s','u','x'), /* X_UNICODE_SCRIPT_CUNEIFORM */
    PACK ('P','h','n','x'), /* X_UNICODE_SCRIPT_PHOENICIAN */
    PACK ('P','h','a','g'), /* X_UNICODE_SCRIPT_PHAGS_PA */
    PACK ('N','k','o','o'), /* X_UNICODE_SCRIPT_NKO */

    /* Unicode-5.1 additions */
    PACK ('K','a','l','i'), /* X_UNICODE_SCRIPT_KAYAH_LI */
    PACK ('L','e','p','c'), /* X_UNICODE_SCRIPT_LEPCHA */
    PACK ('R','j','n','g'), /* X_UNICODE_SCRIPT_REJANG */
    PACK ('S','u','n','d'), /* X_UNICODE_SCRIPT_SUNDANESE */
    PACK ('S','a','u','r'), /* X_UNICODE_SCRIPT_SAURASHTRA */
    PACK ('C','h','a','m'), /* X_UNICODE_SCRIPT_CHAM */
    PACK ('O','l','c','k'), /* X_UNICODE_SCRIPT_OL_CHIKI */
    PACK ('V','a','i','i'), /* X_UNICODE_SCRIPT_VAI */
    PACK ('C','a','r','i'), /* X_UNICODE_SCRIPT_CARIAN */
    PACK ('L','y','c','i'), /* X_UNICODE_SCRIPT_LYCIAN */
    PACK ('L','y','d','i'), /* X_UNICODE_SCRIPT_LYDIAN */

    /* Unicode-5.2 additions */
    PACK ('A','v','s','t'), /* X_UNICODE_SCRIPT_AVESTAN */
    PACK ('B','a','m','u'), /* X_UNICODE_SCRIPT_BAMUM */
    PACK ('E','g','y','p'), /* X_UNICODE_SCRIPT_EGYPTIAN_HIEROGLYPHS */
    PACK ('A','r','m','i'), /* X_UNICODE_SCRIPT_IMPERIAL_ARAMAIC */
    PACK ('P','h','l','i'), /* X_UNICODE_SCRIPT_INSCRIPTIONAL_PAHLAVI */
    PACK ('P','r','t','i'), /* X_UNICODE_SCRIPT_INSCRIPTIONAL_PARTHIAN */
    PACK ('J','a','v','a'), /* X_UNICODE_SCRIPT_JAVANESE */
    PACK ('K','t','h','i'), /* X_UNICODE_SCRIPT_KAITHI */
    PACK ('L','i','s','u'), /* X_UNICODE_SCRIPT_LISU */
    PACK ('M','t','e','i'), /* X_UNICODE_SCRIPT_MEETEI_MAYEK */
    PACK ('S','a','r','b'), /* X_UNICODE_SCRIPT_OLD_SOUTH_ARABIAN */
    PACK ('O','r','k','h'), /* X_UNICODE_SCRIPT_OLD_TURKIC */
    PACK ('S','a','m','r'), /* X_UNICODE_SCRIPT_SAMARITAN */
    PACK ('L','a','n','a'), /* X_UNICODE_SCRIPT_TAI_THAM */
    PACK ('T','a','v','t'), /* X_UNICODE_SCRIPT_TAI_VIET */

    /* Unicode-6.0 additions */
    PACK ('B','a','t','k'), /* X_UNICODE_SCRIPT_BATAK */
    PACK ('B','r','a','h'), /* X_UNICODE_SCRIPT_BRAHMI */
    PACK ('M','a','n','d'), /* X_UNICODE_SCRIPT_MANDAIC */

    /* Unicode-6.1 additions */
    PACK ('C','a','k','m'), /* X_UNICODE_SCRIPT_CHAKMA */
    PACK ('M','e','r','c'), /* X_UNICODE_SCRIPT_MEROITIC_CURSIVE */
    PACK ('M','e','r','o'), /* X_UNICODE_SCRIPT_MEROITIC_HIEROGLYPHS */
    PACK ('P','l','r','d'), /* X_UNICODE_SCRIPT_MIAO */
    PACK ('S','h','r','d'), /* X_UNICODE_SCRIPT_SHARADA */
    PACK ('S','o','r','a'), /* X_UNICODE_SCRIPT_SORA_SOMPENG */
    PACK ('T','a','k','r'), /* X_UNICODE_SCRIPT_TAKRI */

    /* Unicode 7.0 additions */
    PACK ('B','a','s','s'), /* X_UNICODE_SCRIPT_BASSA_VAH */
    PACK ('A','g','h','b'), /* X_UNICODE_SCRIPT_CAUCASIAN_ALBANIAN */
    PACK ('D','u','p','l'), /* X_UNICODE_SCRIPT_DUPLOYAN */
    PACK ('E','l','b','a'), /* X_UNICODE_SCRIPT_ELBASAN */
    PACK ('G','r','a','n'), /* X_UNICODE_SCRIPT_GRANTHA */
    PACK ('K','h','o','j'), /* X_UNICODE_SCRIPT_KHOJKI*/
    PACK ('S','i','n','d'), /* X_UNICODE_SCRIPT_KHUDAWADI */
    PACK ('L','i','n','a'), /* X_UNICODE_SCRIPT_LINEAR_A */
    PACK ('M','a','h','j'), /* X_UNICODE_SCRIPT_MAHAJANI */
    PACK ('M','a','n','i'), /* X_UNICODE_SCRIPT_MANICHAEAN */
    PACK ('M','e','n','d'), /* X_UNICODE_SCRIPT_MENDE_KIKAKUI */
    PACK ('M','o','d','i'), /* X_UNICODE_SCRIPT_MODI */
    PACK ('M','r','o','o'), /* X_UNICODE_SCRIPT_MRO */
    PACK ('N','b','a','t'), /* X_UNICODE_SCRIPT_NABATAEAN */
    PACK ('N','a','r','b'), /* X_UNICODE_SCRIPT_OLD_NORTH_ARABIAN */
    PACK ('P','e','r','m'), /* X_UNICODE_SCRIPT_OLD_PERMIC */
    PACK ('H','m','n','g'), /* X_UNICODE_SCRIPT_PAHAWH_HMONG */
    PACK ('P','a','l','m'), /* X_UNICODE_SCRIPT_PALMYRENE */
    PACK ('P','a','u','c'), /* X_UNICODE_SCRIPT_PAU_CIN_HAU */
    PACK ('P','h','l','p'), /* X_UNICODE_SCRIPT_PSALTER_PAHLAVI */
    PACK ('S','i','d','d'), /* X_UNICODE_SCRIPT_SIDDHAM */
    PACK ('T','i','r','h'), /* X_UNICODE_SCRIPT_TIRHUTA */
    PACK ('W','a','r','a'), /* X_UNICODE_SCRIPT_WARANG_CITI */

    /* Unicode 8.0 additions */
    PACK ('A','h','o','m'), /* X_UNICODE_SCRIPT_AHOM */
    PACK ('H','l','u','w'), /* X_UNICODE_SCRIPT_ANATOLIAN_HIEROGLYPHS */
    PACK ('H','a','t','r'), /* X_UNICODE_SCRIPT_HATRAN */
    PACK ('M','u','l','t'), /* X_UNICODE_SCRIPT_MULTANI */
    PACK ('H','u','n','g'), /* X_UNICODE_SCRIPT_OLD_HUNGARIAN */
    PACK ('S','g','n','w'), /* X_UNICODE_SCRIPT_SIGNWRITING */

    /* Unicode 9.0 additions */
    PACK ('A','d','l','m'), /* X_UNICODE_SCRIPT_ADLAM */
    PACK ('B','h','k','s'), /* X_UNICODE_SCRIPT_BHAIKSUKI */
    PACK ('M','a','r','c'), /* X_UNICODE_SCRIPT_MARCHEN */
    PACK ('N','e','w','a'), /* X_UNICODE_SCRIPT_NEWA */
    PACK ('O','s','g','e'), /* X_UNICODE_SCRIPT_OSAGE */
    PACK ('T','a','n','g'), /* X_UNICODE_SCRIPT_TANGUT */

    /* Unicode 10.0 additions */
    PACK ('G','o','n','m'), /* X_UNICODE_SCRIPT_MASARAM_GONDI */
    PACK ('N','s','h','u'), /* X_UNICODE_SCRIPT_NUSHU */
    PACK ('S','o','y','o'), /* X_UNICODE_SCRIPT_SOYOMBO */
    PACK ('Z','a','n','b'), /* X_UNICODE_SCRIPT_ZANABAZAR_SQUARE */

    /* Unicode 11.0 additions */
    PACK ('D','o','g','r'), /* X_UNICODE_SCRIPT_DOGRA */
    PACK ('G','o','n','g'), /* X_UNICODE_SCRIPT_GUNJALA_GONDI */
    PACK ('R','o','h','g'), /* X_UNICODE_SCRIPT_HANIFI_ROHINGYA */
    PACK ('M','a','k','a'), /* X_UNICODE_SCRIPT_MAKASAR */
    PACK ('M','e','d','f'), /* X_UNICODE_SCRIPT_MEDEFAIDRIN */
    PACK ('S','o','g','o'), /* X_UNICODE_SCRIPT_OLD_SOGDIAN */
    PACK ('S','o','g','d'), /* X_UNICODE_SCRIPT_SOGDIAN */

    /* Unicode 12.0 additions */
    PACK ('E','l','y','m'), /* X_UNICODE_SCRIPT_ELYMAIC */
    PACK ('N','a','n','d'), /* X_UNICODE_SCRIPT_NANDINAGARI */
    PACK ('H','m','n','p'), /* X_UNICODE_SCRIPT_NYIAKENG_PUACHUE_HMONG */
    PACK ('W','c','h','o'), /* X_UNICODE_SCRIPT_WANCHO */

    /* Unicode 13.0 additions */
    PACK ('C', 'h', 'r', 's'), /* X_UNICODE_SCRIPT_CHORASMIAN */
    PACK ('D', 'i', 'a', 'k'), /* X_UNICODE_SCRIPT_DIVES_AKURU */
    PACK ('K', 'i', 't', 's'), /* X_UNICODE_SCRIPT_KHITAN_SMALL_SCRIPT */
    PACK ('Y', 'e', 'z', 'i'), /* X_UNICODE_SCRIPT_YEZIDI */

    /* Unicode 14.0 additions */
    PACK ('C', 'p', 'm', 'n'), /* X_UNICODE_SCRIPT_CYPRO_MINOAN */
    PACK ('O', 'u', 'g', 'r'), /* X_UNICODE_SCRIPT_OLD_UYHUR */
    PACK ('T', 'n', 's', 'a'), /* X_UNICODE_SCRIPT_TANGSA */
    PACK ('T', 'o', 't', 'o'), /* X_UNICODE_SCRIPT_TOTO */
    PACK ('V', 'i', 't', 'h'), /* X_UNICODE_SCRIPT_VITHKUQI */

    /* not really a Unicode script, but part of ISO 15924 */
    PACK ('Z', 'm', 't', 'h'), /* X_UNICODE_SCRIPT_MATH */

    /* Unicode 15.0 additions */
    PACK ('K', 'a', 'w', 'i'), /* X_UNICODE_SCRIPT_KAWI */
    PACK ('N', 'a', 'g', 'm'), /* X_UNICODE_SCRIPT_NAG_MUNDARI */

#undef PACK
};

xuint32 x_unicode_script_to_iso15924(XUnicodeScript script)
{
    if (X_UNLIKELY(script == X_UNICODE_SCRIPT_INVALID_CODE)) {
        return 0;
    }

    if (X_UNLIKELY(script < 0 || script >= (int)X_N_ELEMENTS(iso15924_tags))) {
        return 0x5A7A7A7A;
    }

    return iso15924_tags[script];
}

XUnicodeScript x_unicode_script_from_iso15924(xuint32 iso15924)
{
    unsigned int i;

    if (!iso15924) {
        return X_UNICODE_SCRIPT_INVALID_CODE;
    }

    for (i = 0; i < X_N_ELEMENTS(iso15924_tags); i++) {
        if (iso15924_tags[i] == iso15924) {
            return (XUnicodeScript) i;
        }
    }

    return X_UNICODE_SCRIPT_UNKNOWN;
}
