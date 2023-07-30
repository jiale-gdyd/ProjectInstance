#ifndef TBOX_BASE_LOG_H
#define TBOX_BASE_LOG_H

//! Define log levels
#define LOG_LEVEL_FATAL     0   //!< Program will crash
#define LOG_LEVEL_ERROR     1   //!< Got serious problem, program can't handle
#define LOG_LEVEL_WARN      2   //!< Got inner abnormal situation, but program can handle it
#define LOG_LEVEL_NOTICE    3   //!< It not big problem, but we should notice it, such as invalid data input
#define LOG_LEVEL_INFO      4   //!< Normal message exchange with other program
#define LOG_LEVEL_DEBUG     5   //!< Normal process inside program
#define LOG_LEVEL_TRACE     6   //!< Temporary debugging log

//! Module ID
#ifndef LOG_MODULE_ID
//#warning "Please define LOG_MODULE_ID as your module name, otherwise it will be ???"
#define LOG_MODULE_ID   "???"
#endif //LOG_MODULE_ID

//! Define commonly macros
#define LogPrintf(level, fmt, ...) \
    LogPrintfFunc(LOG_MODULE_ID, __func__, __FILE__, __LINE__, level, true, fmt, ## __VA_ARGS__)

#define LogPuts(level, text) \
    LogPrintfFunc(LOG_MODULE_ID, __func__, __FILE__, __LINE__, level, false, text)

#define LogFatal(fmt, ...)  LogPrintf(LOG_LEVEL_FATAL,  fmt, ## __VA_ARGS__)
#define LogErr(fmt, ...)    LogPrintf(LOG_LEVEL_ERROR,  fmt, ## __VA_ARGS__)
#define LogWarn(fmt, ...)   LogPrintf(LOG_LEVEL_WARN,   fmt, ## __VA_ARGS__)
#define LogNotice(fmt, ...) LogPrintf(LOG_LEVEL_NOTICE, fmt, ## __VA_ARGS__)
#define LogInfo(fmt, ...)   LogPrintf(LOG_LEVEL_INFO,   fmt, ## __VA_ARGS__)

#if !defined(STATIC_LOG_LEVEL) || (STATIC_LOG_LEVEL >= LOG_LEVEL_DEBUG)
    #define LogDbg(fmt, ...)    LogPrintf(LOG_LEVEL_DEBUG, fmt, ## __VA_ARGS__)
#else
    #define LogDbg(fmt, ...)
#endif

#if !defined(STATIC_LOG_LEVEL) || (STATIC_LOG_LEVEL >= LOG_LEVEL_TRACE)
    #define LogTrace(fmt, ...)  LogPrintf(LOG_LEVEL_TRACE, fmt, ## __VA_ARGS__)
    #define LogTag()            LogPuts(LOG_LEVEL_TRACE, "==> Run Here <==")
#else
    #define LogTrace(fmt, ...)
    #define LogTag()
#endif

#define LogUndo() LogPuts(LOG_LEVEL_NOTICE, "!!! Undo !!!")

//! 打印错误码，需要 #include <string.h>
#define LogErrno(err, fmt, ...) LogErr("Errno:%d(%s) " fmt, (err), strerror(err), ## __VA_ARGS__)

#ifdef __cplusplus
extern "C" {
#endif

//!
//! \brief  Log print function
//!
//! \param  module_id   Module Id
//! \param  func_name   Function name
//! \param  file_name   File name
//! \param  line        Code line
//! \param  level       Log level
//! \param  with_args   Whether with args
//! \param  fmt         Log format string
//!
//! \note   We only declare this function here.
//!         It's your duty to implement it.
//!
void LogPrintfFunc(const char *module_id, const char *func_name, const char *file_name,
                   int line, int level, bool with_args, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif
