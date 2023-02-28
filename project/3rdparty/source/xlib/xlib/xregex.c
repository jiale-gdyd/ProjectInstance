#include <stdint.h>
#include <string.h>

#include <xlib/xlib/config.h>
#include <xlib/xlib/xlist.h>
#include <xlib/xlib/xregex.h>
#include <xlib/xlib/xtypes.h>
#include <xlib/xlib/xatomic.h>
#include <xlib/xlib/xthread.h>
#include <xlib/xlib/xlibintl.h>
#include <xlib/xlib/xmessages.h>
#include <xlib/xlib/xstrfuncs.h>
#include <xlib/xlib/xtestutils.h>

#include "pcre2/pcre2.h"

#define X_REGEX_PCRE_GENERIC_MASK       (PCRE2_ANCHORED | PCRE2_NO_UTF_CHECK | PCRE2_ENDANCHORED)

// XRegexCompileFlags的所有可能值的掩码
#define X_REGEX_COMPILE_MASK            (X_REGEX_DEFAULT          | \
                                         X_REGEX_CASELESS         | \
                                         X_REGEX_MULTILINE        | \
                                         X_REGEX_DOTALL           | \
                                         X_REGEX_EXTENDED         | \
                                         X_REGEX_ANCHORED         | \
                                         X_REGEX_DOLLAR_ENDONLY   | \
                                         X_REGEX_UNGREEDY         | \
                                         X_REGEX_RAW              | \
                                         X_REGEX_NO_AUTO_CAPTURE  | \
                                         X_REGEX_OPTIMIZE         | \
                                         X_REGEX_FIRSTLINE        | \
                                         X_REGEX_DUPNAMES         | \
                                         X_REGEX_NEWLINE_CR       | \
                                         X_REGEX_NEWLINE_LF       | \
                                         X_REGEX_NEWLINE_CRLF     | \
                                         X_REGEX_NEWLINE_ANYCRLF  | \
                                         X_REGEX_BSR_ANYCRLF)

#define X_REGEX_PCRE2_COMPILE_MASK      (PCRE2_ALLOW_EMPTY_CLASS    | \
                                         PCRE2_ALT_BSUX             | \
                                         PCRE2_AUTO_CALLOUT         | \
                                         PCRE2_CASELESS             | \
                                         PCRE2_DOLLAR_ENDONLY       | \
                                         PCRE2_DOTALL               | \
                                         PCRE2_DUPNAMES             | \
                                         PCRE2_EXTENDED             | \
                                         PCRE2_FIRSTLINE            | \
                                         PCRE2_MATCH_UNSET_BACKREF  | \
                                         PCRE2_MULTILINE            | \
                                         PCRE2_NEVER_UCP            | \
                                         PCRE2_NEVER_UTF            | \
                                         PCRE2_NO_AUTO_CAPTURE      | \
                                         PCRE2_NO_AUTO_POSSESS      | \
                                         PCRE2_NO_DOTSTAR_ANCHOR    | \
                                         PCRE2_NO_START_OPTIMIZE    | \
                                         PCRE2_UCP                  | \
                                         PCRE2_UNGREEDY             | \
                                         PCRE2_UTF                  | \
                                         PCRE2_NEVER_BACKSLASH_C    | \
                                         PCRE2_ALT_CIRCUMFLEX       | \
                                         PCRE2_ALT_VERBNAMES        | \
                                         PCRE2_USE_OFFSET_LIMIT     | \
                                         PCRE2_EXTENDED_MORE        | \
                                         PCRE2_LITERAL              | \
                                         PCRE2_MATCH_INVALID_UTF    | \
                                         X_REGEX_PCRE_GENERIC_MASK)

#define X_REGEX_COMPILE_NONPCRE_MASK    (PCRE2_UTF)

#define X_REGEX_MATCH_MASK              (X_REGEX_MATCH_DEFAULT          | \
                                         X_REGEX_MATCH_ANCHORED         | \
                                         X_REGEX_MATCH_NOTBOL           | \
                                         X_REGEX_MATCH_NOTEOL           | \
                                         X_REGEX_MATCH_NOTEMPTY         | \
                                         X_REGEX_MATCH_PARTIAL          | \
                                         X_REGEX_MATCH_NEWLINE_CR       | \
                                         X_REGEX_MATCH_NEWLINE_LF       | \
                                         X_REGEX_MATCH_NEWLINE_CRLF     | \
                                         X_REGEX_MATCH_NEWLINE_ANY      | \
                                         X_REGEX_MATCH_NEWLINE_ANYCRLF  | \
                                         X_REGEX_MATCH_BSR_ANYCRLF      | \
                                         X_REGEX_MATCH_BSR_ANY          | \
                                         X_REGEX_MATCH_PARTIAL_SOFT     | \
                                         X_REGEX_MATCH_PARTIAL_HARD     | \
                                         X_REGEX_MATCH_NOTEMPTY_ATSTART)

#define X_REGEX_PCRE2_MATCH_MASK        (PCRE2_NOTBOL                      |\
                                         PCRE2_NOTEOL                      |\
                                         PCRE2_NOTEMPTY                    |\
                                         PCRE2_NOTEMPTY_ATSTART            |\
                                         PCRE2_PARTIAL_SOFT                |\
                                         PCRE2_PARTIAL_HARD                |\
                                         PCRE2_NO_JIT                      |\
                                         PCRE2_COPY_MATCHED_SUBJECT        |\
                                         X_REGEX_PCRE_GENERIC_MASK)

#define X_REGEX_NEWLINE_MASK            (PCRE2_NEWLINE_CR |     \
                                         PCRE2_NEWLINE_LF |     \
                                         PCRE2_NEWLINE_CRLF |   \
                                         PCRE2_NEWLINE_ANYCRLF)

#define X_REGEX_PCRE2_JIT_UNSUPPORTED_OPTIONS (PCRE2_ANCHORED | PCRE2_ENDANCHORED)

#define X_REGEX_COMPILE_NEWLINE_MASK    (X_REGEX_NEWLINE_CR      | \
                                         X_REGEX_NEWLINE_LF      | \
                                         X_REGEX_NEWLINE_CRLF    | \
                                         X_REGEX_NEWLINE_ANYCRLF)

#define X_REGEX_MATCH_NEWLINE_MASK      (X_REGEX_MATCH_NEWLINE_CR      | \
                                         X_REGEX_MATCH_NEWLINE_LF      | \
                                         X_REGEX_MATCH_NEWLINE_CRLF    | \
                                         X_REGEX_MATCH_NEWLINE_ANY    | \
                                         X_REGEX_MATCH_NEWLINE_ANYCRLF)

#define NEXT_CHAR(re, s)                (((re)->compile_opts & X_REGEX_RAW) ? ((s) + 1) : x_utf8_next_char(s))
#define PREV_CHAR(re, s)                (((re)->compile_opts & X_REGEX_RAW) ? ((s) - 1) : x_utf8_prev_char(s))

struct _XMatchInfo {
    xint                ref_count;
    XRegex              *regex;
    uint32_t            match_opts;
    xint                matches;
    uint32_t            n_subpatterns;
    xint                pos;
    uint32_t            n_offsets;
    xint                *offsets;
    xint                *workspace;
    PCRE2_SIZE          n_workspace;
    const xchar         *string;
    xssize              string_len;
    pcre2_match_context *match_context;
    pcre2_match_data    *match_data;
};

typedef enum {
    JIT_STATUS_DEFAULT,
    JIT_STATUS_ENABLED,
    JIT_STATUS_DISABLED
} JITStatus;

struct _XRegex {
    xint               ref_count;
    xchar              *pattern;
    pcre2_code         *pcre_re;
    uint32_t           compile_opts;
    XRegexCompileFlags orig_compile_opts;
    uint32_t           match_opts;
    XRegexMatchFlags   orig_match_opts;
    uint32_t           jit_options;
    JITStatus          jit_status;
};

#define IS_PCRE2_ERROR(ret)             ((ret) < PCRE2_ERROR_NOMATCH && (ret) != PCRE2_ERROR_PARTIAL)

typedef struct _InterpolationData InterpolationData;

static xboolean interpolation_list_needs_match(XList *list);
static xboolean interpolate_replacement(const XMatchInfo *match_info, XString *result, xpointer data);
static XList *split_replacement(const xchar *replacement, XError **error);
static void free_interpolation_data(InterpolationData *data);

static uint32_t get_pcre2_compile_options(XRegexCompileFlags compile_flags)
{
    uint32_t pcre2_flags = 0;

    if (compile_flags & X_REGEX_CASELESS) {
        pcre2_flags |= PCRE2_CASELESS;
    }

    if (compile_flags & X_REGEX_MULTILINE) {
        pcre2_flags |= PCRE2_MULTILINE;
    }

    if (compile_flags & X_REGEX_DOTALL) {
        pcre2_flags |= PCRE2_DOTALL;
    }

    if (compile_flags & X_REGEX_EXTENDED) {
        pcre2_flags |= PCRE2_EXTENDED;
    }

    if (compile_flags & X_REGEX_ANCHORED) {
        pcre2_flags |= PCRE2_ANCHORED;
    }

    if (compile_flags & X_REGEX_DOLLAR_ENDONLY) {
        pcre2_flags |= PCRE2_DOLLAR_ENDONLY;
    }

    if (compile_flags & X_REGEX_UNGREEDY) {
        pcre2_flags |= PCRE2_UNGREEDY;
    }

    if (!(compile_flags & X_REGEX_RAW)) {
        pcre2_flags |= PCRE2_UTF;
    }

    if (compile_flags & X_REGEX_NO_AUTO_CAPTURE) {
        pcre2_flags |= PCRE2_NO_AUTO_CAPTURE;
    }

    if (compile_flags & X_REGEX_FIRSTLINE) {
        pcre2_flags |= PCRE2_FIRSTLINE;
    }

    if (compile_flags & X_REGEX_DUPNAMES) {
        pcre2_flags |= PCRE2_DUPNAMES;
    }

    return pcre2_flags & X_REGEX_PCRE2_COMPILE_MASK;
}

static uint32_t get_pcre2_match_options(XRegexMatchFlags match_flags, XRegexCompileFlags compile_flags)
{
    uint32_t pcre2_flags = 0;

    if (match_flags & X_REGEX_MATCH_ANCHORED) {
        pcre2_flags |= PCRE2_ANCHORED;
    }

    if (match_flags & X_REGEX_MATCH_NOTBOL) {
        pcre2_flags |= PCRE2_NOTBOL;
    }

    if (match_flags & X_REGEX_MATCH_NOTEOL) {
        pcre2_flags |= PCRE2_NOTEOL;
    }

    if (match_flags & X_REGEX_MATCH_NOTEMPTY) {
        pcre2_flags |= PCRE2_NOTEMPTY;
    }

    if (match_flags & X_REGEX_MATCH_PARTIAL_SOFT) {
        pcre2_flags |= PCRE2_PARTIAL_SOFT;
    }

    if (match_flags & X_REGEX_MATCH_PARTIAL_HARD) {
        pcre2_flags |= PCRE2_PARTIAL_HARD;
    }

    if (match_flags & X_REGEX_MATCH_NOTEMPTY_ATSTART) {
        pcre2_flags |= PCRE2_NOTEMPTY_ATSTART;
    }

    if (compile_flags & X_REGEX_RAW) {
        pcre2_flags |= PCRE2_NO_UTF_CHECK;
    }

    return pcre2_flags & X_REGEX_PCRE2_MATCH_MASK;
}

static XRegexCompileFlags x_regex_compile_flags_from_pcre2(uint32_t pcre2_flags)
{
    XRegexCompileFlags compile_flags = (XRegexCompileFlags)X_REGEX_DEFAULT;

    if (pcre2_flags & PCRE2_CASELESS) {
        compile_flags = (XRegexCompileFlags)(compile_flags | X_REGEX_CASELESS);
    }

    if (pcre2_flags & PCRE2_MULTILINE) {
        compile_flags = (XRegexCompileFlags)(compile_flags | X_REGEX_MULTILINE);
    }

    if (pcre2_flags & PCRE2_DOTALL) {
        compile_flags = (XRegexCompileFlags)(compile_flags | X_REGEX_DOTALL);
    }

    if (pcre2_flags & PCRE2_EXTENDED) {
        compile_flags = (XRegexCompileFlags)(compile_flags | X_REGEX_EXTENDED);
    }

    if (pcre2_flags & PCRE2_ANCHORED) {
        compile_flags = (XRegexCompileFlags)(compile_flags | X_REGEX_ANCHORED);
    }

    if (pcre2_flags & PCRE2_DOLLAR_ENDONLY) {
        compile_flags = (XRegexCompileFlags)(compile_flags | X_REGEX_DOLLAR_ENDONLY);
    }

    if (pcre2_flags & PCRE2_UNGREEDY) {
        compile_flags = (XRegexCompileFlags)(compile_flags | X_REGEX_UNGREEDY);
    }

    if (!(pcre2_flags & PCRE2_UTF)) {
        compile_flags = (XRegexCompileFlags)(compile_flags | X_REGEX_RAW);
    }

    if (pcre2_flags & PCRE2_NO_AUTO_CAPTURE) {
        compile_flags = (XRegexCompileFlags)(compile_flags | X_REGEX_NO_AUTO_CAPTURE);
    }

    if (pcre2_flags & PCRE2_FIRSTLINE) {
        compile_flags = (XRegexCompileFlags)(compile_flags | X_REGEX_FIRSTLINE);
    }

    if (pcre2_flags & PCRE2_DUPNAMES) {
        compile_flags = (XRegexCompileFlags)(compile_flags | X_REGEX_DUPNAMES);
    }

    return (XRegexCompileFlags)(compile_flags & X_REGEX_COMPILE_MASK);
}

static XRegexMatchFlags x_regex_match_flags_from_pcre2(uint32_t pcre2_flags)
{
    XRegexMatchFlags match_flags = (XRegexMatchFlags)X_REGEX_MATCH_DEFAULT;

    if (pcre2_flags & PCRE2_ANCHORED) {
        match_flags = (XRegexMatchFlags)(match_flags | X_REGEX_MATCH_ANCHORED);
    }

    if (pcre2_flags & PCRE2_NOTBOL) {
        match_flags = (XRegexMatchFlags)(match_flags | X_REGEX_MATCH_NOTBOL);
    }

    if (pcre2_flags & PCRE2_NOTEOL) {
        match_flags = (XRegexMatchFlags)(match_flags | X_REGEX_MATCH_NOTEOL);
    }

    if (pcre2_flags & PCRE2_NOTEMPTY) {
        match_flags = (XRegexMatchFlags)(match_flags | X_REGEX_MATCH_NOTEMPTY);
    }

    if (pcre2_flags & PCRE2_PARTIAL_SOFT) {
        match_flags = (XRegexMatchFlags)(match_flags | X_REGEX_MATCH_PARTIAL_SOFT);
    }

    if (pcre2_flags & PCRE2_PARTIAL_HARD) {
        match_flags = (XRegexMatchFlags)(match_flags | X_REGEX_MATCH_PARTIAL_HARD);
    }

    if (pcre2_flags & PCRE2_NOTEMPTY_ATSTART) {
        match_flags = (XRegexMatchFlags)(match_flags | X_REGEX_MATCH_NOTEMPTY_ATSTART);
    }

    return (XRegexMatchFlags)(match_flags & X_REGEX_MATCH_MASK);
}

static uint32_t get_pcre2_newline_compile_options(XRegexCompileFlags compile_flags)
{
    compile_flags = (XRegexCompileFlags)(compile_flags & X_REGEX_COMPILE_NEWLINE_MASK);

    switch (compile_flags) {
        case X_REGEX_NEWLINE_CR:
            return PCRE2_NEWLINE_CR;

        case X_REGEX_NEWLINE_LF:
            return PCRE2_NEWLINE_LF;

        case X_REGEX_NEWLINE_CRLF:
            return PCRE2_NEWLINE_CRLF;

        case X_REGEX_NEWLINE_ANYCRLF:
            return PCRE2_NEWLINE_ANYCRLF;

        default:
            if (compile_flags != 0) {
                return 0;
            }
            return PCRE2_NEWLINE_ANY;
    }
}

static uint32_t get_pcre2_newline_match_options(XRegexMatchFlags match_flags)
{
    switch (match_flags & X_REGEX_MATCH_NEWLINE_MASK) {
        case X_REGEX_MATCH_NEWLINE_CR:
            return PCRE2_NEWLINE_CR;

        case X_REGEX_MATCH_NEWLINE_LF:
            return PCRE2_NEWLINE_LF;

        case X_REGEX_MATCH_NEWLINE_CRLF:
            return PCRE2_NEWLINE_CRLF;

        case X_REGEX_MATCH_NEWLINE_ANY:
            return PCRE2_NEWLINE_ANY;

        case X_REGEX_MATCH_NEWLINE_ANYCRLF:
            return PCRE2_NEWLINE_ANYCRLF;

        default:
            return 0;
    }
}

static uint32_t get_pcre2_bsr_compile_options(XRegexCompileFlags compile_flags)
{
    if (compile_flags & X_REGEX_BSR_ANYCRLF) {
        return PCRE2_BSR_ANYCRLF;
    }

    return PCRE2_BSR_UNICODE;
}

static uint32_t get_pcre2_bsr_match_options(XRegexMatchFlags match_flags)
{
    if (match_flags & X_REGEX_MATCH_BSR_ANYCRLF) {
        return PCRE2_BSR_ANYCRLF;
    }

    if (match_flags & X_REGEX_MATCH_BSR_ANY) {
        return PCRE2_BSR_UNICODE;
    }

    return 0;
}

static char *get_pcre2_error_string(int errcode)
{
    int err_length;
    PCRE2_UCHAR8 error_msg[2048];

    err_length = pcre2_get_error_message(errcode, error_msg, X_N_ELEMENTS(error_msg));
    if (err_length <= 0) {
        return NULL;
    }

    x_assert((size_t) err_length < X_N_ELEMENTS(error_msg));
    return x_memdup2(error_msg, err_length + 1);
}

static const xchar *translate_match_error(xint errcode)
{
    switch (errcode) {
        case PCRE2_ERROR_NOMATCH:
            break;

        case PCRE2_ERROR_NULL:
            x_critical("A NULL argument was passed to PCRE");
            break;

        case PCRE2_ERROR_BADOPTION:
            return "bad options";

        case PCRE2_ERROR_BADMAGIC:
            return _("corrupted object");

        case PCRE2_ERROR_NOMEMORY:
            return _("out of memory");

        case PCRE2_ERROR_NOSUBSTRING:
            break;

        case PCRE2_ERROR_MATCHLIMIT:
        case PCRE2_ERROR_JIT_STACKLIMIT:
            return _("backtracking limit reached");

        case PCRE2_ERROR_CALLOUT:
            break;

        case PCRE2_ERROR_BADUTFOFFSET:
            break;

        case PCRE2_ERROR_PARTIAL:
            break;

        case PCRE2_ERROR_INTERNAL:
            return _("internal error");

        case PCRE2_ERROR_DFA_UITEM:
            return _("the pattern contains items not supported for partial matching");

        case PCRE2_ERROR_DFA_UCOND:
            return _("back references as conditions are not supported for partial matching");

        case PCRE2_ERROR_DFA_WSSIZE:
            break;

        case PCRE2_ERROR_DFA_RECURSE:
        case PCRE2_ERROR_RECURSIONLIMIT:
            return _("recursion limit reached");

        case PCRE2_ERROR_BADOFFSET:
            return _("bad offset");

        case PCRE2_ERROR_RECURSELOOP:
            return _("recursion loop");

        case PCRE2_ERROR_JIT_BADOPTION:
            return _("matching mode is requested that was not compiled for JIT");

        default:
            break;
    }

    return NULL;
}

static char *get_match_error_message(int errcode)
{
    char *error_string;
    const char *msg = translate_match_error(errcode);
    if (msg) {
        return x_strdup(msg);
    }

    error_string = get_pcre2_error_string(errcode);
    if (error_string) {
        return error_string;
    }

    return x_strdup(_("unknown error"));
}

static void translate_compile_error(xint *errcode, const xchar **errmsg)
{
    xint original_errcode = *errcode;

    *errcode = -1;
    *errmsg = NULL;

    switch (original_errcode) {
        case PCRE2_ERROR_END_BACKSLASH:
            *errcode = X_REGEX_ERROR_STRAY_BACKSLASH;
            *errmsg = _("\\ at end of pattern");
            break;

        case PCRE2_ERROR_END_BACKSLASH_C:
            *errcode = X_REGEX_ERROR_MISSING_CONTROL_CHAR;
            *errmsg = _("\\c at end of pattern");
            break;

        case PCRE2_ERROR_UNKNOWN_ESCAPE:
        case PCRE2_ERROR_UNSUPPORTED_ESCAPE_SEQUENCE:
            *errcode = X_REGEX_ERROR_UNRECOGNIZED_ESCAPE;
            *errmsg = _("unrecognized character following \\");
            break;

        case PCRE2_ERROR_QUANTIFIER_OUT_OF_ORDER:
            *errcode = X_REGEX_ERROR_QUANTIFIERS_OUT_OF_ORDER;
            *errmsg = _("numbers out of order in {} quantifier");
            break;

        case PCRE2_ERROR_QUANTIFIER_TOO_BIG:
            *errcode = X_REGEX_ERROR_QUANTIFIER_TOO_BIG;
            *errmsg = _("number too big in {} quantifier");
            break;

        case PCRE2_ERROR_MISSING_SQUARE_BRACKET:
            *errcode = X_REGEX_ERROR_UNTERMINATED_CHARACTER_CLASS;
            *errmsg = _("missing terminating ] for character class");
            break;

        case PCRE2_ERROR_ESCAPE_INVALID_IN_CLASS:
            *errcode = X_REGEX_ERROR_INVALID_ESCAPE_IN_CHARACTER_CLASS;
            *errmsg = _("invalid escape sequence in character class");
            break;

        case PCRE2_ERROR_CLASS_RANGE_ORDER:
            *errcode = X_REGEX_ERROR_RANGE_OUT_OF_ORDER;
            *errmsg = _("range out of order in character class");
            break;

        case PCRE2_ERROR_QUANTIFIER_INVALID:
        case PCRE2_ERROR_INTERNAL_UNEXPECTED_REPEAT:
            *errcode = X_REGEX_ERROR_NOTHING_TO_REPEAT;
            *errmsg = _("nothing to repeat");
            break;

        case PCRE2_ERROR_INVALID_AFTER_PARENS_QUERY:
            *errcode = X_REGEX_ERROR_UNRECOGNIZED_CHARACTER;
            *errmsg = _("unrecognized character after (? or (?-");
            break;

        case PCRE2_ERROR_POSIX_CLASS_NOT_IN_CLASS:
            *errcode = X_REGEX_ERROR_POSIX_NAMED_CLASS_OUTSIDE_CLASS;
            *errmsg = _("POSIX named classes are supported only within a class");
            break;

        case PCRE2_ERROR_POSIX_NO_SUPPORT_COLLATING:
            *errcode = X_REGEX_ERROR_POSIX_COLLATING_ELEMENTS_NOT_SUPPORTED;
            *errmsg = _("POSIX collating elements are not supported");
            break;

        case PCRE2_ERROR_MISSING_CLOSING_PARENTHESIS:
        case PCRE2_ERROR_UNMATCHED_CLOSING_PARENTHESIS:
        case PCRE2_ERROR_PARENS_QUERY_R_MISSING_CLOSING:
            *errcode = X_REGEX_ERROR_UNMATCHED_PARENTHESIS;
            *errmsg = _("missing terminating )");
            break;

        case PCRE2_ERROR_BAD_SUBPATTERN_REFERENCE:
            *errcode = X_REGEX_ERROR_INEXISTENT_SUBPATTERN_REFERENCE;
            *errmsg = _("reference to non-existent subpattern");
            break;

        case PCRE2_ERROR_MISSING_COMMENT_CLOSING:
            *errcode = X_REGEX_ERROR_UNTERMINATED_COMMENT;
            *errmsg = _("missing ) after comment");
            break;

        case PCRE2_ERROR_PATTERN_TOO_LARGE:
            *errcode = X_REGEX_ERROR_EXPRESSION_TOO_LARGE;
            *errmsg = _("regular expression is too large");
            break;

        case PCRE2_ERROR_MISSING_CONDITION_CLOSING:
            *errcode = X_REGEX_ERROR_MALFORMED_CONDITION;
            *errmsg = _("malformed number or name after (?(");
            break;

        case PCRE2_ERROR_LOOKBEHIND_NOT_FIXED_LENGTH:
            *errcode = X_REGEX_ERROR_VARIABLE_LENGTH_LOOKBEHIND;
            *errmsg = _("lookbehind assertion is not fixed length");
            break;

        case PCRE2_ERROR_TOO_MANY_CONDITION_BRANCHES:
            *errcode = X_REGEX_ERROR_TOO_MANY_CONDITIONAL_BRANCHES;
            *errmsg = _("conditional group contains more than two branches");
            break;

        case PCRE2_ERROR_CONDITION_ASSERTION_EXPECTED:
            *errcode = X_REGEX_ERROR_ASSERTION_EXPECTED;
            *errmsg = _("assertion expected after (?(");
            break;

        case PCRE2_ERROR_BAD_RELATIVE_REFERENCE:
            *errcode = X_REGEX_ERROR_INVALID_RELATIVE_REFERENCE;
            *errmsg = _("a numbered reference must not be zero");
            break;

        case PCRE2_ERROR_UNKNOWN_POSIX_CLASS:
            *errcode = X_REGEX_ERROR_UNKNOWN_POSIX_CLASS_NAME;
            *errmsg = _("unknown POSIX class name");
            break;

        case PCRE2_ERROR_CODE_POINT_TOO_BIG:
        case PCRE2_ERROR_INVALID_HEXADECIMAL:
            *errcode = X_REGEX_ERROR_HEX_CODE_TOO_LARGE;
            *errmsg = _("character value in \\x{...} sequence is too large");
            break;

        case PCRE2_ERROR_LOOKBEHIND_INVALID_BACKSLASH_C:
            *errcode = X_REGEX_ERROR_SINGLE_BYTE_MATCH_IN_LOOKBEHIND;
            *errmsg = _("\\C not allowed in lookbehind assertion");
            break;

        case PCRE2_ERROR_MISSING_NAME_TERMINATOR:
            *errcode = X_REGEX_ERROR_MISSING_SUBPATTERN_NAME_TERMINATOR;
            *errmsg = _("missing terminator in subpattern name");
            break;

        case PCRE2_ERROR_DUPLICATE_SUBPATTERN_NAME:
            *errcode = X_REGEX_ERROR_DUPLICATE_SUBPATTERN_NAME;
            *errmsg = _("two named subpatterns have the same name");
            break;

        case PCRE2_ERROR_MALFORMED_UNICODE_PROPERTY:
            *errcode = X_REGEX_ERROR_MALFORMED_PROPERTY;
            *errmsg = _("malformed \\P or \\p sequence");
            break;

        case PCRE2_ERROR_UNKNOWN_UNICODE_PROPERTY:
            *errcode = X_REGEX_ERROR_UNKNOWN_PROPERTY;
            *errmsg = _("unknown property name after \\P or \\p");
            break;

        case PCRE2_ERROR_SUBPATTERN_NAME_TOO_LONG:
            *errcode = X_REGEX_ERROR_SUBPATTERN_NAME_TOO_LONG;
            *errmsg = _("subpattern name is too long (maximum 32 characters)");
            break;

        case PCRE2_ERROR_TOO_MANY_NAMED_SUBPATTERNS:
            *errcode = X_REGEX_ERROR_TOO_MANY_SUBPATTERNS;
            *errmsg = _("too many named subpatterns (maximum 10,000)");
            break;

        case PCRE2_ERROR_OCTAL_BYTE_TOO_BIG:
            *errcode = X_REGEX_ERROR_INVALID_OCTAL_VALUE;
            *errmsg = _("octal value is greater than \\377");
            break;

        case PCRE2_ERROR_DEFINE_TOO_MANY_BRANCHES:
            *errcode = X_REGEX_ERROR_TOO_MANY_BRANCHES_IN_DEFINE;
            *errmsg = _("DEFINE group contains more than one branch");
            break;

        case PCRE2_ERROR_INTERNAL_UNKNOWN_NEWLINE:
            *errcode = X_REGEX_ERROR_INCONSISTENT_NEWLINE_OPTIONS;
            *errmsg = _("inconsistent NEWLINE options");
            break;

        case PCRE2_ERROR_BACKSLASH_G_SYNTAX:
            *errcode = X_REGEX_ERROR_MISSING_BACK_REFERENCE;
            *errmsg = _("\\g is not followed by a braced, angle-bracketed, or quoted name or number, or by a plain number");
            break;

        case PCRE2_ERROR_VERB_ARGUMENT_NOT_ALLOWED:
            *errcode = X_REGEX_ERROR_BACKTRACKING_CONTROL_VERB_ARGUMENT_FORBIDDEN;
            *errmsg = _("an argument is not allowed for (*ACCEPT), (*FAIL), or (*COMMIT)");
            break;

        case PCRE2_ERROR_VERB_UNKNOWN:
            *errcode = X_REGEX_ERROR_UNKNOWN_BACKTRACKING_CONTROL_VERB;
            *errmsg = _("(*VERB) not recognized");
            break;

        case PCRE2_ERROR_SUBPATTERN_NUMBER_TOO_BIG:
            *errcode = X_REGEX_ERROR_NUMBER_TOO_BIG;
            *errmsg = _("number is too big");
            break;

        case PCRE2_ERROR_SUBPATTERN_NAME_EXPECTED:
            *errcode = X_REGEX_ERROR_MISSING_SUBPATTERN_NAME;
            *errmsg = _("missing subpattern name after (?&");
            break;

        case PCRE2_ERROR_SUBPATTERN_NAMES_MISMATCH:
            *errcode = X_REGEX_ERROR_EXTRA_SUBPATTERN_NAME;
            *errmsg = _("different names for subpatterns of the same number are not allowed");
            break;

        case PCRE2_ERROR_MARK_MISSING_ARGUMENT:
            *errcode = X_REGEX_ERROR_BACKTRACKING_CONTROL_VERB_ARGUMENT_REQUIRED;
            *errmsg = _("(*MARK) must have an argument");
            break;

        case PCRE2_ERROR_BACKSLASH_C_SYNTAX:
            *errcode = X_REGEX_ERROR_INVALID_CONTROL_CHAR;
            *errmsg = _( "\\c must be followed by an ASCII character");
            break;

        case PCRE2_ERROR_BACKSLASH_K_SYNTAX:
            *errcode = X_REGEX_ERROR_MISSING_NAME;
            *errmsg = _("\\k is not followed by a braced, angle-bracketed, or quoted name");
            break;

        case PCRE2_ERROR_BACKSLASH_N_IN_CLASS:
            *errcode = X_REGEX_ERROR_NOT_SUPPORTED_IN_CLASS;
            *errmsg = _("\\N is not supported in a class");
            break;

        case PCRE2_ERROR_VERB_NAME_TOO_LONG:
            *errcode = X_REGEX_ERROR_NAME_TOO_LONG;
            *errmsg = _("name is too long in (*MARK), (*PRUNE), (*SKIP), or (*THEN)");
            break;

        case PCRE2_ERROR_INTERNAL_CODE_OVERFLOW:
            *errcode = X_REGEX_ERROR_INTERNAL;
            *errmsg = _("code overflow");
            break;

        case PCRE2_ERROR_UNRECOGNIZED_AFTER_QUERY_P:
            *errcode = X_REGEX_ERROR_UNRECOGNIZED_CHARACTER;
            *errmsg = _("unrecognized character after (?P");
            break;

        case PCRE2_ERROR_INTERNAL_OVERRAN_WORKSPACE:
            *errcode = X_REGEX_ERROR_INTERNAL;
            *errmsg = _("overran compiling workspace");
            break;

        case PCRE2_ERROR_INTERNAL_MISSING_SUBPATTERN:
            *errcode = X_REGEX_ERROR_INTERNAL;
            *errmsg = _("previously-checked referenced subpattern not found");
            break;

        case PCRE2_ERROR_HEAP_FAILED:
        case PCRE2_ERROR_INTERNAL_PARSED_OVERFLOW:
        case PCRE2_ERROR_UNICODE_NOT_SUPPORTED:
        case PCRE2_ERROR_UNICODE_DISALLOWED_CODE_POINT:
        case PCRE2_ERROR_NO_SURROGATES_IN_UTF16:
        case PCRE2_ERROR_INTERNAL_BAD_CODE_LOOKBEHINDS:
        case PCRE2_ERROR_UNICODE_PROPERTIES_UNAVAILABLE:
        case PCRE2_ERROR_INTERNAL_STUDY_ERROR:
        case PCRE2_ERROR_UTF_IS_DISABLED:
        case PCRE2_ERROR_UCP_IS_DISABLED:
        case PCRE2_ERROR_INTERNAL_BAD_CODE_AUTO_POSSESS:
        case PCRE2_ERROR_BACKSLASH_C_LIBRARY_DISABLED:
        case PCRE2_ERROR_INTERNAL_BAD_CODE:
        case PCRE2_ERROR_INTERNAL_BAD_CODE_IN_SKIP:
            *errcode = X_REGEX_ERROR_INTERNAL;
            break;

        case PCRE2_ERROR_INVALID_SUBPATTERN_NAME:
        case PCRE2_ERROR_CLASS_INVALID_RANGE:
        case PCRE2_ERROR_ZERO_RELATIVE_REFERENCE:
        case PCRE2_ERROR_PARENTHESES_STACK_CHECK:
        case PCRE2_ERROR_LOOKBEHIND_TOO_COMPLICATED:
        case PCRE2_ERROR_CALLOUT_NUMBER_TOO_BIG:
        case PCRE2_ERROR_MISSING_CALLOUT_CLOSING:
        case PCRE2_ERROR_ESCAPE_INVALID_IN_VERB:
        case PCRE2_ERROR_NULL_PATTERN:
        case PCRE2_ERROR_BAD_OPTIONS:
        case PCRE2_ERROR_PARENTHESES_NEST_TOO_DEEP:
        case PCRE2_ERROR_BACKSLASH_O_MISSING_BRACE:
        case PCRE2_ERROR_INVALID_OCTAL:
        case PCRE2_ERROR_CALLOUT_STRING_TOO_LONG:
        case PCRE2_ERROR_BACKSLASH_U_CODE_POINT_TOO_BIG:
        case PCRE2_ERROR_MISSING_OCTAL_OR_HEX_DIGITS:
        case PCRE2_ERROR_VERSION_CONDITION_SYNTAX:
        case PCRE2_ERROR_CALLOUT_NO_STRING_DELIMITER:
        case PCRE2_ERROR_CALLOUT_BAD_STRING_DELIMITER:
        case PCRE2_ERROR_BACKSLASH_C_CALLER_DISABLED:
        case PCRE2_ERROR_QUERY_BARJX_NEST_TOO_DEEP:
        case PCRE2_ERROR_PATTERN_TOO_COMPLICATED:
        case PCRE2_ERROR_LOOKBEHIND_TOO_LONG:
        case PCRE2_ERROR_PATTERN_STRING_TOO_LONG:
        case PCRE2_ERROR_BAD_LITERAL_OPTIONS:
        default:
            *errcode = X_REGEX_ERROR_COMPILE;
            break;
    }

    x_assert(*errcode != -1);
}

static XMatchInfo *match_info_new(const XRegex *regex, const xchar *string, xint string_len, xint start_position, XRegexMatchFlags match_options, xboolean is_dfa)
{
    XMatchInfo *match_info;

    if (string_len < 0) {
        string_len = strlen(string);
    }

    match_info = x_new0(XMatchInfo, 1);
    match_info->ref_count = 1;
    match_info->regex = x_regex_ref((XRegex *)regex);
    match_info->string = string;
    match_info->string_len = string_len;
    match_info->matches = PCRE2_ERROR_NOMATCH;
    match_info->pos = start_position;
    match_info->match_opts = (XRegexMatchFlags)get_pcre2_match_options(match_options, regex->orig_compile_opts);;

    pcre2_pattern_info(regex->pcre_re, PCRE2_INFO_CAPTURECOUNT, &match_info->n_subpatterns);

    match_info->match_context = pcre2_match_context_create(NULL);

    if (is_dfa) {
        match_info->n_workspace = 100;
        match_info->workspace = x_new(xint, match_info->n_workspace);
    }

    match_info->n_offsets = 2;
    match_info->offsets = x_new0(xint, match_info->n_offsets);
    match_info->offsets[0] = -1;
    match_info->offsets[1] = -1;

    match_info->match_data = pcre2_match_data_create_from_pattern( match_info->regex->pcre_re, NULL);
    return match_info;
}

static xboolean recalc_match_offsets(XMatchInfo *match_info, XError **error)
{
    uint32_t i;
    PCRE2_SIZE *ovector;
    uint32_t pre_n_offset;
    uint32_t ovector_size = 0;

    x_assert(!IS_PCRE2_ERROR(match_info->matches));

    if (match_info->matches == PCRE2_ERROR_PARTIAL) {
        ovector_size = 1;
    } else if (match_info->matches > 0) {
        ovector_size = match_info->matches;
    }

    x_assert(ovector_size != 0);

    if (pcre2_get_ovector_count(match_info->match_data) < ovector_size) {
        x_set_error(error, X_REGEX_ERROR, X_REGEX_ERROR_MATCH, _("Error while matching regular expression %s: %s"), match_info->regex->pattern, _("code overflow"));
        return FALSE;
    }

    pre_n_offset = match_info->n_offsets;
    match_info->n_offsets = ovector_size * 2;
    ovector = pcre2_get_ovector_pointer(match_info->match_data);

    if (match_info->n_offsets != pre_n_offset) {
        match_info->offsets = (xint *)x_realloc_n(match_info->offsets, match_info->n_offsets, sizeof(xint));
    }

    for (i = 0; i < match_info->n_offsets; i++) {
        match_info->offsets[i] = (int) ovector[i];
    }

    return TRUE;
}

static JITStatus enable_jit_with_match_options(XRegex *regex, uint32_t match_options)
{
    xint retval;
    uint32_t old_jit_options, new_jit_options;

    if (!(regex->orig_compile_opts & X_REGEX_OPTIMIZE)) {
        return JIT_STATUS_DISABLED;
    }

    if (regex->jit_status == JIT_STATUS_DISABLED) {
        return JIT_STATUS_DISABLED;
    }

    old_jit_options = regex->jit_options;
    new_jit_options = old_jit_options | PCRE2_JIT_COMPLETE;
    if (match_options & PCRE2_PARTIAL_HARD) {
        new_jit_options |= PCRE2_JIT_PARTIAL_HARD;
    }

    if (match_options & PCRE2_PARTIAL_SOFT) {
        new_jit_options |= PCRE2_JIT_PARTIAL_SOFT;
    }

    if (new_jit_options == old_jit_options) {
        return regex->jit_status;
    }

    retval = pcre2_jit_compile(regex->pcre_re, new_jit_options);
    switch (retval) {
        case 0:
            regex->jit_options = new_jit_options;
            return JIT_STATUS_ENABLED;

        case PCRE2_ERROR_NOMEMORY:
            x_debug("JIT compilation was requested with X_REGEX_OPTIMIZE, "
                    "but JIT was unable to allocate executable memory for the "
                    "compiler. Falling back to interpretive code.");
            return JIT_STATUS_DISABLED;

        case PCRE2_ERROR_JIT_BADOPTION:
            x_debug("JIT compilation was requested with X_REGEX_OPTIMIZE, "
                    "but JIT support is not available. Falling back to "
                    "interpretive code.");
            return JIT_STATUS_DISABLED;

        default:
            x_debug("JIT compilation was requested with X_REGEX_OPTIMIZE, "
                    "but request for JIT support had unexpectedly failed (error %d). "
                    "Falling back to interpretive code.", retval);
            return JIT_STATUS_DISABLED;
    }

    x_assert_not_reached();
}

XRegex *x_match_info_get_regex(const XMatchInfo *match_info)
{
    x_return_val_if_fail(match_info != NULL, NULL);
    return match_info->regex;
}

const xchar *x_match_info_get_string(const XMatchInfo *match_info)
{
    x_return_val_if_fail(match_info != NULL, NULL);
    return match_info->string;
}

XMatchInfo *x_match_info_ref(XMatchInfo *match_info)
{
    x_return_val_if_fail(match_info != NULL, NULL);
    x_atomic_int_inc(&match_info->ref_count);
    return match_info;
}

void x_match_info_unref(XMatchInfo *match_info)
{
    if (x_atomic_int_dec_and_test(&match_info->ref_count)) {
        x_regex_unref(match_info->regex);
        if (match_info->match_context) {
            pcre2_match_context_free(match_info->match_context);
        }

        if (match_info->match_data) {
            pcre2_match_data_free(match_info->match_data);
        }

        x_free(match_info->offsets);
        x_free(match_info->workspace);
        x_free(match_info);
    }
}

void
x_match_info_free(XMatchInfo *match_info)
{
    if (match_info == NULL) {
        return;
    }

    x_match_info_unref(match_info);
}

xboolean x_match_info_next(XMatchInfo *match_info, XError **error)
{
    uint32_t opts;
    xint prev_match_end;
    JITStatus jit_status;
    xint prev_match_start;

    x_return_val_if_fail(match_info != NULL, FALSE);
    x_return_val_if_fail(error == NULL || *error == NULL, FALSE);
    x_return_val_if_fail(match_info->pos >= 0, FALSE);

    prev_match_start = match_info->offsets[0];
    prev_match_end = match_info->offsets[1];

    if (match_info->pos > match_info->string_len) {
        match_info->pos = -1;
        match_info->matches = PCRE2_ERROR_NOMATCH;
        return FALSE;
    }

    opts = match_info->regex->match_opts | match_info->match_opts;

    jit_status = enable_jit_with_match_options(match_info->regex, (XRegexMatchFlags)opts);
    if (jit_status == JIT_STATUS_ENABLED) {
        match_info->matches = pcre2_jit_match(match_info->regex->pcre_re, (PCRE2_SPTR8) match_info->string, match_info->string_len, match_info->pos, opts, match_info->match_data, match_info->match_context);
    } else {
        match_info->matches = pcre2_match(match_info->regex->pcre_re, (PCRE2_SPTR8) match_info->string, match_info->string_len, match_info->pos, opts, match_info->match_data, match_info->match_context);
    }

    if (IS_PCRE2_ERROR(match_info->matches)) {
        xchar *error_msg = get_match_error_message(match_info->matches);
        x_set_error(error, X_REGEX_ERROR, X_REGEX_ERROR_MATCH, _("Error while matching regular expression %s: %s"), match_info->regex->pattern, error_msg);
        x_clear_pointer(&error_msg, x_free);

        return FALSE;
    } else if (match_info->matches == 0){
        match_info->n_offsets *= 2;
        match_info->offsets = (xint *)x_realloc_n(match_info->offsets, match_info->n_offsets, sizeof(xint));

        pcre2_match_data_free(match_info->match_data);
        match_info->match_data = pcre2_match_data_create(match_info->n_offsets, NULL);

        return x_match_info_next(match_info, error);
    } else if (match_info->matches == PCRE2_ERROR_NOMATCH) {
        match_info->pos = -1;
        return FALSE;
    } else {
        if (!recalc_match_offsets(match_info, error)) {
            return FALSE;
        }
    }

    if (match_info->pos == match_info->offsets[1]) {
        if (match_info->pos > match_info->string_len) {
            match_info->pos = -1;
            match_info->matches = PCRE2_ERROR_NOMATCH;
            return FALSE;
        }

        match_info->pos = NEXT_CHAR(match_info->regex, &match_info->string[match_info->pos]) - match_info->string;
    } else {
        match_info->pos = match_info->offsets[1];
    }

    x_assert(match_info->matches < 0 || (uint32_t)match_info->matches <= match_info->n_subpatterns + 1);

    if (match_info->matches >= 0 && prev_match_start == match_info->offsets[0] && prev_match_end == match_info->offsets[1]) {
        return x_match_info_next(match_info, error);
    }

    return match_info->matches >= 0;
}

xboolean x_match_info_matches(const XMatchInfo *match_info)
{
    x_return_val_if_fail(match_info != NULL, FALSE);
    return match_info->matches >= 0;
}

xint x_match_info_get_match_count(const XMatchInfo *match_info)
{
    x_return_val_if_fail(match_info, -1);

    if (match_info->matches == PCRE2_ERROR_NOMATCH) {
        return 0;
    } else if (match_info->matches < PCRE2_ERROR_NOMATCH) {
        return -1;
    } else {
        return match_info->matches;
    }
}

xboolean x_match_info_is_partial_match(const XMatchInfo *match_info)
{
    x_return_val_if_fail(match_info != NULL, FALSE);
    return match_info->matches == PCRE2_ERROR_PARTIAL;
}

xchar *x_match_info_expand_references(const XMatchInfo *match_info, const xchar *string_to_expand, XError **error)
{
    XList *list;
    XString *result;
    XError *tmp_error = NULL;

    x_return_val_if_fail(string_to_expand != NULL, NULL);
    x_return_val_if_fail(error == NULL || *error == NULL, NULL);

    list = split_replacement(string_to_expand, &tmp_error);
    if (tmp_error != NULL) {
        x_propagate_error(error, tmp_error);
        return NULL;
    }

    if (!match_info && interpolation_list_needs_match(list)) {
        x_critical("String '%s' contains references to the match, can't expand references without XMatchInfo object", string_to_expand);
        return NULL;
    }

    result = x_string_sized_new(strlen(string_to_expand));
    interpolate_replacement(match_info, result, list);

    x_list_free_full(list, (XDestroyNotify)free_interpolation_data);

    return x_string_free(result, FALSE);
}

xchar *x_match_info_fetch(const XMatchInfo *match_info, xint match_num)
{
    xint start, end;
    xchar *match = NULL;

    x_return_val_if_fail(match_info != NULL, NULL);
    x_return_val_if_fail(match_num >= 0, NULL);

    if (!x_match_info_fetch_pos(match_info, match_num, &start, &end)) {
        match = NULL;
    } else if (start == -1) {
        match = x_strdup("");
    } else {
        match = x_strndup(&match_info->string[start], end - start);
    }

    return match;
}

xboolean x_match_info_fetch_pos(const XMatchInfo *match_info, xint match_num, xint *start_pos, xint *end_pos)
{
    x_return_val_if_fail(match_info != NULL, FALSE);
    x_return_val_if_fail(match_num >= 0, FALSE);

    if (match_info->matches < 0) {
        return FALSE;
    }

    if ((uint32_t)match_num >= MAX(match_info->n_subpatterns + 1, (uint32_t)match_info->matches)) {
        return FALSE;
    }

    if (start_pos != NULL) {
        *start_pos = (match_num < match_info->matches) ? match_info->offsets[2 * match_num] : -1;
    }

    if (end_pos != NULL) {
        *end_pos = (match_num < match_info->matches) ? match_info->offsets[2 * match_num + 1] : -1;
    }

    return TRUE;
}

static xint get_matched_substring_number(const XMatchInfo *match_info, const xchar *name)
{
    xuchar *entry;
    xint entrysize;
    PCRE2_SPTR first, last;

    if (!(match_info->regex->compile_opts & PCRE2_DUPNAMES)) {
        return pcre2_substring_number_from_name(match_info->regex->pcre_re, (PCRE2_SPTR8) name);
    }

    entrysize = pcre2_substring_nametable_scan(match_info->regex->pcre_re, (PCRE2_SPTR8)name, &first, &last);
    if (entrysize <= 0) {
        return entrysize;
    }

    for (entry = (xuchar *)first; entry <= (xuchar *)last; entry += entrysize) {
        xuint n = (entry[0] << 8) + entry[1];
        if (((n * 2) < match_info->n_offsets) && (match_info->offsets[n * 2] >= 0)) {
            return n;
        }
    }

    return (first[0] << 8) + first[1];
}

xchar *x_match_info_fetch_named(const XMatchInfo *match_info, const xchar *name)
{
    xint num;

    x_return_val_if_fail(match_info != NULL, NULL);
    x_return_val_if_fail(name != NULL, NULL);

    num = get_matched_substring_number(match_info, name);
    if (num < 0) {
        return NULL;
    } else {
        return x_match_info_fetch(match_info, num);
    }
}

xboolean x_match_info_fetch_named_pos(const XMatchInfo *match_info, const xchar *name, xint *start_pos, xint *end_pos)
{
    xint num;

    x_return_val_if_fail(match_info != NULL, FALSE);
    x_return_val_if_fail(name != NULL, FALSE);

    num = get_matched_substring_number(match_info, name);
    if (num < 0) {
        return FALSE;
    }

    return x_match_info_fetch_pos(match_info, num, start_pos, end_pos);
}

xchar **x_match_info_fetch_all(const XMatchInfo *match_info)
{
    xint i;
    xchar **result;

    x_return_val_if_fail(match_info != NULL, NULL);

    if (match_info->matches < 0) {
        return NULL;
    }

    result = x_new(xchar *, match_info->matches + 1);
    for (i = 0; i < match_info->matches; i++) {
        result[i] = x_match_info_fetch(match_info, i);
    }

    result[i] = NULL;

    return result;
}

X_DEFINE_QUARK(x-regex-error-quark, x_regex_error)

XRegex *x_regex_ref(XRegex *regex)
{
    x_return_val_if_fail(regex != NULL, NULL);
    x_atomic_int_inc(&regex->ref_count);
    return regex;
}

void x_regex_unref(XRegex *regex)
{
    x_return_if_fail(regex != NULL);

    if (x_atomic_int_dec_and_test(&regex->ref_count)) {
        x_free(regex->pattern);
        if (regex->pcre_re != NULL) {
            pcre2_code_free(regex->pcre_re);
        }
        x_free(regex);
    }
}

static pcre2_code *regex_compile(const xchar *pattern, uint32_t compile_options, uint32_t newline_options, uint32_t bsr_options, XError **error);
static uint32_t get_pcre2_inline_compile_options(pcre2_code *re, uint32_t compile_options);

XRegex *x_regex_new(const xchar *pattern, XRegexCompileFlags compile_options, XRegexMatchFlags match_options, XError **error)
{
    XRegex *regex;
    pcre2_code *re;
    uint32_t bsr_options;
    uint32_t newline_options;
    uint32_t pcre_match_options;
    static xsize initialised = 0;
    uint32_t pcre_compile_options;

    x_return_val_if_fail(pattern != NULL, NULL);
    x_return_val_if_fail(error == NULL || *error == NULL, NULL);
X_GNUC_BEGIN_IGNORE_DEPRECATIONS
    x_return_val_if_fail((compile_options & ~(X_REGEX_COMPILE_MASK | X_REGEX_JAVASCRIPT_COMPAT)) == 0, NULL);
X_GNUC_END_IGNORE_DEPRECATIONS
    x_return_val_if_fail((match_options & ~X_REGEX_MATCH_MASK) == 0, NULL);

    if (x_once_init_enter(&initialised)) {
        int supports_utf8;

        pcre2_config(PCRE2_CONFIG_UNICODE, &supports_utf8);
        if (!supports_utf8) {
            x_critical(_("PCRE library is compiled without UTF8 support"));
        }

        x_once_init_leave(&initialised, supports_utf8 ? 1 : 2);
    }

    if (X_UNLIKELY(initialised != 1)) {
        x_set_error_literal(error, X_REGEX_ERROR, X_REGEX_ERROR_COMPILE,  _("PCRE library is compiled with incompatible options"));
        return NULL;
    }

    pcre_compile_options = get_pcre2_compile_options(compile_options);
    pcre_match_options = get_pcre2_match_options(match_options, compile_options);

    newline_options = get_pcre2_newline_match_options(match_options);
    if (newline_options == 0) {
        newline_options = get_pcre2_newline_compile_options(compile_options);
    }

    if (newline_options == 0) {
        x_set_error(error, X_REGEX_ERROR, X_REGEX_ERROR_INCONSISTENT_NEWLINE_OPTIONS, "Invalid newline flags");
        return NULL;
    }

    bsr_options = get_pcre2_bsr_match_options(match_options);
    if (!bsr_options) {
        bsr_options = get_pcre2_bsr_compile_options(compile_options);
    }

    re = regex_compile(pattern, pcre_compile_options, newline_options, bsr_options, error);
    if (re == NULL) {
        return NULL;
    }

    pcre_compile_options |= get_pcre2_inline_compile_options(re, pcre_compile_options);

    regex = x_new0(XRegex, 1);
    regex->ref_count = 1;
    regex->pattern = x_strdup(pattern);
    regex->pcre_re = re;
    regex->compile_opts = pcre_compile_options;
    regex->orig_compile_opts = compile_options;
    regex->match_opts = pcre_match_options;
    regex->orig_match_opts = match_options;
    regex->jit_status = enable_jit_with_match_options(regex, regex->match_opts);

    return regex;
}

static pcre2_code *regex_compile(const xchar *pattern, uint32_t compile_options, uint32_t newline_options, uint32_t bsr_options, XError **error)
{
    xint errcode;
    pcre2_code *re;
    const xchar *errmsg;
    PCRE2_SIZE erroffset;
    pcre2_compile_context *context;

    context = pcre2_compile_context_create(NULL);

    if (pcre2_set_newline(context, newline_options) != 0) {
        x_set_error(error, X_REGEX_ERROR, X_REGEX_ERROR_INCONSISTENT_NEWLINE_OPTIONS, "Invalid newline flags");
        pcre2_compile_context_free(context);
        return NULL;
    }

    if (pcre2_set_bsr(context, bsr_options) != 0) {
        x_set_error(error, X_REGEX_ERROR, X_REGEX_ERROR_INCONSISTENT_NEWLINE_OPTIONS, "Invalid BSR flags");
        pcre2_compile_context_free(context);
        return NULL;
    }

    if (compile_options & PCRE2_UTF) {
        compile_options |= PCRE2_NO_UTF_CHECK;
    }

    compile_options = (XRegexCompileFlags)(compile_options | PCRE2_UCP);

    re = pcre2_compile((PCRE2_SPTR8)pattern, PCRE2_ZERO_TERMINATED, compile_options, &errcode, &erroffset, context);
    pcre2_compile_context_free(context);

    if (re == NULL) {
        XError *tmp_error;
        xchar *offset_str;
        int original_errcode;
        xchar *pcre2_errmsg = NULL;

        original_errcode = errcode;
        translate_compile_error(&errcode, &errmsg);

        if (!errmsg) {
            errmsg = _("unknown error");
            pcre2_errmsg = get_pcre2_error_string(original_errcode);
        }

        erroffset = x_utf8_pointer_to_offset(pattern, &pattern[erroffset]);

        offset_str = x_strdup_printf("%" X_XSIZE_FORMAT, erroffset);
        tmp_error = x_error_new(X_REGEX_ERROR, errcode, _("Error while compiling regular expression ‘%s’ " "at char %s: %s"), pattern, offset_str, pcre2_errmsg ? pcre2_errmsg : errmsg);
        x_propagate_error(error, tmp_error);
        x_free(offset_str);
        x_clear_pointer(&pcre2_errmsg, x_free);

        return NULL;
    }

    return re;
}

static uint32_t get_pcre2_inline_compile_options(pcre2_code *re, uint32_t compile_options)
{
    uint32_t pcre_compile_options;
    uint32_t nonpcre_compile_options;

    nonpcre_compile_options = compile_options & X_REGEX_COMPILE_NONPCRE_MASK;
    pcre2_pattern_info(re, PCRE2_INFO_ALLOPTIONS, &pcre_compile_options);

    compile_options = pcre_compile_options & X_REGEX_PCRE2_COMPILE_MASK;
    compile_options |= nonpcre_compile_options;

    if (!(compile_options & PCRE2_DUPNAMES)) {
        uint32_t jchanged = 0;
        pcre2_pattern_info(re, PCRE2_INFO_JCHANGED, &jchanged);
        if (jchanged) {
            compile_options |= PCRE2_DUPNAMES;
        }
    }

    return compile_options;
}

const xchar *x_regex_get_pattern(const XRegex *regex)
{
    x_return_val_if_fail(regex != NULL, NULL);
    return regex->pattern;
}

xint x_regex_get_max_backref(const XRegex *regex)
{
    uint32_t value;

    pcre2_pattern_info(regex->pcre_re, PCRE2_INFO_BACKREFMAX, &value);
    return value;
}

xint x_regex_get_capture_count(const XRegex *regex)
{
    uint32_t value;

    pcre2_pattern_info(regex->pcre_re, PCRE2_INFO_CAPTURECOUNT, &value);
    return value;
}

xboolean x_regex_get_has_cr_or_lf(const XRegex *regex)
{
    uint32_t value;

    pcre2_pattern_info(regex->pcre_re, PCRE2_INFO_HASCRORLF, &value);
    return !!value;
}

xint x_regex_get_max_lookbehind(const XRegex *regex)
{
    uint32_t max_lookbehind;

    pcre2_pattern_info(regex->pcre_re, PCRE2_INFO_MAXLOOKBEHIND, &max_lookbehind);
    return max_lookbehind;
}

XRegexCompileFlags x_regex_get_compile_flags(const XRegex *regex)
{
    uint32_t info_value;
    XRegexCompileFlags extra_flags;

    x_return_val_if_fail(regex != NULL, (XRegexCompileFlags)0);

    extra_flags = (XRegexCompileFlags)(regex->orig_compile_opts & X_REGEX_OPTIMIZE);

    pcre2_pattern_info(regex->pcre_re, PCRE2_INFO_NEWLINE, &info_value);
    switch (info_value) {
        case PCRE2_NEWLINE_ANYCRLF:
            extra_flags = (XRegexCompileFlags)(extra_flags | X_REGEX_NEWLINE_ANYCRLF);
            break;

        case PCRE2_NEWLINE_CRLF:
            extra_flags = (XRegexCompileFlags)(extra_flags | X_REGEX_NEWLINE_CRLF);
            break;

        case PCRE2_NEWLINE_LF:
            extra_flags = (XRegexCompileFlags)(extra_flags | X_REGEX_NEWLINE_LF);
            break;

        case PCRE2_NEWLINE_CR:
            extra_flags = (XRegexCompileFlags)(extra_flags | X_REGEX_NEWLINE_CR);
            break;

        default:
            break;
    }

    pcre2_pattern_info(regex->pcre_re, PCRE2_INFO_BSR, &info_value);
    switch (info_value) {
        case PCRE2_BSR_ANYCRLF:
            extra_flags = (XRegexCompileFlags)(extra_flags | X_REGEX_BSR_ANYCRLF);
            break;

        default:
            break;
    }

    return (XRegexCompileFlags)(x_regex_compile_flags_from_pcre2(regex->compile_opts) | extra_flags);
}

XRegexMatchFlags x_regex_get_match_flags(const XRegex *regex)
{
    uint32_t flags;

    flags = x_regex_match_flags_from_pcre2(regex->match_opts);
    flags |= (regex->orig_match_opts & X_REGEX_MATCH_NEWLINE_MASK);
    flags |= (regex->orig_match_opts & (X_REGEX_MATCH_BSR_ANY | X_REGEX_MATCH_BSR_ANYCRLF));

    return (XRegexMatchFlags)flags;
}

xboolean x_regex_match_simple(const xchar *pattern, const xchar *string, XRegexCompileFlags compile_options, XRegexMatchFlags match_options)
{
    XRegex *regex;
    xboolean result;

    regex = x_regex_new(pattern, compile_options, X_REGEX_MATCH_DEFAULT, NULL);
    if (!regex) {
        return FALSE;
    }

    result = x_regex_match_full(regex, string, -1, 0, match_options, NULL, NULL);
    x_regex_unref(regex);

    return result;
}

xboolean x_regex_match(const XRegex *regex, const xchar *string, XRegexMatchFlags match_options, XMatchInfo **match_info)
{
    return x_regex_match_full(regex, string, -1, 0, match_options, match_info, NULL);
}

xboolean x_regex_match_full(const XRegex *regex, const xchar *string, xssize string_len, xint start_position, XRegexMatchFlags match_options, XMatchInfo **match_info, XError **error)
{
    XMatchInfo *info;
    xboolean match_ok;

    x_return_val_if_fail(regex != NULL, FALSE);
    x_return_val_if_fail(string != NULL, FALSE);
    x_return_val_if_fail(start_position >= 0, FALSE);
    x_return_val_if_fail(error == NULL || *error == NULL, FALSE);
    x_return_val_if_fail((match_options & ~X_REGEX_MATCH_MASK) == 0, FALSE);

    info = match_info_new(regex, string, string_len, start_position, match_options, FALSE);
    match_ok = x_match_info_next(info, error);
    if (match_info != NULL) {
        *match_info = info;
    } else {
        x_match_info_free(info);
    }

    return match_ok;
}

xboolean x_regex_match_all(const XRegex *regex, const xchar *string, XRegexMatchFlags match_options, XMatchInfo **match_info)
{
    return x_regex_match_all_full(regex, string, -1, 0, match_options, match_info, NULL);
}

xboolean x_regex_match_all_full(const XRegex *regex, const xchar *string, xssize string_len, xint start_position, XRegexMatchFlags match_options, XMatchInfo **match_info, XError **error)
{
    xboolean done;
    xboolean retval;
    XMatchInfo *info;
    pcre2_code *pcre_re;
    uint32_t bsr_options;
    uint32_t newline_options;

    x_return_val_if_fail(regex != NULL, FALSE);
    x_return_val_if_fail(string != NULL, FALSE);
    x_return_val_if_fail(start_position >= 0, FALSE);
    x_return_val_if_fail(error == NULL || *error == NULL, FALSE);
    x_return_val_if_fail((match_options & ~X_REGEX_MATCH_MASK) == 0, FALSE);

    newline_options = get_pcre2_newline_match_options(match_options);
    if (!newline_options) {
        newline_options = get_pcre2_newline_compile_options(regex->orig_compile_opts);
    }

    bsr_options = get_pcre2_bsr_match_options(match_options);
    if (!bsr_options) {
        bsr_options = get_pcre2_bsr_compile_options(regex->orig_compile_opts);
    }

    pcre_re = regex_compile(regex->pattern, regex->compile_opts | PCRE2_NO_AUTO_POSSESS, newline_options, bsr_options, error);
    if (pcre_re == NULL) {
        return FALSE;
    }

    info = match_info_new(regex, string, string_len, start_position, match_options, TRUE);

    done = FALSE;
    while (!done) {
        done = TRUE;
        info->matches = pcre2_dfa_match(pcre_re, (PCRE2_SPTR8)info->string, info->string_len, info->pos, (regex->match_opts | info->match_opts), info->match_data, info->match_context, info->workspace, info->n_workspace);

        if (info->matches == PCRE2_ERROR_DFA_WSSIZE) {
            info->n_workspace *= 2;
            info->workspace = (xint *)x_realloc_n(info->workspace, info->n_workspace, sizeof (xint));
            done = FALSE;
        } else if (info->matches == 0) {
            info->n_offsets *= 2;
            info->offsets = (xint *)x_realloc_n(info->offsets, info->n_offsets, sizeof (xint));
            pcre2_match_data_free(info->match_data);
            info->match_data = pcre2_match_data_create(info->n_offsets, NULL);
            done = FALSE;
        } else if (IS_PCRE2_ERROR(info->matches)) {
            xchar *error_msg = get_match_error_message(info->matches);
            x_set_error(error, X_REGEX_ERROR, X_REGEX_ERROR_MATCH, _("Error while matching regular expression %s: %s"), regex->pattern, error_msg);
            x_clear_pointer(&error_msg, x_free);
        } else if (info->matches != PCRE2_ERROR_NOMATCH) {
            if (!recalc_match_offsets(info, error)) {
                info->matches = PCRE2_ERROR_NOMATCH;
            }
        }
    }

    pcre2_code_free(pcre_re);

    info->pos = -1;
    retval = info->matches >= 0;

    if (match_info != NULL) {
        *match_info = info;
    } else {
        x_match_info_free(info);
    }

    return retval;
}

xint x_regex_get_string_number(const XRegex *regex, const xchar  *name)
{
    xint num;

    x_return_val_if_fail(regex != NULL, -1);
    x_return_val_if_fail(name != NULL, -1);

    num = pcre2_substring_number_from_name(regex->pcre_re, (PCRE2_SPTR8)name);
    if (num == PCRE2_ERROR_NOSUBSTRING) {
        num = -1;
    }

    return num;
}

xchar **x_regex_split_simple(const xchar *pattern, const xchar *string, XRegexCompileFlags compile_options, XRegexMatchFlags  match_options)
{
    XRegex *regex;
    xchar **result;

    regex = x_regex_new(pattern, compile_options, (XRegexMatchFlags)0, NULL);
    if (!regex) {
        return NULL;
    }

    result = x_regex_split_full(regex, string, -1, 0, match_options, 0, NULL);
    x_regex_unref(regex);

    return result;
}

xchar **x_regex_split(const XRegex *regex, const xchar *string, XRegexMatchFlags match_options)
{
    return x_regex_split_full(regex, string, -1, 0, match_options, 0, NULL);
}

xchar **x_regex_split_full(const XRegex *regex, const xchar *string, xssize string_len, xint start_position, XRegexMatchFlags match_options, xint max_tokens, XError **error)
{
    xint i;
    xint token_count;
    xboolean match_ok;
    XList *list, *last;
    xchar **string_list;
    XMatchInfo *match_info;
    xint last_separator_end;
    XError *tmp_error = NULL;
    xboolean last_match_is_empty;

    x_return_val_if_fail(regex != NULL, NULL);
    x_return_val_if_fail(string != NULL, NULL);
    x_return_val_if_fail(start_position >= 0, NULL);
    x_return_val_if_fail(error == NULL || *error == NULL, NULL);
    x_return_val_if_fail((match_options & ~X_REGEX_MATCH_MASK) == 0, NULL);

    if (max_tokens <= 0) {
        max_tokens = X_MAXINT;
    }

    if (string_len < 0) {
        string_len = strlen(string);
    }

    if (string_len - start_position == 0) {
        return x_new0(xchar *, 1);
    }

    if (max_tokens == 1) {
        string_list = x_new0(xchar *, 2);
        string_list[0] = x_strndup(&string[start_position], string_len - start_position);
        return string_list;
    }

    list = NULL;
    token_count = 0;
    last_separator_end = start_position;
    last_match_is_empty = FALSE;

    match_ok = x_regex_match_full(regex, string, string_len, start_position, match_options, &match_info, &tmp_error);

    while (tmp_error == NULL) {
        if (match_ok) {
            last_match_is_empty = (match_info->offsets[0] == match_info->offsets[1]);
            if (last_separator_end != match_info->offsets[1]) {
                xchar *token;
                xint match_count;

                token = x_strndup(string + last_separator_end, match_info->offsets[0] - last_separator_end);
                list = x_list_prepend(list, token);
                token_count++;

                match_count = x_match_info_get_match_count(match_info);
                if (match_count > 1) {
                    for (i = 1; i < match_count; i++) {
                        list = x_list_prepend(list, x_match_info_fetch(match_info, i));
                    }
                }
            }
        } else {
            if (!last_match_is_empty) {
                xchar *token = x_strndup(string + last_separator_end, match_info->string_len - last_separator_end);
                list = x_list_prepend(list, token);
            }

            break;
        }

        if (token_count >= max_tokens - 1) {
            if (last_match_is_empty) {
                match_info->pos = PREV_CHAR(regex, &string[match_info->pos]) - string;
            }

            if (string_len > match_info->pos) {
                xchar *token = x_strndup(string + match_info->pos, string_len - match_info->pos);
                list = x_list_prepend(list, token);
            }

            break;
        }

        last_separator_end = match_info->pos;
        if (last_match_is_empty) {
            last_separator_end = PREV_CHAR(regex, &string[last_separator_end]) - string;
        }

        match_ok = x_match_info_next(match_info, &tmp_error);
    }

    x_match_info_free(match_info);
    if (tmp_error != NULL) {
        x_propagate_error(error, tmp_error);
        x_list_free_full(list, x_free);
        return NULL;
    }

    string_list = x_new(xchar *, x_list_length(list) + 1);
    i = 0;
    for (last = x_list_last(list); last; last = x_list_previous(last)) {
        string_list[i++] = (xchar *)last->data;
    }

    string_list[i] = NULL;
    x_list_free(list);

    return string_list;
}

enum {
    REPL_TYPE_STRING,
    REPL_TYPE_CHARACTER,
    REPL_TYPE_SYMBOLIC_REFERENCE,
    REPL_TYPE_NUMERIC_REFERENCE,
    REPL_TYPE_CHANGE_CASE
};

typedef enum {
    CHANGE_CASE_NONE         = 1 << 0,
    CHANGE_CASE_UPPER        = 1 << 1,
    CHANGE_CASE_LOWER        = 1 << 2,
    CHANGE_CASE_UPPER_SINGLE = 1 << 3,
    CHANGE_CASE_LOWER_SINGLE = 1 << 4,
    CHANGE_CASE_SINGLE_MASK  = CHANGE_CASE_UPPER_SINGLE | CHANGE_CASE_LOWER_SINGLE,
    CHANGE_CASE_LOWER_MASK   = CHANGE_CASE_LOWER | CHANGE_CASE_LOWER_SINGLE,
    CHANGE_CASE_UPPER_MASK   = CHANGE_CASE_UPPER | CHANGE_CASE_UPPER_SINGLE
} ChangeCase;

struct _InterpolationData {
    xchar     *text;
    xint       type;
    xint       num;
    xchar      c;
    ChangeCase change_case;
};

static void free_interpolation_data(InterpolationData *data)
{
    x_free(data->text);
    x_free(data);
}

static const xchar *expand_escape(const xchar *replacement, const xchar *p, InterpolationData *data, XError **error)
{
    xint base = 0;
    xint x, d, h, i;
    const xchar *q, *r;
    XError *tmp_error = NULL;
    const xchar *error_detail;

    p++;
    switch (*p) {
        case 't':
            p++;
            data->c = '\t';
            data->type = REPL_TYPE_CHARACTER;
            break;

        case 'n':
            p++;
            data->c = '\n';
            data->type = REPL_TYPE_CHARACTER;
            break;

        case 'v':
            p++;
            data->c = '\v';
            data->type = REPL_TYPE_CHARACTER;
            break;

        case 'r':
            p++;
            data->c = '\r';
            data->type = REPL_TYPE_CHARACTER;
            break;

        case 'f':
            p++;
            data->c = '\f';
            data->type = REPL_TYPE_CHARACTER;
            break;

        case 'a':
            p++;
            data->c = '\a';
            data->type = REPL_TYPE_CHARACTER;
            break;

        case 'b':
            p++;
            data->c = '\b';
            data->type = REPL_TYPE_CHARACTER;
            break;

        case '\\':
            p++;
            data->c = '\\';
            data->type = REPL_TYPE_CHARACTER;
            break;

        case 'x':
            p++;
            x = 0;
            if (*p == '{') {
                p++;
                do {
                    h = x_ascii_xdigit_value(*p);
                    if (h < 0) {
                        error_detail = _("hexadecimal digit or “}” expected");
                        goto error;
                    }

                    x = x * 16 + h;
                    p++;
                } while (*p != '}');
                p++;
            } else {
                for (i = 0; i < 2; i++) {
                    h = x_ascii_xdigit_value(*p);
                    if (h < 0) {
                        error_detail = _("hexadecimal digit expected");
                        goto error;
                    }

                    x = x * 16 + h;
                    p++;
                }
            }
            data->type = REPL_TYPE_STRING;
            data->text = x_new0(xchar, 8);
            x_unichar_to_utf8(x, data->text);
            break;

        case 'l':
            p++;
            data->type = REPL_TYPE_CHANGE_CASE;
            data->change_case = CHANGE_CASE_LOWER_SINGLE;
            break;

        case 'u':
            p++;
            data->type = REPL_TYPE_CHANGE_CASE;
            data->change_case = CHANGE_CASE_UPPER_SINGLE;
            break;

        case 'L':
            p++;
            data->type = REPL_TYPE_CHANGE_CASE;
            data->change_case = CHANGE_CASE_LOWER;
            break;

        case 'U':
            p++;
            data->type = REPL_TYPE_CHANGE_CASE;
            data->change_case = CHANGE_CASE_UPPER;
            break;

        case 'E':
            p++;
            data->type = REPL_TYPE_CHANGE_CASE;
            data->change_case = CHANGE_CASE_NONE;
            break;

        case 'g':
            p++;
            if (*p != '<') {
                error_detail = _("missing “<” in symbolic reference");
                goto error;
            }

            q = p + 1;
            do {
                p++;
                if (!*p) {
                    error_detail = _("unfinished symbolic reference");
                    goto error;
                }
            } while (*p != '>');

            if (p - q == 0) {
                error_detail = _("zero-length symbolic reference");
                goto error;
            }

            if (x_ascii_isdigit(*q)) {
                x = 0;
                do {
                    h = x_ascii_digit_value(*q);
                    if (h < 0) {
                        error_detail = _("digit expected");
                        p = q;
                        goto error;
                    }

                    x = x * 10 + h;
                    q++;
                } while (q != p);

                data->num = x;
                data->type = REPL_TYPE_NUMERIC_REFERENCE;
            } else {
                r = q;
                do {
                    if (!x_ascii_isalnum(*r)) {
                        error_detail = _("illegal symbolic reference");
                        p = r;
                        goto error;
                    }

                    r++;
                } while (r != p);

                data->text = x_strndup(q, p - q);
                data->type = REPL_TYPE_SYMBOLIC_REFERENCE;
            }
            p++;
            break;

        case '0':
            if (x_ascii_digit_value(*x_utf8_next_char(p)) >= 0) {
                base = 8;
                p = x_utf8_next_char(p);
            }
            X_GNUC_FALLTHROUGH;

        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            x = 0;
            d = 0;
            for (i = 0; i < 3; i++) {
                h = x_ascii_digit_value(*p);
                if (h < 0) {
                    break;
                }

                if (h > 7) {
                    if (base == 8) {
                        break;
                    } else {
                        base = 10;
                    }
                }

                if (i == 2 && base == 10) {
                    break;
                }

                x = x * 8 + h;
                d = d * 10 + h;
                p++;
            }

            if (base == 8 || i == 3) {
                data->type = REPL_TYPE_STRING;
                data->text = x_new0(xchar, 8);
                x_unichar_to_utf8(x, data->text);
            } else {
                data->type = REPL_TYPE_NUMERIC_REFERENCE;
                data->num = d;
            }
            break;

        case 0:
            error_detail = _("stray final “\\”");
            goto error;
            break;

        default:
            error_detail = _("unknown escape sequence");
            goto error;
    }

    return p;

error:
    tmp_error = x_error_new(X_REGEX_ERROR, X_REGEX_ERROR_REPLACE, _("Error while parsing replacement text “%s” at char %lu: %s"), replacement, (xulong)(p - replacement), error_detail);
    x_propagate_error(error, tmp_error);
    return NULL;
}

static XList *split_replacement(const xchar *replacement, XError **error)
{
    XList *list = NULL;
    const xchar *p, *start;
    InterpolationData *data;

    start = p = replacement;
    while (*p) {
        if (*p == '\\') {
            data = x_new0(InterpolationData, 1);
            start = p = expand_escape(replacement, p, data, error);
            if (p == NULL) {
                x_list_free_full(list, (XDestroyNotify)free_interpolation_data);
                free_interpolation_data(data);

                return NULL;
            }

            list = x_list_prepend(list, data);
        } else {
            p++;
            if (*p == '\\' || *p == '\0') {
                if (p - start > 0) {
                    data = x_new0(InterpolationData, 1);
                    data->text = x_strndup(start, p - start);
                    data->type = REPL_TYPE_STRING;
                    list = x_list_prepend(list, data);
                }
            }
        }
    }

    return x_list_reverse(list);
}

#define CHANGE_CASE(c, change_case)             (((change_case) & CHANGE_CASE_LOWER_MASK) ? x_unichar_tolower(c) : x_unichar_toupper(c))

static void string_append(XString *string, const xchar *text, ChangeCase *change_case)
{
    xunichar c;

    if (text[0] == '\0') {
        return;
    }

    if (*change_case == CHANGE_CASE_NONE) {
        x_string_append(string, text);
    } else if (*change_case & CHANGE_CASE_SINGLE_MASK) {
        c = x_utf8_get_char(text);
        x_string_append_unichar(string, CHANGE_CASE(c, *change_case));
        x_string_append(string, x_utf8_next_char(text));
        *change_case = CHANGE_CASE_NONE;
    } else {
        while (*text != '\0') {
            c = x_utf8_get_char(text);
            x_string_append_unichar(string, CHANGE_CASE(c, *change_case));
            text = x_utf8_next_char(text);
        }
    }
}

static xboolean interpolate_replacement(const XMatchInfo *match_info, XString *result, xpointer data)
{
    XList *list;
    xchar *match;
    InterpolationData *idata;
    ChangeCase change_case = CHANGE_CASE_NONE;

    for (list = (XList *)data; list; list = list->next) {
        idata = (InterpolationData *)list->data;

        switch (idata->type) {
            case REPL_TYPE_STRING:
                string_append(result, idata->text, &change_case);
                break;

            case REPL_TYPE_CHARACTER:
                x_string_append_c(result, CHANGE_CASE(idata->c, change_case));
                if (change_case & CHANGE_CASE_SINGLE_MASK) {
                    change_case = CHANGE_CASE_NONE;
                }
                break;

            case REPL_TYPE_NUMERIC_REFERENCE:
                match = x_match_info_fetch(match_info, idata->num);
                if (match) {
                    string_append(result, match, &change_case);
                    x_free(match);
                }
                break;

            case REPL_TYPE_SYMBOLIC_REFERENCE:
                match = x_match_info_fetch_named(match_info, idata->text);
                if (match) {
                    string_append(result, match, &change_case);
                    x_free(match);
                }
                break;

            case REPL_TYPE_CHANGE_CASE:
                change_case = idata->change_case;
                break;
         }
    }

    return FALSE;
}

static xboolean interpolation_list_needs_match(XList *list)
{
    while (list != NULL) {
        InterpolationData *data = (InterpolationData *)list->data;

        if (data->type == REPL_TYPE_SYMBOLIC_REFERENCE || data->type == REPL_TYPE_NUMERIC_REFERENCE) {
            return TRUE;
        }

        list = list->next;
    }

    return FALSE;
}

xchar *x_regex_replace(const XRegex *regex, const xchar *string, xssize string_len, xint start_position, const xchar *replacement, XRegexMatchFlags match_options, XError **error)
{
    XList *list;
    xchar *result;
    XError *tmp_error = NULL;

    x_return_val_if_fail(regex != NULL, NULL);
    x_return_val_if_fail(string != NULL, NULL);
    x_return_val_if_fail(start_position >= 0, NULL);
    x_return_val_if_fail(replacement != NULL, NULL);
    x_return_val_if_fail(error == NULL || *error == NULL, NULL);
    x_return_val_if_fail((match_options & ~X_REGEX_MATCH_MASK) == 0, NULL);

    list = split_replacement(replacement, &tmp_error);
    if (tmp_error != NULL) {
        x_propagate_error(error, tmp_error);
        return NULL;
    }

    result = x_regex_replace_eval(regex, string, string_len, start_position, match_options, interpolate_replacement, (xpointer)list, &tmp_error);
    if (tmp_error != NULL) {
        x_propagate_error(error, tmp_error);
    }
    x_list_free_full(list, (XDestroyNotify)free_interpolation_data);

    return result;
}

static xboolean literal_replacement(const XMatchInfo *match_info, XString *result, xpointer data)
{
    x_string_append(result, (const xchar *)data);
    return FALSE;
}

xchar *x_regex_replace_literal(const XRegex *regex, const xchar *string, xssize string_len, xint start_position, const xchar *replacement, XRegexMatchFlags match_options, XError **error)
{
    x_return_val_if_fail(replacement != NULL, NULL);
    x_return_val_if_fail((match_options & ~X_REGEX_MATCH_MASK) == 0, NULL);

    return x_regex_replace_eval(regex, string, string_len, start_position, match_options, literal_replacement, (xpointer)replacement, error);
}

xchar *x_regex_replace_eval(const XRegex *regex, const xchar *string, xssize string_len, xint start_position, XRegexMatchFlags match_options, XRegexEvalCallback eval, xpointer user_data, XError **error)
{
    XString *result;
    xint str_pos = 0;
    xboolean done = FALSE;
    XMatchInfo *match_info;
    XError *tmp_error = NULL;

    x_return_val_if_fail(regex != NULL, NULL);
    x_return_val_if_fail(string != NULL, NULL);
    x_return_val_if_fail(start_position >= 0, NULL);
    x_return_val_if_fail(eval != NULL, NULL);
    x_return_val_if_fail((match_options & ~X_REGEX_MATCH_MASK) == 0, NULL);

    if (string_len < 0) {
        string_len = strlen(string);
    }
    result = x_string_sized_new(string_len);

    x_regex_match_full(regex, string, string_len, start_position, match_options, &match_info, &tmp_error);
    while (!done && x_match_info_matches(match_info)) {
        x_string_append_len(result, string + str_pos, match_info->offsets[0] - str_pos);
        done = (*eval) (match_info, result, user_data);
        str_pos = match_info->offsets[1];
        x_match_info_next(match_info, &tmp_error);
    }

    x_match_info_free(match_info);
    if (tmp_error != NULL) {
        x_propagate_error(error, tmp_error);
        x_string_free(result, TRUE);
        return NULL;
    }

    x_string_append_len(result, string + str_pos, string_len - str_pos);
    return x_string_free(result, FALSE);
}

xboolean x_regex_check_replacement(const xchar *replacement, xboolean *has_references, XError **error)
{
    XList *list;
    XError *tmp = NULL;

    list = split_replacement(replacement, &tmp);
    if (tmp) {
        x_propagate_error(error, tmp);
        return FALSE;
    }

    if (has_references) {
        *has_references = interpolation_list_needs_match(list);
    }

    x_list_free_full(list, (XDestroyNotify)free_interpolation_data);
    return TRUE;
}

xchar *x_regex_escape_nul(const xchar *string, xint length)
{
    XString *escaped;
    xint backslashes;
    const xchar *p, *piece_start, *end;

    x_return_val_if_fail(string != NULL, NULL);

    if (length < 0) {
        return x_strdup(string);
    }

    end = string + length;
    p = piece_start = string;
    escaped = x_string_sized_new(length + 1);

    backslashes = 0;
    while (p < end) {
        switch (*p) {
            case '\0':
                if (p != piece_start) {
                    x_string_append_len(escaped, piece_start, p - piece_start);
                }

                if ((backslashes & 1) == 0) {
                    x_string_append_c(escaped, '\\');
                }

                x_string_append_c(escaped, 'x');
                x_string_append_c(escaped, '0');
                x_string_append_c(escaped, '0');
                piece_start = ++p;
                backslashes = 0;
                break;

            case '\\':
                backslashes++;
                ++p;
                break;

            default:
                backslashes = 0;
                p = x_utf8_next_char(p);
                break;
        }
    }

    if (piece_start < end) {
        x_string_append_len(escaped, piece_start, end - piece_start);
    }

    return x_string_free(escaped, FALSE);
}

xchar *x_regex_escape_string(const xchar *string, xint length)
{
    XString *escaped;
    const char *p, *piece_start, *end;

    x_return_val_if_fail(string != NULL, NULL);

    if (length < 0) {
        length = strlen(string);
    }

    end = string + length;
    p = piece_start = string;
    escaped = x_string_sized_new(length + 1);

    while (p < end) {
        switch (*p) {
            case '\0':
            case '\\':
            case '|':
            case '(':
            case ')':
            case '[':
            case ']':
            case '{':
            case '}':
            case '^':
            case '$':
            case '*':
            case '+':
            case '?':
            case '.':
                if (p != piece_start) {
                    x_string_append_len(escaped, piece_start, p - piece_start);
                }

                x_string_append_c(escaped, '\\');
                if (*p == '\0') {
                    x_string_append_c(escaped, '0');
                } else {
                    x_string_append_c(escaped, *p);
                }
                piece_start = ++p;
                break;

            default:
                p = x_utf8_next_char(p);
                break;
        }
    }

    if (piece_start < end) {
        x_string_append_len(escaped, piece_start, end - piece_start);
    }

    return x_string_free(escaped, FALSE);
}
