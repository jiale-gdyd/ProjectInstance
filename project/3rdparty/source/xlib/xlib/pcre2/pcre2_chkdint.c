#ifndef PCRE2_PCRE2TEST
#include "config.h"
#include "pcre2_internal.h"
#endif

BOOL
PRIV(ckd_smul)(PCRE2_SIZE *r, int a, int b)
{
#ifdef HAVE_BUILTIN_MUL_OVERFLOW
PCRE2_SIZE m;

if (__builtin_mul_overflow(a, b, &m)) return TRUE;

*r = m;
#else
INT64_OR_DOUBLE m;

#ifdef PCRE2_DEBUG
if (a < 0 || b < 0) abort();
#endif

m = (INT64_OR_DOUBLE)a * (INT64_OR_DOUBLE)b;

#if defined INT64_MAX || defined int64_t
if (sizeof(m) > sizeof(*r) && m > (INT64_OR_DOUBLE)PCRE2_SIZE_MAX) return TRUE;
*r = (PCRE2_SIZE)m;
#else
if (m > PCRE2_SIZE_MAX) return TRUE;
*r = m;
#endif

#endif

return FALSE;
}

/* End of pcre_chkdint.c */