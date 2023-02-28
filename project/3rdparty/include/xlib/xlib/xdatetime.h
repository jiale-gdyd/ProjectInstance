#ifndef __X_DATE_TIME_H__
#define __X_DATE_TIME_H__

#include "xtimezone.h"

X_BEGIN_DECLS

#define X_TIME_SPAN_DAY                 (X_XINT64_CONSTANT(86400000000))
#define X_TIME_SPAN_HOUR                (X_XINT64_CONSTANT(3600000000))
#define X_TIME_SPAN_MINUTE              (X_XINT64_CONSTANT(60000000))
#define X_TIME_SPAN_SECOND              (X_XINT64_CONSTANT(1000000))
#define X_TIME_SPAN_MILLISECOND         (X_XINT64_CONSTANT(1000))

typedef xint64 XTimeSpan;
typedef struct _XDateTime XDateTime;

XLIB_AVAILABLE_IN_ALL
void x_date_time_unref(XDateTime *datetime);

XLIB_AVAILABLE_IN_ALL
XDateTime *x_date_time_ref(XDateTime *datetime);

XLIB_AVAILABLE_IN_ALL
XDateTime *x_date_time_new_now(XTimeZone *tz);

XLIB_AVAILABLE_IN_ALL
XDateTime *x_date_time_new_now_local(void);

XLIB_AVAILABLE_IN_ALL
XDateTime *x_date_time_new_now_utc(void);

XLIB_AVAILABLE_IN_ALL
XDateTime *x_date_time_new_from_unix_local(xint64 t);

XLIB_AVAILABLE_IN_ALL
XDateTime *x_date_time_new_from_unix_utc(xint64 t);

X_GNUC_BEGIN_IGNORE_DEPRECATIONS
XLIB_DEPRECATED_IN_2_62_FOR(x_date_time_new_from_unix_local)
XDateTime *x_date_time_new_from_timeval_local(const XTimeVal *tv);

XLIB_DEPRECATED_IN_2_62_FOR(x_date_time_new_from_unix_utc)
XDateTime *x_date_time_new_from_timeval_utc(const XTimeVal *tv);
X_GNUC_END_IGNORE_DEPRECATIONS

XLIB_AVAILABLE_IN_2_56
XDateTime *x_date_time_new_from_iso8601(const xchar *text, XTimeZone *default_tz);

XLIB_AVAILABLE_IN_ALL
XDateTime *x_date_time_new(XTimeZone *tz, xint year, xint month, xint day, xint hour, xint minute, xdouble seconds);

XLIB_AVAILABLE_IN_ALL
XDateTime *x_date_time_new_local(xint year, xint month, xint day, xint hour, xint minute, xdouble seconds);

XLIB_AVAILABLE_IN_ALL
XDateTime *x_date_time_new_utc(xint year, xint month, xint day, xint hour, xint minute, xdouble seconds);

XLIB_AVAILABLE_IN_ALL
X_GNUC_WARN_UNUSED_RESULT
XDateTime *x_date_time_add(XDateTime *datetime, XTimeSpan timespan);

XLIB_AVAILABLE_IN_ALL
X_GNUC_WARN_UNUSED_RESULT
XDateTime *x_date_time_add_years(XDateTime *datetime, xint years);

XLIB_AVAILABLE_IN_ALL
X_GNUC_WARN_UNUSED_RESULT
XDateTime * x_date_time_add_months(XDateTime *datetime, xint months);

XLIB_AVAILABLE_IN_ALL
X_GNUC_WARN_UNUSED_RESULT
XDateTime *x_date_time_add_weeks(XDateTime *datetime, xint weeks);

XLIB_AVAILABLE_IN_ALL
X_GNUC_WARN_UNUSED_RESULT
XDateTime *x_date_time_add_days(XDateTime *datetime, xint days);

XLIB_AVAILABLE_IN_ALL
X_GNUC_WARN_UNUSED_RESULT
XDateTime *x_date_time_add_hours(XDateTime *datetime, xint hours);

XLIB_AVAILABLE_IN_ALL
X_GNUC_WARN_UNUSED_RESULT
XDateTime *x_date_time_add_minutes(XDateTime *datetime, xint minutes);

XLIB_AVAILABLE_IN_ALL
X_GNUC_WARN_UNUSED_RESULT
XDateTime *x_date_time_add_seconds(XDateTime *datetime, xdouble seconds);

XLIB_AVAILABLE_IN_ALL
X_GNUC_WARN_UNUSED_RESULT
XDateTime *x_date_time_add_full(XDateTime *datetime, xint years, xint months, xint days, xint hours, xint minutes, xdouble seconds);

XLIB_AVAILABLE_IN_ALL
xint x_date_time_compare(xconstpointer dt1, xconstpointer dt2);

XLIB_AVAILABLE_IN_ALL
XTimeSpan x_date_time_difference(XDateTime *end, XDateTime *begin);

XLIB_AVAILABLE_IN_ALL
xuint x_date_time_hash(xconstpointer datetime);

XLIB_AVAILABLE_IN_ALL
xboolean x_date_time_equal(xconstpointer dt1, xconstpointer dt2);

XLIB_AVAILABLE_IN_ALL
void x_date_time_get_ymd(XDateTime *datetime, xint *year, xint *month, xint *day);

XLIB_AVAILABLE_IN_ALL
xint x_date_time_get_year(XDateTime *datetime);

XLIB_AVAILABLE_IN_ALL
xint x_date_time_get_month(XDateTime *datetime);

XLIB_AVAILABLE_IN_ALL
xint x_date_time_get_day_of_month(XDateTime *datetime);

XLIB_AVAILABLE_IN_ALL
xint x_date_time_get_week_numbering_year(XDateTime *datetime);

XLIB_AVAILABLE_IN_ALL
xint x_date_time_get_week_of_year(XDateTime *datetime);

XLIB_AVAILABLE_IN_ALL
xint x_date_time_get_day_of_week(XDateTime *datetime);

XLIB_AVAILABLE_IN_ALL
xint x_date_time_get_day_of_year(XDateTime *datetime);

XLIB_AVAILABLE_IN_ALL
xint x_date_time_get_hour(XDateTime *datetime);

XLIB_AVAILABLE_IN_ALL
xint x_date_time_get_minute(XDateTime *datetime);

XLIB_AVAILABLE_IN_ALL
xint x_date_time_get_second(XDateTime *datetime);

XLIB_AVAILABLE_IN_ALL
xint x_date_time_get_microsecond(XDateTime *datetime);

XLIB_AVAILABLE_IN_ALL
xdouble x_date_time_get_seconds(XDateTime *datetime);

XLIB_AVAILABLE_IN_ALL
xint64 x_date_time_to_unix(XDateTime *datetime);

X_GNUC_BEGIN_IGNORE_DEPRECATIONS
XLIB_DEPRECATED_IN_2_62_FOR(x_date_time_to_unix)
xboolean x_date_time_to_timeval(XDateTime *datetime, XTimeVal *tv);
X_GNUC_END_IGNORE_DEPRECATIONS

XLIB_AVAILABLE_IN_ALL
XTimeSpan x_date_time_get_utc_offset(XDateTime *datetime);

XLIB_AVAILABLE_IN_2_58
XTimeZone *x_date_time_get_timezone(XDateTime *datetime);

XLIB_AVAILABLE_IN_ALL
const xchar *x_date_time_get_timezone_abbreviation(XDateTime *datetime);

XLIB_AVAILABLE_IN_ALL
xboolean x_date_time_is_daylight_savings(XDateTime *datetime);

XLIB_AVAILABLE_IN_ALL
XDateTime *x_date_time_to_timezone(XDateTime *datetime, XTimeZone *tz);

XLIB_AVAILABLE_IN_ALL
XDateTime *x_date_time_to_local(XDateTime *datetime);

XLIB_AVAILABLE_IN_ALL
XDateTime *x_date_time_to_utc(XDateTime *datetime);

XLIB_AVAILABLE_IN_ALL
xchar *x_date_time_format(XDateTime *datetime, const xchar *format) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_2_62
xchar *x_date_time_format_iso8601(XDateTime *datetime) X_GNUC_MALLOC;

X_END_DECLS

#endif
