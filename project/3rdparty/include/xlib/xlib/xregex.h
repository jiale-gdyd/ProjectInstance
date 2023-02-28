#ifndef __X_REGEX_H__
#define __X_REGEX_H__

#include "xerror.h"
#include "xstring.h"

X_BEGIN_DECLS

typedef enum {
    X_REGEX_ERROR_COMPILE,
    X_REGEX_ERROR_OPTIMIZE,
    X_REGEX_ERROR_REPLACE,
    X_REGEX_ERROR_MATCH,
    X_REGEX_ERROR_INTERNAL,

    X_REGEX_ERROR_STRAY_BACKSLASH                              = 101,
    X_REGEX_ERROR_MISSING_CONTROL_CHAR                         = 102,
    X_REGEX_ERROR_UNRECOGNIZED_ESCAPE                          = 103,
    X_REGEX_ERROR_QUANTIFIERS_OUT_OF_ORDER                     = 104,
    X_REGEX_ERROR_QUANTIFIER_TOO_BIG                           = 105,
    X_REGEX_ERROR_UNTERMINATED_CHARACTER_CLASS                 = 106,
    X_REGEX_ERROR_INVALID_ESCAPE_IN_CHARACTER_CLASS            = 107,
    X_REGEX_ERROR_RANGE_OUT_OF_ORDER                           = 108,
    X_REGEX_ERROR_NOTHING_TO_REPEAT                            = 109,
    X_REGEX_ERROR_UNRECOGNIZED_CHARACTER                       = 112,
    X_REGEX_ERROR_POSIX_NAMED_CLASS_OUTSIDE_CLASS              = 113,
    X_REGEX_ERROR_UNMATCHED_PARENTHESIS                        = 114,
    X_REGEX_ERROR_INEXISTENT_SUBPATTERN_REFERENCE              = 115,
    X_REGEX_ERROR_UNTERMINATED_COMMENT                         = 118,
    X_REGEX_ERROR_EXPRESSION_TOO_LARGE                         = 120,
    X_REGEX_ERROR_MEMORY_ERROR                                 = 121,
    X_REGEX_ERROR_VARIABLE_LENGTH_LOOKBEHIND                   = 125,
    X_REGEX_ERROR_MALFORMED_CONDITION                          = 126,
    X_REGEX_ERROR_TOO_MANY_CONDITIONAL_BRANCHES                = 127,
    X_REGEX_ERROR_ASSERTION_EXPECTED                           = 128,
    X_REGEX_ERROR_UNKNOWN_POSIX_CLASS_NAME                     = 130,
    X_REGEX_ERROR_POSIX_COLLATING_ELEMENTS_NOT_SUPPORTED       = 131,
    X_REGEX_ERROR_HEX_CODE_TOO_LARGE                           = 134,
    X_REGEX_ERROR_INVALID_CONDITION                            = 135,
    X_REGEX_ERROR_SINGLE_BYTE_MATCH_IN_LOOKBEHIND              = 136,
    X_REGEX_ERROR_INFINITE_LOOP                                = 140,
    X_REGEX_ERROR_MISSING_SUBPATTERN_NAME_TERMINATOR           = 142,
    X_REGEX_ERROR_DUPLICATE_SUBPATTERN_NAME                    = 143,
    X_REGEX_ERROR_MALFORMED_PROPERTY                           = 146,
    X_REGEX_ERROR_UNKNOWN_PROPERTY                             = 147,
    X_REGEX_ERROR_SUBPATTERN_NAME_TOO_LONG                     = 148,
    X_REGEX_ERROR_TOO_MANY_SUBPATTERNS                         = 149,
    X_REGEX_ERROR_INVALID_OCTAL_VALUE                          = 151,
    X_REGEX_ERROR_TOO_MANY_BRANCHES_IN_DEFINE                  = 154,
    X_REGEX_ERROR_DEFINE_REPETION                              = 155,
    X_REGEX_ERROR_INCONSISTENT_NEWLINE_OPTIONS                 = 156,
    X_REGEX_ERROR_MISSING_BACK_REFERENCE                       = 157,
    X_REGEX_ERROR_INVALID_RELATIVE_REFERENCE                   = 158,
    X_REGEX_ERROR_BACKTRACKING_CONTROL_VERB_ARGUMENT_FORBIDDEN = 159,
    X_REGEX_ERROR_UNKNOWN_BACKTRACKING_CONTROL_VERB            = 160,
    X_REGEX_ERROR_NUMBER_TOO_BIG                               = 161,
    X_REGEX_ERROR_MISSING_SUBPATTERN_NAME                      = 162,
    X_REGEX_ERROR_MISSING_DIGIT                                = 163,
    X_REGEX_ERROR_INVALID_DATA_CHARACTER                       = 164,
    X_REGEX_ERROR_EXTRA_SUBPATTERN_NAME                        = 165,
    X_REGEX_ERROR_BACKTRACKING_CONTROL_VERB_ARGUMENT_REQUIRED  = 166,
    X_REGEX_ERROR_INVALID_CONTROL_CHAR                         = 168,
    X_REGEX_ERROR_MISSING_NAME                                 = 169,
    X_REGEX_ERROR_NOT_SUPPORTED_IN_CLASS                       = 171,
    X_REGEX_ERROR_TOO_MANY_FORWARD_REFERENCES                  = 172,
    X_REGEX_ERROR_NAME_TOO_LONG                                = 175,
    X_REGEX_ERROR_CHARACTER_VALUE_TOO_LARGE                    = 176
} XRegexError;

#define X_REGEX_ERROR x_regex_error_quark()

XLIB_AVAILABLE_IN_ALL
XQuark x_regex_error_quark(void);

typedef enum {
    X_REGEX_DEFAULT XLIB_AVAILABLE_ENUMERATOR_IN_2_74 = 0,
    X_REGEX_CASELESS          = 1 << 0,
    X_REGEX_MULTILINE         = 1 << 1,
    X_REGEX_DOTALL            = 1 << 2,
    X_REGEX_EXTENDED          = 1 << 3,
    X_REGEX_ANCHORED          = 1 << 4,
    X_REGEX_DOLLAR_ENDONLY    = 1 << 5,
    X_REGEX_UNGREEDY          = 1 << 9,
    X_REGEX_RAW               = 1 << 11,
    X_REGEX_NO_AUTO_CAPTURE   = 1 << 12,
    X_REGEX_OPTIMIZE          = 1 << 13,
    X_REGEX_FIRSTLINE         = 1 << 18,
    X_REGEX_DUPNAMES          = 1 << 19,
    X_REGEX_NEWLINE_CR        = 1 << 20,
    X_REGEX_NEWLINE_LF        = 1 << 21,
    X_REGEX_NEWLINE_CRLF      = X_REGEX_NEWLINE_CR | X_REGEX_NEWLINE_LF,
    X_REGEX_NEWLINE_ANYCRLF   = X_REGEX_NEWLINE_CR | 1 << 22,
    X_REGEX_BSR_ANYCRLF       = 1 << 23,
    X_REGEX_JAVASCRIPT_COMPAT XLIB_DEPRECATED_ENUMERATOR_IN_2_74 = 1 << 25
} XRegexCompileFlags;

typedef enum {
    X_REGEX_MATCH_DEFAULT XLIB_AVAILABLE_ENUMERATOR_IN_2_74 = 0,
    X_REGEX_MATCH_ANCHORED         = 1 << 4,
    X_REGEX_MATCH_NOTBOL           = 1 << 7,
    X_REGEX_MATCH_NOTEOL           = 1 << 8,
    X_REGEX_MATCH_NOTEMPTY         = 1 << 10,
    X_REGEX_MATCH_PARTIAL          = 1 << 15,
    X_REGEX_MATCH_NEWLINE_CR       = 1 << 20,
    X_REGEX_MATCH_NEWLINE_LF       = 1 << 21,
    X_REGEX_MATCH_NEWLINE_CRLF     = X_REGEX_MATCH_NEWLINE_CR | X_REGEX_MATCH_NEWLINE_LF,
    X_REGEX_MATCH_NEWLINE_ANY      = 1 << 22,
    X_REGEX_MATCH_NEWLINE_ANYCRLF  = X_REGEX_MATCH_NEWLINE_CR | X_REGEX_MATCH_NEWLINE_ANY,
    X_REGEX_MATCH_BSR_ANYCRLF      = 1 << 23,
    X_REGEX_MATCH_BSR_ANY          = 1 << 24,
    X_REGEX_MATCH_PARTIAL_SOFT     = X_REGEX_MATCH_PARTIAL,
    X_REGEX_MATCH_PARTIAL_HARD     = 1 << 27,
    X_REGEX_MATCH_NOTEMPTY_ATSTART = 1 << 28
} XRegexMatchFlags;

typedef struct _XRegex XRegex;
typedef struct _XMatchInfo XMatchInfo;

typedef xboolean (*XRegexEvalCallback)(const XMatchInfo *match_info, XString *result, xpointer user_data);


XLIB_AVAILABLE_IN_ALL
XRegex *x_regex_new(const xchar *pattern, XRegexCompileFlags compile_options, XRegexMatchFlags match_options, XError **error);

XLIB_AVAILABLE_IN_ALL
XRegex *x_regex_ref(XRegex *regex);

XLIB_AVAILABLE_IN_ALL
void x_regex_unref(XRegex *regex);

XLIB_AVAILABLE_IN_ALL
const xchar *x_regex_get_pattern(const XRegex *regex);

XLIB_AVAILABLE_IN_ALL
xint x_regex_get_max_backref(const XRegex *regex);

XLIB_AVAILABLE_IN_ALL
xint x_regex_get_capture_count(const XRegex *regex);

XLIB_AVAILABLE_IN_ALL
xboolean x_regex_get_has_cr_or_lf(const XRegex *regex);

XLIB_AVAILABLE_IN_2_38
xint x_regex_get_max_lookbehind(const XRegex *regex);

XLIB_AVAILABLE_IN_ALL
xint x_regex_get_string_number(const XRegex *regex, const xchar *name);

XLIB_AVAILABLE_IN_ALL
xchar *x_regex_escape_string(const xchar *string, xint length);

XLIB_AVAILABLE_IN_ALL
xchar *x_regex_escape_nul(const xchar *string, xint length);

XLIB_AVAILABLE_IN_ALL
XRegexCompileFlags x_regex_get_compile_flags(const XRegex *regex);

XLIB_AVAILABLE_IN_ALL
XRegexMatchFlags x_regex_get_match_flags(const XRegex *regex);

XLIB_AVAILABLE_IN_ALL
xboolean x_regex_match_simple(const xchar *pattern, const xchar *string, XRegexCompileFlags compile_options, XRegexMatchFlags match_options);

XLIB_AVAILABLE_IN_ALL
xboolean x_regex_match(const XRegex *regex, const xchar *string, XRegexMatchFlags match_options, XMatchInfo **match_info);

XLIB_AVAILABLE_IN_ALL
xboolean x_regex_match_full(const XRegex *regex, const xchar *string, xssize string_len, xint start_position, XRegexMatchFlags match_options, XMatchInfo **match_info, XError **error);

XLIB_AVAILABLE_IN_ALL
xboolean x_regex_match_all(const XRegex *regex, const xchar *string, XRegexMatchFlags match_options, XMatchInfo **match_info);

XLIB_AVAILABLE_IN_ALL
xboolean x_regex_match_all_full(const XRegex *regex, const xchar *string, xssize string_len, xint start_position, XRegexMatchFlags match_options, XMatchInfo **match_info, XError **error);

XLIB_AVAILABLE_IN_ALL
xchar **x_regex_split_simple(const xchar *pattern, const xchar *string, XRegexCompileFlags compile_options, XRegexMatchFlags match_options);

XLIB_AVAILABLE_IN_ALL
xchar **x_regex_split(const XRegex *regex, const xchar *string, XRegexMatchFlags match_options);

XLIB_AVAILABLE_IN_ALL
xchar **x_regex_split_full(const XRegex *regex, const xchar *string, xssize string_len, xint start_position, XRegexMatchFlags match_options, xint max_tokens, XError **error);

XLIB_AVAILABLE_IN_ALL
xchar *x_regex_replace(const XRegex *regex, const xchar *string, xssize string_len, xint start_position, const xchar *replacement, XRegexMatchFlags match_options, XError **error);

XLIB_AVAILABLE_IN_ALL
xchar *x_regex_replace_literal(const XRegex *regex, const xchar *string, xssize string_len, xint start_position, const xchar *replacement, XRegexMatchFlags match_options, XError **error);

XLIB_AVAILABLE_IN_ALL
xchar *x_regex_replace_eval(const XRegex *regex, const xchar *string, xssize string_len, xint start_position, XRegexMatchFlags match_options, XRegexEvalCallback eval, xpointer user_data, XError **error);

XLIB_AVAILABLE_IN_ALL
xboolean x_regex_check_replacement(const xchar *replacement, xboolean *has_references, XError **error);

XLIB_AVAILABLE_IN_ALL
XRegex *x_match_info_get_regex(const XMatchInfo *match_info);

XLIB_AVAILABLE_IN_ALL
const xchar *x_match_info_get_string(const XMatchInfo *match_info);

XLIB_AVAILABLE_IN_ALL
XMatchInfo *x_match_info_ref(XMatchInfo *match_info);

XLIB_AVAILABLE_IN_ALL
void x_match_info_unref(XMatchInfo *match_info);

XLIB_AVAILABLE_IN_ALL
void x_match_info_free(XMatchInfo *match_info);

XLIB_AVAILABLE_IN_ALL
xboolean x_match_info_next(XMatchInfo *match_info, XError **error);

XLIB_AVAILABLE_IN_ALL
xboolean x_match_info_matches(const XMatchInfo *match_info);

XLIB_AVAILABLE_IN_ALL
xint x_match_info_get_match_count(const XMatchInfo *match_info);

XLIB_AVAILABLE_IN_ALL
xboolean x_match_info_is_partial_match(const XMatchInfo *match_info);

XLIB_AVAILABLE_IN_ALL
xchar *x_match_info_expand_references(const XMatchInfo *match_info, const xchar *string_to_expand, XError **error);

XLIB_AVAILABLE_IN_ALL
xchar *x_match_info_fetch(const XMatchInfo *match_info, xint match_num);

XLIB_AVAILABLE_IN_ALL
xboolean x_match_info_fetch_pos(const XMatchInfo *match_info, xint match_num, xint *start_pos, xint *end_pos);

XLIB_AVAILABLE_IN_ALL
xchar *x_match_info_fetch_named(const XMatchInfo *match_info, const xchar *name);

XLIB_AVAILABLE_IN_ALL
xboolean x_match_info_fetch_named_pos(const XMatchInfo *match_info, const xchar *name, xint *start_pos, xint *end_pos);

XLIB_AVAILABLE_IN_ALL
xchar **x_match_info_fetch_all(const XMatchInfo *match_info);

X_END_DECLS

#endif
