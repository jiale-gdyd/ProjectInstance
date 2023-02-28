#ifndef __X_TIME_ZONE_H__
#define __X_TIME_ZONE_H__

#include "xerror.h"
#include "xtypes.h"

X_BEGIN_DECLS

typedef struct _XTimeZone XTimeZone;

typedef enum {
    X_TIME_TYPE_STANDARD,
    X_TIME_TYPE_DAYLIGHT,
    X_TIME_TYPE_UNIVERSAL
} XTimeType;

XLIB_DEPRECATED_IN_2_68_FOR(x_time_zone_new_identifier)
XTimeZone *x_time_zone_new(const xchar *identifier);

XLIB_AVAILABLE_IN_2_68
XTimeZone *x_time_zone_new_identifier(const xchar *identifier);

XLIB_AVAILABLE_IN_ALL
XTimeZone *x_time_zone_new_utc(void);

XLIB_AVAILABLE_IN_ALL
XTimeZone *x_time_zone_new_local(void);

XLIB_AVAILABLE_IN_2_58
XTimeZone *x_time_zone_new_offset(xint32 seconds);

XLIB_AVAILABLE_IN_ALL
XTimeZone *x_time_zone_ref(XTimeZone *tz);

XLIB_AVAILABLE_IN_ALL
void x_time_zone_unref(XTimeZone *tz);

XLIB_AVAILABLE_IN_ALL
xint x_time_zone_find_interval(XTimeZone *tz, XTimeType type, xint64 time_);

XLIB_AVAILABLE_IN_ALL
xint x_time_zone_adjust_time(XTimeZone *tz, XTimeType type, xint64 *time_);

XLIB_AVAILABLE_IN_ALL
const xchar *x_time_zone_get_abbreviation(XTimeZone *tz, xint interval);

XLIB_AVAILABLE_IN_ALL
xint32 x_time_zone_get_offset(XTimeZone *tz, xint interval);

XLIB_AVAILABLE_IN_ALL
xboolean x_time_zone_is_dst(XTimeZone *tz, xint interval);

XLIB_AVAILABLE_IN_2_58
const xchar *x_time_zone_get_identifier(XTimeZone *tz);

X_END_DECLS

#endif
