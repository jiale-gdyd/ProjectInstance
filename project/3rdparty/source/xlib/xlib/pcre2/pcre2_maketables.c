#ifndef PCRE2_DFTABLES    /* Compiling the library */
#include "config.h"
#include "pcre2_internal.h"
#endif

/*************************************************
*           Create PCRE2 character tables        *
*************************************************/

/* This function builds a set of character tables for use by PCRE2 and returns
a pointer to them. They are build using the ctype functions, and consequently
their contents will depend upon the current locale setting. When compiled as
part of the library, the store is obtained via a general context malloc, if
supplied, but when PCRE2_DFTABLES is defined (when compiling the pcre2_dftables
freestanding auxiliary program) malloc() is used, and the function has a
different name so as not to clash with the prototype in pcre2.h.

Arguments:   none when PCRE2_DFTABLES is defined
               else a PCRE2 general context or NULL
Returns:     pointer to the contiguous block of data
               else NULL if memory allocation failed
*/

#ifdef PCRE2_DFTABLES  /* Included in freestanding pcre2_dftables program */
static const uint8_t *maketables(void)
{
uint8_t *yield = (uint8_t *)malloc(TABLES_LENGTH);

#else  /* Not PCRE2_DFTABLES, that is, compiling the library */
PCRE2_EXP_DEFN const uint8_t * PCRE2_CALL_CONVENTION
pcre2_maketables(pcre2_general_context *gcontext)
{
uint8_t *yield = (uint8_t *)((gcontext != NULL)?
  gcontext->memctl.malloc(TABLES_LENGTH, gcontext->memctl.memory_data) :
  malloc(TABLES_LENGTH));
#endif  /* PCRE2_DFTABLES */

int i;
uint8_t *p;

if (yield == NULL) return NULL;
p = yield;

/* First comes the lower casing table */

for (i = 0; i < 256; i++) *p++ = tolower(i);

/* Next the case-flipping table */

for (i = 0; i < 256; i++) *p++ = islower(i)? toupper(i) : tolower(i);

/* Then the character class tables. Don't try to be clever and save effort on
exclusive ones - in some locales things may be different.

Note that the table for "space" includes everything "isspace" gives, including
VT in the default locale. This makes it work for the POSIX class [:space:].
From PCRE1 release 8.34 and for all PCRE2 releases it is also correct for Perl
space, because Perl added VT at release 5.18.

Note also that it is possible for a character to be alnum or alpha without
being lower or upper, such as "male and female ordinals" (\xAA and \xBA) in the
fr_FR locale (at least under Debian Linux's locales as of 12/2005). So we must
test for alnum specially. */

memset(p, 0, cbit_length);
for (i = 0; i < 256; i++)
  {
  if (isdigit(i))  p[cbit_digit  + i/8] |= 1u << (i&7);
  if (isupper(i))  p[cbit_upper  + i/8] |= 1u << (i&7);
  if (islower(i))  p[cbit_lower  + i/8] |= 1u << (i&7);
  if (isalnum(i))  p[cbit_word   + i/8] |= 1u << (i&7);
  if (i == '_')    p[cbit_word   + i/8] |= 1u << (i&7);
  if (isspace(i))  p[cbit_space  + i/8] |= 1u << (i&7);
  if (isxdigit(i)) p[cbit_xdigit + i/8] |= 1u << (i&7);
  if (isgraph(i))  p[cbit_graph  + i/8] |= 1u << (i&7);
  if (isprint(i))  p[cbit_print  + i/8] |= 1u << (i&7);
  if (ispunct(i))  p[cbit_punct  + i/8] |= 1u << (i&7);
  if (iscntrl(i))  p[cbit_cntrl  + i/8] |= 1u << (i&7);
  }
p += cbit_length;

/* Finally, the character type table. In this, we used to exclude VT from the
white space chars, because Perl didn't recognize it as such for \s and for
comments within regexes. However, Perl changed at release 5.18, so PCRE1
changed at release 8.34 and it's always been this way for PCRE2. */

for (i = 0; i < 256; i++)
  {
  int x = 0;
  if (isspace(i)) x += ctype_space;
  if (isalpha(i)) x += ctype_letter;
  if (islower(i)) x += ctype_lcletter;
  if (isdigit(i)) x += ctype_digit;
  if (isalnum(i) || i == '_') x += ctype_word;
  *p++ = x;
  }

return yield;
}

#ifndef PCRE2_DFTABLES   /* Compiling the library */
PCRE2_EXP_DEFN void PCRE2_CALL_CONVENTION
pcre2_maketables_free(pcre2_general_context *gcontext, const uint8_t *tables)
{
  if (gcontext)
    gcontext->memctl.free((void *)tables, gcontext->memctl.memory_data);
  else
    free((void *)tables);
}
#endif

/* End of pcre2_maketables.c */