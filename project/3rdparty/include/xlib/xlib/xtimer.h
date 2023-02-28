#ifndef __X_TIMER_H__
#define __X_TIMER_H__

#include "xtypes.h"

X_BEGIN_DECLS

#define X_USEC_PER_SEC          1000000

typedef struct _XTimer XTimer;

XLIB_AVAILABLE_IN_ALL
XTimer *x_timer_new(void);

XLIB_AVAILABLE_IN_ALL
void x_timer_destroy(XTimer *timer);

XLIB_AVAILABLE_IN_ALL
void x_timer_start(XTimer *timer);

XLIB_AVAILABLE_IN_ALL
void x_timer_stop(XTimer *timer);

XLIB_AVAILABLE_IN_ALL
void x_timer_reset(XTimer *timer);

XLIB_AVAILABLE_IN_ALL
void x_timer_continue(XTimer *timer);

XLIB_AVAILABLE_IN_ALL
xdouble x_timer_elapsed(XTimer *timer, xulong *microseconds);

XLIB_AVAILABLE_IN_2_62
xboolean x_timer_is_active(XTimer *timer);

XLIB_AVAILABLE_IN_ALL
void x_usleep(xulong microseconds);

X_GNUC_BEGIN_IGNORE_DEPRECATIONS

XLIB_DEPRECATED_IN_2_62
void x_time_val_add(XTimeVal *time_, xlong microseconds);

XLIB_DEPRECATED_IN_2_62_FOR(x_date_time_new_from_iso8601)
xboolean x_time_val_from_iso8601(const xchar *iso_date, XTimeVal *time_);

XLIB_DEPRECATED_IN_2_62_FOR(x_date_time_format)
xchar *x_time_val_to_iso8601(XTimeVal *time_) X_GNUC_MALLOC;

X_GNUC_END_IGNORE_DEPRECATIONS

X_END_DECLS

#endif
