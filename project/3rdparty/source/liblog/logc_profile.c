#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/types.h>

#include "fmacros.h"
#include "logc_profile.h"
#include "logc_xplatform.h"

static void logc_time(char *time_str, size_t time_str_size)
{
    struct tm tmInfo;
    struct timeval st;
    size_t size_len = 0;

    gettimeofday(&st, NULL);
    localtime_r(&st.tv_sec, &tmInfo);
    strftime(time_str, time_str_size, "%Y-%m-%d %H:%M:%S", &tmInfo);

    size_len = strlen(time_str);
    st.tv_usec /= 1000;
    sprintf(time_str + size_len, ".%03d", (int)(st.tv_usec));
}

int logc_profile_inner(int flag, const char *file, const long line, const char *fmt, ...)
{
    va_list args;
    FILE *fp = NULL;
    char time_str[24 + 1];
    static size_t init_flag = 0;
    static char *debug_log = NULL;
    static char *error_log = NULL;

    if (!init_flag) {
        init_flag = 1;
        debug_log = getenv("LOG_PROFILE_DEBUG");
        error_log = getenv("LOG_PROFILE_ERROR");
    }

    switch (flag) {
    case LOGC_DEBUG:
        if (debug_log == NULL) {
            return 0;
        }

        fp = fopen(debug_log, "a");
        if (!fp) {
            return -1;
        }

        logc_time(time_str, sizeof(time_str));
        fprintf(fp, "<D>[%s]<->[%d:%ld][%s:%ld] ", time_str, getpid(), pthread_self(), file, line);
        break;

    case LOGC_WARN:
        if (error_log == NULL) {
            return 0;
        }

        fp = fopen(error_log, "a");
        if (!fp) {
            return -1;
        }

        logc_time(time_str, sizeof(time_str));
        fprintf(fp, "<W>[%s]<->[%d:%ld][%s:%ld] ", time_str, getpid(), pthread_self(), file, line);
        break;

    case LOGC_ERROR:
        if (error_log == NULL) {
            return 0;
        }

        fp = fopen(error_log, "a");
        if (!fp) {
            return -1;
        }

        logc_time(time_str, sizeof(time_str));
        fprintf(fp, "<E>[%s]<->[%d:%ld][%s:%ld] ", time_str, getpid(), pthread_self(), file, line);
        break;
    }

    va_start(args, fmt);
    vfprintf(fp, fmt, args);
    va_end(args);
    fprintf(fp, "\n");

    fclose(fp);
    return 0;
}
