#include "config.h"
#include "pcre2_internal.h"

/*************************************************
*  Create a match data block given ovector size  *
*************************************************/

/* A minimum of 1 is imposed on the number of ovector pairs. A maximum is also 
imposed because the oveccount field in a match data block is uintt6_t. */

PCRE2_EXP_DEFN pcre2_match_data * PCRE2_CALL_CONVENTION
pcre2_match_data_create(uint32_t oveccount, pcre2_general_context *gcontext)
{
pcre2_match_data *yield;
if (oveccount < 1) oveccount = 1;
if (oveccount > UINT16_MAX) oveccount = UINT16_MAX;
yield = (pcre2_match_data *)PRIV(memctl_malloc)(
  offsetof(pcre2_match_data, ovector) + 2*oveccount*sizeof(PCRE2_SIZE),
  (pcre2_memctl *)gcontext);
if (yield == NULL) return NULL;
yield->oveccount = oveccount;
yield->flags = 0;
yield->heapframes = NULL;
yield->heapframes_size = 0;
return yield;
}



/*************************************************
*  Create a match data block using pattern data  *
*************************************************/

/* If no context is supplied, use the memory allocator from the code. */

PCRE2_EXP_DEFN pcre2_match_data * PCRE2_CALL_CONVENTION
pcre2_match_data_create_from_pattern(const pcre2_code *code,
  pcre2_general_context *gcontext)
{
if (gcontext == NULL) gcontext = (pcre2_general_context *)code;
return pcre2_match_data_create(((pcre2_real_code *)code)->top_bracket + 1,
  gcontext);
}



/*************************************************
*            Free a match data block             *
*************************************************/

PCRE2_EXP_DEFN void PCRE2_CALL_CONVENTION
pcre2_match_data_free(pcre2_match_data *match_data)
{
if (match_data != NULL)
  {
  if (match_data->heapframes != NULL)
    match_data->memctl.free(match_data->heapframes,
      match_data->memctl.memory_data);
  if ((match_data->flags & PCRE2_MD_COPIED_SUBJECT) != 0)
    match_data->memctl.free((void *)match_data->subject,
      match_data->memctl.memory_data);
  match_data->memctl.free(match_data, match_data->memctl.memory_data);
  }
}



/*************************************************
*         Get last mark in match                 *
*************************************************/

PCRE2_EXP_DEFN PCRE2_SPTR PCRE2_CALL_CONVENTION
pcre2_get_mark(pcre2_match_data *match_data)
{
return match_data->mark;
}



/*************************************************
*          Get pointer to ovector                *
*************************************************/

PCRE2_EXP_DEFN PCRE2_SIZE * PCRE2_CALL_CONVENTION
pcre2_get_ovector_pointer(pcre2_match_data *match_data)
{
return match_data->ovector;
}



/*************************************************
*          Get number of ovector slots           *
*************************************************/

PCRE2_EXP_DEFN uint32_t PCRE2_CALL_CONVENTION
pcre2_get_ovector_count(pcre2_match_data *match_data)
{
return match_data->oveccount;
}



/*************************************************
*         Get starting code unit in match        *
*************************************************/

PCRE2_EXP_DEFN PCRE2_SIZE PCRE2_CALL_CONVENTION
pcre2_get_startchar(pcre2_match_data *match_data)
{
return match_data->startchar;
}



/*************************************************
*         Get size of match data block           *
*************************************************/

PCRE2_EXP_DEFN PCRE2_SIZE PCRE2_CALL_CONVENTION
pcre2_get_match_data_size(pcre2_match_data *match_data)
{
return offsetof(pcre2_match_data, ovector) +
  2 * (match_data->oveccount) * sizeof(PCRE2_SIZE);
}

/*************************************************
*             Get heapframes size                *
*************************************************/

PCRE2_EXP_DEFN PCRE2_SIZE PCRE2_CALL_CONVENTION
pcre2_get_match_data_heapframes_size(pcre2_match_data *match_data)
{
return match_data->heapframes_size;
}

/* End of pcre2_match_data.c */
