#ifndef __XMODULE_H__
#define __XMODULE_H__

#include "../xlib.h"
#include "xmodule-visibility.h"

X_BEGIN_DECLS

#define X_MODULE_IMPORT             extern

#if __GNUC__ >= 4
#define X_MODULE_EXPORT             __attribute__((visibility("default")))
#else
#define X_MODULE_EXPORT
#endif

typedef enum {
    X_MODULE_BIND_LAZY  = 1 << 0,
    X_MODULE_BIND_LOCAL = 1 << 1,
    X_MODULE_BIND_MASK  = 0x03
} XModuleFlags;

typedef struct _XModule XModule;

typedef void (*XModuleUnload)(XModule *module);
typedef const xchar *(*XModuleCheckInit)(XModule *module);

#define X_MODULE_ERROR x_module_error_quark() XLIB_AVAILABLE_MACRO_IN_2_70

XLIB_AVAILABLE_IN_2_70
XQuark x_module_error_quark(void);

typedef enum {
    X_MODULE_ERROR_FAILED,
    X_MODULE_ERROR_CHECK_FAILED,
} XModuleError
XLIB_AVAILABLE_ENUMERATOR_IN_2_70;

XLIB_AVAILABLE_IN_ALL
xboolean x_module_supported(void) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
XModule *x_module_open(const xchar *file_name, XModuleFlags flags);

XLIB_AVAILABLE_IN_2_70
XModule *x_module_open_full(const xchar *file_name, XModuleFlags flags, XError **error);

XLIB_AVAILABLE_IN_ALL
xboolean x_module_close(XModule *module);

XLIB_AVAILABLE_IN_ALL
void x_module_make_resident(XModule *module);

XLIB_AVAILABLE_IN_ALL
const xchar *x_module_error(void);

XLIB_AVAILABLE_IN_ALL
xboolean x_module_symbol(XModule *module, const xchar *symbol_name, xpointer *symbol);

XLIB_AVAILABLE_IN_ALL
const xchar *x_module_name(XModule *module);

XLIB_DEPRECATED_IN_2_76
xchar *x_module_build_path(const xchar *directory, const xchar *module_name);

X_END_DECLS

#endif
