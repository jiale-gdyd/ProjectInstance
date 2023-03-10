#include "config.h"
#include "pcre2_internal.h"

/*************************************************
*          Default malloc/free functions         *
*************************************************/

/* Ignore the "user data" argument in each case. */

static void *default_malloc(size_t size, void *data)
{
(void)data;
return malloc(size);
}


static void default_free(void *block, void *data)
{
(void)data;
free(block);
}



/*************************************************
*        Get a block and save memory control     *
*************************************************/

/* This internal function is called to get a block of memory in which the
memory control data is to be stored at the start for future use.
Arguments:
  size        amount of memory required
  memctl      pointer to a memctl block or NULL
Returns:      pointer to memory or NULL on failure
*/

extern void *
PRIV(memctl_malloc)(size_t size, pcre2_memctl *memctl)
{
pcre2_memctl *newmemctl;
void *yield = (memctl == NULL)? malloc(size) :
  memctl->malloc(size, memctl->memory_data);
if (yield == NULL) return NULL;
newmemctl = (pcre2_memctl *)yield;
if (memctl == NULL)
  {
  newmemctl->malloc = default_malloc;
  newmemctl->free = default_free;
  newmemctl->memory_data = NULL;
  }
else *newmemctl = *memctl;
return yield;
}



/*************************************************
*          Create and initialize contexts        *
*************************************************/

/* Initializing for compile and match contexts is done in separate, private
functions so that these can be called from functions such as pcre2_compile()
when an external context is not supplied. The initializing functions have an
option to set up default memory management. */

PCRE2_EXP_DEFN pcre2_general_context * PCRE2_CALL_CONVENTION
pcre2_general_context_create(void *(*private_malloc)(size_t, void *),
  void (*private_free)(void *, void *), void *memory_data)
{
pcre2_general_context *gcontext;
if (private_malloc == NULL) private_malloc = default_malloc;
if (private_free == NULL) private_free = default_free;
gcontext = private_malloc(sizeof(pcre2_real_general_context), memory_data);
if (gcontext == NULL) return NULL;
gcontext->memctl.malloc = private_malloc;
gcontext->memctl.free = private_free;
gcontext->memctl.memory_data = memory_data;
return gcontext;
}


/* A default compile context is set up to save having to initialize at run time
when no context is supplied to the compile function. */

const pcre2_compile_context PRIV(default_compile_context) = {
  { default_malloc, default_free, NULL },    /* Default memory handling */
  NULL,                                      /* Stack guard */
  NULL,                                      /* Stack guard data */
  PRIV(default_tables),                      /* Character tables */
  PCRE2_UNSET,                               /* Max pattern length */
  BSR_DEFAULT,                               /* Backslash R default */
  NEWLINE_DEFAULT,                           /* Newline convention */
  PARENS_NEST_LIMIT,                         /* As it says */
  0 };                                       /* Extra options */

/* The create function copies the default into the new memory, but must
override the default memory handling functions if a gcontext was provided. */

PCRE2_EXP_DEFN pcre2_compile_context * PCRE2_CALL_CONVENTION
pcre2_compile_context_create(pcre2_general_context *gcontext)
{
pcre2_compile_context *ccontext = PRIV(memctl_malloc)(
  sizeof(pcre2_real_compile_context), (pcre2_memctl *)gcontext);
if (ccontext == NULL) return NULL;
*ccontext = PRIV(default_compile_context);
if (gcontext != NULL)
  *((pcre2_memctl *)ccontext) = *((pcre2_memctl *)gcontext);
return ccontext;
}


/* A default match context is set up to save having to initialize at run time
when no context is supplied to a match function. */

const pcre2_match_context PRIV(default_match_context) = {
  { default_malloc, default_free, NULL },
#ifdef SUPPORT_JIT
  NULL,          /* JIT callback */
  NULL,          /* JIT callback data */
#endif
  NULL,          /* Callout function */
  NULL,          /* Callout data */
  NULL,          /* Substitute callout function */
  NULL,          /* Substitute callout data */
  PCRE2_UNSET,   /* Offset limit */
  HEAP_LIMIT,
  MATCH_LIMIT,
  MATCH_LIMIT_DEPTH };

/* The create function copies the default into the new memory, but must
override the default memory handling functions if a gcontext was provided. */

PCRE2_EXP_DEFN pcre2_match_context * PCRE2_CALL_CONVENTION
pcre2_match_context_create(pcre2_general_context *gcontext)
{
pcre2_match_context *mcontext = PRIV(memctl_malloc)(
  sizeof(pcre2_real_match_context), (pcre2_memctl *)gcontext);
if (mcontext == NULL) return NULL;
*mcontext = PRIV(default_match_context);
if (gcontext != NULL)
  *((pcre2_memctl *)mcontext) = *((pcre2_memctl *)gcontext);
return mcontext;
}


/* A default convert context is set up to save having to initialize at run time
when no context is supplied to the convert function. */

const pcre2_convert_context PRIV(default_convert_context) = {
  { default_malloc, default_free, NULL },    /* Default memory handling */
#ifdef _WIN32
  CHAR_BACKSLASH,                            /* Default path separator */
  CHAR_GRAVE_ACCENT                          /* Default escape character */
#else  /* Not Windows */
  CHAR_SLASH,                                /* Default path separator */
  CHAR_BACKSLASH                             /* Default escape character */
#endif
  };

/* The create function copies the default into the new memory, but must
override the default memory handling functions if a gcontext was provided. */

PCRE2_EXP_DEFN pcre2_convert_context * PCRE2_CALL_CONVENTION
pcre2_convert_context_create(pcre2_general_context *gcontext)
{
pcre2_convert_context *ccontext = PRIV(memctl_malloc)(
  sizeof(pcre2_real_convert_context), (pcre2_memctl *)gcontext);
if (ccontext == NULL) return NULL;
*ccontext = PRIV(default_convert_context);
if (gcontext != NULL)
  *((pcre2_memctl *)ccontext) = *((pcre2_memctl *)gcontext);
return ccontext;
}


/*************************************************
*              Context copy functions            *
*************************************************/

PCRE2_EXP_DEFN pcre2_general_context * PCRE2_CALL_CONVENTION
pcre2_general_context_copy(pcre2_general_context *gcontext)
{
pcre2_general_context *newcontext =
  gcontext->memctl.malloc(sizeof(pcre2_real_general_context),
  gcontext->memctl.memory_data);
if (newcontext == NULL) return NULL;
memcpy(newcontext, gcontext, sizeof(pcre2_real_general_context));
return newcontext;
}


PCRE2_EXP_DEFN pcre2_compile_context * PCRE2_CALL_CONVENTION
pcre2_compile_context_copy(pcre2_compile_context *ccontext)
{
pcre2_compile_context *newcontext =
  ccontext->memctl.malloc(sizeof(pcre2_real_compile_context),
  ccontext->memctl.memory_data);
if (newcontext == NULL) return NULL;
memcpy(newcontext, ccontext, sizeof(pcre2_real_compile_context));
return newcontext;
}


PCRE2_EXP_DEFN pcre2_match_context * PCRE2_CALL_CONVENTION
pcre2_match_context_copy(pcre2_match_context *mcontext)
{
pcre2_match_context *newcontext =
  mcontext->memctl.malloc(sizeof(pcre2_real_match_context),
  mcontext->memctl.memory_data);
if (newcontext == NULL) return NULL;
memcpy(newcontext, mcontext, sizeof(pcre2_real_match_context));
return newcontext;
}


PCRE2_EXP_DEFN pcre2_convert_context * PCRE2_CALL_CONVENTION
pcre2_convert_context_copy(pcre2_convert_context *ccontext)
{
pcre2_convert_context *newcontext =
  ccontext->memctl.malloc(sizeof(pcre2_real_convert_context),
  ccontext->memctl.memory_data);
if (newcontext == NULL) return NULL;
memcpy(newcontext, ccontext, sizeof(pcre2_real_convert_context));
return newcontext;
}


/*************************************************
*              Context free functions            *
*************************************************/

PCRE2_EXP_DEFN void PCRE2_CALL_CONVENTION
pcre2_general_context_free(pcre2_general_context *gcontext)
{
if (gcontext != NULL)
  gcontext->memctl.free(gcontext, gcontext->memctl.memory_data);
}


PCRE2_EXP_DEFN void PCRE2_CALL_CONVENTION
pcre2_compile_context_free(pcre2_compile_context *ccontext)
{
if (ccontext != NULL)
  ccontext->memctl.free(ccontext, ccontext->memctl.memory_data);
}


PCRE2_EXP_DEFN void PCRE2_CALL_CONVENTION
pcre2_match_context_free(pcre2_match_context *mcontext)
{
if (mcontext != NULL)
  mcontext->memctl.free(mcontext, mcontext->memctl.memory_data);
}


PCRE2_EXP_DEFN void PCRE2_CALL_CONVENTION
pcre2_convert_context_free(pcre2_convert_context *ccontext)
{
if (ccontext != NULL)
  ccontext->memctl.free(ccontext, ccontext->memctl.memory_data);
}


/*************************************************
*             Set values in contexts             *
*************************************************/

/* All these functions return 0 for success or PCRE2_ERROR_BADDATA if invalid
data is given. Only some of the functions are able to test the validity of the
data. */


/* ------------ Compile context ------------ */

PCRE2_EXP_DEFN int PCRE2_CALL_CONVENTION
pcre2_set_character_tables(pcre2_compile_context *ccontext,
  const uint8_t *tables)
{
ccontext->tables = tables;
return 0;
}

PCRE2_EXP_DEFN int PCRE2_CALL_CONVENTION
pcre2_set_bsr(pcre2_compile_context *ccontext, uint32_t value)
{
switch(value)
  {
  case PCRE2_BSR_ANYCRLF:
  case PCRE2_BSR_UNICODE:
  ccontext->bsr_convention = value;
  return 0;

  default:
  return PCRE2_ERROR_BADDATA;
  }
}

PCRE2_EXP_DEFN int PCRE2_CALL_CONVENTION
pcre2_set_max_pattern_length(pcre2_compile_context *ccontext, PCRE2_SIZE length)
{
ccontext->max_pattern_length = length;
return 0;
}

PCRE2_EXP_DEFN int PCRE2_CALL_CONVENTION
pcre2_set_newline(pcre2_compile_context *ccontext, uint32_t newline)
{
switch(newline)
  {
  case PCRE2_NEWLINE_CR:
  case PCRE2_NEWLINE_LF:
  case PCRE2_NEWLINE_CRLF:
  case PCRE2_NEWLINE_ANY:
  case PCRE2_NEWLINE_ANYCRLF:
  case PCRE2_NEWLINE_NUL:
  ccontext->newline_convention = newline;
  return 0;

  default:
  return PCRE2_ERROR_BADDATA;
  }
}

PCRE2_EXP_DEFN int PCRE2_CALL_CONVENTION
pcre2_set_parens_nest_limit(pcre2_compile_context *ccontext, uint32_t limit)
{
ccontext->parens_nest_limit = limit;
return 0;
}

PCRE2_EXP_DEFN int PCRE2_CALL_CONVENTION
pcre2_set_compile_extra_options(pcre2_compile_context *ccontext, uint32_t options)
{
ccontext->extra_options = options;
return 0;
}

PCRE2_EXP_DEFN int PCRE2_CALL_CONVENTION
pcre2_set_compile_recursion_guard(pcre2_compile_context *ccontext,
  int (*guard)(uint32_t, void *), void *user_data)
{
ccontext->stack_guard = guard;
ccontext->stack_guard_data = user_data;
return 0;
}


/* ------------ Match context ------------ */

PCRE2_EXP_DEFN int PCRE2_CALL_CONVENTION
pcre2_set_callout(pcre2_match_context *mcontext,
  int (*callout)(pcre2_callout_block *, void *), void *callout_data)
{
mcontext->callout = callout;
mcontext->callout_data = callout_data;
return 0;
}

PCRE2_EXP_DEFN int PCRE2_CALL_CONVENTION
pcre2_set_substitute_callout(pcre2_match_context *mcontext,
  int (*substitute_callout)(pcre2_substitute_callout_block *, void *),
    void *substitute_callout_data)
{
mcontext->substitute_callout = substitute_callout;
mcontext->substitute_callout_data = substitute_callout_data;
return 0;
}

PCRE2_EXP_DEFN int PCRE2_CALL_CONVENTION
pcre2_set_heap_limit(pcre2_match_context *mcontext, uint32_t limit)
{
mcontext->heap_limit = limit;
return 0;
}

PCRE2_EXP_DEFN int PCRE2_CALL_CONVENTION
pcre2_set_match_limit(pcre2_match_context *mcontext, uint32_t limit)
{
mcontext->match_limit = limit;
return 0;
}

PCRE2_EXP_DEFN int PCRE2_CALL_CONVENTION
pcre2_set_depth_limit(pcre2_match_context *mcontext, uint32_t limit)
{
mcontext->depth_limit = limit;
return 0;
}

PCRE2_EXP_DEFN int PCRE2_CALL_CONVENTION
pcre2_set_offset_limit(pcre2_match_context *mcontext, PCRE2_SIZE limit)
{
mcontext->offset_limit = limit;
return 0;
}

/* These functions became obsolete at release 10.30. The first is kept as a
synonym for backwards compatibility. The second now does nothing. Exclude both
from coverage reports. */

/* LCOV_EXCL_START */

PCRE2_EXP_DEFN int PCRE2_CALL_CONVENTION
pcre2_set_recursion_limit(pcre2_match_context *mcontext, uint32_t limit)
{
return pcre2_set_depth_limit(mcontext, limit);
}

PCRE2_EXP_DEFN int PCRE2_CALL_CONVENTION
pcre2_set_recursion_memory_management(pcre2_match_context *mcontext,
  void *(*mymalloc)(size_t, void *), void (*myfree)(void *, void *),
  void *mydata)
{
(void)mcontext;
(void)mymalloc;
(void)myfree;
(void)mydata;
return 0;
}

/* LCOV_EXCL_STOP */


/* ------------ Convert context ------------ */

PCRE2_EXP_DEFN int PCRE2_CALL_CONVENTION
pcre2_set_glob_separator(pcre2_convert_context *ccontext, uint32_t separator)
{
if (separator != CHAR_SLASH && separator != CHAR_BACKSLASH &&
    separator != CHAR_DOT) return PCRE2_ERROR_BADDATA;
ccontext->glob_separator = separator;
return 0;
}

PCRE2_EXP_DEFN int PCRE2_CALL_CONVENTION
pcre2_set_glob_escape(pcre2_convert_context *ccontext, uint32_t escape)
{
if (escape > 255 || (escape != 0 && !ispunct(escape)))
  return PCRE2_ERROR_BADDATA;
ccontext->glob_escape = escape;
return 0;
}