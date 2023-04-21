#include <stdlib.h>

#include <xlib/xlib/config.h>
#include <xlib/xlib/xmem.h>
#include <xlib/xlib/xunicode.h>
#include <xlib/xlib/xunicomp.h>
#include <xlib/xlib/xunidecomp.h>
#include <xlib/xlib/xunicodeprivate.h>

#define CC_PART1(Page, Char) \
    ((combining_class_table_part1[Page] >= X_UNICODE_MAX_TABLE_INDEX) ? (combining_class_table_part1[Page] - X_UNICODE_MAX_TABLE_INDEX) : (cclass_data[combining_class_table_part1[Page]][Char]))

#define CC_PART2(Page, Char) \
    ((combining_class_table_part2[Page] >= X_UNICODE_MAX_TABLE_INDEX) ? (combining_class_table_part2[Page] - X_UNICODE_MAX_TABLE_INDEX) : (cclass_data[combining_class_table_part2[Page]][Char]))

#define COMBINING_CLASS(Char) \
     (((Char) <= X_UNICODE_LAST_CHAR_PART1) ? CC_PART1((Char) >> 8, (Char) & 0xff) : (((Char) >= 0xe0000 && (Char) <= X_UNICODE_LAST_CHAR) ? CC_PART2(((Char) - 0xe0000) >> 8, (Char) & 0xff) : 0))

xint x_unichar_combining_class(xunichar uc)
{
    return COMBINING_CLASS(uc);
}

#define SBase                   0xAC00 
#define LBase                   0x1100 
#define VBase                   0x1161 
#define TBase                   0x11A7
#define LCount                  19 
#define VCount                  21
#define TCount                  28
#define NCount                  (VCount * TCount)
#define SCount                  (LCount * NCount)

void x_unicode_canonical_ordering(xunichar *string, xsize len)
{
    xsize i;
    int swap = 1;

    while (swap) {
        int last;
        swap = 0;
        last = COMBINING_CLASS(string[0]);
        for (i = 0; i < len - 1; ++i) {
            int next = COMBINING_CLASS(string[i + 1]);
            if (next != 0 && last > next) {
                xsize j;

                for (j = i + 1; j > 0; --j) {
                    xunichar t;
                    if (COMBINING_CLASS(string[j - 1]) <= next) {
                        break;
                    }

                    t = string[j];
                    string[j] = string[j - 1];
                    string[j - 1] = t;
                    swap = 1;
                }

                next = last;
            }

            last = next;
        }
    }
}

static void decompose_hangul(xunichar s, xunichar *r, xsize *result_len)
{
    xint SIndex = s - SBase;
    xint TIndex = SIndex % TCount;

    if (r) {
        r[0] = LBase + SIndex / NCount;
        r[1] = VBase + (SIndex % NCount) / TCount;
    }

    if (TIndex) {
        if (r) {
            r[2] = TBase + TIndex;
        }

        *result_len = 3;
    } else {
        *result_len = 2;
    }
}

static const xchar *find_decomposition (xunichar ch, xboolean compat)
{
    int start = 0;
    int end = X_N_ELEMENTS(decomp_table);

    if (ch >= decomp_table[start].ch && ch <= decomp_table[end - 1].ch) {
        while (TRUE) {
            int half = (start + end) / 2;
            if (ch == decomp_table[half].ch) {
                int offset;
                if (compat) {
                    offset = decomp_table[half].compat_offset;
                    if (offset == X_UNICODE_NOT_PRESENT_OFFSET) {
                        offset = decomp_table[half].canon_offset;
                    }
                } else {
                    offset = decomp_table[half].canon_offset;
                    if (offset == X_UNICODE_NOT_PRESENT_OFFSET) {
                        return NULL;
                    }
                }

                return &(decomp_expansion_string[offset]);
            } else if (half == start){
                break;
            } else if (ch > decomp_table[half].ch){
                start = half;
            } else{
                end = half;
            }
        }
    }

    return NULL;
}

xunichar *x_unicode_canonical_decomposition(xunichar ch, xsize *result_len)
{
    xunichar *r;
    const xchar *p;
    const xchar *decomp;

    if (ch >= SBase && ch < SBase + SCount) {
        decompose_hangul(ch, NULL, result_len);
        r = (xunichar *)x_malloc(*result_len * sizeof(xunichar));
        decompose_hangul(ch, r, result_len);
    } else if ((decomp = find_decomposition(ch, FALSE)) != NULL) {
        int i;

        *result_len = x_utf8_strlen(decomp, -1);
        r = (xunichar *)x_malloc(*result_len * sizeof(xunichar));

        for (p = decomp, i = 0; *p != '\0'; p = x_utf8_next_char(p), i++) {
            r[i] = x_utf8_get_char(p);
        }
    } else {
        r = (xunichar *)x_malloc(sizeof(xunichar));
        *r = ch;
        *result_len = 1;
    }

    return r;
}

static xboolean combine_hangul(xunichar a, xunichar b, xunichar *result)
{
    xint LIndex = a - LBase;
    xint SIndex = a - SBase;
    xint VIndex = b - VBase;
    xint TIndex = b - TBase;

    if (0 <= LIndex && LIndex < LCount && 0 <= VIndex && VIndex < VCount) {
        *result = SBase + (LIndex * VCount + VIndex) * TCount;
        return TRUE;
    } else if (0 <= SIndex && SIndex < SCount && (SIndex % TCount) == 0 && 0 < TIndex && TIndex < TCount) {
        *result = a + TIndex;
        return TRUE;
    }

    return FALSE;
}

#define CI(Page, Char) \
    ((compose_table[Page] >= X_UNICODE_MAX_TABLE_INDEX) ? (compose_table[Page] - X_UNICODE_MAX_TABLE_INDEX) : (compose_data[compose_table[Page]][Char]))

#define COMPOSE_INDEX(Char) \
    (((Char >> 8) > (COMPOSE_TABLE_LAST)) ? 0 : CI((Char) >> 8, (Char) & 0xff))

static xboolean combine(xunichar a, xunichar b, xunichar *result)
{
    xushort index_a, index_b;

    if (combine_hangul(a, b, result)) {
        return TRUE;
    }

    index_a = COMPOSE_INDEX(a);

    if (index_a >= COMPOSE_FIRST_SINGLE_START && index_a < COMPOSE_SECOND_START) {
        if (b == compose_first_single[index_a - COMPOSE_FIRST_SINGLE_START][0]) {
            *result = compose_first_single[index_a - COMPOSE_FIRST_SINGLE_START][1];
            return TRUE;
        } else {
            return FALSE;
        }
    }

    index_b = COMPOSE_INDEX(b);
    if (index_b >= COMPOSE_SECOND_SINGLE_START) {
        if (a == compose_second_single[index_b - COMPOSE_SECOND_SINGLE_START][0]) {
            *result = compose_second_single[index_b - COMPOSE_SECOND_SINGLE_START][1];
            return TRUE;
        } else {
            return FALSE;
        }
    }

    if (index_a >= COMPOSE_FIRST_START && index_a < COMPOSE_FIRST_SINGLE_START && index_b >= COMPOSE_SECOND_START && index_b < COMPOSE_SECOND_SINGLE_START) {
        xunichar res = compose_array[index_a - COMPOSE_FIRST_START][index_b - COMPOSE_SECOND_START];
        if (res) {
            *result = res;
            return TRUE;
        }
    }

    return FALSE;
}

xunichar *_x_utf8_normalize_wc(const xchar *str, xssize max_len, XNormalizeMode mode)
{
    xsize n_wc;
    const char *p;
    xsize last_start;
    xunichar *wc_buffer;
    xboolean do_compat = (mode == X_NORMALIZE_NFKC || mode == X_NORMALIZE_NFKD);
    xboolean do_compose = (mode == X_NORMALIZE_NFC || mode == X_NORMALIZE_NFKC);

    n_wc = 0;
    p = str;

    while ((max_len < 0 || p < str + max_len) && *p) {
        xunichar wc;
        const xchar *decomp;
        const char *next, *between;

        next = x_utf8_next_char(p);
        if (max_len < 0) {
            for (between = &p[1]; between < next; between++) {
                if (X_UNLIKELY(!*between)) {
                    return NULL;
                }
            }
        } else {
            if (X_UNLIKELY(next > str + max_len)) {
                return NULL;
            }
        }

        wc = x_utf8_get_char(p);
        if (X_UNLIKELY(wc == (xunichar)-1)) {
            return NULL;
        } else if (wc >= SBase && wc < SBase + SCount) {
            xsize result_len;
            decompose_hangul(wc, NULL, &result_len);
            n_wc += result_len;
        } else {
            decomp = find_decomposition(wc, do_compat);
            if (decomp) {
                n_wc += x_utf8_strlen(decomp, -1);
            } else {
                n_wc++;
            }
        }

        p = next;
    }

    wc_buffer = x_new(xunichar, n_wc + 1);

    last_start = 0;
    n_wc = 0;
    p = str;

    while ((max_len < 0 || p < str + max_len) && *p) {
        int cc;
        const xchar *decomp;
        xsize old_n_wc = n_wc;
        xunichar wc = x_utf8_get_char(p);

        if (wc >= SBase && wc < SBase + SCount) {
            xsize result_len;
            decompose_hangul(wc, wc_buffer + n_wc, &result_len);
            n_wc += result_len;
        } else {
            decomp = find_decomposition(wc, do_compat);
            if (decomp) {
                const char *pd;
                for (pd = decomp; *pd != '\0'; pd = x_utf8_next_char(pd)) {
                    wc_buffer[n_wc++] = x_utf8_get_char(pd);
                }
            } else {
                wc_buffer[n_wc++] = wc;
            }
        }

        if (n_wc > 0) {
            cc = COMBINING_CLASS(wc_buffer[old_n_wc]);
            if (cc == 0) {
                x_unicode_canonical_ordering(wc_buffer + last_start, n_wc - last_start);
                last_start = old_n_wc;
            }
        }

        p = x_utf8_next_char(p);
    }

    if (n_wc > 0) {
        x_unicode_canonical_ordering(wc_buffer + last_start, n_wc - last_start);
        last_start = n_wc;
        (void)last_start;
    }

    wc_buffer[n_wc] = 0;

    if (do_compose && n_wc > 0) {
        xsize i, j;
        last_start = 0;
        int last_cc = 0;

        for (i = 0; i < n_wc; i++) {
            int cc = COMBINING_CLASS(wc_buffer[i]);

            if (i > 0 && (last_cc == 0 || last_cc < cc) && combine(wc_buffer[last_start], wc_buffer[i], &wc_buffer[last_start])) {
                for (j = i + 1; j < n_wc; j++) {
                    wc_buffer[j-1] = wc_buffer[j];
                }

                n_wc--;
                i--;

                if (i == last_start) {
                    last_cc = 0;
                } else {
                    last_cc = COMBINING_CLASS(wc_buffer[i-1]);
                }

                continue;
            }

            if (cc == 0) {
                last_start = i;
            }

            last_cc = cc;
        }
    }

    wc_buffer[n_wc] = 0;
    return wc_buffer;
}

xchar *x_utf8_normalize(const xchar *str, xssize len, XNormalizeMode mode)
{
    xchar *result = NULL;
    xunichar *result_wc = _x_utf8_normalize_wc(str, len, mode);

    if (X_LIKELY(result_wc != NULL)) {
        result = x_ucs4_to_utf8(result_wc, -1, NULL, NULL, NULL);
        x_free(result_wc);
    }

    return result;
}

static xboolean decompose_hangul_step(xunichar ch, xunichar *a, xunichar *b)
{
    xint SIndex, TIndex;

    if (ch < SBase || ch >= SBase + SCount) {
        return FALSE;
    }

    SIndex = ch - SBase;
    TIndex = SIndex % TCount;

    if (TIndex) {
        *a = ch - TIndex;
        *b = TBase + TIndex;
    } else {
        *a = LBase + SIndex / NCount;
        *b = VBase + (SIndex % NCount) / TCount;
    }

    return TRUE;
}

xboolean x_unichar_decompose(xunichar ch, xunichar *a, xunichar *b)
{
    xint start = 0;
    xint end = X_N_ELEMENTS(decomp_step_table);

    if (decompose_hangul_step(ch, a, b)) {
        return TRUE;
    }

    if (ch >= decomp_step_table[start].ch && ch <= decomp_step_table[end - 1].ch) {
        while (TRUE) {
            xint half = (start + end) / 2;
            const decomposition_step *p = &(decomp_step_table[half]);
            if (ch == p->ch) {
                *a = p->a;
                *b = p->b;
                return TRUE;
            } else if (half == start) {
                break;
            } else if (ch > p->ch) {
                start = half;
            } else {
                end = half;
            }
        }
    }

    *a = ch;
    *b = 0;

    return FALSE;
}

xboolean x_unichar_compose(xunichar a, xunichar b, xunichar *ch)
{
    if (combine(a, b, ch)) {
        return TRUE;
    }

    *ch = 0;
    return FALSE;
}

xsize x_unichar_fully_decompose(xunichar ch, xboolean  compat, xunichar *result, xsize result_len)
{
    const xchar *p;
    const xchar *decomp;

    if (ch >= SBase && ch < SBase + SCount) {
        xsize len, i;
        xunichar buffer[3];
        decompose_hangul(ch, result ? buffer : NULL, &len);
        if (result) {
            for (i = 0; i < len && i < result_len; i++) {
                result[i] = buffer[i];
            }
        }

        return len;
    } else if ((decomp = find_decomposition(ch, compat)) != NULL) {
        xsize len, i;

        len = x_utf8_strlen(decomp, -1);
        for (p = decomp, i = 0; i < len && i < result_len; p = x_utf8_next_char(p), i++) {
            result[i] = x_utf8_get_char(p);
        }

        return len;
    }

    if (result && result_len >= 1) {
        *result = ch;
    }

    return 1;
}
