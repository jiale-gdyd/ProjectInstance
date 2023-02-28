#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>

#include <xlib/xlib/config.h>
#include <xlib/xlib/xlibconfig.h>
#include <xlib/xlib/xmem.h>
#include <xlib/xlib/xdate.h>
#include <xlib/xlib/xthread.h>
#include <xlib/xlib/xunicode.h>
#include <xlib/xlib/xconvert.h>
#include <xlib/xlib/xstrfuncs.h>
#include <xlib/xlib/xtestutils.h>

XDate *x_date_new(void)
{
    XDate *d = x_new0(XDate, 1);
    return d;
}

XDate *x_date_new_dmy(XDateDay day, XDateMonth m, XDateYear y)
{
    XDate *d;
    x_return_val_if_fail(x_date_valid_dmy(day, m, y), NULL);

    d = x_new(XDate, 1);
    d->julian = FALSE;
    d->dmy = TRUE;
    d->month = m;
    d->day = day;
    d->year = y;

    x_assert(x_date_valid(d));
    return d;
}

XDate *x_date_new_julian(xuint32 julian_day)
{
    XDate *d;
    x_return_val_if_fail(x_date_valid_julian(julian_day), NULL);

    d = x_new(XDate, 1);
    d->julian = TRUE;
    d->dmy = FALSE;
    d->julian_days = julian_day;

    x_assert(x_date_valid(d));
    return d;
}

void x_date_free(XDate *date)
{
    x_return_if_fail(date != NULL);
    x_free(date);
}

XDate *x_date_copy(const XDate *date)
{
    XDate *res;
    x_return_val_if_fail(date != NULL, NULL);

    if (x_date_valid(date)) {
        res = x_date_new_julian(x_date_get_julian(date));
    } else {
        res = x_date_new();
        *res = *date;
    }

    return res;
}

xboolean x_date_valid(const XDate *d)
{
    x_return_val_if_fail(d != NULL, FALSE);
    return (d->julian || d->dmy);
}

static const xuint8 days_in_months[2][13] = {
    /* error, jan feb mar apr may jun jul aug sep oct nov dec */
    {  0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }, 
    {  0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 } /* leap year */
};

static const xuint16 days_in_year[2][14] = {
    /* 0, jan feb mar apr may  jun  jul  aug  sep  oct  nov  dec */
    {  0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 }, 
    {  0, 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 }
};

xboolean x_date_valid_month(XDateMonth m)
{
    return (((xint)m > X_DATE_BAD_MONTH) && ((xint)m < 13));
}

xboolean x_date_valid_year(XDateYear y)
{
    return (y > X_DATE_BAD_YEAR);
}

xboolean x_date_valid_day(XDateDay d)
{
    return ((d > X_DATE_BAD_DAY) && (d < 32));
}

xboolean x_date_valid_weekday(XDateWeekday w)
{
    return (((xint)w > X_DATE_BAD_WEEKDAY) && ((xint)w < 8));
}

xboolean x_date_valid_julian(xuint32 j)
{
    return (j > X_DATE_BAD_JULIAN);
}

xboolean x_date_valid_dmy(XDateDay d, XDateMonth m,  XDateYear y)
{
    return ((m > X_DATE_BAD_MONTH) && (m < 13) && (d > X_DATE_BAD_DAY) && (y > X_DATE_BAD_YEAR)  && (d <= (x_date_is_leap_year(y) ? days_in_months[1][m] : days_in_months[0][m])) );
}

static void x_date_update_julian(const XDate *const_d)
{
    xint idx;
    XDateYear year;
    XDate *d = (XDate *)const_d;

    x_return_if_fail(d != NULL);
    x_return_if_fail(d->dmy != 0);
    x_return_if_fail(!d->julian);
    x_return_if_fail(x_date_valid_dmy(d->day, (XDateMonth)d->month, d->year));

    year = d->year - 1;
    d->julian_days = year * 365U;
    d->julian_days += (year >>= 2);
    d->julian_days -= (year /= 25);
    d->julian_days += year >> 2;

    idx = x_date_is_leap_year(d->year) ? 1 : 0;
    d->julian_days += days_in_year[idx][d->month] + d->day;

    x_return_if_fail(x_date_valid_julian(d->julian_days));
    d->julian = TRUE;
}

static void x_date_update_dmy(const XDate *const_d)
{
    XDateYear y;
    XDateMonth m;
    XDateDay day;
    xuint32 A, B, C, D, E, M;
    XDate *d = (XDate *)const_d;

    x_return_if_fail(d != NULL);
    x_return_if_fail(d->julian);
    x_return_if_fail(!d->dmy);
    x_return_if_fail(x_date_valid_julian(d->julian_days));

    A = d->julian_days + 1721425 + 32045;
    B = (4 *(A + 36524)) / 146097 - 1;
    C = A - (146097 * B)/ 4;
    D = (4 * (C + 365)) / 1461 - 1;
    E = C - ((1461 * D) / 4);
    M = (5 * (E - 1) + 2) / 153;

    m = (XDateMonth)(M + 3 - (12 * (M / 10)));
    day = E - (153 * M + 2) / 5;
    y = 100 * B + D - 4800 + (M / 10);

    d->month = m;
    d->day = day;
    d->year = y;
    d->dmy = TRUE;
}

XDateWeekday x_date_get_weekday(const XDate *d)
{
    x_return_val_if_fail(x_date_valid(d), X_DATE_BAD_WEEKDAY);

    if (!d->julian) {
        x_date_update_julian(d);
    }
    x_return_val_if_fail(d->julian, X_DATE_BAD_WEEKDAY);

    return (XDateWeekday)(((d->julian_days - 1) % 7) + 1);
}

XDateMonth x_date_get_month(const XDate *d)
{
    x_return_val_if_fail(x_date_valid(d), X_DATE_BAD_MONTH);

    if (!d->dmy) {
        x_date_update_dmy(d);
    }
    x_return_val_if_fail(d->dmy, X_DATE_BAD_MONTH);

    return (XDateMonth)d->month;
}

XDateYear x_date_get_year(const XDate *d)
{
    x_return_val_if_fail(x_date_valid(d), X_DATE_BAD_YEAR);

    if (!d->dmy) {
        x_date_update_dmy(d);
    }
    x_return_val_if_fail(d->dmy, X_DATE_BAD_YEAR);

    return d->year;
}

XDateDay x_date_get_day(const XDate *d)
{
    x_return_val_if_fail(x_date_valid(d), X_DATE_BAD_DAY);

    if (!d->dmy) {
        x_date_update_dmy(d);
    }
    x_return_val_if_fail(d->dmy, X_DATE_BAD_DAY);  

    return d->day;
}

xuint32 x_date_get_julian(const XDate *d)
{
    x_return_val_if_fail(x_date_valid(d), X_DATE_BAD_JULIAN);

    if (!d->julian) {
        x_date_update_julian(d);
    }
    x_return_val_if_fail(d->julian, X_DATE_BAD_JULIAN);

    return d->julian_days;
}

xuint x_date_get_day_of_year(const XDate *d)
{
    xint idx;

    x_return_val_if_fail(x_date_valid(d), 0);

    if (!d->dmy) {
        x_date_update_dmy(d);
    }
    x_return_val_if_fail(d->dmy, 0);

    idx = x_date_is_leap_year(d->year) ? 1 : 0;
    return (days_in_year[idx][d->month] + d->day);
}

xuint x_date_get_monday_week_of_year(const XDate *d)
{
    xuint day;
    XDate first;
    XDateWeekday wd;

    x_return_val_if_fail(x_date_valid(d), 0);

    if (!d->dmy) {
        x_date_update_dmy(d);
    }
    x_return_val_if_fail(d->dmy, 0);

    x_date_clear(&first, 1);
    x_date_set_dmy(&first, 1, (XDateMonth)1, d->year);

    wd = (XDateWeekday)(x_date_get_weekday(&first) - 1);
    day = x_date_get_day_of_year(d) - 1;

    return ((day + wd)/7U + (wd == 0 ? 1 : 0));
}

xuint x_date_get_sunday_week_of_year(const XDate *d)
{
    xuint day;
    XDate first;
    XDateWeekday wd;

    x_return_val_if_fail(x_date_valid(d), 0);

    if (!d->dmy) {
        x_date_update_dmy(d);
    }
    x_return_val_if_fail(d->dmy, 0);

    x_date_clear(&first, 1);
    x_date_set_dmy(&first, 1, (XDateMonth)1, d->year);

    wd = x_date_get_weekday(&first);
    if (wd == 7) {
        wd = (XDateWeekday)0;
    }
    day = x_date_get_day_of_year(d) - 1;

    return ((day + wd)/7U + (wd == 0 ? 1 : 0));
}

xuint x_date_get_iso8601_week_of_year(const XDate *d)
{
    xuint j, d4, L, d1, w;

    x_return_val_if_fail(x_date_valid(d), 0);

    if (!d->julian) {
        x_date_update_julian(d);
    }
    x_return_val_if_fail(d->julian, 0);

    j  = d->julian_days + 1721425;
    d4 = (j + 31741 - (j % 7)) % 146097 % 36524 % 1461;
    L  = d4 / 1460;
    d1 = ((d4 - L) % 365) + L;
    w  = d1 / 7 + 1;

    return w;
}

xint x_date_days_between(const XDate *d1, const XDate *d2)
{
    x_return_val_if_fail(x_date_valid(d1), 0);
    x_return_val_if_fail(x_date_valid(d2), 0);

    return (xint)x_date_get_julian(d2) - (xint)x_date_get_julian(d1);
}

void x_date_clear(XDate *d, xuint ndates)
{
    x_return_if_fail(d != NULL);
    x_return_if_fail(ndates != 0);

    memset(d, 0x0, ndates * sizeof(XDate));
}

X_LOCK_DEFINE_STATIC(x_date_global);

static xchar *long_month_names[13] = {
    NULL,
};

static xchar *long_month_names_alternative[13] = {
    NULL,
};

static xchar *short_month_names[13] = {
    NULL, 
};

static xchar *short_month_names_alternative[13] = {
    NULL,
};

static xchar *current_locale = NULL;

static XDateDMY dmy_order[3] = {
    X_DATE_DAY, X_DATE_MONTH, X_DATE_YEAR
};

static xint locale_era_adjust = 0;
static xboolean using_twodigit_years = FALSE;
static const XDateYear twodigit_start_year = 1930;

struct _XDateParseTokens {
    xint  num_ints;
    xint  n[3];
    xuint month;
};

typedef struct _XDateParseTokens XDateParseTokens;

static inline xboolean update_month_match(xsize *longest, const xchar *haystack, const xchar *needle)
{
    xsize length;

    if (needle == NULL) {
        return FALSE;
    }

    length = strlen(needle);
    if (*longest >= length) {
        return FALSE;
    }

    if (strstr(haystack, needle) == NULL) {
        return FALSE;
    }

    *longest = length;
    return TRUE;
}

#define NUM_LEN         10

static void x_date_fill_parse_tokens(const xchar *str, XDateParseTokens *pt)
{
    xint i;
    const xuchar *s;
    xchar num[4][NUM_LEN+1];

    num[0][0] = num[1][0] = num[2][0] = num[3][0] = '\0';

    s = (const xuchar *) str;
    pt->num_ints = 0;

    while (*s && pt->num_ints < 4) {
        i = 0;
        while (*s && x_ascii_isdigit(*s) && i < NUM_LEN) {
            num[pt->num_ints][i] = *s;
            ++s; 
            ++i;
        }

        if (i > 0) {
            num[pt->num_ints][i] = '\0';
            ++(pt->num_ints);
        }

        if (*s == '\0') {
            break;
        }
    
        ++s;
    }

    pt->n[0] = pt->num_ints > 0 ? atoi(num[0]) : 0;
    pt->n[1] = pt->num_ints > 1 ? atoi(num[1]) : 0;
    pt->n[2] = pt->num_ints > 2 ? atoi(num[2]) : 0;

    pt->month = X_DATE_BAD_MONTH;
    if (pt->num_ints < 3) {
        xsize longest = 0;
        xchar *casefold;
        xchar *normalized;

        casefold = x_utf8_casefold(str, -1);
        normalized = x_utf8_normalize(casefold, -1, X_NORMALIZE_ALL);
        x_free(casefold);

        for (i = 1; i < 13; ++i) {
            if (update_month_match(&longest, normalized, long_month_names[i])) {
                pt->month = i;
            }

            if (update_month_match(&longest, normalized, long_month_names_alternative[i])) {
                pt->month = i;
            }

            if (update_month_match(&longest, normalized, short_month_names[i])) {
                pt->month = i;
            }

            if (update_month_match(&longest, normalized, short_month_names_alternative[i])) {
                pt->month = i;
            }
        }

        x_free(normalized);
    }
}

static void x_date_prepare_to_parse(const xchar *str, XDateParseTokens *pt)
{
    XDate d;
    xboolean recompute_localeinfo = FALSE;
    const xchar *locale = setlocale(LC_TIME, NULL);

    x_return_if_fail(locale != NULL);
    x_date_clear(&d, 1);

    if ((current_locale == NULL) || (strcmp(locale, current_locale) != 0)) {
        recompute_localeinfo = TRUE;
    }

    if (recompute_localeinfo) {
        int i = 1;
        xchar buf[128];
        XDateParseTokens testpt;

        x_free(current_locale);
        current_locale = x_strdup(locale);

        short_month_names[0] = "Error";
        long_month_names[0] = "Error";

        while (i < 13) {
            xchar *casefold;

            x_date_set_dmy(&d, 1, (XDateMonth)i, 1976);
            x_return_if_fail(x_date_valid(&d));
            x_date_strftime(buf, 127, "%b", &d);

            casefold = x_utf8_casefold(buf, -1);
            x_free(short_month_names[i]);
            short_month_names[i] = x_utf8_normalize(casefold, -1, X_NORMALIZE_ALL);
            x_free(casefold);

            x_date_strftime(buf, 127, "%B", &d);
            casefold = x_utf8_casefold(buf, -1);
            x_free(long_month_names[i]);
            long_month_names[i] = x_utf8_normalize(casefold, -1, X_NORMALIZE_ALL);
            x_free(casefold);

            x_date_strftime(buf, 127, "%Ob", &d);
            casefold = x_utf8_casefold(buf, -1);
            x_free(short_month_names_alternative[i]);
            short_month_names_alternative[i] = x_utf8_normalize(casefold, -1, X_NORMALIZE_ALL);
            x_free(casefold);

            x_date_strftime(buf, 127, "%OB", &d);
            casefold = x_utf8_casefold(buf, -1);
            x_free(long_month_names_alternative[i]);
            long_month_names_alternative[i] = x_utf8_normalize(casefold, -1, X_NORMALIZE_ALL);
            x_free(casefold);

            ++i;
        }

        x_date_set_dmy(&d, 4, (XDateMonth)7, 1976);
        x_date_strftime(buf, 127, "%x", &d);
        x_date_fill_parse_tokens(buf, &testpt);

        using_twodigit_years = FALSE;
        locale_era_adjust = 0;
        dmy_order[0] = X_DATE_DAY;
        dmy_order[1] = X_DATE_MONTH;
        dmy_order[2] = X_DATE_YEAR;

        i = 0;
        while (i < testpt.num_ints) {
            switch (testpt.n[i]) {
                case 7:
                    dmy_order[i] = X_DATE_MONTH;
                    break;

                case 4:
                    dmy_order[i] = X_DATE_DAY;
                    break;

                case 76:
                    using_twodigit_years = TRUE;
                    X_GNUC_FALLTHROUGH;

                case 1976:
                    dmy_order[i] = X_DATE_YEAR;
                    break;

                default:
                    locale_era_adjust = 1976 - testpt.n[i];
                    dmy_order[i] = X_DATE_YEAR;
                    break;
            }

            ++i;
        }
    }

    x_date_fill_parse_tokens(str, pt);
}

static xuint convert_twodigit_year(xuint y)
{
    if (using_twodigit_years && y < 100) {
        xuint two = twodigit_start_year % 100;
        xuint century = (twodigit_start_year / 100) * 100;

        if (y < two) {
            century += 100;
        }
        y += century;
    }

    return y;
}

void x_date_set_parse(XDate *d, const xchar *str)
{
    xsize str_len;
    XDateParseTokens pt;
    xuint m = X_DATE_BAD_MONTH, day = X_DATE_BAD_DAY, y = X_DATE_BAD_YEAR;

    x_return_if_fail(d != NULL);

    x_date_clear(d, 1);

    str_len = strlen(str);
    if (str_len > 200) {
        return;
    }

    if (!x_utf8_validate_len(str, str_len, NULL)) {
        return;
    }

    X_LOCK(x_date_global);

    x_date_prepare_to_parse(str, &pt);

    //DEBUG_MSG(("Found %d ints, '%d' '%d' '%d' and written out month %d", pt.num_ints, pt.n[0], pt.n[1], pt.n[2], pt.month));

    if (pt.num_ints == 4)  {
        X_UNLOCK(x_date_global);
        return;
    }

    if (pt.num_ints > 1) {
        int i = 0;
        int j = 0;

        x_assert(pt.num_ints < 4);

        while (i < pt.num_ints && j < 3)  {
            switch (dmy_order[j]) {
                case X_DATE_MONTH: {
                    if (pt.num_ints == 2 && pt.month != X_DATE_BAD_MONTH) {
                        m = pt.month;
                        ++j;
                        continue;
                    } else {
                        m = pt.n[i];
                    }
                }
                break;

                case X_DATE_DAY: {
                    if (pt.num_ints == 2 && pt.month == X_DATE_BAD_MONTH) {
                        day = 1;
                        ++j;
                        continue;
                    }

                    day = pt.n[i];
                }
                break;

                case X_DATE_YEAR: {
                    y  = pt.n[i];
                    if (locale_era_adjust != 0) {
                        y += locale_era_adjust;
                    }

                    y = convert_twodigit_year (y);
                }
                break;

                default:
                    break;
            }

            ++i;
            ++j;
        }

        if (pt.num_ints == 3 && !x_date_valid_dmy(day, (XDateMonth)m, y)) {
            y   = pt.n[0];
            m   = pt.n[1];
            day = pt.n[2];

            if (using_twodigit_years && y < 100) {
                y = X_DATE_BAD_YEAR;
            }
        } else if (pt.num_ints == 2) {
            if (m == X_DATE_BAD_MONTH && pt.month != X_DATE_BAD_MONTH) {
                m = pt.month;
            }
        }
    } else if (pt.num_ints == 1)  {
        if (pt.month != X_DATE_BAD_MONTH) {
            m = pt.month;
            day  = 1;
            y = pt.n[0];
        } else {
            m = (pt.n[0]/100) % 100;
            day = pt.n[0] % 100;
            y  = pt.n[0]/10000;
            y = convert_twodigit_year(y);
        }
    }

    if (y < 8000 && x_date_valid_dmy(day, (XDateMonth)m, y))  {
        d->month = m;
        d->day = day;
        d->year = y;
        d->dmy = TRUE;
    }

    X_UNLOCK(x_date_global);
}

void x_date_set_time_t(XDate *date, time_t timet)
{
    struct tm tm;

    x_return_if_fail(date != NULL);
    localtime_r(&timet, &tm);

    date->julian = FALSE;
    date->month = tm.tm_mon + 1;
    date->day = tm.tm_mday;
    date->year = tm.tm_year + 1900;

    x_return_if_fail(x_date_valid_dmy(date->day, (XDateMonth)date->month, date->year));
    date->dmy = TRUE;
}

X_GNUC_BEGIN_IGNORE_DEPRECATIONS
void x_date_set_time(XDate *date, XTime time_)
{
    x_date_set_time_t(date, (time_t)time_);
}
X_GNUC_END_IGNORE_DEPRECATIONS

X_GNUC_BEGIN_IGNORE_DEPRECATIONS
void x_date_set_time_val(XDate *date, XTimeVal *timeval)
{
    x_date_set_time_t(date, (time_t)timeval->tv_sec);
}
X_GNUC_END_IGNORE_DEPRECATIONS

void x_date_set_month(XDate *d,  XDateMonth m)
{
    x_return_if_fail(d != NULL);
    x_return_if_fail(x_date_valid_month(m));

    if (d->julian && !d->dmy) {
        x_date_update_dmy(d);
    }
    d->julian = FALSE;

    d->month = m;
    if (x_date_valid_dmy(d->day, (XDateMonth)d->month, d->year)) {
        d->dmy = TRUE;
    } else {
        d->dmy = FALSE;
    }
}

void x_date_set_day(XDate *d, XDateDay  day)
{
    x_return_if_fail(d != NULL);
    x_return_if_fail(x_date_valid_day(day));

    if (d->julian && !d->dmy) {
        x_date_update_dmy(d);
    }
    d->julian = FALSE;

    d->day = day;
    if (x_date_valid_dmy(d->day, (XDateMonth)d->month, d->year)) {
        d->dmy = TRUE;
    } else {
        d->dmy = FALSE;
    }
}

void x_date_set_year(XDate *d, XDateYear y)
{
    x_return_if_fail(d != NULL);
    x_return_if_fail(x_date_valid_year(y));

    if (d->julian && !d->dmy)  {
        x_date_update_dmy(d);
    }
    d->julian = FALSE;

    d->year = y;
    if (x_date_valid_dmy(d->day, (XDateMonth)d->month, d->year)) {
        d->dmy = TRUE;
    } else {
        d->dmy = FALSE;
    }
}

void x_date_set_dmy(XDate *d, XDateDay day, XDateMonth m, XDateYear y)
{
    x_return_if_fail(d != NULL);
    x_return_if_fail(x_date_valid_dmy(day, m, y));

    d->julian = FALSE;
    d->month = m;
    d->day   = day;
    d->year  = y;
    d->dmy = TRUE;
}

void x_date_set_julian (XDate *d, xuint32 j)
{
    x_return_if_fail(d != NULL);
    x_return_if_fail(x_date_valid_julian(j));

    d->julian_days = j;
    d->julian = TRUE;
    d->dmy = FALSE;
}

xboolean x_date_is_first_of_month(const XDate *d)
{
    x_return_val_if_fail(x_date_valid(d), FALSE);

    if (!d->dmy) {
        x_date_update_dmy(d);
    }

    x_return_val_if_fail(d->dmy, FALSE);

    if (d->day == 1) {
        return TRUE;
    } else {
        return FALSE;
    }
}

xboolean x_date_is_last_of_month(const XDate *d)
{
    xint idx;

    x_return_val_if_fail(x_date_valid(d), FALSE);

    if (!d->dmy) {
        x_date_update_dmy(d);
    }

    x_return_val_if_fail(d->dmy, FALSE);
    idx = x_date_is_leap_year(d->year) ? 1 : 0;

    if (d->day == days_in_months[idx][d->month]) {
        return TRUE;
    } else {
        return FALSE;
    }
}

void x_date_add_days(XDate *d, xuint ndays)
{
    x_return_if_fail(x_date_valid(d));

    if (!d->julian) {
        x_date_update_julian(d);
    }

    x_return_if_fail(d->julian);
    x_return_if_fail(ndays <= X_MAXUINT32 - d->julian_days);

    d->julian_days += ndays;
    d->dmy = FALSE;
}

void x_date_subtract_days(XDate *d, xuint ndays)
{
    x_return_if_fail(x_date_valid(d));

    if (!d->julian) {
        x_date_update_julian(d);
    }

    x_return_if_fail(d->julian);
    x_return_if_fail(d->julian_days > ndays);

    d->julian_days -= ndays;
    d->dmy = FALSE;
}

void x_date_add_months(XDate *d, xuint nmonths)
{
    xint idx;
    xuint years, months;

    x_return_if_fail(x_date_valid(d));

    if (!d->dmy) {
        x_date_update_dmy(d);
    }

    x_return_if_fail(d->dmy != 0);
    x_return_if_fail(nmonths <= X_MAXUINT - (d->month - 1));

    nmonths += d->month - 1;
    years  = nmonths / 12;
    months = nmonths % 12;

    x_return_if_fail(years <= (xuint)(X_MAXUINT16 - d->year));

    d->month = months + 1;
    d->year += years;
    
    idx = x_date_is_leap_year(d->year) ? 1 : 0;

    if (d->day > days_in_months[idx][d->month]) {
        d->day = days_in_months[idx][d->month];
    }

    d->julian = FALSE;
    x_return_if_fail(x_date_valid(d));
}

void x_date_subtract_months(XDate *d, xuint nmonths)
{
    xint idx;
    xuint years, months;

    x_return_if_fail(x_date_valid(d));

    if (!d->dmy) {
        x_date_update_dmy(d);
    }
    x_return_if_fail(d->dmy != 0);

    years  = nmonths / 12;
    months = nmonths % 12;

    x_return_if_fail(d->year > years);
    d->year -= years;

    if (d->month > months) {
        d->month -= months;
    } else {
        months -= d->month;
        d->month = 12 - months;
        d->year -= 1;
    }

    idx = x_date_is_leap_year(d->year) ? 1 : 0;
    if (d->day > days_in_months[idx][d->month]) {
        d->day = days_in_months[idx][d->month];
    }
    d->julian = FALSE;

    x_return_if_fail(x_date_valid(d));
}

void x_date_add_years(XDate *d, xuint nyears)
{
    x_return_if_fail(x_date_valid(d));

    if (!d->dmy) {
        x_date_update_dmy(d);
    }

    x_return_if_fail(d->dmy != 0);
    x_return_if_fail(nyears <= (xuint)(X_MAXUINT16 - d->year));

    d->year += nyears;
    if (d->month == 2 && d->day == 29) {
        if (!x_date_is_leap_year(d->year)) {
            d->day = 28;
        }
    }

    d->julian = FALSE;
}

void x_date_subtract_years(XDate *d, xuint nyears)
{
    x_return_if_fail(x_date_valid(d));

    if (!d->dmy) {
        x_date_update_dmy(d);
    }

    x_return_if_fail(d->dmy != 0);
    x_return_if_fail(d->year > nyears);

    d->year -= nyears;
    if (d->month == 2 && d->day == 29) {
        if (!x_date_is_leap_year(d->year)) {
            d->day = 28;
        }
    }

    d->julian = FALSE;
}

xboolean x_date_is_leap_year(XDateYear year)
{
    x_return_val_if_fail(x_date_valid_year(year), FALSE);
    return ( (((year % 4) == 0) && ((year % 100) != 0)) || (year % 400) == 0 );
}

xuint8 x_date_get_days_in_month(XDateMonth month, XDateYear year)
{
    xint idx;

    x_return_val_if_fail(x_date_valid_year(year), 0);
    x_return_val_if_fail(x_date_valid_month(month), 0);

    idx = x_date_is_leap_year(year) ? 1 : 0;
    return days_in_months[idx][month];
}

xuint8 x_date_get_monday_weeks_in_year(XDateYear year)
{
    XDate d;

    x_return_val_if_fail(x_date_valid_year(year), 0);

    x_date_clear(&d, 1);

    x_date_set_dmy(&d, 1, (XDateMonth)1, year);
    if (x_date_get_weekday(&d) == X_DATE_MONDAY) {
        return 53;
    }

    x_date_set_dmy(&d, 31, (XDateMonth)12, year);
    if (x_date_get_weekday(&d) == X_DATE_MONDAY) {
        return 53;
    }

    if (x_date_is_leap_year(year)) {
        x_date_set_dmy(&d, 2, (XDateMonth)1, year);
        if (x_date_get_weekday(&d) == X_DATE_MONDAY) {
            return 53;
        }

        x_date_set_dmy(&d, 30, (XDateMonth)12, year);
        if (x_date_get_weekday(&d) == X_DATE_MONDAY) {
            return 53;
        }
    }

    return 52;
}

xuint8 x_date_get_sunday_weeks_in_year(XDateYear year)
{
    XDate d;

    x_return_val_if_fail(x_date_valid_year(year), 0);

    x_date_clear(&d, 1);

    x_date_set_dmy(&d, 1, (XDateMonth)1, year);
    if (x_date_get_weekday(&d) == X_DATE_SUNDAY) {
        return 53;
    }

    x_date_set_dmy(&d, 31, (XDateMonth)12, year);
    if (x_date_get_weekday(&d) == X_DATE_SUNDAY) {
        return 53;
    }

    if (x_date_is_leap_year(year))  {
        x_date_set_dmy(&d, 2, (XDateMonth)1, year);
        if (x_date_get_weekday(&d) == X_DATE_SUNDAY) {
            return 53;
        }

        x_date_set_dmy(&d, 30, (XDateMonth)12, year);
        if (x_date_get_weekday(&d) == X_DATE_SUNDAY) {
            return 53;
        }
    }

    return 52;
}

xint x_date_compare (const XDate *lhs, const XDate *rhs)
{
    x_return_val_if_fail(lhs != NULL, 0);
    x_return_val_if_fail(rhs != NULL, 0);
    x_return_val_if_fail(x_date_valid(lhs), 0);
    x_return_val_if_fail(x_date_valid(rhs), 0);

    while (TRUE) {
        if (lhs->julian && rhs->julian)  {
            if (lhs->julian_days < rhs->julian_days) {
                return -1;
            } else if (lhs->julian_days > rhs->julian_days) {
                return 1;
            } else {
                return 0;
            }
        } else if (lhs->dmy && rhs->dmy)  {
            if (lhs->year < rhs->year) {
                return -1;
            } else if (lhs->year > rhs->year) {
                return 1;
            } else {
                if (lhs->month < rhs->month) {
                    return -1;
                } else if (lhs->month > rhs->month) {
                    return 1;
                } else {
                    if (lhs->day < rhs->day) {
                        return -1;
                    } else if (lhs->day > rhs->day) {
                        return 1;
                    } else {
                        return 0;
                    }
                }
            }
        } else {
            if (!lhs->julian) {
                x_date_update_julian(lhs);
            }

            if (!rhs->julian) {
                x_date_update_julian(rhs);
            }

            x_return_val_if_fail(lhs->julian, 0);
            x_return_val_if_fail(rhs->julian, 0);
        }
    }

    return 0;
}

void x_date_to_struct_tm(const XDate *d, struct tm *tm)
{
    XDateWeekday day;

    x_return_if_fail(x_date_valid(d));
    x_return_if_fail(tm != NULL);

    if (!d->dmy) {
        x_date_update_dmy(d);
    }
    x_return_if_fail(d->dmy != 0);

    memset(tm, 0x0, sizeof(struct tm));

    tm->tm_mday = d->day;
    tm->tm_mon  = d->month - 1;
    tm->tm_year = ((int)d->year) - 1900;

    day = x_date_get_weekday(d);
    if (day == 7) {
        day = (XDateWeekday)0;
    }
    tm->tm_wday = (int)day;

    tm->tm_yday = x_date_get_day_of_year(d) - 1;
    tm->tm_isdst = -1;
}

void x_date_clamp(XDate *date, const XDate *min_date, const XDate *max_date)
{
    x_return_if_fail(x_date_valid(date));

    if (min_date != NULL) {
        x_return_if_fail(x_date_valid(min_date));
    }

    if (max_date != NULL) {
        x_return_if_fail(x_date_valid(max_date));
    }

    if (min_date != NULL && max_date != NULL) {
        x_return_if_fail(x_date_compare(min_date, max_date) <= 0);
    }

    if (min_date && x_date_compare(date, min_date) < 0) {
        *date = *min_date;
    }

    if (max_date && x_date_compare(max_date, date) < 0) {
        *date = *max_date;
    }
}

void x_date_order(XDate *date1, XDate *date2)
{
    x_return_if_fail(x_date_valid(date1));
    x_return_if_fail(x_date_valid(date2));

    if (x_date_compare(date1, date2) > 0) {
        XDate tmp = *date1;
        *date1 = *date2;
        *date2 = tmp;
    }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"

xsize x_date_strftime(xchar *s, xsize slen, const xchar *format, const XDate *d)
{
    xsize retval;
    struct tm tm;
    xsize tmplen;
    xchar *tmpbuf;
    xchar *convbuf;
    xsize tmpbufsize;
    xsize convlen = 0;
    XError *error = NULL;
    xchar *locale_format;
    xsize locale_format_len = 0;

    x_return_val_if_fail(x_date_valid(d), 0);
    x_return_val_if_fail(slen > 0, 0); 
    x_return_val_if_fail(format != NULL, 0);
    x_return_val_if_fail(s != NULL, 0);

    x_date_to_struct_tm(d, &tm);

    locale_format = x_locale_from_utf8(format, -1, NULL, &locale_format_len, &error);
    if (error) {
        x_warning(X_STRLOC "Error converting format to locale encoding: %s", error->message);
        x_error_free(error);

        s[0] = '\0';
        return 0;
    }

    tmpbufsize = MAX(128, locale_format_len * 2);
    while (TRUE) {
        tmpbuf = (xchar *)x_malloc(tmpbufsize);

        tmpbuf[0] = '\1';
        tmplen = strftime(tmpbuf, tmpbufsize, locale_format, &tm);

        if (tmplen == 0 && tmpbuf[0] != '\0') {
            x_free(tmpbuf);
            tmpbufsize *= 2;

            if (tmpbufsize > 65536) {
                x_warning(X_STRLOC "Maximum buffer size for x_date_strftimeexceeded: giving up");
                x_free(locale_format);

                s[0] = '\0';
                return 0;
            }
        } else {
            break;
        }
    }
    x_free(locale_format);

    convbuf = x_locale_to_utf8(tmpbuf, tmplen, NULL, &convlen, &error);
    x_free(tmpbuf);

    if (error) {
        x_warning(X_STRLOC "Error converting results of strftime to UTF-8: %s", error->message);
        x_error_free (error);

        x_assert(convbuf == NULL);

        s[0] = '\0';
        return 0;
    }

    if (slen <= convlen) {
        xchar *end = x_utf8_find_prev_char(convbuf, convbuf + slen);
        x_assert(end != NULL);
        convlen = end - convbuf;

        retval = 0;
    } else {
        retval = convlen;
    }

    memcpy(s, convbuf, convlen);
    s[convlen] = '\0';
    x_free(convbuf);

    return retval;
}

#pragma GCC diagnostic pop
