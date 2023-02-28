#ifndef __X_KEY_FILE_H__
#define __X_KEY_FILE_H__

#include "xbytes.h"
#include "xerror.h"

X_BEGIN_DECLS

typedef enum {
    X_KEY_FILE_ERROR_UNKNOWN_ENCODING,
    X_KEY_FILE_ERROR_PARSE,
    X_KEY_FILE_ERROR_NOT_FOUND,
    X_KEY_FILE_ERROR_KEY_NOT_FOUND,
    X_KEY_FILE_ERROR_GROUP_NOT_FOUND,
    X_KEY_FILE_ERROR_INVALID_VALUE
} XKeyFileError;

#define X_KEY_FILE_ERROR        x_key_file_error_quark()

XLIB_AVAILABLE_IN_ALL
XQuark x_key_file_error_quark (void);

typedef struct _XKeyFile XKeyFile;

typedef enum {
    X_KEY_FILE_NONE              = 0,
    X_KEY_FILE_KEEP_COMMENTS     = 1 << 0,
    X_KEY_FILE_KEEP_TRANSLATIONS = 1 << 1
} XKeyFileFlags;

XLIB_AVAILABLE_IN_ALL
XKeyFile *x_key_file_new(void);

XLIB_AVAILABLE_IN_ALL
XKeyFile *x_key_file_ref(XKeyFile *key_file);

XLIB_AVAILABLE_IN_ALL
void x_key_file_unref(XKeyFile *key_file);

XLIB_AVAILABLE_IN_ALL
void x_key_file_free(XKeyFile *key_file);

XLIB_AVAILABLE_IN_ALL
void x_key_file_set_list_separator(XKeyFile *key_file, xchar separator);

XLIB_AVAILABLE_IN_ALL
xboolean x_key_file_load_from_file(XKeyFile *key_file, const xchar *file, XKeyFileFlags flags, XError **error);

XLIB_AVAILABLE_IN_ALL
xboolean x_key_file_load_from_data(XKeyFile *key_file, const xchar *data, xsize length, XKeyFileFlags flags, XError **error);

XLIB_AVAILABLE_IN_2_50
xboolean x_key_file_load_from_bytes(XKeyFile *key_file, XBytes *bytes, XKeyFileFlags flags, XError **error);

XLIB_AVAILABLE_IN_ALL
xboolean x_key_file_load_from_dirs(XKeyFile *key_file, const xchar *file, const xchar **search_dirs, xchar **full_path, XKeyFileFlags flags, XError **error);

XLIB_AVAILABLE_IN_ALL
xboolean x_key_file_load_from_data_dirs(XKeyFile *key_file, const xchar *file, xchar **full_path, XKeyFileFlags flags, XError **error);

XLIB_AVAILABLE_IN_ALL
xchar *x_key_file_to_data(XKeyFile *key_file, xsize *length, XError **error) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_2_40
xboolean x_key_file_save_to_file(XKeyFile *key_file, const xchar *filename, XError **error);

XLIB_AVAILABLE_IN_ALL
xchar *x_key_file_get_start_group(XKeyFile *key_file) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_ALL
xchar  **x_key_file_get_groups(XKeyFile *key_file, xsize *length);

XLIB_AVAILABLE_IN_ALL
xchar **x_key_file_get_keys(XKeyFile *key_file, const xchar *group_name, xsize *length, XError **error);

XLIB_AVAILABLE_IN_ALL
xboolean  x_key_file_has_group(XKeyFile *key_file, const xchar *group_name);

XLIB_AVAILABLE_IN_ALL
xboolean x_key_file_has_key(XKeyFile *key_file, const xchar *group_name, const xchar *key, XError **error);

XLIB_AVAILABLE_IN_ALL
xchar *x_key_file_get_value(XKeyFile *key_file, const xchar *group_name, const xchar *key, XError **error) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_ALL
void x_key_file_set_value(XKeyFile *key_file, const xchar *group_name, const xchar *key, const xchar *value);

XLIB_AVAILABLE_IN_ALL
xchar *x_key_file_get_string(XKeyFile *key_file, const xchar *group_name, const xchar *key, XError **error) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_ALL
void x_key_file_set_string(XKeyFile *key_file, const xchar *group_name, const xchar *key, const xchar *string);

XLIB_AVAILABLE_IN_ALL
xchar *x_key_file_get_locale_string(XKeyFile *key_file, const xchar *group_name, const xchar *key, const xchar *locale, XError **error) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_2_56
xchar *x_key_file_get_locale_for_key(XKeyFile *key_file, const xchar *group_name, const xchar *key, const xchar *locale) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_ALL
void x_key_file_set_locale_string(XKeyFile *key_file, const xchar *group_name, const xchar *key, const xchar *locale, const xchar *string);

XLIB_AVAILABLE_IN_ALL
xboolean x_key_file_get_boolean(XKeyFile *key_file, const xchar *group_name, const xchar *key, XError **error);

XLIB_AVAILABLE_IN_ALL
void x_key_file_set_boolean(XKeyFile *key_file, const xchar *group_name, const xchar *key, xboolean value);

XLIB_AVAILABLE_IN_ALL
xint x_key_file_get_integer(XKeyFile *key_file, const xchar *group_name, const xchar *key, XError **error);

XLIB_AVAILABLE_IN_ALL
void x_key_file_set_integer(XKeyFile *key_file, const xchar *group_name, const xchar *key, xint value);

XLIB_AVAILABLE_IN_ALL
xint64 x_key_file_get_int64(XKeyFile *key_file, const xchar *group_name, const xchar *key, XError **error);

XLIB_AVAILABLE_IN_ALL
void x_key_file_set_int64(XKeyFile *key_file, const xchar *group_name, const xchar *key, xint64 value);

XLIB_AVAILABLE_IN_ALL
xuint64 x_key_file_get_uint64(XKeyFile *key_file, const xchar *group_name, const xchar *key, XError **error);

XLIB_AVAILABLE_IN_ALL
void x_key_file_set_uint64(XKeyFile *key_file, const xchar *group_name, const xchar *key, xuint64 value);

XLIB_AVAILABLE_IN_ALL
xdouble x_key_file_get_double(XKeyFile *key_file, const xchar *group_name, const xchar *key, XError **error);

XLIB_AVAILABLE_IN_ALL
void x_key_file_set_double(XKeyFile *key_file, const xchar *group_name, const xchar *key, xdouble value);

XLIB_AVAILABLE_IN_ALL
xchar **x_key_file_get_string_list(XKeyFile *key_file, const xchar *group_name, const xchar *key, xsize *length, XError **error);

XLIB_AVAILABLE_IN_ALL
void x_key_file_set_string_list(XKeyFile *key_file, const xchar *group_name, const xchar *key, const xchar *const list[], xsize length);

XLIB_AVAILABLE_IN_ALL
xchar **x_key_file_get_locale_string_list(XKeyFile *key_file, const xchar *group_name, const xchar *key, const xchar *locale, xsize *length, XError **error);

XLIB_AVAILABLE_IN_ALL
void x_key_file_set_locale_string_list(XKeyFile *key_file, const xchar *group_name, const xchar *key, const xchar *locale, const xchar *const list[], xsize length);

XLIB_AVAILABLE_IN_ALL
xboolean *x_key_file_get_boolean_list(XKeyFile *key_file, const xchar *group_name, const xchar *key, xsize *length, XError **error) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_ALL
void x_key_file_set_boolean_list(XKeyFile *key_file, const xchar *group_name, const xchar *key, xboolean list[], xsize length);

XLIB_AVAILABLE_IN_ALL
xint *x_key_file_get_integer_list(XKeyFile *key_file, const xchar *group_name, const xchar *key, xsize *length, XError **error) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_ALL
void x_key_file_set_double_list(XKeyFile *key_file, const xchar *group_name, const xchar *key, xdouble list[], xsize length);

XLIB_AVAILABLE_IN_ALL
xdouble *x_key_file_get_double_list(XKeyFile *key_file, const xchar *group_name, const xchar *key, xsize *length, XError **error) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_ALL
void x_key_file_set_integer_list(XKeyFile *key_file, const xchar *group_name, const xchar *key, xint list[], xsize length);

XLIB_AVAILABLE_IN_ALL
xboolean x_key_file_set_comment(XKeyFile *key_file, const xchar *group_name, const xchar *key, const xchar *comment, XError **error);

XLIB_AVAILABLE_IN_ALL
xchar *x_key_file_get_comment(XKeyFile *key_file, const xchar *group_name, const xchar *key, XError **error) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_ALL
xboolean x_key_file_remove_comment(XKeyFile *key_file, const xchar *group_name, const xchar *key, XError **error);

XLIB_AVAILABLE_IN_ALL
xboolean x_key_file_remove_key(XKeyFile *key_file, const xchar *group_name, const xchar *key, XError **error);

XLIB_AVAILABLE_IN_ALL
xboolean x_key_file_remove_group(XKeyFile *key_file, const xchar *group_name, XError **error);

#define X_KEY_FILE_DESKTOP_GROUP                "Desktop Entry"

#define X_KEY_FILE_DESKTOP_KEY_TYPE             "Type"
#define X_KEY_FILE_DESKTOP_KEY_VERSION          "Version"
#define X_KEY_FILE_DESKTOP_KEY_NAME             "Name"
#define X_KEY_FILE_DESKTOP_KEY_GENERIC_NAME     "GenericName"
#define X_KEY_FILE_DESKTOP_KEY_NO_DISPLAY       "NoDisplay"
#define X_KEY_FILE_DESKTOP_KEY_COMMENT          "Comment"
#define X_KEY_FILE_DESKTOP_KEY_ICON             "Icon"
#define X_KEY_FILE_DESKTOP_KEY_HIDDEN           "Hidden"
#define X_KEY_FILE_DESKTOP_KEY_ONLY_SHOW_IN     "OnlyShowIn"
#define X_KEY_FILE_DESKTOP_KEY_NOT_SHOW_IN      "NotShowIn"
#define X_KEY_FILE_DESKTOP_KEY_TRY_EXEC         "TryExec"
#define X_KEY_FILE_DESKTOP_KEY_EXEC             "Exec"
#define X_KEY_FILE_DESKTOP_KEY_PATH             "Path"
#define X_KEY_FILE_DESKTOP_KEY_TERMINAL         "Terminal"
#define X_KEY_FILE_DESKTOP_KEY_MIME_TYPE        "MimeType"
#define X_KEY_FILE_DESKTOP_KEY_CATEGORIES       "Categories"
#define X_KEY_FILE_DESKTOP_KEY_STARTUP_NOTIFY   "StartupNotify"
#define X_KEY_FILE_DESKTOP_KEY_STARTUP_WM_CLASS "StartupWMClass"
#define X_KEY_FILE_DESKTOP_KEY_URL              "URL"
#define X_KEY_FILE_DESKTOP_KEY_DBUS_ACTIVATABLE "DBusActivatable"
#define X_KEY_FILE_DESKTOP_KEY_ACTIONS          "Actions"

#define X_KEY_FILE_DESKTOP_TYPE_APPLICATION     "Application"
#define X_KEY_FILE_DESKTOP_TYPE_LINK            "Link"
#define X_KEY_FILE_DESKTOP_TYPE_DIRECTORY       "Directory"

X_END_DECLS

#endif
