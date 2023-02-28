#include "config.h"
#include "pcre2_internal.h"


/* Dummy function */

#ifndef SUPPORT_UNICODE
PCRE2_SPTR
PRIV(extuni)(uint32_t c, PCRE2_SPTR eptr, PCRE2_SPTR start_subject,
  PCRE2_SPTR end_subject, BOOL utf, int *xcount)
{
(void)c;
(void)eptr;
(void)start_subject;
(void)end_subject;
(void)utf;
(void)xcount;
return NULL;
}
#else


/*************************************************
*      Match an extended grapheme sequence       *
*************************************************/

/*
Arguments:
  c              the first character
  eptr           pointer to next character
  start_subject  pointer to start of subject
  end_subject    pointer to end of subject
  utf            TRUE if in UTF mode
  xcount         pointer to count of additional characters,
                   or NULL if count not needed

Returns:         pointer after the end of the sequence
*/

PCRE2_SPTR
PRIV(extuni)(uint32_t c, PCRE2_SPTR eptr, PCRE2_SPTR start_subject,
  PCRE2_SPTR end_subject, BOOL utf, int *xcount)
{
int lgb = UCD_GRAPHBREAK(c);

while (eptr < end_subject)
  {
  int rgb;
  int len = 1;
  if (!utf) c = *eptr; else { GETCHARLEN(c, eptr, len); }
  rgb = UCD_GRAPHBREAK(c);
  if ((PRIV(ucp_gbtable)[lgb] & (1u << rgb)) == 0) break;

  /* Not breaking between Regional Indicators is allowed only if there
  are an even number of preceding RIs. */

  if (lgb == ucp_gbRegional_Indicator && rgb == ucp_gbRegional_Indicator)
    {
    int ricount = 0;
    PCRE2_SPTR bptr = eptr - 1;
    if (utf) BACKCHAR(bptr);

    /* bptr is pointing to the left-hand character */

    while (bptr > start_subject)
      {
      bptr--;
      if (utf)
        {
        BACKCHAR(bptr);
        GETCHAR(c, bptr);
        }
      else
      c = *bptr;
      if (UCD_GRAPHBREAK(c) != ucp_gbRegional_Indicator) break;
      ricount++;
      }
    if ((ricount & 1) != 0) break;  /* Grapheme break required */
    }

  /* If Extend or ZWJ follows Extended_Pictographic, do not update lgb; this
  allows any number of them before a following Extended_Pictographic. */

  if ((rgb != ucp_gbExtend && rgb != ucp_gbZWJ) ||
       lgb != ucp_gbExtended_Pictographic)
    lgb = rgb;

  eptr += len;
  if (xcount != NULL) *xcount += 1;
  }

return eptr;
}

#endif  /* SUPPORT_UNICODE */

/* End of pcre2_extuni.c */
