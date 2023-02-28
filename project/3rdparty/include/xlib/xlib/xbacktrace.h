#ifndef __X_BACKTRACE_H__
#define __X_BACKTRACE_H__

#include "xtypes.h"
#ifdef __sun__
#include <sys/select.h>
#endif
#include <signal.h>

X_BEGIN_DECLS

XLIB_AVAILABLE_IN_ALL
void x_on_error_query(const xchar *prg_name);

XLIB_AVAILABLE_IN_ALL
void x_on_error_stack_trace(const xchar *prg_name);

#if (defined (__i386__) || defined (__x86_64__)) && defined (__GNUC__) && __GNUC__ >= 2
#define X_BREAKPOINT()        X_STMT_START{ __asm__ __volatile__ ("int $03"); } X_STMT_END
#elif (defined (_MSC_VER) || defined (__DMC__)) && defined (_M_IX86)
#define X_BREAKPOINT()        X_STMT_START{ __asm int 3h } X_STMT_END
#elif defined (_MSC_VER)
#define X_BREAKPOINT()        X_STMT_START{ __debugbreak(); } X_STMT_END
#elif defined (__alpha__) && !defined(__osf__) && defined (__GNUC__) && __GNUC__ >= 2
#define X_BREAKPOINT()        X_STMT_START{ __asm__ __volatile__ ("bpt"); } X_STMT_END
#elif defined (__APPLE__) || (defined(_WIN32) && (defined(__clang__) || defined(__GNUC__)))
#define X_BREAKPOINT()        X_STMT_START{ __builtin_trap(); } X_STMT_END
#else
#define X_BREAKPOINT()        X_STMT_START{ raise (SIGTRAP); } X_STMT_END
#endif

X_END_DECLS

#endif
