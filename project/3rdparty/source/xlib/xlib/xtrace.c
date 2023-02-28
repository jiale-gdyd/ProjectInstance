#include <stdarg.h>
#include <xlib/xlib/config.h>
#include <xlib/xlib/xtrace-private.h>

void (x_trace_mark)(xint64 begin_time_nsec, xint64 duration_nsec, const xchar *group, const xchar *name, const xchar *message_format, ...)
{

}

xuint (x_trace_define_int64_counter)(const char *group, const char *name, const char *description)
{
    return (xuint)-1;
}

void (x_trace_set_int64_counter)(xuint id, xint64 val)
{

}
