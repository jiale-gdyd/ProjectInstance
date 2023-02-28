#ifndef __X_UTILS_H__
#define __X_UTILS_H__

#include <stdarg.h>
#include "xtypes.h"

X_BEGIN_DECLS

XLIB_AVAILABLE_IN_ALL
const xchar *x_get_user_name(void);

XLIB_AVAILABLE_IN_ALL
const xchar *x_get_real_name(void);

XLIB_AVAILABLE_IN_ALL
const xchar *x_get_home_dir(void);

XLIB_AVAILABLE_IN_ALL
const xchar *x_get_tmp_dir(void);

XLIB_AVAILABLE_IN_ALL
const xchar *x_get_host_name(void);

XLIB_AVAILABLE_IN_ALL
const xchar *x_get_prgname(void);

XLIB_AVAILABLE_IN_ALL
void x_set_prgname(const xchar *prgname);

XLIB_AVAILABLE_IN_ALL
const xchar *x_get_application_name(void);

XLIB_AVAILABLE_IN_ALL
void x_set_application_name(const xchar *application_name);

XLIB_AVAILABLE_IN_2_64
xchar *x_get_os_info(const xchar *key_name);

#define X_OS_INFO_KEY_NAME                  XLIB_AVAILABLE_MACRO_IN_2_64 "NAME"
#define X_OS_INFO_KEY_PRETTY_NAME           XLIB_AVAILABLE_MACRO_IN_2_64 "PRETTY_NAME"
#define X_OS_INFO_KEY_VERSION               XLIB_AVAILABLE_MACRO_IN_2_64 "VERSION"
#define X_OS_INFO_KEY_VERSION_CODENAME      XLIB_AVAILABLE_MACRO_IN_2_64 "VERSION_CODENAME"
#define X_OS_INFO_KEY_VERSION_ID            XLIB_AVAILABLE_MACRO_IN_2_64 "VERSION_ID"
#define X_OS_INFO_KEY_ID                    XLIB_AVAILABLE_MACRO_IN_2_64 "ID"
#define X_OS_INFO_KEY_HOME_URL              XLIB_AVAILABLE_MACRO_IN_2_64 "HOME_URL"
#define X_OS_INFO_KEY_DOCUMENTATION_URL     XLIB_AVAILABLE_MACRO_IN_2_64 "DOCUMENTATION_URL"
#define X_OS_INFO_KEY_SUPPORT_URL           XLIB_AVAILABLE_MACRO_IN_2_64 "SUPPORT_URL"
#define X_OS_INFO_KEY_BUG_REPORT_URL        XLIB_AVAILABLE_MACRO_IN_2_64 "BUG_REPORT_URL"
#define X_OS_INFO_KEY_PRIVACY_POLICY_URL    XLIB_AVAILABLE_MACRO_IN_2_64 "PRIVACY_POLICY_URL"

XLIB_AVAILABLE_IN_ALL
void x_reload_user_special_dirs_cache(void);

XLIB_AVAILABLE_IN_ALL
const xchar *x_get_user_data_dir(void);

XLIB_AVAILABLE_IN_ALL
const xchar *x_get_user_config_dir(void);

XLIB_AVAILABLE_IN_ALL
const xchar *x_get_user_cache_dir(void);

XLIB_AVAILABLE_IN_2_72
const xchar *x_get_user_state_dir(void);

XLIB_AVAILABLE_IN_ALL
const xchar *const *x_get_system_data_dirs(void);

XLIB_AVAILABLE_IN_ALL
const xchar *const *x_get_system_config_dirs(void);

XLIB_AVAILABLE_IN_ALL
const xchar *x_get_user_runtime_dir(void);

typedef enum {
    X_USER_DIRECTORY_DESKTOP,
    X_USER_DIRECTORY_DOCUMENTS,
    X_USER_DIRECTORY_DOWNLOAD,
    X_USER_DIRECTORY_MUSIC,
    X_USER_DIRECTORY_PICTURES,
    X_USER_DIRECTORY_PUBLIC_SHARE,
    X_USER_DIRECTORY_TEMPLATES,
    X_USER_DIRECTORY_VIDEOS,

    X_USER_N_DIRECTORIES
} XUserDirectory;

XLIB_AVAILABLE_IN_ALL
const xchar *x_get_user_special_dir(XUserDirectory directory);

typedef struct _XDebugKey XDebugKey;
struct _XDebugKey {
    const xchar *key;
    xuint       value;
};

XLIB_AVAILABLE_IN_ALL
xuint x_parse_debug_string(const xchar *string, const XDebugKey *keys, xuint nkeys);

XLIB_AVAILABLE_IN_ALL
xint x_snprintf(xchar *string, xulong n, xchar const *format, ...) X_GNUC_PRINTF(3, 4);

XLIB_AVAILABLE_IN_ALL
xint x_vsnprintf(xchar *string, xulong n, xchar const *format, va_list args) X_GNUC_PRINTF(3, 0);

XLIB_AVAILABLE_IN_ALL
void x_nullify_pointer(xpointer *nullify_location);

typedef enum {
    X_FORMAT_SIZE_DEFAULT     = 0,
    X_FORMAT_SIZE_LONG_FORMAT = 1 << 0,
    X_FORMAT_SIZE_IEC_UNITS   = 1 << 1,
    X_FORMAT_SIZE_BITS        = 1 << 2,
    X_FORMAT_SIZE_ONLY_VALUE XLIB_AVAILABLE_ENUMERATOR_IN_2_74 = 1 << 3,
    X_FORMAT_SIZE_ONLY_UNIT XLIB_AVAILABLE_ENUMERATOR_IN_2_74 = 1 << 4
} XFormatSizeFlags;

XLIB_AVAILABLE_IN_2_30
xchar *x_format_size_full(xuint64 size, XFormatSizeFlags flags);

XLIB_AVAILABLE_IN_2_30
xchar *x_format_size(xuint64 size);

XLIB_DEPRECATED_IN_2_30_FOR(x_format_size)
xchar *x_format_size_for_display(xoffset size);

#define x_ATEXIT(proc)                  (atexit(proc)) XLIB_DEPRECATED_MACRO_IN_2_32
#define x_memmove(dest, src, len)       X_STMT_START { memmove((dest), (src), (len)); } X_STMT_END XLIB_DEPRECATED_MACRO_IN_2_40_FOR(memmove)

typedef void (*XVoidFunc)(void) XLIB_DEPRECATED_TYPE_IN_2_32;
#define ATEXIT(proc)                    x_ATEXIT(proc) XLIB_DEPRECATED_MACRO_IN_2_32

X_GNUC_BEGIN_IGNORE_DEPRECATIONS
XLIB_DEPRECATED
void x_atexit(XVoidFunc func);
X_GNUC_END_IGNORE_DEPRECATIONS

XLIB_AVAILABLE_IN_ALL
xchar *x_find_program_in_path (const xchar *program);

#define x_bit_nth_lsf(mask, nth_bit)    x_bit_nth_lsf_impl(mask, nth_bit)
#define x_bit_nth_msf(mask, nth_bit)    x_bit_nth_msf_impl(mask, nth_bit)
#define x_bit_storage(number)           x_bit_storage_impl(number)

XLIB_AVAILABLE_IN_ALL
xint (x_bit_nth_lsf)(xulong mask, xint nth_bit);

XLIB_AVAILABLE_IN_ALL
xint (x_bit_nth_msf)(xulong mask, xint nth_bit);

XLIB_AVAILABLE_IN_ALL
xuint (x_bit_storage)(xulong number);

static inline xint x_bit_nth_lsf_impl(xulong mask, xint nth_bit)
{
    if (X_UNLIKELY(nth_bit < -1)) {
        nth_bit = -1;
    }

    while (nth_bit < ((XLIB_SIZEOF_LONG * 8) - 1)) {
        nth_bit++;
        if (mask & (1UL << nth_bit)) {
            return nth_bit;
        }
    }

    return -1;
}

static inline xint x_bit_nth_msf_impl(xulong mask, xint nth_bit)
{
    if (nth_bit < 0 || X_UNLIKELY(nth_bit > XLIB_SIZEOF_LONG * 8)) {
        nth_bit = XLIB_SIZEOF_LONG * 8;
    }

    while (nth_bit > 0) {
        nth_bit--;
        if (mask & (1UL << nth_bit)) {
            return nth_bit;
        }
    }

    return -1;
}

static inline xuint x_bit_storage_impl(xulong number)
{
#if defined(__GNUC__) && (__GNUC__ >= 4) && defined(__OPTIMIZE__)
    return X_LIKELY(number) ? ((XLIB_SIZEOF_LONG * 8U - 1) ^ (xuint) __builtin_clzl(number)) + 1 : 1;
#else
    xuint n_bits = 0;

    do {
        n_bits++;
        number >>= 1;
    } while (number);

    return n_bits;
#endif
}

#if XLIB_VERSION_MAX_ALLOWED >= GLIB_VERSION_2_50
#include <stdlib.h>
#define x_abort()                                       abort()
#endif

#define X_WIN32_DLLMAIN_FOR_DLL_NAME(static, dll_name)  XLIB_DEPRECATED_MACRO_IN_2_26

X_END_DECLS

#endif
