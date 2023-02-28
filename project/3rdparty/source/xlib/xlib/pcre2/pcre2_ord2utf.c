#include "config.h"
#include "pcre2_internal.h"

#ifndef SUPPORT_UNICODE
unsigned int
PRIV(ord2utf)(uint32_t cvalue, PCRE2_UCHAR *buffer)
{
(void)(cvalue);
(void)(buffer);
return 0;
}
#else  /* SUPPORT_UNICODE */


/*************************************************
*          Convert code point to UTF             *
*************************************************/

/*
Arguments:
  cvalue     the character value
  buffer     pointer to buffer for result

Returns:     number of code units placed in the buffer
*/

unsigned int
PRIV(ord2utf)(uint32_t cvalue, PCRE2_UCHAR *buffer)
{
/* Convert to UTF-8 */

#if PCRE2_CODE_UNIT_WIDTH == 8
int i, j;
for (i = 0; i < PRIV(utf8_table1_size); i++)
  if ((int)cvalue <= PRIV(utf8_table1)[i]) break;
buffer += i;
for (j = i; j > 0; j--)
 {
 *buffer-- = 0x80 | (cvalue & 0x3f);
 cvalue >>= 6;
 }
*buffer = PRIV(utf8_table2)[i] | cvalue;
return i + 1;

/* Convert to UTF-16 */

#elif PCRE2_CODE_UNIT_WIDTH == 16
if (cvalue <= 0xffff)
  {
  *buffer = (PCRE2_UCHAR)cvalue;
  return 1;
  }
cvalue -= 0x10000;
*buffer++ = 0xd800 | (cvalue >> 10);
*buffer = 0xdc00 | (cvalue & 0x3ff);
return 2;

/* Convert to UTF-32 */

#else
*buffer = (PCRE2_UCHAR)cvalue;
return 1;
#endif
}
#endif  /* SUPPORT_UNICODE */

/* End of pcre_ord2utf.c */
