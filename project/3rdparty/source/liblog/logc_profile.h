#ifndef LIBC_PROFILE_H
#define LIBC_PROFILE_H

#include <stdarg.h>

#define EMPTY()
#define logc_assert(expr, rv)               \
    if (!(expr)) {                          \
        logc_error(#expr" is null or 0");   \
        return rv;                          \
    }

enum logc_profile_flag {
    LOGC_DEBUG = 0,
    LOGC_WARN  = 1,
    LOGC_ERROR = 2
};

#if defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901L
#define logc_debug(...)                   logc_profile_inner(LOGC_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define logc_warn(...)                    logc_profile_inner(LOGC_WARN, __FILE__, __LINE__, __VA_ARGS__)
#define logc_error(...)                   logc_profile_inner(LOGC_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define logc_profile(flag, ...)           logc_profile_inner(flag, __FILE__, __LINE__, __VA_ARGS__)
#elif defined __GNUC__
#define logc_debug(fmt, args...)          logc_profile_inner(LOGC_DEBUG, __FILE__, __LINE__, fmt, ##args)
#define logc_warn(fmt, args...)           logc_profile_inner(LOGC_WARN, __FILE__, __LINE__, fmt, ##args)
#define logc_error(fmt, args...)          logc_profile_inner(LOGC_ERROR, __FILE__, __LINE__, fmt, ##args)
#define logc_profile(flag, fmt, args...)  logc_profile_inner(flag, __FILE__, __LINE__, fmt, ##args)
#endif

int logc_profile_inner(int flag, const char *file, const long line, const char *fmt, ...);

#endif
