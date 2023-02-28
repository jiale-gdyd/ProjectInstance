#ifndef __XLIB_TRACE_H__
#define __XLIB_TRACE_H__

#include "config.h"

#ifndef SIZEOF_CHAR
#error "config.h must be included prior to xlib_trace.h"
#endif

#if defined(HAVE_DTRACE) && !defined(__clang_analyzer__)

#include "xlib_probes.h"
#define TRACE(probe)            probe
#else
#define TRACE(probe)

#endif

#endif
