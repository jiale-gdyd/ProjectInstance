#ifndef _GNU_SOURCE
#define _GNU_SOURCE         1
#endif

#ifndef __USE_GNU
#define __USE_GNU
#endif

#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include "langinfo.h"
#include <sys/time.h>

#include <xlib/xlib/config.h>

#include <xlib/xlib/xhash.h>
#include <xlib/xlib/xmain.h>
#include <xlib/xlib/xslice.h>
#include <xlib/xlib/xatomic.h>
#include <xlib/xlib/xthread.h>
#include <xlib/xlib/xcharset.h>
#include <xlib/xlib/xconvert.h>
#include <xlib/xlib/xlibintl.h>
#include <xlib/xlib/xdatetime.h>
#include <xlib/xlib/xstrfuncs.h>
#include <xlib/xlib/xtimezone.h>
#include <xlib/xlib/xfileutils.h>
#include <xlib/xlib/xtestutils.h>
#include <xlib/xlib/xmappedfile.h>
#include <xlib/xlib/xconvertprivate.h>
#include <xlib/xlib/xcharsetprivate.h>
#include <xlib/xlib/xdatetime-private.h>

struct _XDateTime {
    xuint64   usec;
    XTimeZone *tz;
    xint      interval;
    xint32    days;
    xint      ref_count;
};

#define DAYS_IN_4YEARS                                  1461
#define DAYS_IN_100YEARS                                36524
#define DAYS_IN_400YEARS                                146097

#define USEC_PER_SECOND                                 (X_XINT64_CONSTANT(1000000))
#define USEC_PER_MINUTE                                 (X_XINT64_CONSTANT(60000000))
#define USEC_PER_HOUR                                   (X_XINT64_CONSTANT(3600000000))
#define USEC_PER_MILLISECOND                            (X_XINT64_CONSTANT(1000))
#define USEC_PER_DAY                                    (X_XINT64_CONSTANT(86400000000))
#define SEC_PER_DAY                                     (X_XINT64_CONSTANT(86400))

#define GREGORIAN_LEAP(y)                               ((((y) % 4) == 0) && (!((((y) % 100) == 0) && (((y) % 400) != 0))))
#define JULIAN_YEAR(d)                                  ((d)->julian / 365.25)
#define DAYS_PER_PERIOD                                 (X_XINT64_CONSTANT(2914695))

#define SECS_PER_MINUTE                                 (60)
#define SECS_PER_HOUR                                   (60 * SECS_PER_MINUTE)
#define SECS_PER_DAY                                    (24 * SECS_PER_HOUR)
#define SECS_PER_YEAR                                   (365 * SECS_PER_DAY)
#define SECS_PER_JULIAN                                 (DAYS_PER_PERIOD * SECS_PER_DAY)

#define UNIX_EPOCH_START                                719163
#define INSTANT_TO_UNIX(instant)                        ((instant)/USEC_PER_SECOND - UNIX_EPOCH_START * SEC_PER_DAY)
#define INSTANT_TO_UNIX_USECS(instant)                  ((instant) - UNIX_EPOCH_START * SEC_PER_DAY * USEC_PER_SECOND)
#define UNIX_TO_INSTANT(unix)                           (((xint64)(unix) + UNIX_EPOCH_START * SEC_PER_DAY) * USEC_PER_SECOND)
#define UNIX_USECS_TO_INSTANT(unix_usecs)               ((xint64)(unix_usecs) + UNIX_EPOCH_START * SEC_PER_DAY * USEC_PER_SECOND)
#define UNIX_TO_INSTANT_IS_VALID(unix)                  ((xint64)(unix) <= INSTANT_TO_UNIX(X_MAXINT64))
#define UNIX_USECS_TO_INSTANT_IS_VALID(unix_usecs)      ((xint64)(unix_usecs) <= INSTANT_TO_UNIX_USECS(X_MAXINT64))

static const xuint16 days_in_months[2][13] = {
    { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
    { 0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
};

static const xuint16 days_in_year[2][13] = {
    {  0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 },
    {  0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 }
};

#define GET_AMPM(d)                                     ((x_date_time_get_hour(d) < 12) ? nl_langinfo(AM_STR) : nl_langinfo(PM_STR))
#define GET_AMPM_IS_LOCALE                              TRUE

#define PREFERRED_DATE_TIME_FMT                         nl_langinfo(D_T_FMT)
#define PREFERRED_DATE_FMT                              nl_langinfo(D_FMT)
#define PREFERRED_TIME_FMT                              nl_langinfo(T_FMT)
#define PREFERRED_12HR_TIME_FMT                         nl_langinfo(T_FMT_AMPM)

static const xint weekday_item[2][7] = {
    { ABDAY_2, ABDAY_3, ABDAY_4, ABDAY_5, ABDAY_6, ABDAY_7, ABDAY_1 },
    { DAY_2, DAY_3, DAY_4, DAY_5, DAY_6, DAY_7, DAY_1 }
};

static const xint month_item[2][12] = {
    { ABMON_1, ABMON_2, ABMON_3, ABMON_4, ABMON_5, ABMON_6, ABMON_7, ABMON_8, ABMON_9, ABMON_10, ABMON_11, ABMON_12 },
    { MON_1, MON_2, MON_3, MON_4, MON_5, MON_6, MON_7, MON_8, MON_9, MON_10, MON_11, MON_12 },
};

#define WEEKDAY_ABBR(d)                                 nl_langinfo(weekday_item[0][x_date_time_get_day_of_week(d) - 1])
#define WEEKDAY_ABBR_IS_LOCALE                          TRUE
#define WEEKDAY_FULL(d)                                 nl_langinfo(weekday_item[1][x_date_time_get_day_of_week(d) - 1])
#define WEEKDAY_FULL_IS_LOCALE                          TRUE
#define MONTH_ABBR(d)                                   nl_langinfo(month_item[0][x_date_time_get_month(d) - 1])
#define MONTH_ABBR_IS_LOCALE                            TRUE
#define MONTH_FULL(d)                                   nl_langinfo(month_item[1][x_date_time_get_month(d) - 1])
#define MONTH_FULL_IS_LOCALE                            TRUE

#define MONTH_FULL_WITH_DAY(d)                          MONTH_FULL(d)
#define MONTH_FULL_WITH_DAY_IS_LOCALE                   MONTH_FULL_IS_LOCALE

static const xint alt_month_item[12] = {
    ALTMON_1, ALTMON_2, ALTMON_3, ALTMON_4, ALTMON_5, ALTMON_6,
    ALTMON_7, ALTMON_8, ALTMON_9, ALTMON_10, ALTMON_11, ALTMON_12
};


#define MONTH_FULL_STANDALONE(d)                        nl_langinfo(alt_month_item[x_date_time_get_month(d) - 1])
#define MONTH_FULL_STANDALONE_IS_LOCALE                 TRUE

#define MONTH_ABBR_WITH_DAY(d)                          MONTH_ABBR(d)
#define MONTH_ABBR_WITH_DAY_IS_LOCALE                   MONTH_ABBR_IS_LOCALE

static const xint ab_alt_month_item[12] = {
    _NL_ABALTMON_1, _NL_ABALTMON_2, _NL_ABALTMON_3, _NL_ABALTMON_4,
    _NL_ABALTMON_5, _NL_ABALTMON_6, _NL_ABALTMON_7, _NL_ABALTMON_8,
    _NL_ABALTMON_9, _NL_ABALTMON_10, _NL_ABALTMON_11, _NL_ABALTMON_12
};

#define MONTH_ABBR_STANDALONE(d)                        nl_langinfo(ab_alt_month_item[x_date_time_get_month(d) - 1])
#define MONTH_ABBR_STANDALONE_IS_LOCALE                 TRUE

#ifdef HAVE_LANGINFO_ERA
#define PREFERRED_ERA_DATE_TIME_FMT                     nl_langinfo(ERA_D_T_FMT)
#define PREFERRED_ERA_DATE_FMT                          nl_langinfo(ERA_D_FMT)
#define PREFERRED_ERA_TIME_FMT                          nl_langinfo(ERA_T_FMT)

#define ERA_DESCRIPTION                                 nl_langinfo(ERA)
#define ERA_DESCRIPTION_IS_LOCALE                       TRUE
#define ERA_DESCRIPTION_N_SEGMENTS                      (int)(xintptr)nl_langinfo(_NL_TIME_ERA_NUM_ENTRIES)

#else

#define PREFERRED_ERA_DATE_TIME_FMT                     PREFERRED_DATE_TIME_FMT
#define PREFERRED_ERA_DATE_FMT                          PREFERRED_DATE_FMT
#define PREFERRED_ERA_TIME_FMT                          PREFERRED_TIME_FMT

#define ERA_DESCRIPTION                                 NULL
#define ERA_DESCRIPTION_IS_LOCALE                       FALSE
#define ERA_DESCRIPTION_N_SEGMENTS                      0
#endif 

static const xchar *get_fallback_ampm(xint hour)
{
    if (hour < 12) {
        return C_("XDateTime", "AM");
    } else {
        return C_("XDateTime", "PM");
    }
}

static inline xint ymd_to_days(xint year, xint month, xint day)
{
    xint64 days;

    days = ((xint64) year - 1) * 365 + ((year - 1) / 4) - ((year - 1) / 100) + ((year - 1) / 400);

    days += days_in_year[0][month - 1];
    if (GREGORIAN_LEAP(year) && month > 2) {
        day++;
    }
    days += day;

    return days;
}

static void x_date_time_get_week_number(XDateTime *datetime, xint *week_number, xint *day_of_week, xint *day_of_year)
{
    xint a, b, c, d, e, f, g, n, s, month = -1, day = -1, year = -1;

    x_date_time_get_ymd(datetime, &year, &month, &day);

    if (month <= 2) {
        a = x_date_time_get_year(datetime) - 1;
        b = (a / 4) - (a / 100) + (a / 400);
        c = ((a - 1) / 4) - ((a - 1) / 100) + ((a - 1) / 400);
        s = b - c;
        e = 0;
        f = day - 1 + (31 * (month - 1));
    } else {
        a = year;
        b = (a / 4) - (a / 100) + (a / 400);
        c = ((a - 1) / 4) - ((a - 1) / 100) + ((a - 1) / 400);
        s = b - c;
        e = s + 1;
        f = day + (((153 * (month - 3)) + 2) / 5) + 58 + s;
    }

    g = (a + b) % 7;
    d = (f + g - e) % 7;
    n = f + 3 - d;

    if (week_number) {
        if (n < 0) {
            *week_number = 53 - ((g - s) / 5);
        } else if (n > 364 + s) {
            *week_number = 1;
        } else {
            *week_number = (n / 7) + 1;
        }
    }

    if (day_of_week) {
        *day_of_week = d + 1;
    }

    if (day_of_year) {
        *day_of_year = f + 1;
    }
}

static XDateTime *x_date_time_alloc(XTimeZone *tz)
{
    XDateTime *datetime;

    datetime = x_slice_new0(XDateTime);
    datetime->tz = x_time_zone_ref(tz);
    datetime->ref_count = 1;

    return datetime;
}

XDateTime *x_date_time_ref(XDateTime *datetime)
{
    x_return_val_if_fail(datetime != NULL, NULL);
    x_return_val_if_fail(datetime->ref_count > 0, NULL);

    x_atomic_int_inc(&datetime->ref_count);
    return datetime;
}

void x_date_time_unref(XDateTime *datetime)
{
    x_return_if_fail(datetime != NULL);
    x_return_if_fail(datetime->ref_count > 0);

    if (x_atomic_int_dec_and_test(&datetime->ref_count)) {
        x_time_zone_unref(datetime->tz);
        x_slice_free(XDateTime, datetime);
    }
}

static xint64 x_date_time_to_instant(XDateTime *datetime)
{
    xint64 offset;

    offset = x_time_zone_get_offset(datetime->tz, datetime->interval);
    offset *= USEC_PER_SECOND;

    return datetime->days * USEC_PER_DAY + datetime->usec - offset;
}

static XDateTime *x_date_time_from_instant(XTimeZone *tz, xint64 instant)
{
    xint64 offset;
    XDateTime *datetime;

    if (instant < 0 || instant > X_XINT64_CONSTANT(1000000000000000000)) {
        return NULL;
    }

    datetime = x_date_time_alloc(tz);
    datetime->interval = x_time_zone_find_interval(tz, X_TIME_TYPE_UNIVERSAL, INSTANT_TO_UNIX(instant));
    offset = x_time_zone_get_offset(datetime->tz, datetime->interval);
    offset *= USEC_PER_SECOND;

    instant += offset;

    datetime->days = instant / USEC_PER_DAY;
    datetime->usec = instant % USEC_PER_DAY;

    if (datetime->days < 1 || 3652059 < datetime->days) {
        x_date_time_unref(datetime);
        datetime = NULL;
    }

    return datetime;
}

static xboolean x_date_time_deal_with_date_change(XDateTime *datetime)
{
    xint64 usec;
    xint64 full_time;
    XTimeType was_dst;

    if (datetime->days < 1 || datetime->days > 3652059) {
        return FALSE;
    }

    was_dst = (XTimeType)x_time_zone_is_dst(datetime->tz, datetime->interval);
    full_time = datetime->days * USEC_PER_DAY + datetime->usec;

    usec = full_time % USEC_PER_SECOND;
    full_time /= USEC_PER_SECOND;
    full_time -= UNIX_EPOCH_START * SEC_PER_DAY;

    datetime->interval = x_time_zone_adjust_time(datetime->tz, was_dst, &full_time);
    full_time += UNIX_EPOCH_START * SEC_PER_DAY;
    full_time *= USEC_PER_SECOND;
    full_time += usec;

    datetime->days = full_time / USEC_PER_DAY;
    datetime->usec = full_time % USEC_PER_DAY;

    return TRUE;
}

static XDateTime *x_date_time_replace_days(XDateTime *datetime, xint days)
{
    XDateTime *newt;

    newt = x_date_time_alloc(datetime->tz);
    newt->interval = datetime->interval;
    newt->usec = datetime->usec;
    newt->days = days;

    if (!x_date_time_deal_with_date_change(newt)) {
        x_date_time_unref(newt);
        newt = NULL;
    }

    return newt;
}

X_GNUC_BEGIN_IGNORE_DEPRECATIONS
static XDateTime *x_date_time_new_from_timeval(XTimeZone *tz, const XTimeVal *tv)
{
    xint64 tv_sec = tv->tv_sec;

    if (tv_sec > X_MAXINT64 - 1 || !UNIX_TO_INSTANT_IS_VALID(tv_sec + 1)) {
        return NULL;
    }

    return x_date_time_from_instant(tz, tv->tv_usec + UNIX_TO_INSTANT(tv->tv_sec));
}
X_GNUC_END_IGNORE_DEPRECATIONS

static XDateTime *x_date_time_new_from_unix(XTimeZone *tz, xint64 usecs)
{
    if (!UNIX_USECS_TO_INSTANT_IS_VALID(usecs)) {
        return NULL;
    }

    return x_date_time_from_instant(tz, UNIX_USECS_TO_INSTANT(usecs));
}

XDateTime *x_date_time_new_now(XTimeZone *tz)
{
    xint64 now_us;

    x_return_val_if_fail(tz != NULL, NULL);

    now_us = x_get_real_time();
    return x_date_time_new_from_unix(tz, now_us);
}

XDateTime *x_date_time_new_now_local(void)
{
    XTimeZone *local;
    XDateTime *datetime;

    local = x_time_zone_new_local();
    datetime = x_date_time_new_now(local);
    x_time_zone_unref(local);

    return datetime;
}

XDateTime *x_date_time_new_now_utc(void)
{
    XTimeZone *utc;
    XDateTime *datetime;

    utc = x_time_zone_new_utc();
    datetime = x_date_time_new_now(utc);
    x_time_zone_unref(utc);

    return datetime;
}

XDateTime *x_date_time_new_from_unix_local(xint64 t)
{
    XTimeZone *local;
    XDateTime *datetime;

    if (t > X_MAXINT64 / USEC_PER_SECOND || t < X_MININT64 / USEC_PER_SECOND) {
        return NULL;
    }

    return x_date_time_new_from_unix_local_usec(t * USEC_PER_SECOND);
}

XDateTime *x_date_time_new_from_unix_local_usec(xint64 usecs)
{
    XTimeZone *local;
    XDateTime *datetime;

    local = x_time_zone_new_local();
    datetime = x_date_time_new_from_unix(local, usecs);
    x_time_zone_unref(local);

    return datetime;
}

XDateTime *x_date_time_new_from_unix_utc(xint64 t)
{
    if (t > X_MAXINT64 / USEC_PER_SECOND || t < X_MININT64 / USEC_PER_SECOND) {
        return NULL;
    }

    return x_date_time_new_from_unix_utc_usec(t * USEC_PER_SECOND);
}

XDateTime *x_date_time_new_from_unix_utc_usec(xint64 usecs)
{
    XTimeZone *utc;
    XDateTime *datetime;

    utc = x_time_zone_new_utc();
    datetime = x_date_time_new_from_unix(utc, usecs);
    x_time_zone_unref(utc);

    return datetime;
}

X_GNUC_BEGIN_IGNORE_DEPRECATIONS
XDateTime *x_date_time_new_from_timeval_local(const XTimeVal *tv)
{
    XTimeZone *local;
    XDateTime *datetime;

    local = x_time_zone_new_local();
    datetime = x_date_time_new_from_timeval(local, tv);
    x_time_zone_unref(local);

    return datetime;
}
X_GNUC_END_IGNORE_DEPRECATIONS

X_GNUC_BEGIN_IGNORE_DEPRECATIONS
XDateTime *x_date_time_new_from_timeval_utc(const XTimeVal *tv)
{
    XTimeZone *utc;
    XDateTime *datetime;

    utc = x_time_zone_new_utc();
    datetime = x_date_time_new_from_timeval(utc, tv);
    x_time_zone_unref(utc);

    return datetime;
}
X_GNUC_END_IGNORE_DEPRECATIONS

static xboolean get_iso8601_int(const xchar *text, xsize length, xint *value)
{
    xsize i;
    xuint v = 0;

    if (length < 1 || length > 4) {
        return FALSE;
    }

    for (i = 0; i < length; i++) {
        const xchar c = text[i];
        if (c < '0' || c > '9') {
            return FALSE;
        }

        v = v * 10 + (c - '0');
    }

    *value = v;
    return TRUE;
}

static xboolean get_iso8601_seconds(const xchar *text, xsize length, xdouble *value)
{
    xsize i;
    xuint64 divisor = 1, v = 0;

    if (length < 2) {
        return FALSE;
    }

    for (i = 0; i < 2; i++) {
        const xchar c = text[i];
        if (c < '0' || c > '9') {
            return FALSE;
        }

        v = v * 10 + (c - '0');
    }

    if (length > 2 && !(text[i] == '.' || text[i] == ',')) {
        return FALSE;
    }

    if (v >= 60.0 && v <= 61.0) {
        v = 59.0;
    }

    i++;
    if (i == length) {
        return FALSE;
    }

    for (; i < length; i++) {
        const xchar c = text[i];
        if (c < '0' || c > '9' || v > (X_MAXUINT64 - (c - '0')) / 10 || divisor > X_MAXUINT64 / 10) {
            return FALSE;
        }

        v = v * 10 + (c - '0');
        divisor *= 10;
    }

    *value = (xdouble) v / divisor;
    return TRUE;
}

static XDateTime *x_date_time_new_ordinal(XTimeZone *tz, xint year, xint ordinal_day, xint hour, xint minute, xdouble seconds)
{
    XDateTime *dt;

    if (ordinal_day < 1 || ordinal_day > (GREGORIAN_LEAP(year) ? 366 : 365)) {
        return NULL;
    }

    dt = x_date_time_new(tz, year, 1, 1, hour, minute, seconds);
    if (dt == NULL) {
        return NULL;
    }
    dt->days += ordinal_day - 1;

    return dt;
}

static XDateTime *x_date_time_new_week(XTimeZone *tz, xint year, xint week, xint week_day, xint hour, xint minute, xdouble seconds)
{
    xint64 p;
    XDateTime *dt;
    xint max_week, jan4_week_day, ordinal_day;

    p = (year * 365 + (year / 4) - (year / 100) + (year / 400)) % 7;
    max_week = p == 4 ? 53 : 52;

    if (week < 1 || week > max_week || week_day < 1 || week_day > 7) {
        return NULL;
    }

    dt = x_date_time_new(tz, year, 1, 4, 0, 0, 0);
    if (dt == NULL) {
        return NULL;
    }

    x_date_time_get_week_number(dt, NULL, &jan4_week_day, NULL);
    x_date_time_unref(dt);

    ordinal_day = (week * 7) + week_day - (jan4_week_day + 3);
    if (ordinal_day < 0) {
        year--;
        ordinal_day += GREGORIAN_LEAP(year) ? 366 : 365;
    } else if (ordinal_day > (GREGORIAN_LEAP(year) ? 366 : 365)) {
        ordinal_day -= (GREGORIAN_LEAP(year) ? 366 : 365);
        year++;
    }

    return x_date_time_new_ordinal(tz, year, ordinal_day, hour, minute, seconds);
}

static XDateTime *parse_iso8601_date(const xchar *text, xsize length, xint hour, xint minute, xdouble seconds, XTimeZone *tz)
{
    if (length == 10 && text[4] == '-' && text[7] == '-') {
        int year, month, day;
        if (!get_iso8601_int(text, 4, &year) || !get_iso8601_int(text + 5, 2, &month) || !get_iso8601_int(text + 8, 2, &day)) {
            return NULL;
        }

        return x_date_time_new(tz, year, month, day, hour, minute, seconds);
    } else if (length == 8 && text[4] == '-') {
        xint year, ordinal_day;
        if (!get_iso8601_int(text, 4, &year) || !get_iso8601_int(text + 5, 3, &ordinal_day)) {
            return NULL;
        }

        return x_date_time_new_ordinal(tz, year, ordinal_day, hour, minute, seconds);
    } else if (length == 10 && text[4] == '-' && text[5] == 'W' && text[8] == '-')  {
        xint year, week, week_day;
        if (!get_iso8601_int(text, 4, &year) || !get_iso8601_int(text + 6, 2, &week) || !get_iso8601_int(text + 9, 1, &week_day)) {
            return NULL;
        }

        return x_date_time_new_week(tz, year, week, week_day, hour, minute, seconds);
    } else if (length == 8 && text[4] == 'W') {
        xint year, week, week_day;
        if (!get_iso8601_int(text, 4, &year) || !get_iso8601_int(text + 5, 2, &week) || !get_iso8601_int(text + 7, 1, &week_day)) {
            return NULL;
        }

        return x_date_time_new_week(tz, year, week, week_day, hour, minute, seconds);
    } else if (length == 8) {
        int year, month, day;
        if (!get_iso8601_int(text, 4, &year) || !get_iso8601_int(text + 4, 2, &month) || !get_iso8601_int(text + 6, 2, &day)) {
            return NULL;
        }

        return x_date_time_new(tz, year, month, day, hour, minute, seconds);
    } else if (length == 7) {
        xint year, ordinal_day;
        if (!get_iso8601_int(text, 4, &year) || !get_iso8601_int(text + 4, 3, &ordinal_day)) {
            return NULL;
        }

        return x_date_time_new_ordinal(tz, year, ordinal_day, hour, minute, seconds);
    } else {
        return FALSE;
    }
}

static XTimeZone *parse_iso8601_timezone(const xchar *text, xsize length, xssize *tz_offset)
{
    XTimeZone *tz;
    xint offset_sign = 1;
    xint i, tz_length, offset_hours, offset_minutes;

    if (length > 0 && text[length - 1] == 'Z') {
        *tz_offset = length - 1;
        return x_time_zone_new_utc();
    }

    for (i = length - 1; i >= 0; i--) {
        if (text[i] == '+' || text[i] == '-') {
            offset_sign = text[i] == '-' ? -1 : 1;
            break;
        }
    }

    if (i < 0) {
        return NULL;
    }

    tz_length = length - i;
    if (tz_length == 6 && text[i+3] == ':') {
        if (!get_iso8601_int(text + i + 1, 2, &offset_hours) || !get_iso8601_int(text + i + 4, 2, &offset_minutes)) {
            return NULL;
        }
    } else if (tz_length == 5) {
        if (!get_iso8601_int(text + i + 1, 2, &offset_hours) || !get_iso8601_int(text + i + 3, 2, &offset_minutes)) {
            return NULL;
        }
    } else if (tz_length == 3) {
        if (!get_iso8601_int(text + i + 1, 2, &offset_hours)) {
            return NULL;
        }

        offset_minutes = 0;
    } else {
        return NULL;
    }

    *tz_offset = i;
    tz = x_time_zone_new_identifier(text + i);

    if (tz == NULL || x_time_zone_get_offset(tz, 0) != offset_sign * (offset_hours * 3600 + offset_minutes * 60)) {
        x_clear_pointer(&tz, x_time_zone_unref);
        return NULL;
    }

    return tz;
}

static xboolean parse_iso8601_time(const xchar *text, xsize length, xint *hour, xint *minute, xdouble *seconds, XTimeZone **tz)
{
    xssize tz_offset = -1;

    *tz = parse_iso8601_timezone(text, length, &tz_offset);
    if (tz_offset >= 0) {
        length = tz_offset;
    }

    if (length >= 8 && text[2] == ':' && text[5] == ':') {
        return get_iso8601_int(text, 2, hour) && get_iso8601_int(text + 3, 2, minute) && get_iso8601_seconds (text + 6, length - 6, seconds);
    } else if (length >= 6) {
        return get_iso8601_int(text, 2, hour) && get_iso8601_int(text + 2, 2, minute) && get_iso8601_seconds (text + 4, length - 4, seconds);
    } else {
        return FALSE;
    }
}

XDateTime *x_date_time_new_from_iso8601(const xchar *text, XTimeZone *default_tz)
{
    XTimeZone *tz = NULL;
    xdouble seconds = 0.0;
    xint hour = 0, minute = 0;
    XDateTime *datetime = NULL;
    xint length, date_length = -1;

    x_return_val_if_fail(text != NULL, NULL);

    for (length = 0; text[length] != '\0'; length++) {
        if (date_length < 0 && (text[length] == 'T' || text[length] == 't' || text[length] == ' ')) {
            date_length = length;
        }
    }

    if (date_length < 0) {
        return NULL;
    }

    if (!parse_iso8601_time(text + date_length + 1, length - (date_length + 1), &hour, &minute, &seconds, &tz)) {
        goto out;
    }

    if (tz == NULL && default_tz == NULL) {
        return NULL;
    }

    datetime = parse_iso8601_date(text, date_length, hour, minute, seconds, tz ? tz : default_tz);

out:
    if (tz != NULL) {
        x_time_zone_unref(tz);
    }

    return datetime;
}

XDateTime *x_date_time_new(XTimeZone *tz, xint year, xint month, xint day, xint hour, xint minute, xdouble seconds)
{
    xint64 full_time;
    XDateTime *datetime;
    volatile xint64 usec;
    volatile xdouble usecd;

    x_return_val_if_fail(tz != NULL, NULL);

    if (year < 1 || year > 9999 ||
        month < 1 || month > 12 ||
        day < 1 || day > days_in_months[GREGORIAN_LEAP(year)][month] ||
        hour < 0 || hour > 23 ||
        minute < 0 || minute > 59 ||
        isnan (seconds) ||
        seconds < 0.0 || seconds >= 60.0)
    {
        return NULL;
    }

    datetime = x_date_time_alloc(tz);
    datetime->days = ymd_to_days(year, month, day);
    datetime->usec = (hour * USEC_PER_HOUR) + (minute * USEC_PER_MINUTE) + (xint64) (seconds * USEC_PER_SECOND);

    full_time = SEC_PER_DAY * (ymd_to_days(year, month, day) - UNIX_EPOCH_START) + SECS_PER_HOUR * hour + SECS_PER_MINUTE * minute + (int)seconds;
    datetime->interval = x_time_zone_adjust_time(datetime->tz, X_TIME_TYPE_STANDARD, &full_time);

    usec = seconds * USEC_PER_SECOND;
    usecd = (usec + 1) * 1e-6;
    if (usecd <= seconds) {
        usec++;
    }

    full_time += UNIX_EPOCH_START * SEC_PER_DAY;
    datetime->days = full_time / SEC_PER_DAY;
    datetime->usec = (full_time % SEC_PER_DAY) * USEC_PER_SECOND;
    datetime->usec += usec % USEC_PER_SECOND;

    return datetime;
}

XDateTime *x_date_time_new_local(xint year, xint month, xint day, xint hour, xint minute, xdouble seconds)
{
    XTimeZone *local;
    XDateTime *datetime;

    local = x_time_zone_new_local();
    datetime = x_date_time_new(local, year, month, day, hour, minute, seconds);
    x_time_zone_unref(local);

    return datetime;
}

XDateTime *x_date_time_new_utc(xint year, xint month, xint day, xint hour, xint minute, xdouble seconds)
{
    XTimeZone *utc;
    XDateTime *datetime;

    utc = x_time_zone_new_utc();
    datetime = x_date_time_new(utc, year, month, day, hour, minute, seconds);
    x_time_zone_unref(utc);

    return datetime;
}

XDateTime *x_date_time_add(XDateTime *datetime, XTimeSpan timespan)
{
    x_return_val_if_fail(datetime != NULL, NULL);
    return x_date_time_from_instant(datetime->tz, timespan + x_date_time_to_instant(datetime));
}

XDateTime *x_date_time_add_years(XDateTime *datetime, xint years)
{
    xint year, month, day;

    x_return_val_if_fail(datetime != NULL, NULL);

    if (years < -10000 || years > 10000) {
        return NULL;
    }

    x_date_time_get_ymd(datetime, &year, &month, &day);
    year += years;

    if (month == 2 && day == 29 && !GREGORIAN_LEAP(year)) {
        day = 28;
    }

    return x_date_time_replace_days(datetime, ymd_to_days(year, month, day));
}

XDateTime *x_date_time_add_months(XDateTime *datetime, xint months)
{
    xint year, month, day;

    x_return_val_if_fail(datetime != NULL, NULL);
    x_date_time_get_ymd(datetime, &year, &month, &day);

    if (months < -120000 || months > 120000) {
        return NULL;
    }

    year += months / 12;
    month += months % 12;
    if (month < 1) {
        month += 12;
        year--;
    } else if (month > 12) {
        month -= 12;
        year++;
    }

    day = MIN(day, days_in_months[GREGORIAN_LEAP(year)][month]);

    return x_date_time_replace_days(datetime, ymd_to_days(year, month, day));
}

XDateTime *x_date_time_add_weeks(XDateTime *datetime, xint weeks)
{
    x_return_val_if_fail(datetime != NULL, NULL);
    return x_date_time_add_days(datetime, weeks * 7);
}

XDateTime *x_date_time_add_days(XDateTime *datetime, xint days)
{
    x_return_val_if_fail(datetime != NULL, NULL);

    if (days < -3660000 || days > 3660000) {
        return NULL;
    }

    return x_date_time_replace_days(datetime, datetime->days + days);
}

XDateTime *x_date_time_add_hours(XDateTime *datetime, xint hours)
{
    return x_date_time_add(datetime, hours * USEC_PER_HOUR);
}

XDateTime *x_date_time_add_minutes(XDateTime *datetime, xint minutes)
{
    return x_date_time_add(datetime, minutes * USEC_PER_MINUTE);
}

XDateTime *x_date_time_add_seconds(XDateTime *datetime, xdouble seconds)
{
    return x_date_time_add(datetime, seconds * USEC_PER_SECOND);
}

XDateTime *x_date_time_add_full(XDateTime *datetime, xint years, xint months, xint days, xint hours, xint minutes, xdouble seconds)
{
    xint interval;
    XDateTime *newt;
    xint64 full_time;
    xint year, month, day;

    x_return_val_if_fail(datetime != NULL, NULL);
    x_date_time_get_ymd(datetime, &year, &month, &day);

    months += years * 12;

    if (months < -120000 || months > 120000) {
        return NULL;
    }

    if (days < -3660000 || days > 3660000) {
        return NULL;
    }

    year += months / 12;
    month += months % 12;
    if (month < 1) {
        month += 12;
        year--;
    } else if (month > 12) {
        month -= 12;
        year++;
    }

    day = MIN(day, days_in_months[GREGORIAN_LEAP(year)][month]);

    full_time = datetime->usec / USEC_PER_SECOND + SEC_PER_DAY * (ymd_to_days(year, month, day) + days - UNIX_EPOCH_START);
    interval = x_time_zone_adjust_time(datetime->tz, (XTimeType)x_time_zone_is_dst(datetime->tz, datetime->interval), &full_time);

    full_time -= x_time_zone_get_offset(datetime->tz, interval);
    full_time += UNIX_EPOCH_START * SEC_PER_DAY;
    full_time = full_time * USEC_PER_SECOND + datetime->usec % USEC_PER_SECOND;
    full_time += (hours * USEC_PER_HOUR) + (minutes * USEC_PER_MINUTE) + (xint64)(seconds * USEC_PER_SECOND);

    interval = x_time_zone_find_interval(datetime->tz, X_TIME_TYPE_UNIVERSAL, INSTANT_TO_UNIX(full_time));
    full_time += USEC_PER_SECOND * x_time_zone_get_offset(datetime->tz, interval);

    newt = x_date_time_alloc(datetime->tz);
    newt->interval = interval;
    newt->days = full_time / USEC_PER_DAY;
    newt->usec = full_time % USEC_PER_DAY;

    return newt;
}

xint x_date_time_compare(xconstpointer dt1, xconstpointer dt2)
{
    xint64 difference;

    difference = x_date_time_difference((XDateTime *)dt1, (XDateTime *)dt2);
    if (difference < 0) {
        return -1;
    } else if (difference > 0) {
        return 1;
    } else {
        return 0;
    }
}

XTimeSpan x_date_time_difference(XDateTime *end, XDateTime *begin)
{
    x_return_val_if_fail(begin != NULL, 0);
    x_return_val_if_fail(end != NULL, 0);

    return x_date_time_to_instant(end) - x_date_time_to_instant(begin);
}

xuint x_date_time_hash (xconstpointer datetime)
{
    x_return_val_if_fail(datetime != NULL, 0);
    return x_date_time_to_instant((XDateTime *)datetime);
}

xboolean x_date_time_equal(xconstpointer dt1, xconstpointer dt2)
{
    return x_date_time_difference((XDateTime *)dt1, (XDateTime *)dt2) == 0;
}

void x_date_time_get_ymd(XDateTime *datetime, xint *year, xint *month, xint *day)
{
    xint the_day;
    xint the_year;
    xboolean leap;
    xint the_month;
    xint y4_cycles;
    xint y1_cycles;
    xint preceding;
    xint y100_cycles;
    xint remaining_days;

    x_return_if_fail(datetime != NULL);

    remaining_days = datetime->days;
    remaining_days--;

    the_year = (remaining_days / DAYS_IN_400YEARS) * 400 + 1;
    remaining_days = remaining_days % DAYS_IN_400YEARS;

    y100_cycles = remaining_days / DAYS_IN_100YEARS;
    remaining_days = remaining_days % DAYS_IN_100YEARS;
    the_year += y100_cycles * 100;

    y4_cycles = remaining_days / DAYS_IN_4YEARS;
    remaining_days = remaining_days % DAYS_IN_4YEARS;
    the_year += y4_cycles * 4;

    y1_cycles = remaining_days / 365;
    the_year += y1_cycles;
    remaining_days = remaining_days % 365;

    if (y1_cycles == 4 || y100_cycles == 4) {
        x_assert(remaining_days == 0);

        the_year--;
        the_month = 12;
        the_day = 31;
        goto end;
    }

    leap = y1_cycles == 3 && (y4_cycles != 24 || y100_cycles == 3);

    x_assert(leap == GREGORIAN_LEAP(the_year));

    the_month = (remaining_days + 50) >> 5;
    preceding = (days_in_year[0][the_month - 1] + (the_month > 2 && leap));
    if (preceding > remaining_days) {
        the_month -= 1;
        preceding -= leap ? days_in_months[1][the_month] : days_in_months[0][the_month];
    }

    remaining_days -= preceding;
    x_assert(0 <= remaining_days);

    the_day = remaining_days + 1;

end:
    if (year) {
        *year = the_year;
    }

    if (month) {
        *month = the_month;
    }

    if (day) {
        *day = the_day;
    }
}

xint x_date_time_get_year(XDateTime *datetime)
{
    xint year;

    x_return_val_if_fail(datetime != NULL, 0);
    x_date_time_get_ymd(datetime, &year, NULL, NULL);

    return year;
}

xint x_date_time_get_month(XDateTime *datetime)
{
    xint month;

    x_return_val_if_fail(datetime != NULL, 0);
    x_date_time_get_ymd(datetime, NULL, &month, NULL);

    return month;
}

xint x_date_time_get_day_of_month(XDateTime *datetime)
{
    xuint is_leap;
    xuint16 last = 0;
    xint day_of_year, i;

    x_return_val_if_fail(datetime != NULL, 0);

    is_leap = GREGORIAN_LEAP(x_date_time_get_year(datetime)) ? 1 : 0;
    x_date_time_get_week_number(datetime, NULL, NULL, &day_of_year);

    for (i = 1; i <= 12; i++) {
        if (days_in_year[is_leap][i] >= day_of_year) {
            return day_of_year - last;
        }

        last = days_in_year[is_leap][i];
    }

    x_warn_if_reached();
    return 0;
}

xint x_date_time_get_week_numbering_year(XDateTime *datetime)
{
    xint year = -1, month = -1, day = -1, weekday;

    x_date_time_get_ymd(datetime, &year, &month, &day);
    weekday = x_date_time_get_day_of_week(datetime);

    if (month == 1 && (day - weekday) <= -4) {
        return year - 1;
    } else if (month == 12 && (day - weekday) >= 28) {
        return year + 1;
    } else {
        return year;
    }
}

xint x_date_time_get_week_of_year(XDateTime *datetime)
{
    xint weeknum;

    x_return_val_if_fail(datetime != NULL, 0);
    x_date_time_get_week_number(datetime, &weeknum, NULL, NULL);

    return weeknum;
}

xint x_date_time_get_day_of_week(XDateTime *datetime)
{
    x_return_val_if_fail(datetime != NULL, 0);
    return (datetime->days - 1) % 7 + 1;
}

xint x_date_time_get_day_of_year(XDateTime *datetime)
{
    xint doy = 0;

    x_return_val_if_fail(datetime != NULL, 0);

    x_date_time_get_week_number(datetime, NULL, NULL, &doy);
    return doy;
}

xint x_date_time_get_hour(XDateTime *datetime)
{
    x_return_val_if_fail(datetime != NULL, 0);
    return (datetime->usec / USEC_PER_HOUR);
}

xint x_date_time_get_minute(XDateTime *datetime)
{
    x_return_val_if_fail(datetime != NULL, 0);
    return (datetime->usec % USEC_PER_HOUR) / USEC_PER_MINUTE;
}

xint x_date_time_get_second(XDateTime *datetime)
{
    x_return_val_if_fail(datetime != NULL, 0);
    return (datetime->usec % USEC_PER_MINUTE) / USEC_PER_SECOND;
}

xint x_date_time_get_microsecond(XDateTime *datetime)
{
    x_return_val_if_fail(datetime != NULL, 0);
    return (datetime->usec % USEC_PER_SECOND);
}

xdouble x_date_time_get_seconds(XDateTime *datetime)
{
    x_return_val_if_fail(datetime != NULL, 0);
    return (datetime->usec % USEC_PER_MINUTE) / 1000000.0;
}

xint64 x_date_time_to_unix(XDateTime *datetime)
{
    x_return_val_if_fail(datetime != NULL, 0);
    return INSTANT_TO_UNIX(x_date_time_to_instant(datetime));
}

xint64 x_date_time_to_unix_usec(XDateTime *datetime)
{
    x_return_val_if_fail(datetime != NULL, 0);
    return INSTANT_TO_UNIX_USECS(x_date_time_to_instant(datetime));
}

X_GNUC_BEGIN_IGNORE_DEPRECATIONS
xboolean x_date_time_to_timeval(XDateTime *datetime, XTimeVal *tv)
{
    x_return_val_if_fail(datetime != NULL, FALSE);

    tv->tv_sec = INSTANT_TO_UNIX(x_date_time_to_instant(datetime));
    tv->tv_usec = datetime->usec % USEC_PER_SECOND;

    return TRUE;
}
X_GNUC_END_IGNORE_DEPRECATIONS

XTimeSpan x_date_time_get_utc_offset(XDateTime *datetime)
{
    xint offset;

    x_return_val_if_fail(datetime != NULL, 0);
    offset = x_time_zone_get_offset(datetime->tz, datetime->interval);

    return (xint64)offset * USEC_PER_SECOND;
}

XTimeZone *x_date_time_get_timezone(XDateTime *datetime)
{
    x_return_val_if_fail(datetime != NULL, NULL);
    x_assert(datetime->tz != NULL);
    return datetime->tz;
}

const xchar *x_date_time_get_timezone_abbreviation(XDateTime *datetime)
{
    x_return_val_if_fail(datetime != NULL, NULL);
    return x_time_zone_get_abbreviation(datetime->tz, datetime->interval);
}

xboolean x_date_time_is_daylight_savings(XDateTime *datetime)
{
    x_return_val_if_fail(datetime != NULL, FALSE);
    return x_time_zone_is_dst(datetime->tz, datetime->interval);
}

XDateTime *x_date_time_to_timezone(XDateTime *datetime, XTimeZone *tz)
{
    x_return_val_if_fail(datetime != NULL, NULL);
    x_return_val_if_fail(tz != NULL, NULL);

    return x_date_time_from_instant(tz, x_date_time_to_instant(datetime));
}

XDateTime *x_date_time_to_local(XDateTime *datetime)
{
    XDateTime *newt;
    XTimeZone *local;

    local = x_time_zone_new_local();
    newt = x_date_time_to_timezone(datetime, local);
    x_time_zone_unref(local);

    return newt;
}

XDateTime *x_date_time_to_utc(XDateTime *datetime)
{
    XTimeZone *utc;
    XDateTime *newt;

    utc = x_time_zone_new_utc();
    newt = x_date_time_to_timezone(datetime, utc);
    x_time_zone_unref(utc);

    return newt;
}

static xboolean format_z(XString *outstr, xint offset, xuint colons)
{
    xint hours;
    xint minutes;
    xint seconds;
    xchar sign = offset >= 0 ? '+' : '-';

    offset = ABS (offset);
    hours = offset / 3600;
    minutes = offset / 60 % 60;
    seconds = offset % 60;

    switch (colons) {
        case 0:
            x_string_append_printf(outstr, "%c%02d%02d", sign, hours, minutes);
            break;

        case 1:
            x_string_append_printf(outstr, "%c%02d:%02d", sign, hours, minutes);
            break;

        case 2:
            x_string_append_printf(outstr, "%c%02d:%02d:%02d", sign, hours, minutes, seconds);
            break;

        case 3:
            x_string_append_printf(outstr, "%c%02d", sign, hours);
            if (minutes != 0 || seconds != 0) {
                x_string_append_printf(outstr, ":%02d", minutes);
                if (seconds != 0) {
                    x_string_append_printf(outstr, ":%02d", seconds);
                }
            }
            break;

        default:
            return FALSE;
    }

    return TRUE;
}

static const xchar *const *initialize_alt_digits(void)
{
    xuint i;
    xchar *digit;
    xsize digit_len;
    const xchar *locale_digit;
#define N_DIGITS 10
#define MAX_UTF8_ENCODING_LEN 4
    static xchar buffer[N_DIGITS * (MAX_UTF8_ENCODING_LEN + 1)];
#undef N_DIGITS
#undef MAX_UTF8_ENCODING_LEN

    xchar *buffer_end = buffer;
    static const xchar *alt_digits[10];

    for (i = 0; i != 10; ++i) {
        locale_digit = nl_langinfo(_NL_CTYPE_OUTDIGIT0_MB + i);
        if (x_strcmp0(locale_digit, "") == 0) {
            return NULL;
        }

        digit = _x_ctype_locale_to_utf8(locale_digit, -1, NULL, &digit_len, NULL);
        if (digit == NULL) {
            return NULL;
        }

        x_assert(digit_len < (xsize)(buffer + sizeof(buffer) - buffer_end));

        alt_digits[i] = buffer_end;
        buffer_end = x_stpcpy(buffer_end, digit);
        buffer_end += 1;

        x_free(digit);
    }

    return alt_digits;
}

static XEraDescriptionSegment *date_time_lookup_era(XDateTime *datetime, xboolean locale_is_utf8)
{
    XEraDate datetime_date;
    static XMutex era_mutex;
    XPtrArray *local_era_description;
    static XPtrArray *static_era_description = NULL;
    static const char *static_era_description_locale = NULL;
    const char *current_lc_time = setlocale(LC_TIME, NULL);

    x_mutex_lock(&era_mutex);

    if (static_era_description_locale != current_lc_time) {
        char *tmp = NULL;
        size_t era_description_str_len;
        const char *era_description_str;

        era_description_str = ERA_DESCRIPTION;
        if (era_description_str != NULL) {
            {
                const char *s = era_description_str;
                int n_entries = ERA_DESCRIPTION_N_SEGMENTS;

                for (int i = 1; i < n_entries; i++) {
                    const char *next_semicolon = strchr(s, ';');
                    const char *next_nul = strchr(s, '\0');

                    if (next_semicolon != NULL && next_semicolon < next_nul) {
                        s = next_semicolon + 1;
                    } else {
                        s = next_nul + 1;
                    }
                }

                era_description_str_len = strlen(s) + (s - era_description_str);
                era_description_str = tmp = x_memdup2(era_description_str, era_description_str_len + 1);
                s = era_description_str;

                for (int i = 1; i < n_entries; i++) {
                    char *next_nul = strchr(s, '\0');

                    if ((size_t)(next_nul - era_description_str) >= era_description_str_len) {
                        break;
                    }

                    *next_nul = ';';
                    s = next_nul + 1;
                }
            }

            if (!locale_is_utf8 && ERA_DESCRIPTION_IS_LOCALE) {
                char *tmp2 = NULL;
                era_description_str = tmp2 = x_locale_to_utf8(era_description_str, -1, NULL, NULL, NULL);
                x_free(tmp);
                tmp = x_steal_pointer(&tmp2);
            }

            x_clear_pointer(&static_era_description, x_ptr_array_unref);

            if (era_description_str != NULL) {
                static_era_description = _x_era_description_parse(era_description_str);
            }

            if (static_era_description == NULL) {
                x_warning("Could not parse ERA description: %s", era_description_str);
            }
        } else {
            x_clear_pointer(&static_era_description, x_ptr_array_unref);
        }

        x_free(tmp);
        static_era_description_locale = current_lc_time;
    }

    if (static_era_description == NULL) {
        x_mutex_unlock(&era_mutex);
        return NULL;
    }

    local_era_description = x_ptr_array_ref(static_era_description);
    x_mutex_unlock(&era_mutex);

    datetime_date.type = X_ERA_DATE_SET;
    datetime_date.year = x_date_time_get_year(datetime);
    datetime_date.month = x_date_time_get_month(datetime);
    datetime_date.day = x_date_time_get_day_of_month(datetime);

    for (unsigned int i = 0; i < local_era_description->len; i++) {
        XEraDescriptionSegment *segment = x_ptr_array_index(local_era_description, i);

        if ((_x_era_date_compare(&segment->start_date, &datetime_date) <= 0 &&
           _x_era_date_compare(&datetime_date, &segment->end_date) <= 0) ||
          (_x_era_date_compare(&segment->end_date, &datetime_date) <= 0 &&
           _x_era_date_compare(&datetime_date, &segment->start_date) <= 0))
        {
            x_ptr_array_unref(local_era_description);
            return _x_era_description_segment_ref(segment);
        }
    }

    x_ptr_array_unref(local_era_description);
    return NULL;
}

static void format_number(XString *str, xboolean use_alt_digits, const xchar *pad, xint width, xuint32 number)
{
    const xchar *ascii_digits[10] = {
        "0", "1", "2", "3", "4", "5", "6", "7", "8", "9"
    };
    xint i = 0;
    const xchar *tmp[10];
    const xchar *const *digits = ascii_digits;

    x_return_if_fail(width <= 10);

    if (use_alt_digits) {
        static xsize initialised;
        static const xchar *const *alt_digits = NULL;

        if X_UNLIKELY(x_once_init_enter(&initialised)) {
            alt_digits = initialize_alt_digits();
            if (alt_digits == NULL) {
                alt_digits = ascii_digits;
            }
            x_once_init_leave(&initialised, TRUE);
        }

        digits = alt_digits;
    }

    do {
        tmp[i++] = digits[number % 10];
        number /= 10;
    } while (number);

    while (pad && i < width) {
        tmp[i++] = *pad == '0' ? digits[0] : pad;
    }
    x_assert(i <= 10);

    while (i) {
        x_string_append(str, tmp[--i]);
    }
}

static xboolean format_ampm(XDateTime *datetime, XString *outstr, xboolean locale_is_utf8, xboolean uppercase)
{
    const xchar *ampm;
    xchar *tmp = NULL, *ampm_dup;

    ampm = GET_AMPM(datetime);

    if (!ampm || ampm[0] == '\0') {
        ampm = get_fallback_ampm(x_date_time_get_hour(datetime));
    }

    if (!locale_is_utf8 && GET_AMPM_IS_LOCALE) {
        ampm = tmp = x_locale_to_utf8(ampm, -1, NULL, NULL, NULL);
        if (tmp == NULL) {
            return FALSE;
        }
    }

    if (uppercase) {
        ampm_dup = x_utf8_strup(ampm, -1);
    } else {
        ampm_dup = x_utf8_strdown(ampm, -1);
    }
    x_free(tmp);

    x_string_append(outstr, ampm_dup);
    x_free(ampm_dup);

    return TRUE;
}

static xboolean x_date_time_format_utf8(XDateTime *datetime, const xchar *format, XString *outstr, xboolean locale_is_utf8);

static xboolean x_date_time_format_locale(XDateTime *datetime, const xchar *locale_format, XString *outstr, xboolean locale_is_utf8)
{
    xboolean success;
    xchar *utf8_format;

    if (locale_is_utf8) {
        return x_date_time_format_utf8(datetime, locale_format, outstr, locale_is_utf8);
    }

    utf8_format = _x_time_locale_to_utf8(locale_format, -1, NULL, NULL, NULL);
    if (utf8_format == NULL) {
        return FALSE;
    }

    success = x_date_time_format_utf8(datetime, utf8_format, outstr, locale_is_utf8);
    x_free(utf8_format);

    return success;
}

static inline xboolean string_append(XString *string, const xchar *s, xboolean do_strup, xboolean s_is_utf8)
{
    xchar *utf8;
    xsize utf8_len;
    char *tmp = NULL;

    if (s_is_utf8) {
        if (do_strup) {
            s = tmp = x_utf8_strup(s, -1);
        }

        x_string_append(string, s);
    } else {
        utf8 = _x_time_locale_to_utf8(s, -1, NULL, &utf8_len, NULL);
        if (utf8 == NULL) {
            return FALSE;
        }

        if (do_strup) {
            tmp = x_utf8_strup(utf8, utf8_len);
            x_free(utf8);
            utf8 = x_steal_pointer(&tmp);
        }

        x_string_append_len(string, utf8, utf8_len);
        x_free(utf8);
    }

    x_free(tmp);
    return TRUE;
}

static xboolean x_date_time_format_utf8(XDateTime *datetime, const xchar *utf8_format, XString *outstr, xboolean locale_is_utf8)
{
    xuint len;
    xunichar c;
    xuint colons;
    const xchar *tz;
    char *tmp = NULL;
    const xchar *name;
    xboolean name_is_utf8;
    const xchar *pad = "";
    const xchar *mod = "";
    xboolean alt_era = FALSE;
    xboolean pad_set = FALSE;
    xboolean mod_case = FALSE;
    xboolean alt_digits = FALSE;

    while (*utf8_format) {
        len = strcspn(utf8_format, "%");
        if (len) {
            x_string_append_len(outstr, utf8_format, len);
        }

        utf8_format += len;
        if (!*utf8_format) {
            break;
        }

        x_assert(*utf8_format == '%');
        utf8_format++;
        if (!*utf8_format) {
            break;
        }

        colons = 0;
        alt_era = FALSE;
        alt_digits = FALSE;
        pad_set = FALSE;
        mod_case = FALSE;

        next_mod:
        c = x_utf8_get_char(utf8_format);
        utf8_format = x_utf8_next_char(utf8_format);
        switch (c) {
            case 'a':
                name = WEEKDAY_ABBR(datetime);
                if (x_strcmp0(name, "") == 0) {
                    return FALSE;
                }

                name_is_utf8 = locale_is_utf8 || !WEEKDAY_ABBR_IS_LOCALE;
                if (!string_append(outstr, name, mod_case, name_is_utf8)) {
                    return FALSE;
                }
                break;

            case 'A':
                name = WEEKDAY_FULL(datetime);
                if (x_strcmp0(name, "") == 0) {
                    return FALSE;
                }

                name_is_utf8 = locale_is_utf8 || !WEEKDAY_FULL_IS_LOCALE;
                if (!string_append(outstr, name, mod_case, name_is_utf8)) {
                    return FALSE;
                }
                break;

            case 'b':
                name = alt_digits ? MONTH_ABBR_STANDALONE(datetime) : MONTH_ABBR_WITH_DAY(datetime);
                if (x_strcmp0(name, "") == 0) {
                    return FALSE;
                }

                name_is_utf8 = locale_is_utf8 || ((alt_digits && !MONTH_ABBR_STANDALONE_IS_LOCALE) || (!alt_digits && !MONTH_ABBR_WITH_DAY_IS_LOCALE));
                if (!string_append(outstr, name, mod_case, name_is_utf8)) {
                    return FALSE;
                }
                break;

            case 'B':
                name = alt_digits ? MONTH_FULL_STANDALONE(datetime) : MONTH_FULL_WITH_DAY(datetime);
                if (x_strcmp0(name, "") == 0) {
                    return FALSE;
                }

                name_is_utf8 = locale_is_utf8 || ((alt_digits && !MONTH_FULL_STANDALONE_IS_LOCALE) || (!alt_digits && !MONTH_FULL_WITH_DAY_IS_LOCALE));
                if (!string_append(outstr, name, mod_case, name_is_utf8)) {
                    return FALSE;
                }
                break;

            case 'c': {
                const char *subformat = alt_era ? PREFERRED_ERA_DATE_TIME_FMT : PREFERRED_DATE_TIME_FMT;
                if (alt_era && x_strcmp0(subformat, "") == 0) {
                    subformat = PREFERRED_DATE_TIME_FMT;
                }

                if (x_strcmp0(subformat, "") == 0) {
                    return FALSE;
                }

                if (!x_date_time_format_locale(datetime, subformat, outstr, locale_is_utf8)) {
                    return FALSE;
                }
            }
            break;

            case 'C':
                if (alt_era) {
                    XEraDescriptionSegment *era = date_time_lookup_era(datetime, locale_is_utf8);
                    if (era != NULL) {
                        x_string_append(outstr, era->era_name);
                        _x_era_description_segment_unref(era);
                        break;
                    }
                }
                format_number(outstr, alt_digits, pad_set ? pad : "0", 2, x_date_time_get_year(datetime) / 100);
                break;

            case 'd':
                format_number(outstr, alt_digits, pad_set ? pad : "0", 2, x_date_time_get_day_of_month(datetime));
                break;

            case 'e':
                format_number(outstr, alt_digits, pad_set ? pad : "\u2007", 2, x_date_time_get_day_of_month(datetime));
                break;

            case 'f':
                x_string_append_printf(outstr, "%06" X_XUINT64_FORMAT, datetime->usec % X_TIME_SPAN_SECOND);
                break;

            case 'F':
                x_string_append_printf(outstr, "%d-%02d-%02d", x_date_time_get_year(datetime), x_date_time_get_month(datetime), x_date_time_get_day_of_month(datetime));
                break;

            case 'g':
                format_number(outstr, alt_digits, pad_set ? pad : "0", 2, x_date_time_get_week_numbering_year(datetime) % 100);
                break;

            case 'G':
                format_number(outstr, alt_digits, pad_set ? pad : 0, 0, x_date_time_get_week_numbering_year(datetime));
                break;

            case 'h':
                name = alt_digits ? MONTH_ABBR_STANDALONE(datetime) : MONTH_ABBR_WITH_DAY(datetime);
                if (x_strcmp0(name, "") == 0) {
                    return FALSE;
                }

                name_is_utf8 = locale_is_utf8 || ((alt_digits && !MONTH_ABBR_STANDALONE_IS_LOCALE) || (!alt_digits && !MONTH_ABBR_WITH_DAY_IS_LOCALE));
                if (!string_append(outstr, name, mod_case, name_is_utf8)) {
                    return FALSE;
                }
                break;

            case 'H':
                format_number(outstr, alt_digits, pad_set ? pad : "0", 2, x_date_time_get_hour(datetime));
                break;

            case 'I':
                format_number(outstr, alt_digits, pad_set ? pad : "0", 2, (x_date_time_get_hour(datetime) + 11) % 12 + 1);
                break;

            case 'j':
                format_number(outstr, alt_digits, pad_set ? pad : "0", 3, x_date_time_get_day_of_year(datetime));
                break;

            case 'k':
                format_number(outstr, alt_digits, pad_set ? pad : "\u2007", 2, x_date_time_get_hour(datetime));
                break;

            case 'l':
                format_number(outstr, alt_digits, pad_set ? pad : "\u2007", 2, (x_date_time_get_hour(datetime) + 11) % 12 + 1);
                break;

            case 'm':
                format_number(outstr, alt_digits, pad_set ? pad : "0", 2, x_date_time_get_month(datetime));
                break;

            case 'M':
                format_number(outstr, alt_digits, pad_set ? pad : "0", 2, x_date_time_get_minute(datetime));
                break;

            case 'n':
                x_string_append_c(outstr, '\n');
                break;

            case 'O':
                alt_digits = TRUE;
                goto next_mod;

            case 'E':
                alt_era = TRUE;
                goto next_mod;

            case 'p':
                if (!format_ampm(datetime, outstr, locale_is_utf8, mod_case && x_strcmp0(mod, "#") == 0 ? FALSE : TRUE)) {
                    return FALSE;
                }
                break;

            case 'P':
                if (!format_ampm(datetime, outstr, locale_is_utf8, mod_case && x_strcmp0(mod, "^") == 0 ? TRUE : FALSE)) {
                    return FALSE;
                }
                break;

            case 'r':         {
                if (x_strcmp0(PREFERRED_12HR_TIME_FMT, "") == 0) {
                    return FALSE;
                }

                if (!x_date_time_format_locale(datetime, PREFERRED_12HR_TIME_FMT, outstr, locale_is_utf8)) {
                    return FALSE;
                }
            }
            break;

            case 'R':
                x_string_append_printf(outstr, "%02d:%02d", x_date_time_get_hour(datetime), x_date_time_get_minute(datetime));
                break;

            case 's':
                x_string_append_printf(outstr, "%" X_XINT64_FORMAT, x_date_time_to_unix(datetime));
                break;

            case 'S':
                format_number(outstr, alt_digits, pad_set ? pad : "0", 2, x_date_time_get_second(datetime));
                break;

            case 't':
                x_string_append_c(outstr, '\t');
                break;

            case 'T':
                x_string_append_printf(outstr, "%02d:%02d:%02d", x_date_time_get_hour(datetime), x_date_time_get_minute(datetime), x_date_time_get_second(datetime));
                break;

            case 'u':
                format_number(outstr, alt_digits, 0, 0, x_date_time_get_day_of_week(datetime));
                break;

            case 'V':
                format_number(outstr, alt_digits, pad_set ? pad : "0", 2, x_date_time_get_week_of_year(datetime));
                break;

            case 'w':
                format_number(outstr, alt_digits, 0, 0, x_date_time_get_day_of_week(datetime) % 7);
                break;

            case 'x': {
                const char *subformat = alt_era ? PREFERRED_ERA_DATE_FMT : PREFERRED_DATE_FMT;
                if (alt_era && x_strcmp0(subformat, "") == 0) {
                    subformat = PREFERRED_DATE_FMT;
                }

                if (x_strcmp0(subformat, "") == 0) {
                    return FALSE;
                }

                if (!x_date_time_format_locale(datetime, subformat, outstr, locale_is_utf8)) {
                return FALSE;
                }
            }
            break;

            case 'X': {
                const char *subformat = alt_era ? PREFERRED_ERA_TIME_FMT : PREFERRED_TIME_FMT;
                if (alt_era && x_strcmp0(subformat, "") == 0) {
                    subformat = PREFERRED_TIME_FMT;
                }

                if (x_strcmp0(subformat, "") == 0) {
                    return FALSE;
                }

                if (!x_date_time_format_locale (datetime, subformat, outstr, locale_is_utf8)) {
                    return FALSE;
                }
            }
            break;

            case 'y':
                if (alt_era) {
                    XEraDescriptionSegment *era = date_time_lookup_era(datetime, locale_is_utf8);
                    if (era != NULL) {
                        int delta = x_date_time_get_year(datetime) - era->start_date.year;
                        if ((x_date_time_get_year(datetime) < 0) != (era->start_date.year < 0)) {
                            delta -= 1;
                        }

                        format_number(outstr, alt_digits, pad_set ? pad : "0", 2, era->offset + delta * era->direction_multiplier);
                        _x_era_description_segment_unref(era);
                        break;
                    }
                }

                format_number(outstr, alt_digits, pad_set ? pad : "0", 2, x_date_time_get_year(datetime) % 100);
                break;

            case 'Y':
                if (alt_era) {
                    XEraDescriptionSegment *era = date_time_lookup_era(datetime, locale_is_utf8);
                    if (era != NULL) {
                        if (!x_date_time_format_utf8(datetime, era->era_format, outstr, locale_is_utf8)) {
                            _x_era_description_segment_unref(era);
                            return FALSE;
                        }

                        _x_era_description_segment_unref(era);
                        break;
                    }
                }

                format_number(outstr, alt_digits, 0, 0, x_date_time_get_year(datetime));
                break;

            case 'z': {
                xint64 offset;
                offset = x_date_time_get_utc_offset(datetime) / USEC_PER_SECOND;
                if (!format_z(outstr, (int) offset, colons)) {
                    return FALSE;
                }
            }
            break;

            case 'Z':
                tz = x_date_time_get_timezone_abbreviation(datetime);
                if (mod_case && x_strcmp0(mod, "#") == 0) {
                    tz = tmp = x_utf8_strdown(tz, -1);
                }
                x_string_append(outstr, tz);
                x_free(tmp);
                break;

            case '%':
                x_string_append_c(outstr, '%');
                break;

            case '-':
                pad_set = TRUE;
                pad = "";
                goto next_mod;

            case '_':
                pad_set = TRUE;
                pad = " ";
                goto next_mod;

            case '0':
                pad_set = TRUE;
                pad = "0";
                goto next_mod;

            case ':':
                if (*utf8_format && *utf8_format != 'z' && *utf8_format != ':') {
                    return FALSE;
                }
                colons++;
                goto next_mod;

            case '^':
                mod_case = TRUE;
                mod = "^";
                goto next_mod;

            case '#':
                mod_case = TRUE;
                mod = "#";
                goto next_mod;

            default:
                return FALSE;
        }
    }

    return TRUE;
}

xchar *x_date_time_format(XDateTime *datetime, const xchar *format)
{
    XString  *outstr;
    const xchar *charset;
    xboolean time_is_utf8_compatible = _x_get_time_charset(&charset) || x_strcmp0("ASCII", charset) == 0 || x_strcmp0("ANSI_X3.4-1968", charset) == 0;

    x_return_val_if_fail(datetime != NULL, NULL);
    x_return_val_if_fail(format != NULL, NULL);
    x_return_val_if_fail(x_utf8_validate(format, -1, NULL), NULL);

    outstr = x_string_sized_new(strlen(format) * 2);

    if (!x_date_time_format_utf8(datetime, format, outstr, time_is_utf8_compatible)) {
        x_string_free(outstr, TRUE);
        return NULL;
    }

    return x_string_free(outstr, FALSE);
}

xchar *x_date_time_format_iso8601(XDateTime *datetime)
{
    xint64 offset;
    XString *outstr = NULL;
    xchar *main_date = NULL;
    xchar *format = "%C%y-%m-%dT%H:%M:%S";

    x_return_val_if_fail(datetime != NULL, NULL);

    if (datetime->usec % X_TIME_SPAN_SECOND != 0) {
        format = "%C%y-%m-%dT%H:%M:%S.%f";
    }

    main_date = x_date_time_format(datetime, format);
    outstr = x_string_new(main_date);
    x_free(main_date);

    offset = x_date_time_get_utc_offset(datetime);
    if (offset == 0) {
        x_string_append_c(outstr, 'Z');
    } else {
        xchar *time_zone = x_date_time_format(datetime, "%:::z");
        x_string_append(outstr, time_zone);
        x_free(time_zone);
    }

    return x_string_free(outstr, FALSE);
}
