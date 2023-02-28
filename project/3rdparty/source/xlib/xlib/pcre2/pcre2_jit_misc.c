#ifndef INCLUDED_FROM_PCRE2_JIT_COMPILE
#error This file must be included from pcre2_jit_compile.c.
#endif

void
PRIV(jit_free_rodata)(void *current, void *allocator_data)
{
#ifndef SUPPORT_JIT
(void)current;
(void)allocator_data;
#else  /* SUPPORT_JIT */
void *next;

SLJIT_UNUSED_ARG(allocator_data);

while (current != NULL)
  {
  next = *(void**)current;
  SLJIT_FREE(current, allocator_data);
  current = next;
  }

#endif /* SUPPORT_JIT */
}

/*************************************************
*           Free JIT compiled code               *
*************************************************/

void
PRIV(jit_free)(void *executable_jit, pcre2_memctl *memctl)
{
#ifndef SUPPORT_JIT
(void)executable_jit;
(void)memctl;
#else  /* SUPPORT_JIT */

executable_functions *functions = (executable_functions *)executable_jit;
void *allocator_data = memctl;
int i;

for (i = 0; i < JIT_NUMBER_OF_COMPILE_MODES; i++)
  {
  if (functions->executable_funcs[i] != NULL)
    sljit_free_code(functions->executable_funcs[i], NULL);
  PRIV(jit_free_rodata)(functions->read_only_data_heads[i], allocator_data);
  }

SLJIT_FREE(functions, allocator_data);

#endif /* SUPPORT_JIT */
}


/*************************************************
*            Free unused JIT memory              *
*************************************************/

PCRE2_EXP_DEFN void PCRE2_CALL_CONVENTION
pcre2_jit_free_unused_memory(pcre2_general_context *gcontext)
{
#ifndef SUPPORT_JIT
(void)gcontext;     /* Suppress warning */
#else  /* SUPPORT_JIT */
SLJIT_UNUSED_ARG(gcontext);
#if (defined SLJIT_EXECUTABLE_ALLOCATOR && SLJIT_EXECUTABLE_ALLOCATOR)
sljit_free_unused_memory_exec();
#endif /* SLJIT_EXECUTABLE_ALLOCATOR */
#endif /* SUPPORT_JIT */
}



/*************************************************
*            Allocate a JIT stack                *
*************************************************/

PCRE2_EXP_DEFN pcre2_jit_stack * PCRE2_CALL_CONVENTION
pcre2_jit_stack_create(size_t startsize, size_t maxsize,
  pcre2_general_context *gcontext)
{
#ifndef SUPPORT_JIT

(void)gcontext;
(void)startsize;
(void)maxsize;
return NULL;

#else  /* SUPPORT_JIT */

pcre2_jit_stack *jit_stack;

if (startsize == 0 || maxsize == 0 || maxsize > SIZE_MAX - STACK_GROWTH_RATE)
  return NULL;
if (startsize > maxsize)
  startsize = maxsize;
startsize = (startsize + STACK_GROWTH_RATE - 1) & ~(STACK_GROWTH_RATE - 1);
maxsize = (maxsize + STACK_GROWTH_RATE - 1) & ~(STACK_GROWTH_RATE - 1);

jit_stack = PRIV(memctl_malloc)(sizeof(pcre2_real_jit_stack), (pcre2_memctl *)gcontext);
if (jit_stack == NULL) return NULL;
jit_stack->stack = sljit_allocate_stack(startsize, maxsize, &jit_stack->memctl);
if (jit_stack->stack == NULL)
  {
  jit_stack->memctl.free(jit_stack, jit_stack->memctl.memory_data);
  return NULL;
  }
return jit_stack;

#endif
}


/*************************************************
*         Assign a JIT stack to a pattern        *
*************************************************/

PCRE2_EXP_DEFN void PCRE2_CALL_CONVENTION
pcre2_jit_stack_assign(pcre2_match_context *mcontext, pcre2_jit_callback callback,
  void *callback_data)
{
#ifndef SUPPORT_JIT
(void)mcontext;
(void)callback;
(void)callback_data;
#else  /* SUPPORT_JIT */

if (mcontext == NULL) return;
mcontext->jit_callback = callback;
mcontext->jit_callback_data = callback_data;

#endif  /* SUPPORT_JIT */
}


/*************************************************
*               Free a JIT stack                 *
*************************************************/

PCRE2_EXP_DEFN void PCRE2_CALL_CONVENTION
pcre2_jit_stack_free(pcre2_jit_stack *jit_stack)
{
#ifndef SUPPORT_JIT
(void)jit_stack;
#else  /* SUPPORT_JIT */
if (jit_stack != NULL)
  {
  sljit_free_stack((struct sljit_stack *)(jit_stack->stack), &jit_stack->memctl);
  jit_stack->memctl.free(jit_stack, jit_stack->memctl.memory_data);
  }
#endif  /* SUPPORT_JIT */
}


/*************************************************
*               Get target CPU type              *
*************************************************/

const char*
PRIV(jit_get_target)(void)
{
#ifndef SUPPORT_JIT
return "JIT is not supported";
#else  /* SUPPORT_JIT */
return sljit_get_platform_name();
#endif  /* SUPPORT_JIT */
}


/*************************************************
*              Get size of JIT code              *
*************************************************/

size_t
PRIV(jit_get_size)(void *executable_jit)
{
#ifndef SUPPORT_JIT
(void)executable_jit;
return 0;
#else  /* SUPPORT_JIT */
sljit_uw *executable_sizes = ((executable_functions *)executable_jit)->executable_sizes;
SLJIT_COMPILE_ASSERT(JIT_NUMBER_OF_COMPILE_MODES == 3, number_of_compile_modes_changed);
return executable_sizes[0] + executable_sizes[1] + executable_sizes[2];
#endif
}

/* End of pcre2_jit_misc.c */
