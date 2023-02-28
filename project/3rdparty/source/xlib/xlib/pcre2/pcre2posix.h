#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PCRE2_CALL_CONVENTION
#define PCRE2_CALL_CONVENTION
#endif

#define REG_ICASE                   0x0001
#define REG_NEWLINE                 0x0002
#define REG_NOTBOL                  0x0004
#define REG_NOTEOL                  0x0008
#define REG_DOTALL                  0x0010
#define REG_NOSUB                   0x0020
#define REG_UTF                     0x0040
#define REG_STARTEND                0x0080
#define REG_NOTEMPTY                0x0100
#define REG_UNGREEDY                0x0200
#define REG_UCP                     0x0400
#define REG_PEND                    0x0800
#define REG_NOSPEC                  0x1000

#define REG_EXTENDED                0

enum {
    REG_ASSERT = 1,
    REG_BADBR,
    REG_BADPAT,
    REG_BADRPT,
    REG_EBRACE,
    REG_EBRACK,
    REG_ECOLLATE,
    REG_ECTYPE,
    REG_EESCAPE,
    REG_EMPTY,
    REG_EPAREN,
    REG_ERANGE,
    REG_ESIZE,
    REG_ESPACE,
    REG_ESUBREG,
    REG_INVARG,
    REG_NOMATCH
};

typedef struct {
    void       *re_pcre2_code;
    void       *re_match_data;
    const char *re_endp;
    size_t     re_nsub;
    size_t     re_erroffset;
    int        re_cflags;
} regex_t;

typedef int regoff_t;

typedef struct {
    regoff_t rm_so;
    regoff_t rm_eo;
} regmatch_t;

#ifndef PCRE2POSIX_EXP_DECL
#ifdef __cplusplus
#define PCRE2POSIX_EXP_DECL         extern "C"
#define PCRE2POSIX_EXP_DEFN         extern "C"
#else
#define PCRE2POSIX_EXP_DECL         extern
#define PCRE2POSIX_EXP_DEFN         extern
#endif
#endif

PCRE2POSIX_EXP_DECL int PCRE2_CALL_CONVENTION pcre2_regcomp(regex_t *, const char *, int);
PCRE2POSIX_EXP_DECL int PCRE2_CALL_CONVENTION pcre2_regexec(const regex_t *, const char *, size_t, regmatch_t *, int);

PCRE2POSIX_EXP_DECL void PCRE2_CALL_CONVENTION pcre2_regfree(regex_t *);
PCRE2POSIX_EXP_DECL size_t PCRE2_CALL_CONVENTION pcre2_regerror(int, const regex_t *, char *, size_t);

#define regcomp                     pcre2_regcomp
#define regexec                     pcre2_regexec
#define regerror                    pcre2_regerror
#define regfree                     pcre2_regfree

#define PCRE2regcomp                pcre2_regcomp
#define PCRE2regexec                pcre2_regexec
#define PCRE2regerror               pcre2_regerror
#define PCRE2regfree                pcre2_regfree

#ifdef __cplusplus
}
#endif
