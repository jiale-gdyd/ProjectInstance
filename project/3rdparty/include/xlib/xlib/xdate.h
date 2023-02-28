#ifndef __X_DATE_H__
#define __X_DATE_H__

#include <time.h>
#include "xtypes.h"
#include "xquark.h"

X_BEGIN_DECLS

typedef xint32 XTime XLIB_DEPRECATED_TYPE_IN_2_62_FOR(XDateTime);
typedef xuint8 XDateDay;
typedef xuint16 XDateYear;
typedef struct _XDate XDate;

typedef enum {
    X_DATE_DAY   = 0,
    X_DATE_MONTH = 1,
    X_DATE_YEAR  = 2
} XDateDMY;

typedef enum {
    X_DATE_BAD_WEEKDAY = 0,
    X_DATE_MONDAY      = 1,
    X_DATE_TUESDAY     = 2,
    X_DATE_WEDNESDAY   = 3,
    X_DATE_THURSDAY    = 4,
    X_DATE_FRIDAY      = 5,
    X_DATE_SATURDAY    = 6,
    X_DATE_SUNDAY      = 7
} XDateWeekday;

typedef enum {
    X_DATE_BAD_MONTH = 0,
    X_DATE_JANUARY   = 1,
    X_DATE_FEBRUARY  = 2,
    X_DATE_MARCH     = 3,
    X_DATE_APRIL     = 4,
    X_DATE_MAY       = 5,
    X_DATE_JUNE      = 6,
    X_DATE_JULY      = 7,
    X_DATE_AUGUST    = 8,
    X_DATE_SEPTEMBER = 9,
    X_DATE_OCTOBER   = 10,
    X_DATE_NOVEMBER  = 11,
    X_DATE_DECEMBER  = 12
} XDateMonth;

#define X_DATE_BAD_JULIAN               0U
#define X_DATE_BAD_DAY                  0U
#define X_DATE_BAD_YEAR                 0U

struct _XDate {
    xuint julian_days : 32;
    xuint julian : 1;
    xuint dmy    : 1;
    xuint day    : 6;
    xuint month  : 4;
    xuint year   : 16;
};

XLIB_AVAILABLE_IN_ALL
XDate *x_date_new(void);

XLIB_AVAILABLE_IN_ALL
XDate *x_date_new_dmy(XDateDay day, XDateMonth month, XDateYear year);

XLIB_AVAILABLE_IN_ALL
XDate *x_date_new_julian(xuint32 julian_day);

XLIB_AVAILABLE_IN_ALL
void x_date_free(XDate *date);

XLIB_AVAILABLE_IN_2_56
XDate *x_date_copy(const XDate *date);

XLIB_AVAILABLE_IN_ALL
xboolean x_date_valid(const XDate *date);

XLIB_AVAILABLE_IN_ALL
xboolean x_date_valid_day(XDateDay day) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
xboolean x_date_valid_month(XDateMonth month) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
xboolean x_date_valid_year(XDateYear year) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
xboolean x_date_valid_weekday(XDateWeekday weekday) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
xboolean x_date_valid_julian(xuint32 julian_date) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
xboolean x_date_valid_dmy(XDateDay day, XDateMonth month, XDateYear year) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
XDateWeekday x_date_get_weekday(const XDate *date);

XLIB_AVAILABLE_IN_ALL
XDateMonth x_date_get_month(const XDate *date);

XLIB_AVAILABLE_IN_ALL
XDateYear x_date_get_year(const XDate *date);

XLIB_AVAILABLE_IN_ALL
XDateDay x_date_get_day(const XDate *date);

XLIB_AVAILABLE_IN_ALL
xuint32 x_date_get_julian(const XDate *date);

XLIB_AVAILABLE_IN_ALL
xuint x_date_get_day_of_year(const XDate *date);

XLIB_AVAILABLE_IN_ALL
xuint x_date_get_monday_week_of_year(const XDate *date);

XLIB_AVAILABLE_IN_ALL
xuint x_date_get_sunday_week_of_year(const XDate *date);

XLIB_AVAILABLE_IN_ALL
xuint x_date_get_iso8601_week_of_year(const XDate *date);

XLIB_AVAILABLE_IN_ALL
void x_date_clear(XDate *date, xuint n_dates);

XLIB_AVAILABLE_IN_ALL
void x_date_set_parse(XDate *date, const xchar *str);

XLIB_AVAILABLE_IN_ALL
void x_date_set_time_t(XDate *date, time_t timet);

X_GNUC_BEGIN_IGNORE_DEPRECATIONS
XLIB_DEPRECATED_IN_2_62_FOR(x_date_set_time_t)
void x_date_set_time_val(XDate *date, XTimeVal *timeval);

XLIB_DEPRECATED_FOR(x_date_set_time_t)
void x_date_set_time(XDate *date, XTime time_);
X_GNUC_END_IGNORE_DEPRECATIONS

XLIB_AVAILABLE_IN_ALL
void x_date_set_month(XDate *date, XDateMonth month);

XLIB_AVAILABLE_IN_ALL
void x_date_set_day(XDate *date, XDateDay day);

XLIB_AVAILABLE_IN_ALL
void x_date_set_year(XDate *date, XDateYear year);

XLIB_AVAILABLE_IN_ALL
void x_date_set_dmy(XDate *date, XDateDay day, XDateMonth month, XDateYear y);

XLIB_AVAILABLE_IN_ALL
void x_date_set_julian(XDate *date, xuint32 julian_date);

XLIB_AVAILABLE_IN_ALL
xboolean x_date_is_first_of_month(const XDate *date);

XLIB_AVAILABLE_IN_ALL
xboolean x_date_is_last_of_month(const XDate *date);

XLIB_AVAILABLE_IN_ALL
void x_date_add_days(XDate *date, xuint n_days);

XLIB_AVAILABLE_IN_ALL
void x_date_subtract_days(XDate *date, xuint n_days);

XLIB_AVAILABLE_IN_ALL
void x_date_add_months(XDate *date, xuint n_months);

XLIB_AVAILABLE_IN_ALL
void x_date_subtract_months(XDate *date, xuint n_months);

XLIB_AVAILABLE_IN_ALL
void x_date_add_years(XDate *date, xuint n_years);

XLIB_AVAILABLE_IN_ALL
void x_date_subtract_years(XDate *date, xuint n_years);

XLIB_AVAILABLE_IN_ALL
xboolean x_date_is_leap_year(XDateYear year) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
xuint8 x_date_get_days_in_month(XDateMonth month,XDateYear year) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
xuint8 x_date_get_monday_weeks_in_year(XDateYear year) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
xuint8 x_date_get_sunday_weeks_in_year(XDateYear year) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
xint x_date_days_between(const XDate *date1, const XDate *date2);

XLIB_AVAILABLE_IN_ALL
xint x_date_compare(const XDate *lhs, const XDate *rhs);

XLIB_AVAILABLE_IN_ALL
void x_date_to_struct_tm(const XDate *date, struct tm *tm);

XLIB_AVAILABLE_IN_ALL
void x_date_clamp(XDate *date, const XDate *min_date, const XDate *max_date);

XLIB_AVAILABLE_IN_ALL
void x_date_order(XDate *date1, XDate *date2);

XLIB_AVAILABLE_IN_ALL
xsize x_date_strftime(xchar *s, xsize slen, const xchar *format, const XDate *date);

#define x_date_weekday                      x_date_get_weekday XLIB_DEPRECATED_MACRO_IN_2_26_FOR(x_date_get_weekday)
#define x_date_month                        x_date_get_month XLIB_DEPRECATED_MACRO_IN_2_26_FOR(x_date_get_month)
#define x_date_year                         x_date_get_year XLIB_DEPRECATED_MACRO_IN_2_26_FOR(x_date_get_year)
#define x_date_day                          x_date_get_day XLIB_DEPRECATED_MACRO_IN_2_26_FOR(x_date_get_day)
#define x_date_julian                       x_date_get_julian XLIB_DEPRECATED_MACRO_IN_2_26_FOR(x_date_get_julian)
#define x_date_day_of_year                  x_date_get_day_of_year XLIB_DEPRECATED_MACRO_IN_2_26_FOR(x_date_get_day_of_year)
#define x_date_monday_week_of_year          x_date_get_monday_week_of_year XLIB_DEPRECATED_MACRO_IN_2_26_FOR(x_date_get_monday_week_of_year)
#define x_date_sunday_week_of_year          x_date_get_sunday_week_of_year XLIB_DEPRECATED_MACRO_IN_2_26_FOR(x_date_get_sunday_week_of_year)
#define x_date_days_in_month                x_date_get_days_in_month XLIB_DEPRECATED_MACRO_IN_2_26_FOR(x_date_get_days_in_month)
#define x_date_monday_weeks_in_year         x_date_get_monday_weeks_in_year XLIB_DEPRECATED_MACRO_IN_2_26_FOR(x_date_get_monday_weeks_in_year)
#define x_date_sunday_weeks_in_year         x_date_get_sunday_weeks_in_year XLIB_DEPRECATED_MACRO_IN_2_26_FOR(x_date_get_sunday_weeks_in_year)

X_END_DECLS

#endif
