#include <pwd.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>
#include <unistd.h>
#include <langinfo.h>
#include <sys/stat.h>
#include <sys/auxv.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/utsname.h>

#include <xlib/xlib/config.h>
#include <xlib/xlib/xhash.h>
#include <xlib/xlib/xutils.h>
#include <xlib/xlib/xarray.h>
#include <xlib/xlib/xstdio.h>
#include <xlib/xlib/xquark.h>
#include <xlib/xlib/xthread.h>
#include <xlib/xlib/xlibintl.h>
#include <xlib/xlib/xunicode.h>
#include <xlib/xlib/xgettext.h>
#include <xlib/xlib/xenviron.h>
#include <xlib/xlib/xstrfuncs.h>
#include <xlib/xlib/xlib-init.h>
#include <xlib/xlib/xtestutils.h>
#include <xlib/xlib/xfileutils.h>
#include <xlib/xlib/xlib-private.h>
#include <xlib/xlib/xutilsprivate.h>

X_GNUC_BEGIN_IGNORE_DEPRECATIONS
void x_atexit(XVoidFunc func)
{
    int errsv;
    xint result;

    result = atexit((void (*)(void))func);
    errsv = errno;
    if (result) {
        x_error("Could not register atexit() function: %s", x_strerror(errsv));
    }
}
X_GNUC_END_IGNORE_DEPRECATIONS

static xchar *my_strchrnul(const xchar *str,  xchar c)
{
    xchar *p = (xchar *)str;
    while (*p && (*p != c)) {
        ++p;
    }

    return p;
}

xchar *x_find_program_in_path(const xchar *program)
{
    return x_find_program_for_path(program, NULL, NULL);
}

char *x_find_program_for_path(const char *program, const char *path, const char *working_dir)
{
    xsize len;
    xsize pathlen;
    const xchar *p;
    xchar *name, *freeme;
    char *program_path = NULL;
    const char *original_path = path;
    const char *original_program = program;

    x_return_val_if_fail(program != NULL, NULL);

    if (working_dir && !x_path_is_absolute(program)) {
        program_path = x_build_filename(working_dir, program, NULL);
        program = program_path;
    }

    if (x_path_is_absolute(program) || strchr(original_program, X_DIR_SEPARATOR) != NULL) {
        if (x_file_test(program, X_FILE_TEST_IS_EXECUTABLE) && !x_file_test(program, X_FILE_TEST_IS_DIR)) {
            xchar *out = NULL;

            if (x_path_is_absolute(program)) {
                out = x_strdup(program);
            } else {
                char *cwd = x_get_current_dir();
                out = x_build_filename(cwd, program, NULL);
                x_free(cwd);
            }

            x_free(program_path);
            return x_steal_pointer(&out);
        } else {
            x_clear_pointer(&program_path, x_free);
            if (x_path_is_absolute(original_program)) {
                return NULL;
            }
        }
    }

    program = original_program;
    if X_LIKELY(original_path == NULL) {
        path = x_getenv("PATH");
    } else {
        path = original_path;
    }

    if (path == NULL) {
        path = "/bin:/usr/bin:.";
    }

    len = strlen(program) + 1;
    pathlen = strlen(path);
    freeme = name = (xchar *)x_malloc(pathlen + len + 1);

    memcpy(name + pathlen + 1, program, len);
    name = name + pathlen;
    *name = X_DIR_SEPARATOR;

    p = path;
    do {
        char *startp;
        char *startp_path = NULL;

        path = p;
        p = my_strchrnul(path, X_SEARCHPATH_SEPARATOR);

        if (p == path) {
            startp = name + 1;
        } else {
            startp = (char *)memcpy(name - (p - path), path, p - path);
        }

        if (working_dir && !x_path_is_absolute(startp)) {
            startp_path = x_build_filename(working_dir, startp, NULL);
            startp = startp_path;
        }

        if (x_file_test(startp, X_FILE_TEST_IS_EXECUTABLE) && !x_file_test(startp, X_FILE_TEST_IS_DIR)) {
            xchar *ret;
            if (x_path_is_absolute(startp)) {
                ret = x_strdup(startp);
            } else {
                xchar *cwd = NULL;
                cwd = x_get_current_dir();
                ret = x_build_filename(cwd, startp, NULL);
                x_free(cwd);
            }

            x_free(program_path);
            x_free(startp_path);
            x_free(freeme);

            return ret;
        }

        x_free(startp_path);
    } while (*p++ != '\0');

    x_free(program_path);
    x_free(freeme);

    return NULL;
}

xint (x_bit_nth_lsf)(xulong mask, xint nth_bit)
{
    return x_bit_nth_lsf_impl(mask, nth_bit);
}

xint (x_bit_nth_msf)(xulong mask, xint nth_bit)
{
    return x_bit_nth_msf_impl(mask, nth_bit);
}

xuint (x_bit_storage)(xulong number)
{
    return x_bit_storage_impl(number);
}

X_LOCK_DEFINE_STATIC(x_utils_global);

typedef struct {
    xchar *user_name;
    xchar *real_name;
    xchar *home_dir;
} UserDatabaseEntry;

static xchar *x_tmp_dir = NULL;
static xchar *x_user_data_dir = NULL;
static xchar *x_user_cache_dir = NULL;
static xchar *x_user_state_dir = NULL;
static xchar *x_user_config_dir = NULL;
static xchar *x_user_runtime_dir = NULL;
static xchar **x_system_data_dirs = NULL;
static xchar **x_user_special_dirs = NULL;
static xchar **x_system_config_dirs = NULL;

#define X_USER_DIRS_EXPIRE              15 * 60

static UserDatabaseEntry *x_get_user_database_entry(void)
{
    static UserDatabaseEntry *entry;

    if (x_once_init_enter_pointer(&entry)) {
        static UserDatabaseEntry e;

        {
            xint error;
            struct passwd pwd;
            const char *logname;
            xpointer buffer = NULL;
            struct passwd *pw = NULL;

#ifdef _SC_GETPW_R_SIZE_MAX
            xlong bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
            if (bufsize < 0) {
                bufsize = 64;
            }
#else
            xlong bufsize = 64;
#endif
            logname = x_getenv("LOGNAME");

            do {
                x_free(buffer);
                buffer = x_malloc(bufsize + 6);
                errno = 0;

                if (logname) {
                    error = getpwnam_r(logname, &pwd, (char *)buffer, bufsize, &pw);
                    if (!pw || (pw->pw_uid != getuid())) {
                        error = getpwuid_r(getuid(), &pwd, (char *)buffer, bufsize, &pw);
                    }
                } else {
                    error = getpwuid_r(getuid(), &pwd, (char *)buffer, bufsize, &pw);
                }

                error = error < 0 ? errno : error;
                if (!pw) {
                    if (error == 0 || error == ENOENT) {
                        x_warning("getpwuid_r(): failed due to unknown user id (%lu)", (xulong)getuid ());
                        break;
                    }

                    if (bufsize > 32 * 1024) {
                        x_warning("getpwuid_r(): failed due to: %s.", x_strerror(error));
                        break;
                    }

                    bufsize *= 2;
                }
            } while (!pw);

            if (!pw) {
                pw = getpwuid(getuid());
            }

            if (pw) {
                e.user_name = x_strdup(pw->pw_name);

                if (pw->pw_gecos && *pw->pw_gecos != '\0' && pw->pw_name) {
                    xchar **name_parts;
                    xchar **gecos_fields;
                    xchar *uppercase_pw_name;

                    gecos_fields = x_strsplit(pw->pw_gecos, ",", 0);
                    name_parts = x_strsplit(gecos_fields[0], "&", 0);
                    uppercase_pw_name = x_strdup(pw->pw_name);
                    uppercase_pw_name[0] = x_ascii_toupper(uppercase_pw_name[0]);
                    e.real_name = x_strjoinv(uppercase_pw_name, name_parts);
                    x_strfreev(gecos_fields);
                    x_strfreev(name_parts);
                    x_free(uppercase_pw_name);
                }

                if (!e.home_dir) {
                    e.home_dir = x_strdup(pw->pw_dir);
                }
            }

            x_free (buffer);
        }

        if (!e.user_name) {
            e.user_name = x_strdup("somebody");
        }

        if (!e.real_name) {
            e.real_name = x_strdup("Unknown");
        }

        x_once_init_leave_pointer(&entry, &e);
    }

    return entry;
}

const xchar *x_get_user_name(void)
{
    UserDatabaseEntry *entry;
    entry = x_get_user_database_entry();

    return entry->user_name;
}

const xchar *x_get_real_name(void)
{
    UserDatabaseEntry *entry;
    entry = x_get_user_database_entry();

    return entry->real_name;
}

static xchar *x_home_dir = NULL;

static xchar *x_build_home_dir(void)
{
    xchar *home_dir;

    home_dir = x_strdup(x_getenv("HOME"));
    if (home_dir == NULL) {
        UserDatabaseEntry *entry = x_get_user_database_entry();
        home_dir = x_strdup(entry->home_dir);
    }

    if (home_dir == NULL) {
        x_warning("Could not find home directory: $HOME is not set, and user database could not be read.");
        home_dir = x_strdup("/");
    }

    return x_steal_pointer(&home_dir);
}

const xchar *x_get_home_dir(void)
{
    const xchar *home_dir;

    X_LOCK(x_utils_global);
    if (x_home_dir == NULL) {
        x_home_dir = x_build_home_dir();
    }
    home_dir = x_home_dir;
    X_UNLOCK(x_utils_global);

    return home_dir;
}

void _x_unset_cached_tmp_dir(void)
{
    X_LOCK(x_utils_global);
    x_ignore_leak(x_tmp_dir);
    x_tmp_dir = NULL;
    X_UNLOCK(x_utils_global);
}

const xchar *x_get_tmp_dir(void)
{
    X_LOCK(x_utils_global);

    if (x_tmp_dir == NULL) {
        xchar *tmp;

        tmp = x_strdup(x_getenv("X_TEST_TMPDIR"));
        if (tmp == NULL || *tmp == '\0') {
            x_free(tmp);
            tmp = x_strdup(x_getenv("TMPDIR"));
        }

#ifdef P_tmpdir
        if (tmp == NULL || *tmp == '\0') {
            xsize k;
            x_free(tmp);
            tmp = x_strdup(P_tmpdir);
            k = strlen(tmp);
            if (k > 1 && X_IS_DIR_SEPARATOR(tmp[k - 1])) {
                tmp[k - 1] = '\0';
            }
        }
#endif
        if (tmp == NULL || *tmp == '\0') {
            x_free(tmp);
            tmp = x_strdup("/tmp");
        }

        x_tmp_dir = x_steal_pointer(&tmp);
    }

    X_UNLOCK(x_utils_global);

    return x_tmp_dir;
}

const xchar *x_get_host_name(void)
{
    static xchar *hostname;

    if (x_once_init_enter_pointer(&hostname)) {
        xsize size;
        xchar *tmp;
        xboolean failed;
        xchar *utmp = NULL;
        const xsize size_large = (xsize) 256 * 256;

#ifdef _SC_HOST_NAME_MAX
        {
            xlong max;

            max = sysconf(_SC_HOST_NAME_MAX);
            if (max > 0 && (xsize)max <= X_MAXSIZE - 1) {
                size = (xsize)max + 1;
            } else
#ifdef HOST_NAME_MAX
                size = HOST_NAME_MAX + 1;
#else
                size = _POSIX_HOST_NAME_MAX + 1;
#endif
        }
#else
        size = 256;
#endif
        tmp = (xchar *)x_malloc(size);
        failed = (gethostname(tmp, size) == -1);
        if (failed && size < size_large) {
            x_free(tmp);
            tmp = (xchar *)x_malloc(size_large);
            failed = (gethostname(tmp, size_large) == -1);
        }

        if (failed) {
            x_clear_pointer(&tmp, x_free);
        }
        utmp = tmp;

        x_once_init_leave_pointer(&hostname, failed ? x_strdup("localhost") : utmp);
    }

    return hostname;
}

static const xchar *x_prgname = NULL;

const xchar *x_get_prgname(void)
{
    return x_atomic_pointer_get(&x_prgname);
}

void x_set_prgname(const xchar *prgname)
{
    prgname = x_intern_string(prgname);
    x_atomic_pointer_set(&x_prgname, prgname);
}

xboolean x_set_prgname_once(const xchar *prgname)
{
    prgname = x_intern_string(prgname);
    return x_atomic_pointer_compare_and_exchange(&x_prgname, NULL, prgname);
}

static xchar *x_application_name = NULL;

const xchar *x_get_application_name(void)
{
    const char *retval;

    retval = x_atomic_pointer_get(&x_application_name);
    if (retval) {
        return retval;
    }

    return x_get_prgname();
}

void x_set_application_name(const xchar *application_name)
{
    char *name;

    x_return_if_fail(application_name);
    name = x_strdup(application_name);

    if (!x_atomic_pointer_compare_and_exchange(&x_application_name, NULL, name)) {
        x_warning("x_set_application_name() called multiple times");
        x_free(name);
    }
}

static xchar *get_os_info_from_os_release(const xchar *key_name, const xchar *buffer)
{
    size_t i;
    XStrv lines;
    xchar *prefix;
    xchar *result = NULL;

    lines = x_strsplit(buffer, "\n", -1);
    prefix = x_strdup_printf("%s=", key_name);
    for (i = 0; lines[i] != NULL; i++) {
        const xchar *value;
        const xchar *line = lines[i];

        if (x_str_has_prefix(line, prefix)) {
            value = line + strlen(prefix);
            result = x_shell_unquote(value, NULL);
            if (result == NULL) {
                result = x_strdup(value);
            }

            break;
        }
    }

    x_strfreev(lines);
    x_free(prefix);

    if (result == NULL) {
        if (x_str_equal(key_name, X_OS_INFO_KEY_NAME)) {
            return x_strdup("Linux");
        }

        if (x_str_equal(key_name, X_OS_INFO_KEY_ID)) {
            return x_strdup("linux");
        }

        if (x_str_equal(key_name, X_OS_INFO_KEY_PRETTY_NAME)) {
            return x_strdup("Linux");
        }
    }

    return x_steal_pointer(&result);
}

static xchar *get_os_info_from_uname(const xchar *key_name)
{
    struct utsname info;

    if (uname(&info) == -1) {
        return NULL;
    }

    if (strcmp(key_name, X_OS_INFO_KEY_NAME) == 0) {
        return x_strdup(info.sysname);
    } else if (strcmp(key_name, X_OS_INFO_KEY_VERSION) == 0) {
        return x_strdup(info.release);
    } else if (strcmp (key_name, X_OS_INFO_KEY_PRETTY_NAME) == 0) {
        return x_strdup_printf("%s %s", info.sysname, info.release);
    } else if (strcmp(key_name, X_OS_INFO_KEY_ID) == 0) {
        xchar *result = x_ascii_strdown(info.sysname, -1);

        x_strcanon(result, "abcdefghijklmnopqrstuvwxyz0123456789_-.", '_');
        return x_steal_pointer(&result);
    } else if (strcmp(key_name, X_OS_INFO_KEY_VERSION_ID) == 0) {
        xchar *result;

        if (strcmp(info.sysname, "NetBSD") == 0) {
            const xchar *c;
            xssize len = X_MAXSSIZE;

            if ((c = strchr(info.release, '-')) != NULL) {
                len = MIN(len, c - info.release);
            }

            if ((c = strchr(info.release, '_')) != NULL) {
                len = MIN(len, c - info.release);
            }

            if (len == X_MAXSSIZE) {
                len = -1;
            }

            result = x_ascii_strdown(info.release, len);
        } else if (strcmp(info.sysname, "GNU") == 0) {
            xssize len = -1;
            const xchar *c = strchr(info.release, '/');

            if (c != NULL) {
                len = c - info.release;
            }

            result = x_ascii_strdown(info.release, len);
        } else if (x_str_has_prefix(info.sysname, "GNU/") || strcmp(info.sysname, "FreeBSD") == 0 || strcmp(info.sysname, "DragonFly") == 0) {
            const xchar *c;
            xssize len = X_MAXSSIZE;

            if ((c = strchr(info.release, '-')) != NULL) {
                len = MIN(len, c - info.release);
            }

            if ((c = strchr(info.release, '(')) != NULL) {
                len = MIN(len, c - info.release);
            }

            if (len == X_MAXSSIZE) {
                len = -1;
            }

            result = x_ascii_strdown(info.release, len);
        } else {
            result = x_ascii_strdown(info.release, -1);
        }

        x_strcanon(result, "abcdefghijklmnopqrstuvwxyz0123456789_-.", '_');
        return x_steal_pointer(&result);
    } else {
        return NULL;
    }
}

xchar *x_get_os_info(const xchar *key_name)
{
    xsize i;
    xchar *buffer = NULL;
    xchar *result = NULL;
    const xchar *const os_release_files[] = { "/etc/os-release", "/usr/lib/os-release" };

    x_return_val_if_fail(key_name != NULL, NULL);

    for (i = 0; i < X_N_ELEMENTS(os_release_files); i++) {
        XError *error = NULL;
        xboolean file_missing;

        if (x_file_get_contents(os_release_files[i], &buffer, NULL, &error)) {
            break;
        }

        file_missing = x_error_matches(error, X_FILE_ERROR, X_FILE_ERROR_NOENT);
        x_clear_error(&error);

        if (!file_missing) {
            return NULL;
        }
    }

    if (buffer != NULL) {
        result = get_os_info_from_os_release(key_name, buffer);
    } else {
        result = get_os_info_from_uname(key_name);
    }

    x_free(buffer);
    return x_steal_pointer(&result);
}

static void set_str_if_different(xchar **global_str, const xchar *type, const xchar *new_value)
{
    if (*global_str == NULL || !x_str_equal(new_value, *global_str)) {
        x_debug("x_set_user_dirs: Setting %s to %s", type, new_value);

        x_ignore_leak(*global_str);
        *global_str = x_strdup(new_value);
    }
}

static void set_strv_if_different(xchar ***global_strv, const xchar *type, const xchar *const *new_value)
{
    if (*global_strv == NULL || !x_strv_equal(new_value, (const xchar * const *) *global_strv)) {
        xchar *new_value_str = x_strjoinv(":", (xchar **)new_value);
        x_debug("x_set_user_dirs: Setting %s to %s", type, new_value_str);
        x_free(new_value_str);

        x_ignore_strv_leak(*global_strv);
        *global_strv = x_strdupv((xchar **)new_value);
    }
}

void x_set_user_dirs(const xchar *first_dir_type, ...)
{
    va_list args;
    const xchar *dir_type;

    X_LOCK(x_utils_global);

    va_start(args, first_dir_type);
    for (dir_type = first_dir_type; dir_type != NULL; dir_type = va_arg(args, const xchar *)) {
        xconstpointer dir_value = va_arg(args, xconstpointer);
        x_assert(dir_value != NULL);

        if (x_str_equal(dir_type, "HOME")) {
            set_str_if_different(&x_home_dir, dir_type, (const xchar *)dir_value);
        } else if (x_str_equal(dir_type, "XDG_CACHE_HOME")) {
            set_str_if_different(&x_user_cache_dir, dir_type, (const xchar *)dir_value);
        } else if (x_str_equal(dir_type, "XDG_CONFIG_DIRS")) {
            set_strv_if_different(&x_system_config_dirs, dir_type, (const xchar *const *)dir_value);
        } else if (x_str_equal(dir_type, "XDG_CONFIG_HOME")) {
            set_str_if_different(&x_user_config_dir, dir_type, (const xchar *)dir_value);
        } else if (x_str_equal(dir_type, "XDG_DATA_DIRS")) {
            set_strv_if_different(&x_system_data_dirs, dir_type, (const xchar *const *)dir_value);
        } else if (x_str_equal(dir_type, "XDG_DATA_HOME")) {
            set_str_if_different(&x_user_data_dir, dir_type, (const xchar *)dir_value);
        } else if (x_str_equal(dir_type, "XDG_STATE_HOME")) {
            set_str_if_different(&x_user_state_dir, dir_type, (const xchar *)dir_value);
        } else if (x_str_equal(dir_type, "XDG_RUNTIME_DIR")) {
            set_str_if_different(&x_user_runtime_dir, dir_type, (const xchar *)dir_value);
        } else {
            x_assert_not_reached();
        }
    }

    va_end(args);
    X_UNLOCK(x_utils_global);
}

static xchar *x_build_user_data_dir(void)
{
    xchar *data_dir = NULL;
    const xchar *data_dir_env = x_getenv("XDG_DATA_HOME");

    if (data_dir_env && data_dir_env[0]) {
        data_dir = x_strdup(data_dir_env);
    }

    if (!data_dir || !data_dir[0]) {
        xchar *home_dir = x_build_home_dir();
        x_free(data_dir);
        data_dir = x_build_filename(home_dir, ".local", "share", NULL);
        x_free(home_dir);
    }

    return x_steal_pointer(&data_dir);
}

const xchar *x_get_user_data_dir(void)
{
    const xchar *user_data_dir;

    X_LOCK(x_utils_global);
    if (x_user_data_dir == NULL) {
        x_user_data_dir = x_build_user_data_dir();
    }
    user_data_dir = x_user_data_dir;
    X_UNLOCK(x_utils_global);

    return user_data_dir;
}

static xchar *x_build_user_config_dir(void)
{
    xchar *config_dir = NULL;
    const xchar *config_dir_env = x_getenv("XDG_CONFIG_HOME");

    if (config_dir_env && config_dir_env[0]) {
        config_dir = x_strdup(config_dir_env);
    }

    if (!config_dir || !config_dir[0]) {
        xchar *home_dir = x_build_home_dir();
        x_free(config_dir);
        config_dir = x_build_filename(home_dir, ".config", NULL);
        x_free(home_dir);
    }

    return x_steal_pointer(&config_dir);
}

const xchar *x_get_user_config_dir(void)
{
    const xchar *user_config_dir;

    X_LOCK(x_utils_global);
    if (x_user_config_dir == NULL) {
        x_user_config_dir = x_build_user_config_dir();
    }
    user_config_dir = x_user_config_dir;
    X_UNLOCK(x_utils_global);

    return user_config_dir;
}

static xchar *x_build_user_cache_dir(void)
{
    xchar *cache_dir = NULL;
    const xchar *cache_dir_env = x_getenv("XDG_CACHE_HOME");

    if (cache_dir_env && cache_dir_env[0]) {
        cache_dir = x_strdup(cache_dir_env);
    }

    if (!cache_dir || !cache_dir[0]) {
        xchar *home_dir = x_build_home_dir();
        x_free(cache_dir);
        cache_dir = x_build_filename(home_dir, ".cache", NULL);
        x_free(home_dir);
    }

    return x_steal_pointer(&cache_dir);
}

const xchar *x_get_user_cache_dir(void)
{
    const xchar *user_cache_dir;

    X_LOCK(x_utils_global);
    if (x_user_cache_dir == NULL) {
        x_user_cache_dir = x_build_user_cache_dir();
    }
    user_cache_dir = x_user_cache_dir;
    X_UNLOCK(x_utils_global);

    return user_cache_dir;
}

static xchar *x_build_user_state_dir(void)
{
    xchar *state_dir = NULL;
    const xchar *state_dir_env = x_getenv("XDG_STATE_HOME");

    if (state_dir_env && state_dir_env[0]) {
        state_dir = x_strdup(state_dir_env);
    }

    if (!state_dir || !state_dir[0]) {
        xchar *home_dir = x_build_home_dir();
        x_free(state_dir);
        state_dir = x_build_filename(home_dir, ".local/state", NULL);
        x_free(home_dir);
    }

    return x_steal_pointer(&state_dir);
}

const xchar *x_get_user_state_dir(void)
{
    const xchar *user_state_dir;

    X_LOCK(x_utils_global);
    if (x_user_state_dir == NULL) {
        x_user_state_dir = x_build_user_state_dir();
    }
    user_state_dir = x_user_state_dir;
    X_UNLOCK(x_utils_global);

    return user_state_dir;
}

static xchar *x_build_user_runtime_dir(void)
{
    xchar *runtime_dir = NULL;
    const xchar *runtime_dir_env = x_getenv("XDG_RUNTIME_DIR");

    if (runtime_dir_env && runtime_dir_env[0]) {
        runtime_dir = x_strdup(runtime_dir_env);
    } else {
        runtime_dir = x_build_user_cache_dir();
        (void)x_mkdir(runtime_dir, 0700);
    }

    return x_steal_pointer(&runtime_dir);
}

const xchar *x_get_user_runtime_dir(void)
{
    const xchar *user_runtime_dir;

    X_LOCK(x_utils_global);
    if (x_user_runtime_dir == NULL) {
        x_user_runtime_dir = x_build_user_runtime_dir();
    }
    user_runtime_dir = x_user_runtime_dir;
    X_UNLOCK(x_utils_global);

    return user_runtime_dir;
}

static void load_user_special_dirs(void)
{
    xchar *data;
    xchar **lines;
    xint n_lines, i;
    xchar *config_file;
    xchar *config_dir = NULL;

    config_dir = x_build_user_config_dir();
    config_file = x_build_filename(config_dir, "user-dirs.dirs", NULL);
    x_free(config_dir);

    if (!x_file_get_contents(config_file, &data, NULL, NULL)) {
        x_free(config_file);
        return;
    }

    lines = x_strsplit(data, "\n", -1);
    n_lines = x_strv_length(lines);
    x_free(data);

    for (i = 0; i < n_lines; i++) {
        xint len;
        xchar *d, *p;
        XUserDirectory directory;
        xchar *buffer = lines[i];
        xboolean is_relative = FALSE;

        len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = 0;
        }

        p = buffer;
        while (*p == ' ' || *p == '\t') {
            p++;
        }

        if (strncmp(p, "XDG_DESKTOP_DIR", strlen("XDG_DESKTOP_DIR")) == 0) {
            directory = X_USER_DIRECTORY_DESKTOP;
            p += strlen ("XDG_DESKTOP_DIR");
        } else if (strncmp(p, "XDG_DOCUMENTS_DIR", strlen("XDG_DOCUMENTS_DIR")) == 0) {
            directory = X_USER_DIRECTORY_DOCUMENTS;
            p += strlen("XDG_DOCUMENTS_DIR");
        } else if (strncmp(p, "XDG_DOWNLOAD_DIR", strlen("XDG_DOWNLOAD_DIR")) == 0) {
            directory = X_USER_DIRECTORY_DOWNLOAD;
            p += strlen ("XDG_DOWNLOAD_DIR");
        } else if (strncmp(p, "XDG_MUSIC_DIR", strlen("XDG_MUSIC_DIR")) == 0) {
            directory = X_USER_DIRECTORY_MUSIC;
            p += strlen ("XDG_MUSIC_DIR");
        } else if (strncmp(p, "XDG_PICTURES_DIR", strlen("XDG_PICTURES_DIR")) == 0) {
            directory = X_USER_DIRECTORY_PICTURES;
            p += strlen("XDG_PICTURES_DIR");
        } else if (strncmp(p, "XDG_PUBLICSHARE_DIR", strlen("XDG_PUBLICSHARE_DIR")) == 0) {
            directory = X_USER_DIRECTORY_PUBLIC_SHARE;
            p += strlen("XDG_PUBLICSHARE_DIR");
        } else if (strncmp(p, "XDG_TEMPLATES_DIR", strlen("XDG_TEMPLATES_DIR")) == 0) {
            directory = X_USER_DIRECTORY_TEMPLATES;
            p += strlen("XDG_TEMPLATES_DIR");
        } else if (strncmp(p, "XDG_VIDEOS_DIR", strlen("XDG_VIDEOS_DIR")) == 0) {
            directory = X_USER_DIRECTORY_VIDEOS;
            p += strlen("XDG_VIDEOS_DIR");
        } else {
            continue;
        }

        while (*p == ' ' || *p == '\t') {
            p++;
        }

        if (*p != '=') {
            continue;
        }
        p++;

        while (*p == ' ' || *p == '\t') {
            p++;
        }

        if (*p != '"') {
            continue;
        }
        p++;

        if (strncmp(p, "$HOME", 5) == 0) {
            p += 5;
            is_relative = TRUE;
        } else if (*p != '/') {
            continue;
        }

        d = strrchr(p, '"');
        if (!d) {
            continue;
        }
        *d = 0;

        d = p;
        len = strlen(d);
        if (d[len - 1] == '/') {
            d[len - 1] = 0;
        }

        if (is_relative) {
            xchar *home_dir = x_build_home_dir();
            x_user_special_dirs[directory] = x_build_filename(home_dir, d, NULL);
            x_free(home_dir);
        } else {
            x_user_special_dirs[directory] = x_strdup(d);
        }
    }

    x_strfreev(lines);
    x_free(config_file);
}

void x_reload_user_special_dirs_cache(void)
{
    int i;

    X_LOCK(x_utils_global);
    if (x_user_special_dirs != NULL) {
        char *old_val;
        char **old_g_user_special_dirs = x_user_special_dirs;

        x_user_special_dirs = x_new0(xchar *, X_USER_N_DIRECTORIES);
        load_user_special_dirs();

        for (i = 0; i < X_USER_N_DIRECTORIES; i++) {
            old_val = old_g_user_special_dirs[i];
            if (x_user_special_dirs[i] == NULL) {
                x_user_special_dirs[i] = old_val;
            } else if (x_strcmp0(old_val, x_user_special_dirs[i]) == 0) {
                x_free(x_user_special_dirs[i]);
                x_user_special_dirs[i] = old_val;
            } else {
                x_free(old_val);
            }
        }

        x_free(old_g_user_special_dirs);
    }
    X_UNLOCK(x_utils_global);
}

const xchar *x_get_user_special_dir(XUserDirectory directory)
{
    const xchar *user_special_dir;

    x_return_val_if_fail(directory >= X_USER_DIRECTORY_DESKTOP && directory < X_USER_N_DIRECTORIES, NULL);

    X_LOCK(x_utils_global);
    if (X_UNLIKELY(x_user_special_dirs == NULL)) {
        x_user_special_dirs = x_new0(xchar *, X_USER_N_DIRECTORIES);

        load_user_special_dirs();

        if (x_user_special_dirs[X_USER_DIRECTORY_DESKTOP] == NULL) {
            xchar *home_dir = x_build_home_dir ();
            x_user_special_dirs[X_USER_DIRECTORY_DESKTOP] = x_build_filename(home_dir, "Desktop", NULL);
            x_free(home_dir);
        }
    }
    user_special_dir = x_user_special_dirs[directory];
    X_UNLOCK(x_utils_global);

    return user_special_dir;
}

static xchar **x_build_system_data_dirs(void)
{
    xchar **data_dir_vector = NULL;
    xchar *data_dirs = (xchar *)x_getenv("XDG_DATA_DIRS");

    if (!data_dirs || !data_dirs[0]) {
        data_dirs = "/usr/local/share/:/usr/share/";
    }
    data_dir_vector = x_strsplit(data_dirs, X_SEARCHPATH_SEPARATOR_S, 0);

    return x_steal_pointer(&data_dir_vector);
}

const xchar *const *x_get_system_data_dirs(void)
{
    const xchar *const *system_data_dirs;

    X_LOCK(x_utils_global);
    if (x_system_data_dirs == NULL) {
        x_system_data_dirs = x_build_system_data_dirs();
    }
    system_data_dirs = (const xchar *const *)x_system_data_dirs;
    X_UNLOCK(x_utils_global);

    return system_data_dirs;
}

static xchar **x_build_system_config_dirs(void)
{
    xchar **conf_dir_vector = NULL;
    const xchar *conf_dirs = x_getenv("XDG_CONFIG_DIRS");

    if (!conf_dirs || !conf_dirs[0]) {
        conf_dirs = "/etc/xdg";
    }

    conf_dir_vector = x_strsplit(conf_dirs, X_SEARCHPATH_SEPARATOR_S, 0);
    return x_steal_pointer(&conf_dir_vector);
}

const xchar *const *x_get_system_config_dirs(void)
{
    const xchar *const *system_config_dirs;

    X_LOCK(x_utils_global);
    if (x_system_config_dirs == NULL) {
        x_system_config_dirs = x_build_system_config_dirs();
    }
    system_config_dirs = (const xchar *const *)x_system_config_dirs;
    X_UNLOCK(x_utils_global);

    return system_config_dirs;
}

void x_nullify_pointer(xpointer *nullify_location)
{
    x_return_if_fail(nullify_location != NULL);
    *nullify_location = NULL;
}

#define KILOBYTE_FACTOR         (X_XOFFSET_CONSTANT(1000))
#define MEGABYTE_FACTOR         (KILOBYTE_FACTOR * KILOBYTE_FACTOR)
#define GIGABYTE_FACTOR         (MEGABYTE_FACTOR * KILOBYTE_FACTOR)
#define TERABYTE_FACTOR         (GIGABYTE_FACTOR * KILOBYTE_FACTOR)
#define PETABYTE_FACTOR         (TERABYTE_FACTOR * KILOBYTE_FACTOR)
#define EXABYTE_FACTOR          (PETABYTE_FACTOR * KILOBYTE_FACTOR)

#define KIBIBYTE_FACTOR         (X_XOFFSET_CONSTANT(1024))
#define MEBIBYTE_FACTOR         (KIBIBYTE_FACTOR * KIBIBYTE_FACTOR)
#define GIBIBYTE_FACTOR         (MEBIBYTE_FACTOR * KIBIBYTE_FACTOR)
#define TEBIBYTE_FACTOR         (GIBIBYTE_FACTOR * KIBIBYTE_FACTOR)
#define PEBIBYTE_FACTOR         (TEBIBYTE_FACTOR * KIBIBYTE_FACTOR)
#define EXBIBYTE_FACTOR         (PEBIBYTE_FACTOR * KIBIBYTE_FACTOR)

xchar *x_format_size(xuint64 size)
{
    return x_format_size_full(size, X_FORMAT_SIZE_DEFAULT);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"

xchar *x_format_size_full(xuint64 size, XFormatSizeFlags flags)
{
    struct Format {
        xuint64 factor;
        char    string[10];
    };

    typedef enum {
        FORMAT_BYTES,
        FORMAT_BYTES_IEC,
        FORMAT_BITS,
        FORMAT_BITS_IEC
    } FormatIndex;

    const struct Format formats[4][6] = {
        {
        /* Translators: A unit symbol for size formatting, showing for example: "13.0 kB" */
        { KILOBYTE_FACTOR, N_("kB") },
        /* Translators: A unit symbol for size formatting, showing for example: "13.0 MB" */
        { MEGABYTE_FACTOR, N_("MB") },
        /* Translators: A unit symbol for size formatting, showing for example: "13.0 GB" */
        { GIGABYTE_FACTOR, N_("GB") },
        /* Translators: A unit symbol for size formatting, showing for example: "13.0 TB" */
        { TERABYTE_FACTOR, N_("TB") },
        /* Translators: A unit symbol for size formatting, showing for example: "13.0 PB" */
        { PETABYTE_FACTOR, N_("PB") },
        /* Translators: A unit symbol for size formatting, showing for example: "13.0 EB" */
        { EXABYTE_FACTOR,  N_("EB") }
        },
        {
        /* Translators: A unit symbol for size formatting, showing for example: "13.0 KiB" */
        { KIBIBYTE_FACTOR, N_("KiB") },
        /* Translators: A unit symbol for size formatting, showing for example: "13.0 MiB" */
        { MEBIBYTE_FACTOR, N_("MiB") },
        /* Translators: A unit symbol for size formatting, showing for example: "13.0 GiB" */
        { GIBIBYTE_FACTOR, N_("GiB") },
        /* Translators: A unit symbol for size formatting, showing for example: "13.0 TiB" */
        { TEBIBYTE_FACTOR, N_("TiB") },
        /* Translators: A unit symbol for size formatting, showing for example: "13.0 PiB" */
        { PEBIBYTE_FACTOR, N_("PiB") },
        /* Translators: A unit symbol for size formatting, showing for example: "13.0 EiB" */
        { EXBIBYTE_FACTOR, N_("EiB") }
        },
        {
        /* Translators: A unit symbol for size formatting, showing for example: "13.0 kbit" */
        { KILOBYTE_FACTOR, N_("kbit") },
        /* Translators: A unit symbol for size formatting, showing for example: "13.0 Mbit" */
        { MEGABYTE_FACTOR, N_("Mbit") },
        /* Translators: A unit symbol for size formatting, showing for example: "13.0 Gbit" */
        { GIGABYTE_FACTOR, N_("Gbit") },
        /* Translators: A unit symbol for size formatting, showing for example: "13.0 Tbit" */
        { TERABYTE_FACTOR, N_("Tbit") },
        /* Translators: A unit symbol for size formatting, showing for example: "13.0 Pbit" */
        { PETABYTE_FACTOR, N_("Pbit") },
        /* Translators: A unit symbol for size formatting, showing for example: "13.0 Ebit" */
        { EXABYTE_FACTOR,  N_("Ebit") }
        },
        {
        /* Translators: A unit symbol for size formatting, showing for example: "13.0 Kibit" */
        { KIBIBYTE_FACTOR, N_("Kibit") },
        /* Translators: A unit symbol for size formatting, showing for example: "13.0 Mibit" */
        { MEBIBYTE_FACTOR, N_("Mibit") },
        /* Translators: A unit symbol for size formatting, showing for example: "13.0 Gibit" */
        { GIBIBYTE_FACTOR, N_("Gibit") },
        /* Translators: A unit symbol for size formatting, showing for example: "13.0 Tibit" */
        { TEBIBYTE_FACTOR, N_("Tibit") },
        /* Translators: A unit symbol for size formatting, showing for example: "13.0 Pibit" */
        { PEBIBYTE_FACTOR, N_("Pibit") },
        /* Translators: A unit symbol for size formatting, showing for example: "13.0 Eibit" */
        { EXBIBYTE_FACTOR, N_("Eibit") }
        }
    };

    XString *string;
    FormatIndex index;

    x_return_val_if_fail((flags & (X_FORMAT_SIZE_LONG_FORMAT | X_FORMAT_SIZE_ONLY_VALUE)) != (X_FORMAT_SIZE_LONG_FORMAT | X_FORMAT_SIZE_ONLY_VALUE), NULL);
    x_return_val_if_fail((flags & (X_FORMAT_SIZE_LONG_FORMAT | X_FORMAT_SIZE_ONLY_UNIT)) != (X_FORMAT_SIZE_LONG_FORMAT | X_FORMAT_SIZE_ONLY_UNIT), NULL);
    x_return_val_if_fail((flags & (X_FORMAT_SIZE_ONLY_VALUE | X_FORMAT_SIZE_ONLY_UNIT)) != (X_FORMAT_SIZE_ONLY_VALUE | X_FORMAT_SIZE_ONLY_UNIT), NULL);

    string = x_string_new(NULL);

    switch (flags & ~(X_FORMAT_SIZE_LONG_FORMAT | X_FORMAT_SIZE_ONLY_VALUE | X_FORMAT_SIZE_ONLY_UNIT)) {
        case X_FORMAT_SIZE_DEFAULT:
            index = FORMAT_BYTES;
            break;

        case (X_FORMAT_SIZE_DEFAULT | X_FORMAT_SIZE_IEC_UNITS):
            index = FORMAT_BYTES_IEC;
            break;

        case X_FORMAT_SIZE_BITS:
            index = FORMAT_BITS;
            break;

        case (X_FORMAT_SIZE_BITS | X_FORMAT_SIZE_IEC_UNITS):
            index = FORMAT_BITS_IEC;
            break;

        default:
            x_assert_not_reached();
    }

    if (size < formats[index][0].factor) {
        const char *units;

        if (index == FORMAT_BYTES || index == FORMAT_BYTES_IEC) {
            units = x_dngettext(GETTEXT_PACKAGE, "byte", "bytes", (xuint)size);
        } else {
            units = x_dngettext(GETTEXT_PACKAGE, "bit", "bits", (xuint)size);
        }

        if ((flags & X_FORMAT_SIZE_ONLY_UNIT) != 0) {
            x_string_append(string, units);
        } else if ((flags & X_FORMAT_SIZE_ONLY_VALUE) != 0) {
            x_string_printf(string, C_("format-size", "%u"), (xuint) size);
        } else {
            x_string_printf(string, C_("format-size", "%u %s"), (xuint) size, units);
        }

        flags = (XFormatSizeFlags)(flags & ~X_FORMAT_SIZE_LONG_FORMAT);
    } else {
        xsize i;
        xdouble value;
        const xchar *units;
        const xsize n = X_N_ELEMENTS(formats[index]);
        const struct Format * f = &formats[index][n - 1];

        for (i = 1; i < n; i++) {
            if (size < formats[index][i].factor) {
                f = &formats[index][i - 1];
                break;
            }
        }

        units = _(f->string);
        value = (xdouble)size / (xdouble)f->factor;

        if ((flags & X_FORMAT_SIZE_ONLY_UNIT) != 0) {
            x_string_append(string, units);
        } else if ((flags & X_FORMAT_SIZE_ONLY_VALUE) != 0) {
            x_string_printf(string, C_("format-size", "%.1f"), value);
        } else {
            x_string_printf (string, C_("format-size", "%.1f %s"), value, units);
        }
    }

    if (flags & X_FORMAT_SIZE_LONG_FORMAT) {
        xchar *formatted_number;
        const xchar *translated_format;
        xuint plural_form = size < 1000 ? size : size % 1000 + 1000;

        if (index == FORMAT_BYTES || index == FORMAT_BYTES_IEC) {
            translated_format = x_dngettext(GETTEXT_PACKAGE, "%s byte", "%s bytes", plural_form);
        } else {
            translated_format = x_dngettext(GETTEXT_PACKAGE, "%s bit", "%s bits", plural_form);
        }
        formatted_number = x_strdup_printf("%'" X_XUINT64_FORMAT, size);

        x_string_append(string, " (");
        x_string_append_printf(string, translated_format, formatted_number);
        x_free(formatted_number);
        x_string_append(string, ")");
    }

    return x_string_free(string, FALSE);
}

#pragma GCC diagnostic pop

xchar *x_format_size_for_display(xoffset size)
{
    if (size < (xoffset)KIBIBYTE_FACTOR) {
        return x_strdup_printf(x_dngettext(GETTEXT_PACKAGE, "%u byte", "%u bytes",(xuint)size), (xuint)size);
    } else {
        xdouble displayed_size;

        if (size < (xoffset)MEBIBYTE_FACTOR) {
            displayed_size = (xdouble)size / (xdouble)KIBIBYTE_FACTOR;
            return x_strdup_printf(_("%.1f KB"), displayed_size);
        } else if (size < (xoffset)GIBIBYTE_FACTOR) {
            displayed_size = (xdouble) size / (xdouble)MEBIBYTE_FACTOR;
            return x_strdup_printf(_("%.1f MB"), displayed_size);
        } else if (size < (xoffset)TEBIBYTE_FACTOR) {
            displayed_size = (xdouble)size / (xdouble)GIBIBYTE_FACTOR;
            return x_strdup_printf(_("%.1f GB"), displayed_size);
        } else if (size < (xoffset)PEBIBYTE_FACTOR) {
            displayed_size = (xdouble)size / (xdouble)TEBIBYTE_FACTOR;
            return x_strdup_printf(_("%.1f TB"), displayed_size);
        } else if (size < (xoffset)EXBIBYTE_FACTOR) {
            displayed_size = (xdouble)size / (xdouble)PEBIBYTE_FACTOR;
            return x_strdup_printf(_("%.1f PB"), displayed_size);
        } else {
            displayed_size = (xdouble)size / (xdouble)EXBIBYTE_FACTOR;
            return x_strdup_printf(_("%.1f EB"), displayed_size);
        }
    }
}

xboolean x_check_setuid (void)
{
#if defined(HAVE_SYS_AUXV_H) && defined(HAVE_GETAUXVAL) && defined(AT_SECURE)
    int errsv;
    unsigned long value;

    errno = 0;
    value = getauxval(AT_SECURE);
    errsv = errno;
    if (errsv) {
        x_error("getauxval () failed: %s", x_strerror(errsv));
    }

    return value;
#elif defined(X_OS_UNIX)
    uid_t ruid, euid, suid;
    gid_t rgid, egid, sgid;

    static xsize check_setuid_initialised;
    static xboolean is_setuid;

    if (x_once_init_enter(&check_setuid_initialised)) {
#ifdef HAVE_GETRESUID
        int getresuid(uid_t *ruid, uid_t *euid, uid_t *suid);
        int getresgid(gid_t *rgid, gid_t *egid, gid_t *sgid);
      
        if (getresuid(&ruid, &euid, &suid) != 0 || getresgid(&rgid, &egid, &sgid) != 0)
#endif
        {
            suid = ruid = getuid();
            sgid = rgid = getgid();
            euid = geteuid();
            egid = getegid();
        }

        is_setuid = (ruid != euid || ruid != suid || rgid != egid || rgid != sgid);
        x_once_init_leave(&check_setuid_initialised, 1);
    }

    return is_setuid;
#endif
}
