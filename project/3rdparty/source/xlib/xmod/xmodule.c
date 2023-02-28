#include <errno.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <xlib/xlib/config.h>
#include <xlib/xlib.h>
#include <xlib/xlib/xstdio.h>
#include <xlib/xmod/xmodule.h>

#include "xmoduleconf.h"

#ifndef O_CLOEXEC
#define O_CLOEXEC   0
#endif

struct _XModule {
    xchar         *file_name;
    xpointer      handle;
    xuint         ref_count : 31;
    xuint         is_resident : 1;
    XModuleUnload unload;
    XModule       *next;
};

static void _x_module_close(xpointer handle);
static xpointer _x_module_open(const xchar *file_name, xboolean bind_lazy, xboolean bind_local, XError **error);

static xpointer _x_module_self(void);
static inline void x_module_set_error(const xchar *error);
static xpointer _x_module_symbol(xpointer handle, const xchar *symbol_name);
static xchar *_x_module_build_path(const xchar *directory, const xchar *module_name);

static inline XModule *x_module_find_by_name(const xchar *name);
static inline XModule *x_module_find_by_handle(xpointer handle);

static XModule *modules = NULL;
static XModule *main_module = NULL;
static xuint module_debug_flags = 0;
static xboolean module_debug_initialized = FALSE;
static XPrivate module_error_private = X_PRIVATE_INIT(x_free);

static inline XModule *x_module_find_by_handle(xpointer handle)
{
    XModule *module;
    XModule *retval = NULL;

    if (main_module && main_module->handle == handle) {
        retval = main_module;
    } else {
        for (module = modules; module; module = module->next) {
            if (handle == module->handle) {
                retval = module;
                break;
            }
        }
    }

    return retval;
}

static inline XModule *x_module_find_by_name(const xchar *name)
{
    XModule *module;
    XModule *retval = NULL;

    for (module = modules; module; module = module->next) {
        if (strcmp(name, module->file_name) == 0) {
            retval = module;
            break;
        }
    }

    return retval;
}

static inline void x_module_set_error_unduped(xchar *error)
{
    x_private_replace(&module_error_private, error);
    errno = 0;
}

static inline void x_module_set_error(const xchar *error)
{
    x_module_set_error_unduped(x_strdup(error));
}

#ifndef X_MODULE_HAVE_DLERROR
#ifdef __NetBSD__
#define dlerror()               x_strerror(errno)
#else
#define dlerror()               "unknown dl-error"
#endif
#endif

#ifndef HAVE_RTLD_LAZY
#define RTLD_LAZY               1
#endif

#ifndef HAVE_RTLD_NOW
#define RTLD_NOW                0
#endif

#ifdef X_MODULE_BROKEN_RTLD_GLOBAL
#undef RTLD_GLOBAL
#undef HAVE_RTLD_GLOBAL
#endif

#ifndef HAVE_RTLD_GLOBAL
#define RTLD_GLOBAL             0
#endif

#if defined(__UCLIBC__)
X_LOCK_DEFINE_STATIC(errors);
#else
#define DLERROR_IS_THREADSAFE   1
#endif

static void lock_dlerror(void)
{
#ifndef DLERROR_IS_THREADSAFE
    X_LOCK(errors);
#endif
}

static void unlock_dlerror(void)
{
#ifndef DLERROR_IS_THREADSAFE
    X_UNLOCK(errors);
#endif
}

static const xchar *fetch_dlerror(xboolean replace_null)
{
  const xchar *msg = dlerror();

    if (!msg && replace_null) {
        return "unknown dl-error";
    }

    return msg;
}

static xpointer _x_module_open(const xchar *file_name, xboolean bind_lazy, xboolean bind_local, XError **error)
{
    xpointer handle;

    lock_dlerror();
    handle = dlopen(file_name, (bind_local ? 0 : RTLD_GLOBAL) | (bind_lazy ? RTLD_LAZY : RTLD_NOW));
    if (!handle) {
        const xchar *message = (const xchar *)fetch_dlerror(TRUE);

        x_module_set_error(message);
        x_set_error_literal(error, X_MODULE_ERROR, X_MODULE_ERROR_FAILED, message);
    }
    unlock_dlerror();

    return handle;
}

static xpointer _x_module_self(void)
{
    xpointer handle;

    lock_dlerror();
    handle = dlopen(NULL, RTLD_GLOBAL | RTLD_LAZY);
    if (!handle) {
        x_module_set_error(fetch_dlerror(TRUE));
    }
    unlock_dlerror();

    return handle;
}

static void _x_module_close(xpointer handle)
{
    lock_dlerror();
    if (dlclose(handle) != 0) {
        x_module_set_error(fetch_dlerror(TRUE));
    }
    unlock_dlerror();
}

static xpointer _x_module_symbol(xpointer handle, const xchar *symbol_name)
{
    xpointer p;
    const xchar *msg;

    lock_dlerror();
    fetch_dlerror(FALSE);
    p = dlsym(handle, symbol_name);
    msg = fetch_dlerror(FALSE);
    if (msg) {
        x_module_set_error(msg);
    }
    unlock_dlerror();

    return p;
}

static xchar *_x_module_build_path(const xchar *directory, const xchar *module_name)
{
    if (directory && *directory) {
        if (strncmp(module_name, "lib", 3) == 0) {
            return x_strconcat(directory, "/", module_name, NULL);
        } else {
            return x_strconcat(directory, "/lib", module_name, "." X_MODULE_SUFFIX, NULL);
        }
    } else if (strncmp(module_name, "lib", 3) == 0) {
        return x_strdup(module_name);
    } else {
        return x_strconcat("lib", module_name, "." X_MODULE_SUFFIX, NULL);
    }
}

#define SUPPORT_OR_RETURN(rv)           { x_module_set_error(NULL); }

X_DEFINE_QUARK(x-module-error-quark, x_module_error)

xboolean x_module_supported(void)
{
    SUPPORT_OR_RETURN(FALSE);
    return TRUE;
}

static xchar *parse_libtool_archive(const xchar *libtool_name)
{
    xchar *name;
    XTokenType token;
    XScanner *scanner;
    xchar *lt_libdir = NULL;
    xchar *lt_dlname = NULL;
    xboolean lt_installed = TRUE;
    const xuint TOKEN_DLNAME = X_TOKEN_LAST + 1;
    const xuint TOKEN_LIBDIR = X_TOKEN_LAST + 3;
    const xuint TOKEN_INSTALLED = X_TOKEN_LAST + 2;

    int fd = x_open(libtool_name, O_RDONLY | O_CLOEXEC, 0);
    if (fd < 0) {
        xchar *display_libtool_name = x_filename_display_name(libtool_name);
        x_module_set_error_unduped(x_strdup_printf("failed to open libtool archive ‘%s’", display_libtool_name));
        x_free(display_libtool_name);
        return NULL;
    }

    scanner = x_scanner_new(NULL);
    x_scanner_input_file(scanner, fd);
    scanner->config->symbol_2_token = TRUE;
    x_scanner_scope_add_symbol(scanner, 0, "dlname", XUINT_TO_POINTER(TOKEN_DLNAME));
    x_scanner_scope_add_symbol(scanner, 0, "installed", XUINT_TO_POINTER(TOKEN_INSTALLED));
    x_scanner_scope_add_symbol(scanner, 0, "libdir", XUINT_TO_POINTER(TOKEN_LIBDIR));

    while (!x_scanner_eof(scanner)) {
        token = x_scanner_get_next_token(scanner);
        if (token == TOKEN_DLNAME || token == TOKEN_INSTALLED || token == TOKEN_LIBDIR) {
            if (x_scanner_get_next_token(scanner) != '=' || x_scanner_get_next_token(scanner) != (token == TOKEN_INSTALLED ? X_TOKEN_IDENTIFIER : X_TOKEN_STRING)) {
                xchar *display_libtool_name = x_filename_display_name(libtool_name);
                x_module_set_error_unduped(x_strdup_printf("unable to parse libtool archive ‘%s’", display_libtool_name));
                x_free(display_libtool_name);

                x_free(lt_dlname);
                x_free(lt_libdir);
                x_scanner_destroy(scanner);
                close(fd);

                return NULL;
            } else {
                if (token == TOKEN_DLNAME) {
                    x_free(lt_dlname);
                    lt_dlname = x_strdup(scanner->value.v_string);
                } else if (token == TOKEN_INSTALLED) {
                    lt_installed = strcmp(scanner->value.v_identifier, "yes") == 0;
                } else {
                    x_free(lt_libdir);
                    lt_libdir = x_strdup(scanner->value.v_string);
                }
            }
        }
    }

    if (!lt_installed) {
        xchar *dir = x_path_get_dirname(libtool_name);
        x_free(lt_libdir);
        lt_libdir = x_strconcat(dir, X_DIR_SEPARATOR_S ".libs", NULL);
        x_free(dir);
    }

    x_clear_pointer(&scanner, x_scanner_destroy);
    close(x_steal_fd(&fd));

    if (lt_libdir == NULL || lt_dlname == NULL) {
        xchar *display_libtool_name = x_filename_display_name(libtool_name);
        x_module_set_error_unduped(x_strdup_printf("unable to parse libtool archive ‘%s’", display_libtool_name));
        x_free(display_libtool_name);

        return NULL;
    }

    name = x_strconcat(lt_libdir, X_DIR_SEPARATOR_S, lt_dlname, NULL);
    x_free(lt_dlname);
    x_free(lt_libdir);

    return name;
}

enum {
    X_MODULE_DEBUG_RESIDENT_MODULES = 1 << 0,
    X_MODULE_DEBUG_BIND_NOW_MODULES = 1 << 1
};

static void _x_module_debug_init(void)
{
    const XDebugKey keys[] = {
        { "resident-modules", X_MODULE_DEBUG_RESIDENT_MODULES },
        { "bind-now-modules", X_MODULE_DEBUG_BIND_NOW_MODULES }
    };

    const xchar *env;
    env = x_getenv("X_DEBUG");

    module_debug_flags = !env ? 0 : x_parse_debug_string(env, keys, X_N_ELEMENTS(keys));
    module_debug_initialized = TRUE;
}

static XRecMutex x_module_global_lock;

XModule *x_module_open_full (const xchar *file_name, XModuleFlags flags, XError **error)
{
    XModule *module;
    xchar *name = NULL;
    xpointer handle = NULL;

    SUPPORT_OR_RETURN(NULL);

    x_return_val_if_fail(error == NULL || *error == NULL, NULL);

    x_rec_mutex_lock(&x_module_global_lock);

    if (X_UNLIKELY(!module_debug_initialized)) {
        _x_module_debug_init();
    }

    if (module_debug_flags & X_MODULE_DEBUG_BIND_NOW_MODULES) {
        flags = (XModuleFlags)(flags & ~X_MODULE_BIND_LAZY);
    }

    if (!file_name) {
        if (!main_module) {
            handle = _x_module_self();

            {
                main_module = x_new(XModule, 1);
                main_module->file_name = NULL;
                main_module->handle = handle;
                main_module->ref_count = 1;
                main_module->is_resident = TRUE;
                main_module->unload = NULL;
                main_module->next = NULL;
            }
        } else {
            main_module->ref_count++;
        }

        x_rec_mutex_unlock(&x_module_global_lock);
        return main_module;
    }

    module = x_module_find_by_name(file_name);
    if (module) {
        module->ref_count++;
        x_rec_mutex_unlock(&x_module_global_lock);
        return module;
    }

    if (x_file_test(file_name, X_FILE_TEST_IS_REGULAR)) {
        name = x_strdup(file_name);
    }

    if (!name) {
        char *basename, *dirname;
        size_t prefix_idx = 0, suffix_idx = 0;
        const char *prefixes[2] = {0}, *suffixes[2] = {0};

        basename = x_path_get_basename(file_name);
        dirname = x_path_get_dirname(file_name);

        if (!x_str_has_prefix(basename, "lib")) {
            prefixes[prefix_idx++] = "lib";
        } else {
            prefixes[prefix_idx++] = "";
        }

        if (!x_str_has_suffix(basename, ".so")) {
            suffixes[suffix_idx++] = ".so";
        }

        for (xuint i = 0; i < prefix_idx; i++) {
            for (xuint j = 0; j < suffix_idx; j++) {
                name = x_strconcat(dirname, X_DIR_SEPARATOR_S, prefixes[i], basename, suffixes[j], NULL);
                if (x_file_test(name, X_FILE_TEST_IS_REGULAR)) {
                    goto name_found;
                }

                x_free(name);
                name = NULL;
            }
        }

name_found:
        x_free(basename);
        x_free(dirname);
    }

    if (!name) {
        name = x_strconcat(file_name, ".la", NULL);
        if (!x_file_test(name, X_FILE_TEST_IS_REGULAR)) {
            x_free(name);
            name = NULL;
        }
    }

    if (!name) {
        xchar *dot = (xchar *)strrchr(file_name, '.');
        xchar *slash = (xchar *)strrchr(file_name, X_DIR_SEPARATOR);

        if (!dot || dot < slash) {
            name = x_strconcat(file_name, "." X_MODULE_SUFFIX, NULL);
        } else {
            name = x_strdup(file_name);
        }
    }

    x_assert(name != NULL);

    if (x_str_has_suffix(name, ".la")) {
        xchar *real_name = parse_libtool_archive(name);
        if (real_name) {
            x_free(name);
            name = real_name;
        }
    }

    handle = _x_module_open(name, (flags & X_MODULE_BIND_LAZY) != 0, (flags & X_MODULE_BIND_LOCAL) != 0, error);
    x_free(name);

    if (handle) {
        xchar *saved_error;
        XModuleCheckInit check_init;
        const xchar *check_failed = NULL;

        module = x_module_find_by_handle(handle);
        if (module) {
            _x_module_close(module->handle);
            module->ref_count++;
            x_module_set_error(NULL);

            x_rec_mutex_unlock(&x_module_global_lock);
            return module;
        }

        saved_error = x_strdup(x_module_error());
        x_module_set_error(NULL);

        module = x_new(XModule, 1);
        module->file_name = x_strdup(file_name);
        module->handle = handle;
        module->ref_count = 1;
        module->is_resident = FALSE;
        module->unload = NULL;
        module->next = modules;
        modules = module;

        if (x_module_symbol(module, "x_module_check_init", (xpointer *)&check_init) && check_init != NULL) {
            check_failed = check_init(module);
        }

        if (!check_failed) {
            x_module_symbol(module, "x_module_unload", (xpointer *)&module->unload);
        }

        if (check_failed) {
            xchar *temp_error;

            temp_error = x_strconcat("XModule (", file_name, ") ", "initialization check failed: ", check_failed, NULL);
            x_module_close(module);
            module = NULL;

            x_module_set_error(temp_error);
            x_set_error_literal(error, X_MODULE_ERROR, X_MODULE_ERROR_CHECK_FAILED, temp_error);
            x_free(temp_error);
        } else {
            x_module_set_error(saved_error);
        }

        x_free(saved_error);
    }

    if (module != NULL && (module_debug_flags & X_MODULE_DEBUG_RESIDENT_MODULES)) {
        x_module_make_resident(module);
    }

    x_rec_mutex_unlock(&x_module_global_lock);
    return module;
}

XModule *x_module_open(const xchar *file_name, XModuleFlags flags)
{
    return x_module_open_full(file_name, flags, NULL);
}

xboolean x_module_close(XModule *module)
{
    SUPPORT_OR_RETURN(FALSE);

    x_return_val_if_fail(module != NULL, FALSE);
    x_return_val_if_fail(module->ref_count > 0, FALSE);

    x_rec_mutex_lock(&x_module_global_lock);

    module->ref_count--;
    if (!module->ref_count && !module->is_resident && module->unload) {
        XModuleUnload unload;

        unload = module->unload;
        module->unload = NULL;
        unload(module);
    }

    if (!module->ref_count && !module->is_resident) {
        XModule *last;
        XModule *node;

        last = NULL;
        node = modules;

        while (node) {
            if (node == module) {
                if (last) {
                    last->next = node->next;
                } else {
                    modules = node->next;
                }

                break;
            }

            last = node;
            node = last->next;
        }

        module->next = NULL;

        _x_module_close(module->handle);
        x_free(module->file_name);
        x_free(module);
    }

    x_rec_mutex_unlock(&x_module_global_lock);
    return x_module_error() == NULL;
}

void x_module_make_resident(XModule *module)
{
    x_return_if_fail(module != NULL);
    module->is_resident = TRUE;
}

const xchar *x_module_error(void)
{
    return (const xchar *)x_private_get(&module_error_private);
}

xboolean x_module_symbol(XModule *module, const xchar *symbol_name, xpointer *symbol)
{
    const xchar *module_error;

    if (symbol) {
        *symbol = NULL;
    }

    SUPPORT_OR_RETURN(FALSE);

    x_return_val_if_fail(module != NULL, FALSE);
    x_return_val_if_fail(symbol_name != NULL, FALSE);
    x_return_val_if_fail(symbol != NULL, FALSE);

    x_rec_mutex_lock(&x_module_global_lock);

#ifdef X_MODULE_NEED_USCORE
    {
        xchar *name;

        name = x_strconcat("_", symbol_name, NULL);
        *symbol = _x_module_symbol(module->handle, name);
        x_free(name);
    }
#else
    *symbol = _x_module_symbol(module->handle, symbol_name);
#endif

    module_error = x_module_error();
    if (module_error) {
        xchar *error;

        error = x_strconcat("'", symbol_name, "': ", module_error, NULL);
        x_module_set_error(error);
        x_free(error);
        *symbol = NULL;
    }

    x_rec_mutex_unlock(&x_module_global_lock);
    return !module_error;
}

const xchar *x_module_name(XModule *module)
{
    x_return_val_if_fail(module != NULL, NULL);

    if (module == main_module) {
        return "main";
    }

    return module->file_name;
}

xchar *x_module_build_path(const xchar *directory, const xchar *module_name)
{
    x_return_val_if_fail(module_name != NULL, NULL);
    return _x_module_build_path(directory, module_name);
}
