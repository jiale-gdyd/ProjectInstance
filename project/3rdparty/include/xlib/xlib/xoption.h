#ifndef __X_OPTION_H__
#define __X_OPTION_H__

#include "xerror.h"
#include "xquark.h"

X_BEGIN_DECLS

typedef struct _XOptionGroup XOptionGroup;
typedef struct _XOptionEntry XOptionEntry;
typedef struct _XOptionContext XOptionContext;

typedef enum {
    X_OPTION_FLAG_NONE         = 0,
    X_OPTION_FLAG_HIDDEN       = 1 << 0,
    X_OPTION_FLAG_IN_MAIN      = 1 << 1,
    X_OPTION_FLAG_REVERSE      = 1 << 2,
    X_OPTION_FLAG_NO_ARG       = 1 << 3,
    X_OPTION_FLAG_FILENAME     = 1 << 4,
    X_OPTION_FLAG_OPTIONAL_ARG = 1 << 5,
    X_OPTION_FLAG_NOALIAS      = 1 << 6
} XOptionFlags;

typedef enum {
    X_OPTION_ARG_NONE,
    X_OPTION_ARG_STRING,
    X_OPTION_ARG_INT,
    X_OPTION_ARG_CALLBACK,
    X_OPTION_ARG_FILENAME,
    X_OPTION_ARG_STRING_ARRAY,
    X_OPTION_ARG_FILENAME_ARRAY,
    X_OPTION_ARG_DOUBLE,
    X_OPTION_ARG_INT64
} XOptionArg;

typedef xboolean (*XOptionArgFunc)(const xchar *option_name, const xchar *value, xpointer data, XError **error);
typedef xboolean (*XOptionParseFunc)(XOptionContext *context, XOptionGroup *group, xpointer data, XError **error);

typedef void (*XOptionErrorFunc)(XOptionContext *context, XOptionGroup *group, xpointer data, XError **error);

#define X_OPTION_ERROR          (x_option_error_quark())

typedef enum {
    X_OPTION_ERROR_UNKNOWN_OPTION,
    X_OPTION_ERROR_BAD_VALUE,
    X_OPTION_ERROR_FAILED
} XOptionError;

XLIB_AVAILABLE_IN_ALL
XQuark x_option_error_quark(void);

struct _XOptionEntry {
    const xchar *long_name;
    xchar        short_name;
    xint         flags;
    XOptionArg   arg;
    xpointer     arg_data;
    const xchar  *description;
    const xchar  *arg_description;
};

#define X_OPTION_REMAINING      ""

#define X_OPTION_ENTRY_NULL      \
    XLIB_AVAILABLE_MACRO_IN_2_70 \
    { NULL, 0, 0, (XOptionArg)0, NULL, NULL, NULL }

XLIB_AVAILABLE_IN_ALL
XOptionContext *x_option_context_new(const xchar *parameter_string);

XLIB_AVAILABLE_IN_ALL
void x_option_context_set_summary(XOptionContext *context, const xchar *summary);

XLIB_AVAILABLE_IN_ALL
const xchar *x_option_context_get_summary(XOptionContext *context);

XLIB_AVAILABLE_IN_ALL
void x_option_context_set_description(XOptionContext *context, const xchar *description);

XLIB_AVAILABLE_IN_ALL
const xchar *x_option_context_get_description(XOptionContext *context);
XLIB_AVAILABLE_IN_ALL
void x_option_context_free(XOptionContext *context);

XLIB_AVAILABLE_IN_ALL
void x_option_context_set_help_enabled(XOptionContext *context, xboolean help_enabled);

XLIB_AVAILABLE_IN_ALL
xboolean x_option_context_get_help_enabled(XOptionContext *context);

XLIB_AVAILABLE_IN_ALL
void x_option_context_set_ignore_unknown_options(XOptionContext *context, xboolean ignore_unknown);

XLIB_AVAILABLE_IN_ALL
xboolean x_option_context_get_ignore_unknown_options(XOptionContext *context);

XLIB_AVAILABLE_IN_2_44
void x_option_context_set_strict_posix(XOptionContext *context, xboolean strict_posix);

XLIB_AVAILABLE_IN_2_44
xboolean x_option_context_get_strict_posix(XOptionContext *context);

XLIB_AVAILABLE_IN_ALL
void x_option_context_add_main_entries(XOptionContext *context, const XOptionEntry *entries, const xchar *translation_domain);

XLIB_AVAILABLE_IN_ALL
xboolean x_option_context_parse(XOptionContext *context, xint *argc, xchar ***argv, XError **error);

XLIB_AVAILABLE_IN_2_40
xboolean x_option_context_parse_strv(XOptionContext *context, xchar ***arguments, XError **error);

XLIB_AVAILABLE_IN_ALL
void x_option_context_set_translate_func(XOptionContext *context, XTranslateFunc func, xpointer data, XDestroyNotify destroy_notify);

XLIB_AVAILABLE_IN_ALL
void x_option_context_set_translation_domain(XOptionContext *context, const xchar *domain);

XLIB_AVAILABLE_IN_ALL
void x_option_context_add_group(XOptionContext *context, XOptionGroup *group);

XLIB_AVAILABLE_IN_ALL
void x_option_context_set_main_group(XOptionContext *context, XOptionGroup *group);

XLIB_AVAILABLE_IN_ALL
XOptionGroup *x_option_context_get_main_group(XOptionContext *context);

XLIB_AVAILABLE_IN_ALL
xchar *x_option_context_get_help(XOptionContext *context, xboolean main_help, XOptionGroup *group);

XLIB_AVAILABLE_IN_ALL
XOptionGroup *x_option_group_new(const xchar *name, const xchar *description, const xchar *help_description, xpointer user_data, XDestroyNotify destroy);

XLIB_AVAILABLE_IN_ALL
void x_option_group_set_parse_hooks(XOptionGroup *group, XOptionParseFunc pre_parse_func, XOptionParseFunc post_parse_func);

XLIB_AVAILABLE_IN_ALL
void x_option_group_set_error_hook(XOptionGroup *group, XOptionErrorFunc error_func);

XLIB_DEPRECATED_IN_2_44
void x_option_group_free(XOptionGroup *group);

XLIB_AVAILABLE_IN_2_44
XOptionGroup *x_option_group_ref(XOptionGroup *group);

XLIB_AVAILABLE_IN_2_44
void x_option_group_unref(XOptionGroup *group);

XLIB_AVAILABLE_IN_ALL
void x_option_group_add_entries(XOptionGroup *group, const XOptionEntry *entries);

XLIB_AVAILABLE_IN_ALL
void x_option_group_set_translate_func(XOptionGroup *group, XTranslateFunc func, xpointer data, XDestroyNotify destroy_notify);

XLIB_AVAILABLE_IN_ALL
void x_option_group_set_translation_domain(XOptionGroup *group, const xchar *domain);

X_END_DECLS

#endif
