#ifndef XINDUN_SDK_LOG_H
#define XINDUN_SDK_LOG_H

#include <linux/kconfig.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdarg.h>

#if defined __GNUC__
#define LIBLOG_PRINTF(m, n)                     __attribute__((format(printf, m, n)))
#else
#define LIBLOG_PRINTF(m, n)
#endif

#if defined(CONFIG_LIBLOG_COLOR)
#define COLOR_OUTPUT_TERMINAL_ENABLE            (1)
#else
#define COLOR_OUTPUT_TERMINAL_ENABLE            (0)
#endif

struct liblog_category;

struct liblog_msg {
    char   *buf;
    size_t len;
    char   *path;
};

typedef int (*liblog_record_func)(struct liblog_msg *msg);

/**
 * 函数名称: liblog_exit
 * 功能描述: 清理liblog api申请的内存，关闭它们打开的文件
 * 输入参数: 无
 * 输出参数: 无
 * 返回说明: 无
 */
void liblog_exit(void);

/**
 * 函数名称: liblog_init
 * 功能描述: 从配置文件中读取配置信息到内存
 * 输入参数: config_file --> 日志配置文件(如果为NULL，寻找LIBLOG_CONF_PATH的值作为配置文件名，如果LIBLOG_CONF_PATH也没有，所有日至以内置格式写到标准输出上)
 * 输出参数: 无
 * 返回说明: 成功返回0，失败返回-1
 */
int liblog_init(const char *config_file);
int liblog_init_from_string(const char *config_string);

/**
 * 函数名称: liblog_reload
 * 功能描述: 从config_file重载配置，并根据这个配置文件来重计算内部的分类规则匹配、重建每个线程的缓存、并设置原有的用户自定义输出函数
 * 输入参数: config_file --> 日志配置文件
 * 输出参数: 无
 * 返回说明: 成功返回0，失败返回-1
 */
int liblog_reload(const char *config_file);
int liblog_reload_from_string(const char *conf_string);

/**
 * 函数名称: liblog_profile
 * 功能描述: 打印所有内存中的配置信息到LIBLOG_PROFILE_ERROR
 * 输入参数: 无
 * 输出参数: 无
 * 返回说明: 无
 */
void liblog_profile(void);

struct liblog_category *liblog_get_category_handle(void);
void liblog_set_category_handle(struct liblog_category *handle);

struct liblog_category *liblog_get_category(const char *cname);
int liblog_level_enabled(struct liblog_category *category, const int level);

void liblog_clean_mdc(void);
char *liblog_get_mdc(const char *key);
void liblog_remove_mdc(const char *key);
int liblog_put_mdc(const char *key, const char *value);

int liblog_level_switch(struct liblog_category *category, int level);
int liblog_level_enabled(struct liblog_category *category, int level);

void liblog_vprintf(struct liblog_category *category, const char *file, size_t filelen, const char *func, size_t funclen, long line, int level, const char *format, va_list args);
void liblog_printf_hex(struct liblog_category *category, const char *file, size_t filelen, const char *func, size_t funclen, long line, int level, const char *buf, size_t buflen);
void liblog_printf(struct liblog_category *category, const char *file, size_t filelen, const char *func, size_t funclen, long line, int level, const char *format, ...) LIBLOG_PRINTF(8, 9);

/**
 * 函数名称: liblog_default_set_category
 * 功能描述: 改变默认分类
 * 输入参数: cname --> 新默认分类名
 * 输出参数: 无
 * 返回说明: 成功返回0，失败返回-1
 */
int liblog_default_set_category(const char *cname);

/**
 * 函数名称: liblog_default_init
 * 功能描述: 从配置文件中读取配置信息到内存(采用内置的一个默认分类)
 * 输入参数: config_file --> 日志配置文件
 *          cname       --> 设置的分类名
 * 输出参数: 无
 * 返回说明: 成功返回0，失败返回-1
 */
int liblog_default_init(const char *config_file, const char *cname);

void liblog_default_vprintf(const char *file, size_t filelen, const char *func, size_t funclen, long line, int level, const char *format, va_list args);
void liblog_default_printf_hex(const char *file, size_t filelen, const char *func, size_t funclen, long line, int level, const void *buf, size_t buflen);
void liblog_default_printf(const char *file, size_t filelen, const char *func, size_t funclen, long line, int level, const char *format, ...) LIBLOG_PRINTF(7, 8);

int liblog_set_record(const char *rname, liblog_record_func record);

const char *liblog_get_version(void);

enum {
    LIBLOG_LEVEL_DEBUG  = 20,
    LIBLOG_LEVEL_INFO   = 40,
    LIBLOG_LEVEL_NOTICE = 60,
    LIBLOG_LEVEL_WARN   = 80,
    LIBLOG_LEVEL_ERROR  = 100,
    LIBLOG_LEVEL_FATAL  = 120
};

#if !defined(__STDC_VERSION__) || (__STDC_VERSION__ < 199901L)
#if defined __GNUC__ && __GNUC__ >= 2
#define __func__                                __FUNCTION__
#else
#define __func__                                "<unknown>"
#endif
#endif

#if defined __STDC_VERSION__ && (__STDC_VERSION__ >= 199901L)
#define liblog_fatal(cat, ...)                  liblog_printf(cat, __FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, LIBLOG_LEVEL_FATAL, __VA_ARGS__)
#define liblog_error(cat, ...)                  liblog_printf(cat, __FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, LIBLOG_LEVEL_ERROR, __VA_ARGS__)
#define liblog_warn(cat, ...)                   liblog_printf(cat, __FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, LIBLOG_LEVEL_WARN, __VA_ARGS__)
#define liblog_notice(cat, ...)                 liblog_printf(cat, __FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, LIBLOG_LEVEL_NOTICE, __VA_ARGS__)
#define liblog_info(cat, ...)                   liblog_printf(cat, __FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, LIBLOG_LEVEL_INFO, __VA_ARGS__)
#define liblog_debug(cat, ...)                  liblog_printf(cat, __FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, LIBLOG_LEVEL_DEBUG, __VA_ARGS__)

#define liblog_default_fatal(...)               liblog_default_printf(__FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, LIBLOG_LEVEL_FATAL, __VA_ARGS__)
#define liblog_default_error(...)               liblog_default_printf(__FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, LIBLOG_LEVEL_ERROR, __VA_ARGS__)
#define liblog_default_warn(...)                liblog_default_printf(__FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, LIBLOG_LEVEL_WARN, __VA_ARGS__)
#define liblog_default_notice(...)              liblog_default_printf(__FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, LIBLOG_LEVEL_NOTICE, __VA_ARGS__)
#define liblog_default_info(...)                liblog_default_printf(__FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, LIBLOG_LEVEL_INFO, __VA_ARGS__)
#define liblog_default_debug(...)               liblog_default_printf(__FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, LIBLOG_LEVEL_DEBUG, __VA_ARGS__)

#elif defined __GNUC__

#define liblog_fatal(cat, format, args...)      liblog_printf(cat, __FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, LIBLOG_LEVEL_FATAL, format, ##args)
#define liblog_error(cat, format, args...)      liblog_printf(cat, __FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, LIBLOG_LEVEL_ERROR, format, ##args)
#define liblog_warn(cat, format, args...)       liblog_printf(cat, __FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, LIBLOG_LEVEL_WARN, format, ##args)
#define liblog_notice(cat, format, args...)     liblog_printf(cat, __FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, LIBLOG_LEVEL_NOTICE, format, ##args)
#define liblog_info(cat, format, args...)       liblog_printf(cat, __FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, LIBLOG_LEVEL_INFO, format, ##args)
#define liblog_debug(cat, format, args...)      liblog_printf(cat, __FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, LIBLOG_LEVEL_DEBUG, format, ##args)

#define liblog_default_fatal(format, args...)   liblog_default_printf(__FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, LIBLOG_LEVEL_FATAL, format, ##args)
#define liblog_default_error(format, args...)   liblog_default_printf(__FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, LIBLOG_LEVEL_ERROR, format, ##args)
#define liblog_default_warn(format, args...)    liblog_default_printf(__FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, LIBLOG_LEVEL_WARN, format, ##args)
#define liblog_default_notice(format, args...)  liblog_default_printf(__FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, LIBLOG_LEVEL_NOTICE, format, ##args)
#define liblog_default_info(format, args...)    liblog_default_printf(__FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, LIBLOG_LEVEL_INFO, format, ##args)
#define liblog_default_debug(format, args...)   liblog_default_printf(__FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, LIBLOG_LEVEL_DEBUG, format, ##args)
#endif

#define liblog_raw(...)                         printf(__VA_ARGS__)

#define liblog_vfatal(cat, format, args)        liblog_vprintf(cat, __FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, LIBLOG_LEVEL_FATAL, format, args)
#define liblog_verror(cat, format, args)        liblog_vprintf(cat, __FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, LIBLOG_LEVEL_ERROR, format, args)
#define liblog_vwarn(cat, format, args)         liblog_vprintf(cat, __FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, LIBLOG_LEVEL_WARN, format, args)
#define liblog_vnotice(cat, format, args)       liblog_vprintf(cat, __FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, LIBLOG_LEVEL_NOTICE, format, args)
#define liblog_vinfo(cat, format, args)         liblog_vprintf(cat, __FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, LIBLOG_LEVEL_INFO, format, args)
#define liblog_vdebug(cat, format, args)        liblog_vprintf(cat, __FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, LIBLOG_LEVEL_DEBUG, format, args)

#define liblog_hex_fatal(cat, buf, buf_len)     liblog_printf_hex(cat, __FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, LIBLOG_LEVEL_FATAL, buf, buf_len)
#define liblog_hex_error(cat, buf, buf_len)     liblog_printf_hex(cat, __FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, LIBLOG_LEVEL_ERROR, buf, buf_len)
#define liblog_hex_warn(cat, buf, buf_len)      liblog_printf_hex(cat, __FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, LIBLOG_LEVEL_WARN, buf, buf_len)
#define liblog_hex_notice(cat, buf, buf_len)    liblog_printf_hex(cat, __FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, LIBLOG_LEVEL_NOTICE, buf, buf_len)
#define liblog_hex_info(cat, buf, buf_len)      liblog_printf_hex(cat, __FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, LIBLOG_LEVEL_INFO, buf, buf_len)
#define liblog_hex_debug(cat, buf, buf_len)     liblog_printf_hex(cat, __FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, LIBLOG_LEVEL_DEBUG, buf, buf_len)

#define liblog_default_vfatal(format, args)     liblog_default_vprintf(__FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, LIBLOG_LEVEL_FATAL, format, args)
#define liblog_default_verror(format, args)     liblog_default_vprintf(__FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, LIBLOG_LEVEL_ERROR, format, args)
#define liblog_default_vwarn(format, args)      liblog_default_vprintf(__FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, LIBLOG_LEVEL_WARN, format, args)
#define liblog_default_vnotice(format, args)    liblog_default_vprintf(__FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, LIBLOG_LEVEL_NOTICE, format, args)
#define liblog_default_vinfo(format, args)      liblog_default_vprintf(__FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, LIBLOG_LEVEL_INFO, format, args)
#define liblog_default_vdebug(format, args)     liblog_default_vprintf(__FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, LIBLOG_LEVEL_DEBUG, format, args)

#define liblog_default_hex_fatal(buf, buf_len)  liblog_default_printf_hex(__FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, LIBLOG_LEVEL_FATAL, buf, buf_len)
#define liblog_default_hex_error(buf, buf_len)  liblog_default_printf_hex(__FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, LIBLOG_LEVEL_ERROR, buf, buf_len)
#define liblog_default_hex_warn(buf, buf_len)   liblog_default_printf_hex(__FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, LIBLOG_LEVEL_WARN, buf, buf_len)
#define liblog_default_hex_notice(buf, buf_len) liblog_default_printf_hex(__FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, LIBLOG_LEVEL_NOTICE, buf, buf_len)
#define liblog_default_hex_info(buf, buf_len)   liblog_default_printf_hex(__FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, LIBLOG_LEVEL_INFO, buf, buf_len)
#define liblog_default_hex_debug(buf, buf_len)  liblog_default_printf_hex(__FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, LIBLOG_LEVEL_DEBUG, buf, buf_len)

#define liblog_fatal_enabled(logc)              liblog_level_enabled(logc, LIBLOG_LEVEL_FATAL)
#define liblog_error_enabled(logc)              liblog_level_enabled(logc, LIBLOG_LEVEL_ERROR)
#define liblog_warn_enabled(logc)               liblog_level_enabled(logc, LIBLOG_LEVEL_WARN)
#define liblog_notice_enabled(logc)             liblog_level_enabled(logc, LIBLOG_LEVEL_NOTICE)
#define liblog_info_enabled(logc)               liblog_level_enabled(logc, LIBLOG_LEVEL_INFO)
#define liblog_debug_enabled(logc)              liblog_level_enabled(logc, LIBLOG_LEVEL_DEBUG)

#ifdef __cplusplus
}
#endif

#endif
