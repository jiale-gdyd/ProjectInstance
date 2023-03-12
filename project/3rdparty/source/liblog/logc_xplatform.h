#ifndef LOGC_XPLATFORM_H
#define LOGC_XPLATFORM_H

#include <limits.h>

#define LOG_INT32_LEN                       (sizeof("-2147483648") - 1)
#define LOG_INT64_LEN                       (sizeof("-9223372036854775808") - 1)

#if ((__GNU__ == 2) && (__GNUC_MINOR__ < 8))
#define LOG_MAX_UINT32_VALUE                ((uint32_t)0xFFFFFFFFLL)
#else
#define LOG_MAX_UINT32_VALUE                ((uint32_t)0xFFFFFFFF)
#endif

#define LOG_MAX_INT32_VALUE                 ((uint32_t)0x7FFFFFFF)

#define LOG_MAXLINES_NO                     (128)
#define LOG_MAXLEN_PATH                     (1024)
#define LOG_MAXLEN_CFG_LINE                 (LOG_MAXLEN_PATH * 4)

#define LOG_FILE_NEWLINE                    "\n"
#define LOG_FILE_NEWLINE_LEN                (1)

#include <string.h>
#include <strings.h>

#define LOG_STRCMP(_a_, _C_, _b_)           (strcmp(_a_, _b_) _C_ 0)
#define LOG_STRNCMP(_a_, _C_, _b_, _n_)     (strncmp(_a_, _b_, _n_) _C_ 0)
#define LOG_STRICMP(_a_, _C_, _b_)          (strcasecmp(_a_, _b_) _C_ 0)
#define LOG_STRNICMP(_a_, _C_, _b_, _n_)    (strncasecmp(_a_, _b_, _n_) _C_ 0)

#define log_fstat                           fstat
#define log_stat                            stat

#define log_fsync                           fdatasync

#endif
