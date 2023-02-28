#include <string.h>
#include <xlib/xlib/config.h>
#include <xlib/xlib/xmem.h>
#include <xlib/xlib/xutils.h>
#include <xlib/xlib/xmacros.h>
#include <xlib/xlib/xpattern.h>
#include <xlib/xlib/xunicode.h>
#include <xlib/xlib/xmessages.h>
#include <xlib/xlib/xstrfuncs.h>

typedef enum {
    X_MATCH_ALL,       /* "*A?A*" */
    X_MATCH_ALL_TAIL,  /* "*A?AA" */
    X_MATCH_HEAD,      /* "AAAA*" */
    X_MATCH_TAIL,      /* "*AAAA" */
    X_MATCH_EXACT,     /* "AAAAA" */
    X_MATCH_LAST
} XMatchType;

struct _XPatternSpec {
    XMatchType match_type;
    xuint      pattern_length;
    xuint      min_length;
    xuint      max_length;
    xchar      *pattern;
};

static inline xboolean x_pattern_ph_match(const xchar *match_pattern, const xchar *match_string, xboolean *wildcard_reached_p)
{
    xchar ch;
    const xchar *pattern, *string;

    pattern = match_pattern;
    string = match_string;

    ch = *pattern;
    pattern++;

    while (ch) {
        switch (ch) {
            case '?':
                if (!*string) {
                    return FALSE;
                }
                string = x_utf8_next_char(string);
                break;

            case '*':
                *wildcard_reached_p = TRUE;
                do {
                    ch = *pattern;
                    pattern++;
                    if (ch == '?') {
                        if (!*string) {
                            return FALSE;
                        }

                        string = x_utf8_next_char(string);
                    }
                } while (ch == '*' || ch == '?');

                if (!ch) {
                    return TRUE;
                }

                do {
                    xboolean next_wildcard_reached = FALSE;
                    while (ch != *string) {
                        if (!*string) {
                            return FALSE;
                        }

                        string = x_utf8_next_char(string);
                    }

                    string++;
                    if (x_pattern_ph_match(pattern, string, &next_wildcard_reached)) {
                        return TRUE;
                    }

                    if (next_wildcard_reached) {
                        return FALSE;
                    }
                } while (*string);
                break;

            default:
                if (ch == *string) {
                    string++;
                } else {
                    return FALSE;
                }
                break;
        }

        ch = *pattern;
        pattern++;
    }

    return *string == 0;
}

xboolean x_pattern_spec_match(XPatternSpec *pspec, xsize string_length, const xchar *string, const xchar *string_reversed)
{
    x_return_val_if_fail(pspec != NULL, FALSE);
    x_return_val_if_fail(string != NULL, FALSE);

    if ((string_length < pspec->min_length) || (string_length > pspec->max_length)) {
        return FALSE;
    }

    xboolean dummy;
    switch (pspec->match_type) {
        case X_MATCH_ALL:
            return x_pattern_ph_match(pspec->pattern, string, &dummy);

        case X_MATCH_ALL_TAIL:
            if (string_reversed)
                return x_pattern_ph_match(pspec->pattern, string_reversed, &dummy);
            else {
                xchar *tmp;
                xboolean result;

                tmp = x_utf8_strreverse(string, string_length);
                result = x_pattern_ph_match(pspec->pattern, tmp, &dummy);
                x_free(tmp);
                return result;
            }

        case X_MATCH_HEAD:
            if (pspec->pattern_length == string_length) {
                return strcmp(pspec->pattern, string) == 0;
            } else if (pspec->pattern_length) {
                return strncmp(pspec->pattern, string, pspec->pattern_length) == 0;
            } else {
                return TRUE;
            }

        case X_MATCH_TAIL:
            if (pspec->pattern_length) {
                return strcmp(pspec->pattern, string + (string_length - pspec->pattern_length)) == 0;
            } else {
                return TRUE;
            }

        case X_MATCH_EXACT:
            if (pspec->pattern_length != string_length) {
                return FALSE;
            } else {
                return strcmp(pspec->pattern, string) == 0;
            }

        default:
            x_return_val_if_fail(pspec->match_type < X_MATCH_LAST, FALSE);
            return FALSE;
    }
}

xboolean x_pattern_match(XPatternSpec *pspec, xuint string_length, const xchar *string, const xchar *string_reversed)
{
    return x_pattern_spec_match(pspec, string_length, string, string_reversed);
}

XPatternSpec *x_pattern_spec_new(const xchar *pattern)
{
    xuint i;
    xchar *d;
    const xchar *s;
    XPatternSpec *pspec;
    xuint pending_jokers = 0;
    xboolean follows_wildcard = FALSE;
    xint hw_pos = -1, tw_pos = -1, hj_pos = -1, tj_pos = -1;
    xboolean seen_joker = FALSE, seen_wildcard = FALSE, more_wildcards = FALSE;

    x_return_val_if_fail(pattern != NULL, NULL);

    pspec = x_new(XPatternSpec, 1);
    pspec->pattern_length = strlen(pattern);
    pspec->min_length = 0;
    pspec->max_length = 0;
    pspec->pattern = x_new(xchar, pspec->pattern_length + 1);
    d = pspec->pattern;

    for (i = 0, s = pattern; *s != 0; s++) {
        switch (*s) {
            case '*':
                if (follows_wildcard) {
                    pspec->pattern_length--;
                    continue;
                }

                follows_wildcard = TRUE;
                if (hw_pos < 0) {
                    hw_pos = i;
                }
                tw_pos = i;
                break;

            case '?':
                pending_jokers++;
                pspec->min_length++;
                pspec->max_length += 4;
                continue;

            default:
                for (; pending_jokers; pending_jokers--, i++) {
                    *d++ = '?';
                    if (hj_pos < 0) {
                        hj_pos = i;
                    }
                    tj_pos = i;
                }
                follows_wildcard = FALSE;
                pspec->min_length++;
                pspec->max_length++;
                break;
        }

        *d++ = *s;
        i++;
    }

    for (; pending_jokers; pending_jokers--) {
        *d++ = '?';
        if (hj_pos < 0) {
            hj_pos = i;
        }
        tj_pos = i;
    }

    *d++ = 0;
    seen_joker = hj_pos >= 0;
    seen_wildcard = hw_pos >= 0;
    more_wildcards = seen_wildcard && hw_pos != tw_pos;
    if (seen_wildcard) {
        pspec->max_length = X_MAXUINT;
    }

    if (!seen_joker && !more_wildcards) {
        if (pspec->pattern[0] == '*') {
            pspec->match_type = X_MATCH_TAIL;
            memmove(pspec->pattern, pspec->pattern + 1, --pspec->pattern_length);
            pspec->pattern[pspec->pattern_length] = 0;
            return pspec;
        }

        if ((pspec->pattern_length > 0) && (pspec->pattern[pspec->pattern_length - 1] == '*')) {
            pspec->match_type = X_MATCH_HEAD;
            pspec->pattern[--pspec->pattern_length] = 0;
            return pspec;
        }

        if (!seen_wildcard) {
            pspec->match_type = X_MATCH_EXACT;
            return pspec;
        }
    }

    tw_pos = pspec->pattern_length - 1 - tw_pos;
    tj_pos = pspec->pattern_length - 1 - tj_pos;
    if (seen_wildcard) {
        pspec->match_type = tw_pos > hw_pos ? X_MATCH_ALL_TAIL : X_MATCH_ALL;
    } else {
        pspec->match_type = tj_pos > hj_pos ? X_MATCH_ALL_TAIL : X_MATCH_ALL;
    }

    if (pspec->match_type == X_MATCH_ALL_TAIL) {
        xchar *tmp = pspec->pattern;
        pspec->pattern = x_utf8_strreverse(pspec->pattern, pspec->pattern_length);
        x_free(tmp);
    }

    return pspec;
}

XPatternSpec *x_pattern_spec_copy(XPatternSpec *pspec)
{
    XPatternSpec *pspec_copy;

    x_return_val_if_fail(pspec != NULL, NULL);

    pspec_copy = x_new(XPatternSpec, 1);
    *pspec_copy = *pspec;
    pspec_copy->pattern = x_strndup(pspec->pattern, pspec->pattern_length);

    return pspec_copy;
}

void x_pattern_spec_free(XPatternSpec *pspec)
{
    x_return_if_fail(pspec != NULL);

    x_free(pspec->pattern);
    x_free(pspec);
}

xboolean x_pattern_spec_equal(XPatternSpec *pspec1, XPatternSpec *pspec2)
{
    x_return_val_if_fail(pspec1 != NULL, FALSE);
    x_return_val_if_fail(pspec2 != NULL, FALSE);

    return (pspec1->pattern_length == pspec2->pattern_length && pspec1->match_type == pspec2->match_type && strcmp(pspec1->pattern, pspec2->pattern) == 0);
}

xboolean x_pattern_spec_match_string(XPatternSpec *pspec, const xchar *string)
{
    x_return_val_if_fail(pspec != NULL, FALSE);
    x_return_val_if_fail(string != NULL, FALSE);

    return x_pattern_spec_match(pspec, strlen (string), string, NULL);
}

xboolean x_pattern_match_string(XPatternSpec *pspec, const xchar *string)
{
    return x_pattern_spec_match_string(pspec, string);
}

xboolean x_pattern_match_simple(const xchar *pattern, const xchar *string)
{
    xboolean ergo;
    XPatternSpec *pspec;

    x_return_val_if_fail(pattern != NULL, FALSE);
    x_return_val_if_fail(string != NULL, FALSE);

    pspec = x_pattern_spec_new(pattern);
    ergo = x_pattern_spec_match(pspec, strlen(string), string, NULL);
    x_pattern_spec_free(pspec);

    return ergo;
}
