#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>

#include <xlib/xlib/config.h>
#include <xlib/xlib/xlibconfig.h>

#include <xlib/xlib/xmem.h>
#include <xlib/xlib/xmain.h>
#include <xlib/xlib/xtypes.h>
#include <xlib/xlib/xtimer.h>
#include <xlib/xlib/xstrfuncs.h>
#include <xlib/xlib/xtestutils.h>

struct _XTimer {
    xuint64 start;
    xuint64 end;
    xuint   active : 1;
};

XTimer *x_timer_new(void)
{
    XTimer *timer;

    timer = x_new(XTimer, 1);
    timer->active = TRUE;
    timer->start = x_get_monotonic_time();

    return timer;
}

void x_timer_destroy(XTimer *timer)
{
    x_return_if_fail(timer != NULL);
    x_free(timer);
}

void x_timer_start(XTimer *timer)
{
    x_return_if_fail(timer != NULL);

    timer->active = TRUE;
    timer->start = x_get_monotonic_time();
}

void x_timer_stop(XTimer *timer)
{
    x_return_if_fail(timer != NULL);

    timer->active = FALSE;
    timer->end = x_get_monotonic_time();
}

void x_timer_reset(XTimer *timer)
{
    x_return_if_fail(timer != NULL);
    timer->start = x_get_monotonic_time();
}

void x_timer_continue(XTimer *timer)
{
    xuint64 elapsed;

    x_return_if_fail(timer != NULL);
    x_return_if_fail(timer->active == FALSE);

    elapsed = timer->end - timer->start;
    timer->start = x_get_monotonic_time();
    timer->start -= elapsed;
    timer->active = TRUE;
}

xdouble x_timer_elapsed(XTimer *timer, xulong *microseconds)
{
    xdouble total;
    xint64 elapsed;

    x_return_val_if_fail(timer != NULL, 0);

    if (timer->active) {
        timer->end = x_get_monotonic_time();
    }

    elapsed = timer->end - timer->start;
    total = elapsed / 1e6;

    if (microseconds) {
        *microseconds = elapsed % 1000000;
    }

    return total;
}

xboolean x_timer_is_active(XTimer *timer)
{
    x_return_val_if_fail(timer != NULL, FALSE);
    return timer->active;
}

void x_usleep(xulong microseconds)
{
    if X_UNLIKELY(microseconds == 0) {
        return;
    }

    struct timespec request, remaining;

    request.tv_sec = microseconds / X_USEC_PER_SEC;
    request.tv_nsec = 1000 * (microseconds % X_USEC_PER_SEC);

    while (nanosleep(&request, &remaining) == -1 && errno == EINTR) {
        request = remaining;
    }
}

X_GNUC_BEGIN_IGNORE_DEPRECATIONS
void  x_time_val_add(XTimeVal *time_, xlong microseconds)
{
    x_return_if_fail(time_ != NULL && time_->tv_usec >= 0 && time_->tv_usec < X_USEC_PER_SEC);

    if (microseconds >= 0) {
        time_->tv_usec += microseconds % X_USEC_PER_SEC;
        time_->tv_sec += microseconds / X_USEC_PER_SEC;
        if (time_->tv_usec >= X_USEC_PER_SEC) {
            time_->tv_usec -= X_USEC_PER_SEC;
            time_->tv_sec++;
        }
    } else {
        microseconds *= -1;
        time_->tv_usec -= microseconds % X_USEC_PER_SEC;
        time_->tv_sec -= microseconds / X_USEC_PER_SEC;
        if (time_->tv_usec < 0) {
            time_->tv_usec += X_USEC_PER_SEC;
            time_->tv_sec--;
        }
    }
}
X_GNUC_END_IGNORE_DEPRECATIONS

static time_t mktime_utc(struct tm *tm)
{
    time_t retval;
    static const xint days_before[] = {
        0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
    };

    retval = timegm(tm);
    return retval;
}

X_GNUC_BEGIN_IGNORE_DEPRECATIONS
xboolean x_time_val_from_iso8601(const xchar *iso_date, XTimeVal *time_)
{
    long val;
    struct tm tm = {0};
    long hour, min, sec;
    long mday, mon, year;

    x_return_val_if_fail(iso_date != NULL, FALSE);
    x_return_val_if_fail(time_ != NULL, FALSE);

    while (x_ascii_isspace(*iso_date)) {
        iso_date++;
    }

    if (*iso_date == '\0') {
        return FALSE;
    }

    if (!x_ascii_isdigit(*iso_date) && *iso_date != '+') {
        return FALSE;
    }

    val = strtoul(iso_date, (char **)&iso_date, 10);
    if (*iso_date == '-') {
        year = val;
        iso_date++;

        mon = strtoul(iso_date, (char **)&iso_date, 10);
        if (*iso_date++ != '-') {
            return FALSE;
        }
        
        mday = strtoul(iso_date, (char **)&iso_date, 10);
    } else {
        mday = val % 100;
        mon = (val % 10000) / 100;
        year = val / 10000;
    }

    if (year < 1900 || year > X_MAXINT) {
        return FALSE;
    }

    if (mon < 1 || mon > 12) {
        return FALSE;
    }

    if (mday < 1 || mday > 31) {
        return FALSE;
    }

    tm.tm_mday = mday;
    tm.tm_mon = mon - 1;
    tm.tm_year = year - 1900;

    if (*iso_date != 'T') {
        return FALSE;
    }
    iso_date++;

    if (!x_ascii_isdigit(*iso_date)) {
        return FALSE;
    }

    val = strtoul(iso_date, (char **)&iso_date, 10);
    if (*iso_date == ':') {
        hour = val;
        iso_date++;
        min = strtoul(iso_date, (char **)&iso_date, 10);
        
        if (*iso_date++ != ':') {
            return FALSE;
        }

        sec = strtoul(iso_date, (char **)&iso_date, 10);
    } else {
        sec = val % 100;
        min = (val % 10000) / 100;
        hour = val / 10000;
    }

    if (hour > 23) {
        return FALSE;
    }

    if (min > 59) {
        return FALSE;
    }

    if (sec > 61) {
        return FALSE;
    }

    tm.tm_hour = hour;
    tm.tm_min = min;
    tm.tm_sec = sec;
    time_->tv_usec = 0;

    if (*iso_date == ',' || *iso_date == '.') {
        xlong mul = 100000;

        while (mul >= 1 && x_ascii_isdigit(*++iso_date)) {
            time_->tv_usec += (*iso_date - '0') * mul;
            mul /= 10;
        }

        while (x_ascii_isdigit(*iso_date)) {
            iso_date++;
        }
    }

    if (*iso_date == 'Z') {
        iso_date++;
        time_->tv_sec = mktime_utc(&tm);
    } else if (*iso_date == '+' || *iso_date == '-') {
        xint sign = (*iso_date == '+') ? -1 : 1;
        
        val = strtoul(iso_date + 1, (char **)&iso_date, 10);
        
        if (*iso_date == ':') {
            hour = val;
            min = strtoul(iso_date + 1, (char **)&iso_date, 10);
        } else {
            hour = val / 100;
            min = val % 100;
        }

        if (hour > 99) {
            return FALSE;
        }

        if (min > 59) {
            return FALSE;
        }

        time_->tv_sec = mktime_utc(&tm) + (time_t)(60 * (xint64)(60 * hour + min) * sign);
    } else {
        tm.tm_isdst = -1;
        time_->tv_sec = mktime(&tm);
    }

    while (x_ascii_isspace(*iso_date)) {
        iso_date++;
    }

    return *iso_date == '\0';
}
X_GNUC_END_IGNORE_DEPRECATIONS

X_GNUC_BEGIN_IGNORE_DEPRECATIONS
xchar *x_time_val_to_iso8601(XTimeVal *time_)
{
    time_t secs;
    xchar *retval;
    struct tm *tm;
    struct tm tm_;

    x_return_val_if_fail(time_ != NULL && time_->tv_usec >= 0 && time_->tv_usec < X_USEC_PER_SEC, NULL);

    secs = time_->tv_sec;
    tm = gmtime_r(&secs, &tm_);

    if (tm == NULL) {
        return NULL;
    }

    if (time_->tv_usec != 0) {
        retval = x_strdup_printf("%4d-%02d-%02dT%02d:%02d:%02d.%06ldZ",
            tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, time_->tv_usec);
    } else {
        retval = x_strdup_printf("%4d-%02d-%02dT%02d:%02d:%02dZ",
            tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
    }

    return retval;
}
X_GNUC_END_IGNORE_DEPRECATIONS
