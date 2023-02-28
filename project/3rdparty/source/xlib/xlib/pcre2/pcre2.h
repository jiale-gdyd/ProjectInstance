#ifndef PCRE2_H_IDEMPOTENT_GUARD
#define PCRE2_H_IDEMPOTENT_GUARD

#define PCRE2_CODE_UNIT_WIDTH                                   8

#define PCRE2_MAJOR                                             10
#define PCRE2_MINOR                                             43
#define PCRE2_PRERELEASE
#define PCRE2_DATE                                              2023-01-15

#ifndef PCRE2_EXP_DECL
#ifdef __cplusplus
#define PCRE2_EXP_DECL                                          extern "C"
#else
#define PCRE2_EXP_DECL                                          extern
#endif
#endif

#ifndef PCRE2_CALL_CONVENTION
#define PCRE2_CALL_CONVENTION
#endif

#include <limits.h>
#include <stdlib.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PCRE2_ANCHORED                                          0x80000000u
#define PCRE2_NO_UTF_CHECK                                      0x40000000u
#define PCRE2_ENDANCHORED                                       0x20000000u

#define PCRE2_ALLOW_EMPTY_CLASS                                 0x00000001u
#define PCRE2_ALT_BSUX                                          0x00000002u
#define PCRE2_AUTO_CALLOUT                                      0x00000004u
#define PCRE2_CASELESS                                          0x00000008u
#define PCRE2_DOLLAR_ENDONLY                                    0x00000010u
#define PCRE2_DOTALL                                            0x00000020u
#define PCRE2_DUPNAMES                                          0x00000040u
#define PCRE2_EXTENDED                                          0x00000080u
#define PCRE2_FIRSTLINE                                         0x00000100u
#define PCRE2_MATCH_UNSET_BACKREF                               0x00000200u
#define PCRE2_MULTILINE                                         0x00000400u
#define PCRE2_NEVER_UCP                                         0x00000800u
#define PCRE2_NEVER_UTF                                         0x00001000u
#define PCRE2_NO_AUTO_CAPTURE                                   0x00002000u
#define PCRE2_NO_AUTO_POSSESS                                   0x00004000u
#define PCRE2_NO_DOTSTAR_ANCHOR                                 0x00008000u
#define PCRE2_NO_START_OPTIMIZE                                 0x00010000u
#define PCRE2_UCP                                               0x00020000u
#define PCRE2_UNGREEDY                                          0x00040000u
#define PCRE2_UTF                                               0x00080000u
#define PCRE2_NEVER_BACKSLASH_C                                 0x00100000u
#define PCRE2_ALT_CIRCUMFLEX                                    0x00200000u
#define PCRE2_ALT_VERBNAMES                                     0x00400000u
#define PCRE2_USE_OFFSET_LIMIT                                  0x00800000u
#define PCRE2_EXTENDED_MORE                                     0x01000000u
#define PCRE2_LITERAL                                           0x02000000u
#define PCRE2_MATCH_INVALID_UTF                                 0x04000000u

#define PCRE2_EXTRA_ALLOW_SURROGATE_ESCAPES                     0x00000001u
#define PCRE2_EXTRA_BAD_ESCAPE_IS_LITERAL                       0x00000002u
#define PCRE2_EXTRA_MATCH_WORD                                  0x00000004u
#define PCRE2_EXTRA_MATCH_LINE                                  0x00000008u
#define PCRE2_EXTRA_ESCAPED_CR_IS_LF                            0x00000010u
#define PCRE2_EXTRA_ALT_BSUX                                    0x00000020u
#define PCRE2_EXTRA_ALLOW_LOOKAROUND_BSK                        0x00000040u
#define PCRE2_EXTRA_CASELESS_RESTRICT                           0x00000080u
#define PCRE2_EXTRA_ASCII_BSD                                   0x00000100u
#define PCRE2_EXTRA_ASCII_BSS                                   0x00000200u
#define PCRE2_EXTRA_ASCII_BSW                                   0x00000400u
#define PCRE2_EXTRA_ASCII_POSIX                                 0x00000800u

#define PCRE2_JIT_COMPLETE                                      0x00000001u
#define PCRE2_JIT_PARTIAL_SOFT                                  0x00000002u
#define PCRE2_JIT_PARTIAL_HARD                                  0x00000004u
#define PCRE2_JIT_INVALID_UTF                                   0x00000100u

#define PCRE2_NOTBOL                                            0x00000001u
#define PCRE2_NOTEOL                                            0x00000002u
#define PCRE2_NOTEMPTY                                          0x00000004u
#define PCRE2_NOTEMPTY_ATSTART                                  0x00000008u
#define PCRE2_PARTIAL_SOFT                                      0x00000010u
#define PCRE2_PARTIAL_HARD                                      0x00000020u
#define PCRE2_DFA_RESTART                                       0x00000040u
#define PCRE2_DFA_SHORTEST                                      0x00000080u
#define PCRE2_SUBSTITUTE_GLOBAL                                 0x00000100u
#define PCRE2_SUBSTITUTE_EXTENDED                               0x00000200u
#define PCRE2_SUBSTITUTE_UNSET_EMPTY                            0x00000400u
#define PCRE2_SUBSTITUTE_UNKNOWN_UNSET                          0x00000800u
#define PCRE2_SUBSTITUTE_OVERFLOW_LENGTH                        0x00001000u
#define PCRE2_NO_JIT                                            0x00002000u
#define PCRE2_COPY_MATCHED_SUBJECT                              0x00004000u
#define PCRE2_SUBSTITUTE_LITERAL                                0x00008000u
#define PCRE2_SUBSTITUTE_MATCHED                                0x00010000u
#define PCRE2_SUBSTITUTE_REPLACEMENT_ONLY                       0x00020000u

#define PCRE2_CONVERT_UTF                                       0x00000001u
#define PCRE2_CONVERT_NO_UTF_CHECK                              0x00000002u
#define PCRE2_CONVERT_POSIX_BASIC                               0x00000004u
#define PCRE2_CONVERT_POSIX_EXTENDED                            0x00000008u
#define PCRE2_CONVERT_GLOB                                      0x00000010u
#define PCRE2_CONVERT_GLOB_NO_WILD_SEPARATOR                    0x00000030u
#define PCRE2_CONVERT_GLOB_NO_STARSTAR                          0x00000050u

#define PCRE2_NEWLINE_CR                                        1
#define PCRE2_NEWLINE_LF                                        2
#define PCRE2_NEWLINE_CRLF                                      3
#define PCRE2_NEWLINE_ANY                                       4
#define PCRE2_NEWLINE_ANYCRLF                                   5
#define PCRE2_NEWLINE_NUL                                       6

#define PCRE2_BSR_UNICODE                                       1
#define PCRE2_BSR_ANYCRLF                                       2

#define PCRE2_ERROR_END_BACKSLASH                               101
#define PCRE2_ERROR_END_BACKSLASH_C                             102
#define PCRE2_ERROR_UNKNOWN_ESCAPE                              103
#define PCRE2_ERROR_QUANTIFIER_OUT_OF_ORDER                     104
#define PCRE2_ERROR_QUANTIFIER_TOO_BIG                          105
#define PCRE2_ERROR_MISSING_SQUARE_BRACKET                      106
#define PCRE2_ERROR_ESCAPE_INVALID_IN_CLASS                     107
#define PCRE2_ERROR_CLASS_RANGE_ORDER                           108
#define PCRE2_ERROR_QUANTIFIER_INVALID                          109
#define PCRE2_ERROR_INTERNAL_UNEXPECTED_REPEAT                  110
#define PCRE2_ERROR_INVALID_AFTER_PARENS_QUERY                  111
#define PCRE2_ERROR_POSIX_CLASS_NOT_IN_CLASS                    112
#define PCRE2_ERROR_POSIX_NO_SUPPORT_COLLATING                  113
#define PCRE2_ERROR_MISSING_CLOSING_PARENTHESIS                 114
#define PCRE2_ERROR_BAD_SUBPATTERN_REFERENCE                    115
#define PCRE2_ERROR_NULL_PATTERN                                116
#define PCRE2_ERROR_BAD_OPTIONS                                 117
#define PCRE2_ERROR_MISSING_COMMENT_CLOSING                     118
#define PCRE2_ERROR_PARENTHESES_NEST_TOO_DEEP                   119
#define PCRE2_ERROR_PATTERN_TOO_LARGE                           120
#define PCRE2_ERROR_HEAP_FAILED                                 121
#define PCRE2_ERROR_UNMATCHED_CLOSING_PARENTHESIS               122
#define PCRE2_ERROR_INTERNAL_CODE_OVERFLOW                      123
#define PCRE2_ERROR_MISSING_CONDITION_CLOSING                   124
#define PCRE2_ERROR_LOOKBEHIND_NOT_FIXED_LENGTH                 125
#define PCRE2_ERROR_ZERO_RELATIVE_REFERENCE                     126
#define PCRE2_ERROR_TOO_MANY_CONDITION_BRANCHES                 127
#define PCRE2_ERROR_CONDITION_ASSERTION_EXPECTED                128
#define PCRE2_ERROR_BAD_RELATIVE_REFERENCE                      129
#define PCRE2_ERROR_UNKNOWN_POSIX_CLASS                         130
#define PCRE2_ERROR_INTERNAL_STUDY_ERROR                        131
#define PCRE2_ERROR_UNICODE_NOT_SUPPORTED                       132
#define PCRE2_ERROR_PARENTHESES_STACK_CHECK                     133
#define PCRE2_ERROR_CODE_POINT_TOO_BIG                          134
#define PCRE2_ERROR_LOOKBEHIND_TOO_COMPLICATED                  135
#define PCRE2_ERROR_LOOKBEHIND_INVALID_BACKSLASH_C              136
#define PCRE2_ERROR_UNSUPPORTED_ESCAPE_SEQUENCE                 137
#define PCRE2_ERROR_CALLOUT_NUMBER_TOO_BIG                      138
#define PCRE2_ERROR_MISSING_CALLOUT_CLOSING                     139
#define PCRE2_ERROR_ESCAPE_INVALID_IN_VERB                      140
#define PCRE2_ERROR_UNRECOGNIZED_AFTER_QUERY_P                  141
#define PCRE2_ERROR_MISSING_NAME_TERMINATOR                     142
#define PCRE2_ERROR_DUPLICATE_SUBPATTERN_NAME                   143
#define PCRE2_ERROR_INVALID_SUBPATTERN_NAME                     144
#define PCRE2_ERROR_UNICODE_PROPERTIES_UNAVAILABLE              145
#define PCRE2_ERROR_MALFORMED_UNICODE_PROPERTY                  146
#define PCRE2_ERROR_UNKNOWN_UNICODE_PROPERTY                    147
#define PCRE2_ERROR_SUBPATTERN_NAME_TOO_LONG                    148
#define PCRE2_ERROR_TOO_MANY_NAMED_SUBPATTERNS                  149
#define PCRE2_ERROR_CLASS_INVALID_RANGE                         150
#define PCRE2_ERROR_OCTAL_BYTE_TOO_BIG                          151
#define PCRE2_ERROR_INTERNAL_OVERRAN_WORKSPACE                  152
#define PCRE2_ERROR_INTERNAL_MISSING_SUBPATTERN                 153
#define PCRE2_ERROR_DEFINE_TOO_MANY_BRANCHES                    154
#define PCRE2_ERROR_BACKSLASH_O_MISSING_BRACE                   155
#define PCRE2_ERROR_INTERNAL_UNKNOWN_NEWLINE                    156
#define PCRE2_ERROR_BACKSLASH_G_SYNTAX                          157
#define PCRE2_ERROR_PARENS_QUERY_R_MISSING_CLOSING              158
#define PCRE2_ERROR_VERB_ARGUMENT_NOT_ALLOWED                   159
#define PCRE2_ERROR_VERB_UNKNOWN                                160
#define PCRE2_ERROR_SUBPATTERN_NUMBER_TOO_BIG                   161
#define PCRE2_ERROR_SUBPATTERN_NAME_EXPECTED                    162
#define PCRE2_ERROR_INTERNAL_PARSED_OVERFLOW                    163
#define PCRE2_ERROR_INVALID_OCTAL                               164
#define PCRE2_ERROR_SUBPATTERN_NAMES_MISMATCH                   165
#define PCRE2_ERROR_MARK_MISSING_ARGUMENT                       166
#define PCRE2_ERROR_INVALID_HEXADECIMAL                         167
#define PCRE2_ERROR_BACKSLASH_C_SYNTAX                          168
#define PCRE2_ERROR_BACKSLASH_K_SYNTAX                          169
#define PCRE2_ERROR_INTERNAL_BAD_CODE_LOOKBEHINDS               170
#define PCRE2_ERROR_BACKSLASH_N_IN_CLASS                        171
#define PCRE2_ERROR_CALLOUT_STRING_TOO_LONG                     172
#define PCRE2_ERROR_UNICODE_DISALLOWED_CODE_POINT               173
#define PCRE2_ERROR_UTF_IS_DISABLED                             174
#define PCRE2_ERROR_UCP_IS_DISABLED                             175
#define PCRE2_ERROR_VERB_NAME_TOO_LONG                          176
#define PCRE2_ERROR_BACKSLASH_U_CODE_POINT_TOO_BIG              177
#define PCRE2_ERROR_MISSING_OCTAL_OR_HEX_DIGITS                 178
#define PCRE2_ERROR_VERSION_CONDITION_SYNTAX                    179
#define PCRE2_ERROR_INTERNAL_BAD_CODE_AUTO_POSSESS              180
#define PCRE2_ERROR_CALLOUT_NO_STRING_DELIMITER                 181
#define PCRE2_ERROR_CALLOUT_BAD_STRING_DELIMITER                182
#define PCRE2_ERROR_BACKSLASH_C_CALLER_DISABLED                 183
#define PCRE2_ERROR_QUERY_BARJX_NEST_TOO_DEEP                   184
#define PCRE2_ERROR_BACKSLASH_C_LIBRARY_DISABLED                185
#define PCRE2_ERROR_PATTERN_TOO_COMPLICATED                     186
#define PCRE2_ERROR_LOOKBEHIND_TOO_LONG                         187
#define PCRE2_ERROR_PATTERN_STRING_TOO_LONG                     188
#define PCRE2_ERROR_INTERNAL_BAD_CODE                           189
#define PCRE2_ERROR_INTERNAL_BAD_CODE_IN_SKIP                   190
#define PCRE2_ERROR_NO_SURROGATES_IN_UTF16                      191
#define PCRE2_ERROR_BAD_LITERAL_OPTIONS                         192
#define PCRE2_ERROR_SUPPORTED_ONLY_IN_UNICODE                   193
#define PCRE2_ERROR_INVALID_HYPHEN_IN_OPTIONS                   194
#define PCRE2_ERROR_ALPHA_ASSERTION_UNKNOWN                     195
#define PCRE2_ERROR_SCRIPT_RUN_NOT_AVAILABLE                    196
#define PCRE2_ERROR_TOO_MANY_CAPTURES                           197
#define PCRE2_ERROR_CONDITION_ATOMIC_ASSERTION_EXPECTED         198
#define PCRE2_ERROR_BACKSLASH_K_IN_LOOKAROUND                   199

#define PCRE2_ERROR_NOMATCH                                     (-1)
#define PCRE2_ERROR_PARTIAL                                     (-2)
#define PCRE2_ERROR_UTF8_ERR1                                   (-3)
#define PCRE2_ERROR_UTF8_ERR2                                   (-4)
#define PCRE2_ERROR_UTF8_ERR3                                   (-5)
#define PCRE2_ERROR_UTF8_ERR4                                   (-6)
#define PCRE2_ERROR_UTF8_ERR5                                   (-7)
#define PCRE2_ERROR_UTF8_ERR6                                   (-8)
#define PCRE2_ERROR_UTF8_ERR7                                   (-9)
#define PCRE2_ERROR_UTF8_ERR8                                   (-10)
#define PCRE2_ERROR_UTF8_ERR9                                   (-11)
#define PCRE2_ERROR_UTF8_ERR10                                  (-12)
#define PCRE2_ERROR_UTF8_ERR11                                  (-13)
#define PCRE2_ERROR_UTF8_ERR12                                  (-14)
#define PCRE2_ERROR_UTF8_ERR13                                  (-15)
#define PCRE2_ERROR_UTF8_ERR14                                  (-16)
#define PCRE2_ERROR_UTF8_ERR15                                  (-17)
#define PCRE2_ERROR_UTF8_ERR16                                  (-18)
#define PCRE2_ERROR_UTF8_ERR17                                  (-19)
#define PCRE2_ERROR_UTF8_ERR18                                  (-20)
#define PCRE2_ERROR_UTF8_ERR19                                  (-21)
#define PCRE2_ERROR_UTF8_ERR20                                  (-22)
#define PCRE2_ERROR_UTF8_ERR21                                  (-23)
#define PCRE2_ERROR_UTF16_ERR1                                  (-24)
#define PCRE2_ERROR_UTF16_ERR2                                  (-25)
#define PCRE2_ERROR_UTF16_ERR3                                  (-26)
#define PCRE2_ERROR_UTF32_ERR1                                  (-27)
#define PCRE2_ERROR_UTF32_ERR2                                  (-28)
#define PCRE2_ERROR_BADDATA                                     (-29)
#define PCRE2_ERROR_MIXEDTABLES                                 (-30)
#define PCRE2_ERROR_BADMAGIC                                    (-31)
#define PCRE2_ERROR_BADMODE                                     (-32)
#define PCRE2_ERROR_BADOFFSET                                   (-33)
#define PCRE2_ERROR_BADOPTION                                   (-34)
#define PCRE2_ERROR_BADREPLACEMENT                              (-35)
#define PCRE2_ERROR_BADUTFOFFSET                                (-36)
#define PCRE2_ERROR_CALLOUT                                     (-37)
#define PCRE2_ERROR_DFA_BADRESTART                              (-38)
#define PCRE2_ERROR_DFA_RECURSE                                 (-39)
#define PCRE2_ERROR_DFA_UCOND                                   (-40)
#define PCRE2_ERROR_DFA_UFUNC                                   (-41)
#define PCRE2_ERROR_DFA_UITEM                                   (-42)
#define PCRE2_ERROR_DFA_WSSIZE                                  (-43)
#define PCRE2_ERROR_INTERNAL                                    (-44)
#define PCRE2_ERROR_JIT_BADOPTION                               (-45)
#define PCRE2_ERROR_JIT_STACKLIMIT                              (-46)
#define PCRE2_ERROR_MATCHLIMIT                                  (-47)
#define PCRE2_ERROR_NOMEMORY                                    (-48)
#define PCRE2_ERROR_NOSUBSTRING                                 (-49)
#define PCRE2_ERROR_NOUNIQUESUBSTRING                           (-50)
#define PCRE2_ERROR_NULL                                        (-51)
#define PCRE2_ERROR_RECURSELOOP                                 (-52)
#define PCRE2_ERROR_DEPTHLIMIT                                  (-53)
#define PCRE2_ERROR_RECURSIONLIMIT                              (-53)
#define PCRE2_ERROR_UNAVAILABLE                                 (-54)
#define PCRE2_ERROR_UNSET                                       (-55)
#define PCRE2_ERROR_BADOFFSETLIMIT                              (-56)
#define PCRE2_ERROR_BADREPESCAPE                                (-57)
#define PCRE2_ERROR_REPMISSINGBRACE                             (-58)
#define PCRE2_ERROR_BADSUBSTITUTION                             (-59)
#define PCRE2_ERROR_BADSUBSPATTERN                              (-60)
#define PCRE2_ERROR_TOOMANYREPLACE                              (-61)
#define PCRE2_ERROR_BADSERIALIZEDDATA                           (-62)
#define PCRE2_ERROR_HEAPLIMIT                                   (-63)
#define PCRE2_ERROR_CONVERT_SYNTAX                              (-64)
#define PCRE2_ERROR_INTERNAL_DUPMATCH                           (-65)
#define PCRE2_ERROR_DFA_UINVALID_UTF                            (-66)

#define PCRE2_INFO_ALLOPTIONS                                   0
#define PCRE2_INFO_ARGOPTIONS                                   1
#define PCRE2_INFO_BACKREFMAX                                   2
#define PCRE2_INFO_BSR                                          3
#define PCRE2_INFO_CAPTURECOUNT                                 4
#define PCRE2_INFO_FIRSTCODEUNIT                                5
#define PCRE2_INFO_FIRSTCODETYPE                                6
#define PCRE2_INFO_FIRSTBITMAP                                  7
#define PCRE2_INFO_HASCRORLF                                    8
#define PCRE2_INFO_JCHANGED                                     9
#define PCRE2_INFO_JITSIZE                                      10
#define PCRE2_INFO_LASTCODEUNIT                                 11
#define PCRE2_INFO_LASTCODETYPE                                 12
#define PCRE2_INFO_MATCHEMPTY                                   13
#define PCRE2_INFO_MATCHLIMIT                                   14
#define PCRE2_INFO_MAXLOOKBEHIND                                15
#define PCRE2_INFO_MINLENGTH                                    16
#define PCRE2_INFO_NAMECOUNT                                    17
#define PCRE2_INFO_NAMEENTRYSIZE                                18
#define PCRE2_INFO_NAMETABLE                                    19
#define PCRE2_INFO_NEWLINE                                      20
#define PCRE2_INFO_DEPTHLIMIT                                   21
#define PCRE2_INFO_RECURSIONLIMIT                               21
#define PCRE2_INFO_SIZE                                         22
#define PCRE2_INFO_HASBACKSLASHC                                23
#define PCRE2_INFO_FRAMESIZE                                    24
#define PCRE2_INFO_HEAPLIMIT                                    25
#define PCRE2_INFO_EXTRAOPTIONS                                 26

#define PCRE2_CONFIG_BSR                                        0
#define PCRE2_CONFIG_JIT                                        1
#define PCRE2_CONFIG_JITTARGET                                  2
#define PCRE2_CONFIG_LINKSIZE                                   3
#define PCRE2_CONFIG_MATCHLIMIT                                 4
#define PCRE2_CONFIG_NEWLINE                                    5
#define PCRE2_CONFIG_PARENSLIMIT                                6
#define PCRE2_CONFIG_DEPTHLIMIT                                 7
#define PCRE2_CONFIG_RECURSIONLIMIT                             7
#define PCRE2_CONFIG_STACKRECURSE                               8
#define PCRE2_CONFIG_UNICODE                                    9
#define PCRE2_CONFIG_UNICODE_VERSION                            10
#define PCRE2_CONFIG_VERSION                                    11
#define PCRE2_CONFIG_HEAPLIMIT                                  12
#define PCRE2_CONFIG_NEVER_BACKSLASH_C                          13
#define PCRE2_CONFIG_COMPILED_WIDTHS                            14
#define PCRE2_CONFIG_TABLES_LENGTH                              15

typedef uint8_t PCRE2_UCHAR8;
typedef uint16_t PCRE2_UCHAR16;
typedef uint32_t PCRE2_UCHAR32;

typedef const PCRE2_UCHAR8 *PCRE2_SPTR8;
typedef const PCRE2_UCHAR16 *PCRE2_SPTR16;
typedef const PCRE2_UCHAR32 *PCRE2_SPTR32;

#define PCRE2_SIZE                                              size_t
#define PCRE2_SIZE_MAX                                          SIZE_MAX
#define PCRE2_ZERO_TERMINATED                                   (~(PCRE2_SIZE)0)
#define PCRE2_UNSET                                             (~(PCRE2_SIZE)0)

#define PCRE2_TYPES_LIST                                            \
struct pcre2_real_general_context;                                  \
typedef struct pcre2_real_general_context pcre2_general_context;    \
                                                                    \
struct pcre2_real_compile_context;                                  \
typedef struct pcre2_real_compile_context pcre2_compile_context;    \
                                                                    \
struct pcre2_real_match_context;                                    \
typedef struct pcre2_real_match_context pcre2_match_context;        \
                                                                    \
struct pcre2_real_convert_context;                                  \
typedef struct pcre2_real_convert_context pcre2_convert_context;    \
                                                                    \
struct pcre2_real_code;                                             \
typedef struct pcre2_real_code pcre2_code;                          \
                                                                    \
struct pcre2_real_match_data;                                       \
typedef struct pcre2_real_match_data pcre2_match_data;              \
                                                                    \
struct pcre2_real_jit_stack;                                        \
typedef struct pcre2_real_jit_stack pcre2_jit_stack;                \
                                                                    \
typedef pcre2_jit_stack *(*pcre2_jit_callback)(void *);

#define PCRE2_CALLOUT_STARTMATCH                                0x00000001u
#define PCRE2_CALLOUT_BACKTRACK                                 0x00000002u

#define PCRE2_STRUCTURE_LIST                                        \
typedef struct pcre2_callout_block {                                \
    uint32_t   version;                                             \
    uint32_t   callout_number;                                      \
    uint32_t   capture_top;                                         \
    uint32_t   capture_last;                                        \
    PCRE2_SIZE *offset_vector;                                      \
    PCRE2_SPTR mark;                                                \
    PCRE2_SPTR subject;                                             \
    PCRE2_SIZE subject_length;                                      \
    PCRE2_SIZE start_match;                                         \
    PCRE2_SIZE current_position;                                    \
    PCRE2_SIZE pattern_position;                                    \
    PCRE2_SIZE next_item_length;                                    \
    PCRE2_SIZE callout_string_offset;                               \
    PCRE2_SIZE callout_string_length;                               \
    PCRE2_SPTR callout_string;                                      \
    uint32_t   callout_flags;                                       \
} pcre2_callout_block;                                              \
                                                                    \
typedef struct pcre2_callout_enumerate_block {                      \
    uint32_t   version;                                             \
    PCRE2_SIZE pattern_position;                                    \
    PCRE2_SIZE next_item_length;                                    \
    uint32_t   callout_number;                                      \
    PCRE2_SIZE callout_string_offset;                               \
    PCRE2_SIZE callout_string_length;                               \
    PCRE2_SPTR callout_string;                                      \
} pcre2_callout_enumerate_block;                                    \
                                                                    \
typedef struct pcre2_substitute_callout_block {                     \
    uint32_t   version;                                             \
    PCRE2_SPTR input;                                               \
    PCRE2_SPTR output;                                              \
    PCRE2_SIZE output_offsets[2];                                   \
    PCRE2_SIZE *ovector;                                            \
    uint32_t   oveccount;                                           \
    uint32_t   subscount;                                           \
} pcre2_substitute_callout_block;

#define PCRE2_GENERAL_INFO_FUNCTIONS                                \
    PCRE2_EXP_DECL int PCRE2_CALL_CONVENTION pcre2_config(uint32_t, void *);

#define PCRE2_GENERAL_CONTEXT_FUNCTIONS                             \
PCRE2_EXP_DECL pcre2_general_context *PCRE2_CALL_CONVENTION pcre2_general_context_copy(pcre2_general_context *); \
PCRE2_EXP_DECL pcre2_general_context *PCRE2_CALL_CONVENTION pcre2_general_context_create(void *(*)(size_t, void *), void (*)(void *, void *), void *); \
PCRE2_EXP_DECL void PCRE2_CALL_CONVENTION pcre2_general_context_free(pcre2_general_context *);

#define PCRE2_COMPILE_CONTEXT_FUNCTIONS                             \
PCRE2_EXP_DECL pcre2_compile_context *PCRE2_CALL_CONVENTION pcre2_compile_context_copy(pcre2_compile_context *); \
PCRE2_EXP_DECL pcre2_compile_context *PCRE2_CALL_CONVENTION pcre2_compile_context_create(pcre2_general_context *);\
PCRE2_EXP_DECL void PCRE2_CALL_CONVENTION pcre2_compile_context_free(pcre2_compile_context *);                      \
PCRE2_EXP_DECL int PCRE2_CALL_CONVENTION pcre2_set_bsr(pcre2_compile_context *, uint32_t);                          \
PCRE2_EXP_DECL int PCRE2_CALL_CONVENTION pcre2_set_character_tables(pcre2_compile_context *, const uint8_t *);      \
PCRE2_EXP_DECL int PCRE2_CALL_CONVENTION pcre2_set_compile_extra_options(pcre2_compile_context *, uint32_t);        \
PCRE2_EXP_DECL int PCRE2_CALL_CONVENTION pcre2_set_max_pattern_length(pcre2_compile_context *, PCRE2_SIZE);         \
PCRE2_EXP_DECL int PCRE2_CALL_CONVENTION pcre2_set_newline(pcre2_compile_context *, uint32_t);                      \
PCRE2_EXP_DECL int PCRE2_CALL_CONVENTION pcre2_set_parens_nest_limit(pcre2_compile_context *, uint32_t);            \
PCRE2_EXP_DECL int PCRE2_CALL_CONVENTION pcre2_set_compile_recursion_guard(pcre2_compile_context *, int (*)(uint32_t, void *), void *);

#define PCRE2_MATCH_CONTEXT_FUNCTIONS \
PCRE2_EXP_DECL pcre2_match_context *PCRE2_CALL_CONVENTION \
  pcre2_match_context_copy(pcre2_match_context *); \
PCRE2_EXP_DECL pcre2_match_context *PCRE2_CALL_CONVENTION \
  pcre2_match_context_create(pcre2_general_context *); \
PCRE2_EXP_DECL void PCRE2_CALL_CONVENTION \
  pcre2_match_context_free(pcre2_match_context *); \
PCRE2_EXP_DECL int PCRE2_CALL_CONVENTION \
  pcre2_set_callout(pcre2_match_context *, \
    int (*)(pcre2_callout_block *, void *), void *); \
PCRE2_EXP_DECL int PCRE2_CALL_CONVENTION \
  pcre2_set_substitute_callout(pcre2_match_context *, \
    int (*)(pcre2_substitute_callout_block *, void *), void *); \
PCRE2_EXP_DECL int PCRE2_CALL_CONVENTION \
  pcre2_set_depth_limit(pcre2_match_context *, uint32_t); \
PCRE2_EXP_DECL int PCRE2_CALL_CONVENTION \
  pcre2_set_heap_limit(pcre2_match_context *, uint32_t); \
PCRE2_EXP_DECL int PCRE2_CALL_CONVENTION \
  pcre2_set_match_limit(pcre2_match_context *, uint32_t); \
PCRE2_EXP_DECL int PCRE2_CALL_CONVENTION \
  pcre2_set_offset_limit(pcre2_match_context *, PCRE2_SIZE); \
PCRE2_EXP_DECL int PCRE2_CALL_CONVENTION \
  pcre2_set_recursion_limit(pcre2_match_context *, uint32_t); \
PCRE2_EXP_DECL int PCRE2_CALL_CONVENTION \
  pcre2_set_recursion_memory_management(pcre2_match_context *, \
    void *(*)(size_t, void *), void (*)(void *, void *), void *);

#define PCRE2_CONVERT_CONTEXT_FUNCTIONS \
PCRE2_EXP_DECL pcre2_convert_context *PCRE2_CALL_CONVENTION \
  pcre2_convert_context_copy(pcre2_convert_context *); \
PCRE2_EXP_DECL pcre2_convert_context *PCRE2_CALL_CONVENTION \
  pcre2_convert_context_create(pcre2_general_context *); \
PCRE2_EXP_DECL void PCRE2_CALL_CONVENTION \
  pcre2_convert_context_free(pcre2_convert_context *); \
PCRE2_EXP_DECL int PCRE2_CALL_CONVENTION \
  pcre2_set_glob_escape(pcre2_convert_context *, uint32_t); \
PCRE2_EXP_DECL int PCRE2_CALL_CONVENTION \
  pcre2_set_glob_separator(pcre2_convert_context *, uint32_t);

#define PCRE2_COMPILE_FUNCTIONS \
PCRE2_EXP_DECL pcre2_code *PCRE2_CALL_CONVENTION \
  pcre2_compile(PCRE2_SPTR, PCRE2_SIZE, uint32_t, int *, PCRE2_SIZE *, \
    pcre2_compile_context *); \
PCRE2_EXP_DECL void PCRE2_CALL_CONVENTION \
  pcre2_code_free(pcre2_code *); \
PCRE2_EXP_DECL pcre2_code *PCRE2_CALL_CONVENTION \
  pcre2_code_copy(const pcre2_code *); \
PCRE2_EXP_DECL pcre2_code *PCRE2_CALL_CONVENTION \
  pcre2_code_copy_with_tables(const pcre2_code *);

#define PCRE2_PATTERN_INFO_FUNCTIONS \
PCRE2_EXP_DECL int PCRE2_CALL_CONVENTION \
  pcre2_pattern_info(const pcre2_code *, uint32_t, void *); \
PCRE2_EXP_DECL int PCRE2_CALL_CONVENTION \
  pcre2_callout_enumerate(const pcre2_code *, \
    int (*)(pcre2_callout_enumerate_block *, void *), void *);

#define PCRE2_MATCH_FUNCTIONS \
PCRE2_EXP_DECL pcre2_match_data *PCRE2_CALL_CONVENTION \
  pcre2_match_data_create(uint32_t, pcre2_general_context *); \
PCRE2_EXP_DECL pcre2_match_data *PCRE2_CALL_CONVENTION \
  pcre2_match_data_create_from_pattern(const pcre2_code *, \
    pcre2_general_context *); \
PCRE2_EXP_DECL int PCRE2_CALL_CONVENTION \
  pcre2_dfa_match(const pcre2_code *, PCRE2_SPTR, PCRE2_SIZE, PCRE2_SIZE, \
    uint32_t, pcre2_match_data *, pcre2_match_context *, int *, PCRE2_SIZE); \
PCRE2_EXP_DECL int PCRE2_CALL_CONVENTION \
  pcre2_match(const pcre2_code *, PCRE2_SPTR, PCRE2_SIZE, PCRE2_SIZE, \
    uint32_t, pcre2_match_data *, pcre2_match_context *); \
PCRE2_EXP_DECL void PCRE2_CALL_CONVENTION \
  pcre2_match_data_free(pcre2_match_data *); \
PCRE2_EXP_DECL PCRE2_SPTR PCRE2_CALL_CONVENTION \
  pcre2_get_mark(pcre2_match_data *); \
PCRE2_EXP_DECL PCRE2_SIZE PCRE2_CALL_CONVENTION \
  pcre2_get_match_data_size(pcre2_match_data *); \
PCRE2_EXP_DECL PCRE2_SIZE PCRE2_CALL_CONVENTION \
  pcre2_get_match_data_heapframes_size(pcre2_match_data *); \
PCRE2_EXP_DECL uint32_t PCRE2_CALL_CONVENTION \
  pcre2_get_ovector_count(pcre2_match_data *); \
PCRE2_EXP_DECL PCRE2_SIZE *PCRE2_CALL_CONVENTION \
  pcre2_get_ovector_pointer(pcre2_match_data *); \
PCRE2_EXP_DECL PCRE2_SIZE PCRE2_CALL_CONVENTION \
  pcre2_get_startchar(pcre2_match_data *);

#define PCRE2_SUBSTRING_FUNCTIONS \
PCRE2_EXP_DECL int PCRE2_CALL_CONVENTION \
  pcre2_substring_copy_byname(pcre2_match_data *, PCRE2_SPTR, PCRE2_UCHAR *, \
    PCRE2_SIZE *); \
PCRE2_EXP_DECL int PCRE2_CALL_CONVENTION \
  pcre2_substring_copy_bynumber(pcre2_match_data *, uint32_t, PCRE2_UCHAR *, \
    PCRE2_SIZE *); \
PCRE2_EXP_DECL void PCRE2_CALL_CONVENTION \
  pcre2_substring_free(PCRE2_UCHAR *); \
PCRE2_EXP_DECL int PCRE2_CALL_CONVENTION \
  pcre2_substring_get_byname(pcre2_match_data *, PCRE2_SPTR, PCRE2_UCHAR **, \
    PCRE2_SIZE *); \
PCRE2_EXP_DECL int PCRE2_CALL_CONVENTION \
  pcre2_substring_get_bynumber(pcre2_match_data *, uint32_t, PCRE2_UCHAR **, \
    PCRE2_SIZE *); \
PCRE2_EXP_DECL int PCRE2_CALL_CONVENTION \
  pcre2_substring_length_byname(pcre2_match_data *, PCRE2_SPTR, PCRE2_SIZE *); \
PCRE2_EXP_DECL int PCRE2_CALL_CONVENTION \
  pcre2_substring_length_bynumber(pcre2_match_data *, uint32_t, PCRE2_SIZE *); \
PCRE2_EXP_DECL int PCRE2_CALL_CONVENTION \
  pcre2_substring_nametable_scan(const pcre2_code *, PCRE2_SPTR, PCRE2_SPTR *, \
    PCRE2_SPTR *); \
PCRE2_EXP_DECL int PCRE2_CALL_CONVENTION \
  pcre2_substring_number_from_name(const pcre2_code *, PCRE2_SPTR); \
PCRE2_EXP_DECL void PCRE2_CALL_CONVENTION \
  pcre2_substring_list_free(PCRE2_SPTR *); \
PCRE2_EXP_DECL int PCRE2_CALL_CONVENTION \
  pcre2_substring_list_get(pcre2_match_data *, PCRE2_UCHAR ***, PCRE2_SIZE **);

#define PCRE2_SERIALIZE_FUNCTIONS \
PCRE2_EXP_DECL int32_t PCRE2_CALL_CONVENTION \
  pcre2_serialize_encode(const pcre2_code **, int32_t, uint8_t **, \
    PCRE2_SIZE *, pcre2_general_context *); \
PCRE2_EXP_DECL int32_t PCRE2_CALL_CONVENTION \
  pcre2_serialize_decode(pcre2_code **, int32_t, const uint8_t *, \
    pcre2_general_context *); \
PCRE2_EXP_DECL int32_t PCRE2_CALL_CONVENTION \
  pcre2_serialize_get_number_of_codes(const uint8_t *); \
PCRE2_EXP_DECL void PCRE2_CALL_CONVENTION \
  pcre2_serialize_free(uint8_t *);

#define PCRE2_SUBSTITUTE_FUNCTION \
PCRE2_EXP_DECL int PCRE2_CALL_CONVENTION \
  pcre2_substitute(const pcre2_code *, PCRE2_SPTR, PCRE2_SIZE, PCRE2_SIZE, \
    uint32_t, pcre2_match_data *, pcre2_match_context *, PCRE2_SPTR, \
    PCRE2_SIZE, PCRE2_UCHAR *, PCRE2_SIZE *);

#define PCRE2_CONVERT_FUNCTIONS \
PCRE2_EXP_DECL int PCRE2_CALL_CONVENTION \
  pcre2_pattern_convert(PCRE2_SPTR, PCRE2_SIZE, uint32_t, PCRE2_UCHAR **, \
    PCRE2_SIZE *, pcre2_convert_context *); \
PCRE2_EXP_DECL void PCRE2_CALL_CONVENTION \
  pcre2_converted_pattern_free(PCRE2_UCHAR *);

#define PCRE2_JIT_FUNCTIONS \
PCRE2_EXP_DECL int PCRE2_CALL_CONVENTION \
  pcre2_jit_compile(pcre2_code *, uint32_t); \
PCRE2_EXP_DECL int PCRE2_CALL_CONVENTION \
  pcre2_jit_match(const pcre2_code *, PCRE2_SPTR, PCRE2_SIZE, PCRE2_SIZE, \
    uint32_t, pcre2_match_data *, pcre2_match_context *); \
PCRE2_EXP_DECL void PCRE2_CALL_CONVENTION \
  pcre2_jit_free_unused_memory(pcre2_general_context *); \
PCRE2_EXP_DECL pcre2_jit_stack *PCRE2_CALL_CONVENTION \
  pcre2_jit_stack_create(size_t, size_t, pcre2_general_context *); \
PCRE2_EXP_DECL void PCRE2_CALL_CONVENTION \
  pcre2_jit_stack_assign(pcre2_match_context *, pcre2_jit_callback, void *); \
PCRE2_EXP_DECL void PCRE2_CALL_CONVENTION \
  pcre2_jit_stack_free(pcre2_jit_stack *);

#define PCRE2_OTHER_FUNCTIONS \
PCRE2_EXP_DECL int PCRE2_CALL_CONVENTION \
  pcre2_get_error_message(int, PCRE2_UCHAR *, PCRE2_SIZE); \
PCRE2_EXP_DECL const uint8_t *PCRE2_CALL_CONVENTION \
  pcre2_maketables(pcre2_general_context *); \
PCRE2_EXP_DECL void PCRE2_CALL_CONVENTION \
  pcre2_maketables_free(pcre2_general_context *, const uint8_t *);

#define PCRE2_JOIN(a,b) a ## b
#define PCRE2_GLUE(a,b) PCRE2_JOIN(a,b)
#define PCRE2_SUFFIX(a) PCRE2_GLUE(a,PCRE2_LOCAL_WIDTH)

#define PCRE2_UCHAR                 PCRE2_SUFFIX(PCRE2_UCHAR)
#define PCRE2_SPTR                  PCRE2_SUFFIX(PCRE2_SPTR)

#define pcre2_code                  PCRE2_SUFFIX(pcre2_code_)
#define pcre2_jit_callback          PCRE2_SUFFIX(pcre2_jit_callback_)
#define pcre2_jit_stack             PCRE2_SUFFIX(pcre2_jit_stack_)

#define pcre2_real_code             PCRE2_SUFFIX(pcre2_real_code_)
#define pcre2_real_general_context  PCRE2_SUFFIX(pcre2_real_general_context_)
#define pcre2_real_compile_context  PCRE2_SUFFIX(pcre2_real_compile_context_)
#define pcre2_real_convert_context  PCRE2_SUFFIX(pcre2_real_convert_context_)
#define pcre2_real_match_context    PCRE2_SUFFIX(pcre2_real_match_context_)
#define pcre2_real_jit_stack        PCRE2_SUFFIX(pcre2_real_jit_stack_)
#define pcre2_real_match_data       PCRE2_SUFFIX(pcre2_real_match_data_)

#define pcre2_callout_block            PCRE2_SUFFIX(pcre2_callout_block_)
#define pcre2_callout_enumerate_block  PCRE2_SUFFIX(pcre2_callout_enumerate_block_)
#define pcre2_substitute_callout_block PCRE2_SUFFIX(pcre2_substitute_callout_block_)
#define pcre2_general_context          PCRE2_SUFFIX(pcre2_general_context_)
#define pcre2_compile_context          PCRE2_SUFFIX(pcre2_compile_context_)
#define pcre2_convert_context          PCRE2_SUFFIX(pcre2_convert_context_)
#define pcre2_match_context            PCRE2_SUFFIX(pcre2_match_context_)
#define pcre2_match_data               PCRE2_SUFFIX(pcre2_match_data_)

#define pcre2_callout_enumerate               PCRE2_SUFFIX(pcre2_callout_enumerate_)
#define pcre2_code_copy                       PCRE2_SUFFIX(pcre2_code_copy_)
#define pcre2_code_copy_with_tables           PCRE2_SUFFIX(pcre2_code_copy_with_tables_)
#define pcre2_code_free                       PCRE2_SUFFIX(pcre2_code_free_)
#define pcre2_compile                         PCRE2_SUFFIX(pcre2_compile_)
#define pcre2_compile_context_copy            PCRE2_SUFFIX(pcre2_compile_context_copy_)
#define pcre2_compile_context_create          PCRE2_SUFFIX(pcre2_compile_context_create_)
#define pcre2_compile_context_free            PCRE2_SUFFIX(pcre2_compile_context_free_)
#define pcre2_config                          PCRE2_SUFFIX(pcre2_config_)
#define pcre2_convert_context_copy            PCRE2_SUFFIX(pcre2_convert_context_copy_)
#define pcre2_convert_context_create          PCRE2_SUFFIX(pcre2_convert_context_create_)
#define pcre2_convert_context_free            PCRE2_SUFFIX(pcre2_convert_context_free_)
#define pcre2_converted_pattern_free          PCRE2_SUFFIX(pcre2_converted_pattern_free_)
#define pcre2_dfa_match                       PCRE2_SUFFIX(pcre2_dfa_match_)
#define pcre2_general_context_copy            PCRE2_SUFFIX(pcre2_general_context_copy_)
#define pcre2_general_context_create          PCRE2_SUFFIX(pcre2_general_context_create_)
#define pcre2_general_context_free            PCRE2_SUFFIX(pcre2_general_context_free_)
#define pcre2_get_error_message               PCRE2_SUFFIX(pcre2_get_error_message_)
#define pcre2_get_mark                        PCRE2_SUFFIX(pcre2_get_mark_)
#define pcre2_get_match_data_heapframes_size  PCRE2_SUFFIX(pcre2_get_match_data_heapframes_size_)
#define pcre2_get_match_data_size             PCRE2_SUFFIX(pcre2_get_match_data_size_)
#define pcre2_get_ovector_pointer             PCRE2_SUFFIX(pcre2_get_ovector_pointer_)
#define pcre2_get_ovector_count               PCRE2_SUFFIX(pcre2_get_ovector_count_)
#define pcre2_get_startchar                   PCRE2_SUFFIX(pcre2_get_startchar_)
#define pcre2_jit_compile                     PCRE2_SUFFIX(pcre2_jit_compile_)
#define pcre2_jit_match                       PCRE2_SUFFIX(pcre2_jit_match_)
#define pcre2_jit_free_unused_memory          PCRE2_SUFFIX(pcre2_jit_free_unused_memory_)
#define pcre2_jit_stack_assign                PCRE2_SUFFIX(pcre2_jit_stack_assign_)
#define pcre2_jit_stack_create                PCRE2_SUFFIX(pcre2_jit_stack_create_)
#define pcre2_jit_stack_free                  PCRE2_SUFFIX(pcre2_jit_stack_free_)
#define pcre2_maketables                      PCRE2_SUFFIX(pcre2_maketables_)
#define pcre2_maketables_free                 PCRE2_SUFFIX(pcre2_maketables_free_)
#define pcre2_match                           PCRE2_SUFFIX(pcre2_match_)
#define pcre2_match_context_copy              PCRE2_SUFFIX(pcre2_match_context_copy_)
#define pcre2_match_context_create            PCRE2_SUFFIX(pcre2_match_context_create_)
#define pcre2_match_context_free              PCRE2_SUFFIX(pcre2_match_context_free_)
#define pcre2_match_data_create               PCRE2_SUFFIX(pcre2_match_data_create_)
#define pcre2_match_data_create_from_pattern  PCRE2_SUFFIX(pcre2_match_data_create_from_pattern_)
#define pcre2_match_data_free                 PCRE2_SUFFIX(pcre2_match_data_free_)
#define pcre2_pattern_convert                 PCRE2_SUFFIX(pcre2_pattern_convert_)
#define pcre2_pattern_info                    PCRE2_SUFFIX(pcre2_pattern_info_)
#define pcre2_serialize_decode                PCRE2_SUFFIX(pcre2_serialize_decode_)
#define pcre2_serialize_encode                PCRE2_SUFFIX(pcre2_serialize_encode_)
#define pcre2_serialize_free                  PCRE2_SUFFIX(pcre2_serialize_free_)
#define pcre2_serialize_get_number_of_codes   PCRE2_SUFFIX(pcre2_serialize_get_number_of_codes_)
#define pcre2_set_bsr                         PCRE2_SUFFIX(pcre2_set_bsr_)
#define pcre2_set_callout                     PCRE2_SUFFIX(pcre2_set_callout_)
#define pcre2_set_character_tables            PCRE2_SUFFIX(pcre2_set_character_tables_)
#define pcre2_set_compile_extra_options       PCRE2_SUFFIX(pcre2_set_compile_extra_options_)
#define pcre2_set_compile_recursion_guard     PCRE2_SUFFIX(pcre2_set_compile_recursion_guard_)
#define pcre2_set_depth_limit                 PCRE2_SUFFIX(pcre2_set_depth_limit_)
#define pcre2_set_glob_escape                 PCRE2_SUFFIX(pcre2_set_glob_escape_)
#define pcre2_set_glob_separator              PCRE2_SUFFIX(pcre2_set_glob_separator_)
#define pcre2_set_heap_limit                  PCRE2_SUFFIX(pcre2_set_heap_limit_)
#define pcre2_set_match_limit                 PCRE2_SUFFIX(pcre2_set_match_limit_)
#define pcre2_set_max_pattern_length          PCRE2_SUFFIX(pcre2_set_max_pattern_length_)
#define pcre2_set_newline                     PCRE2_SUFFIX(pcre2_set_newline_)
#define pcre2_set_parens_nest_limit           PCRE2_SUFFIX(pcre2_set_parens_nest_limit_)
#define pcre2_set_offset_limit                PCRE2_SUFFIX(pcre2_set_offset_limit_)
#define pcre2_set_substitute_callout          PCRE2_SUFFIX(pcre2_set_substitute_callout_)
#define pcre2_substitute                      PCRE2_SUFFIX(pcre2_substitute_)
#define pcre2_substring_copy_byname           PCRE2_SUFFIX(pcre2_substring_copy_byname_)
#define pcre2_substring_copy_bynumber         PCRE2_SUFFIX(pcre2_substring_copy_bynumber_)
#define pcre2_substring_free                  PCRE2_SUFFIX(pcre2_substring_free_)
#define pcre2_substring_get_byname            PCRE2_SUFFIX(pcre2_substring_get_byname_)
#define pcre2_substring_get_bynumber          PCRE2_SUFFIX(pcre2_substring_get_bynumber_)
#define pcre2_substring_length_byname         PCRE2_SUFFIX(pcre2_substring_length_byname_)
#define pcre2_substring_length_bynumber       PCRE2_SUFFIX(pcre2_substring_length_bynumber_)
#define pcre2_substring_list_get              PCRE2_SUFFIX(pcre2_substring_list_get_)
#define pcre2_substring_list_free             PCRE2_SUFFIX(pcre2_substring_list_free_)
#define pcre2_substring_nametable_scan        PCRE2_SUFFIX(pcre2_substring_nametable_scan_)
#define pcre2_substring_number_from_name      PCRE2_SUFFIX(pcre2_substring_number_from_name_)

#define pcre2_set_recursion_limit PCRE2_SUFFIX(pcre2_set_recursion_limit_)

#define pcre2_set_recursion_memory_management PCRE2_SUFFIX(pcre2_set_recursion_memory_management_)

#define PCRE2_TYPES_STRUCTURES_AND_FUNCTIONS \
PCRE2_TYPES_LIST \
PCRE2_STRUCTURE_LIST \
PCRE2_GENERAL_INFO_FUNCTIONS \
PCRE2_GENERAL_CONTEXT_FUNCTIONS \
PCRE2_COMPILE_CONTEXT_FUNCTIONS \
PCRE2_CONVERT_CONTEXT_FUNCTIONS \
PCRE2_CONVERT_FUNCTIONS \
PCRE2_MATCH_CONTEXT_FUNCTIONS \
PCRE2_COMPILE_FUNCTIONS \
PCRE2_PATTERN_INFO_FUNCTIONS \
PCRE2_MATCH_FUNCTIONS \
PCRE2_SUBSTRING_FUNCTIONS \
PCRE2_SERIALIZE_FUNCTIONS \
PCRE2_SUBSTITUTE_FUNCTION \
PCRE2_JIT_FUNCTIONS \
PCRE2_OTHER_FUNCTIONS

#define PCRE2_LOCAL_WIDTH 8
PCRE2_TYPES_STRUCTURES_AND_FUNCTIONS
#undef PCRE2_LOCAL_WIDTH

#define PCRE2_LOCAL_WIDTH 16
PCRE2_TYPES_STRUCTURES_AND_FUNCTIONS
#undef PCRE2_LOCAL_WIDTH

#define PCRE2_LOCAL_WIDTH 32
PCRE2_TYPES_STRUCTURES_AND_FUNCTIONS
#undef PCRE2_LOCAL_WIDTH

#undef PCRE2_TYPES_LIST
#undef PCRE2_STRUCTURE_LIST
#undef PCRE2_GENERAL_INFO_FUNCTIONS
#undef PCRE2_GENERAL_CONTEXT_FUNCTIONS
#undef PCRE2_COMPILE_CONTEXT_FUNCTIONS
#undef PCRE2_CONVERT_CONTEXT_FUNCTIONS
#undef PCRE2_MATCH_CONTEXT_FUNCTIONS
#undef PCRE2_COMPILE_FUNCTIONS
#undef PCRE2_PATTERN_INFO_FUNCTIONS
#undef PCRE2_MATCH_FUNCTIONS
#undef PCRE2_SUBSTRING_FUNCTIONS
#undef PCRE2_SERIALIZE_FUNCTIONS
#undef PCRE2_SUBSTITUTE_FUNCTION
#undef PCRE2_JIT_FUNCTIONS
#undef PCRE2_OTHER_FUNCTIONS
#undef PCRE2_TYPES_STRUCTURES_AND_FUNCTIONS

#undef PCRE2_SUFFIX
#ifndef PCRE2_CODE_UNIT_WIDTH
#error PCRE2_CODE_UNIT_WIDTH must be defined before including pcre2.h.
#error Use 8, 16, or 32; or 0 for a multi-width application.
#else
#if PCRE2_CODE_UNIT_WIDTH == 8 || PCRE2_CODE_UNIT_WIDTH == 16 || PCRE2_CODE_UNIT_WIDTH == 32
#define PCRE2_SUFFIX(a) PCRE2_GLUE(a, PCRE2_CODE_UNIT_WIDTH)
#elif PCRE2_CODE_UNIT_WIDTH == 0
#undef PCRE2_JOIN
#undef PCRE2_GLUE
#define PCRE2_SUFFIX(a) a
#else
#error PCRE2_CODE_UNIT_WIDTH must be 0, 8, 16, or 32.
#endif
#endif

#ifdef __cplusplus
}
#endif

#endif
