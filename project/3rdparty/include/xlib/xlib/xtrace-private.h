#ifndef __XTRACE_PRIVATE_H__
#define __XTRACE_PRIVATE_H__

#include "../xlib.h"

X_BEGIN_DECLS

#define X_TRACE_CURRENT_TIME                0

void (x_trace_mark)(xint64 begin_time_nsec, xint64 duration_nsec, const xchar *group, const xchar *name, const xchar *message_format, ...) X_GNUC_PRINTF (5, 6);

#if defined(X_HAVE_ISO_VARARGS)
#define x_trace_mark(b, d, g, n, m, ...)
#elif defined(X_HAVE_GNUC_VARARGS)
#define x_trace_mark(b, d, g, n, m...)
#else
#endif

void (x_trace_set_int64_counter)(xuint id, xint64 value);
xuint (x_trace_define_int64_counter)(const char *group, const char *name, const char *description);

#define x_trace_set_int64_counter(i, v)
#define x_trace_define_int64_counter(g, n, d)       ((xuint)-1)

X_END_DECLS

#endif
