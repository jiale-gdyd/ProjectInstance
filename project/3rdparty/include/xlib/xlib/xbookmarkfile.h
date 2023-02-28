#ifndef __X_BOOKMARK_FILE_H__
#define __X_BOOKMARK_FILE_H__

#include <time.h>

#include "xerror.h"
#include "xdatetime.h"

X_BEGIN_DECLS

#define X_BOOKMARK_FILE_ERROR               (x_bookmark_file_error_quark())

typedef enum {
    X_BOOKMARK_FILE_ERROR_INVALID_URI,
    X_BOOKMARK_FILE_ERROR_INVALID_VALUE,
    X_BOOKMARK_FILE_ERROR_APP_NOT_REGISTERED,
    X_BOOKMARK_FILE_ERROR_URI_NOT_FOUND,
    X_BOOKMARK_FILE_ERROR_READ,
    X_BOOKMARK_FILE_ERROR_UNKNOWN_ENCODING,
    X_BOOKMARK_FILE_ERROR_WRITE,
    X_BOOKMARK_FILE_ERROR_FILE_NOT_FOUND
} XBookmarkFileError;

XLIB_AVAILABLE_IN_ALL
XQuark x_bookmark_file_error_quark(void);

typedef struct _XBookmarkFile XBookmarkFile;

XLIB_AVAILABLE_IN_ALL
XBookmarkFile *X_bookmark_file_new(void);

XLIB_AVAILABLE_IN_ALL
void x_bookmark_file_free(XBookmarkFile *bookmark);

XLIB_AVAILABLE_IN_2_76
XBookmarkFile *x_bookmark_file_copy(XBookmarkFile *bookmark);

XLIB_AVAILABLE_IN_ALL
xboolean x_bookmark_file_load_from_file(XBookmarkFile *bookmark, const xchar *filename, XError **error);

XLIB_AVAILABLE_IN_ALL
xboolean x_bookmark_file_load_from_data(XBookmarkFile *bookmark, const xchar *data, xsize length, XError **error);

XLIB_AVAILABLE_IN_ALL
xboolean x_bookmark_file_load_from_data_dirs(XBookmarkFile *bookmark, const xchar *file, xchar **full_path, XError **error);

XLIB_AVAILABLE_IN_ALL
xchar *x_bookmark_file_to_data(XBookmarkFile *bookmark, xsize *length, XError **error) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_ALL
xboolean x_bookmark_file_to_file(XBookmarkFile *bookmark, const xchar *filename, XError **error);

XLIB_AVAILABLE_IN_ALL
void x_bookmark_file_set_title(XBookmarkFile *bookmark, const xchar *uri, const xchar *title);

XLIB_AVAILABLE_IN_ALL
xchar *x_bookmark_file_get_title(XBookmarkFile *bookmark, const xchar *uri, XError **error) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_ALL
void x_bookmark_file_set_description(XBookmarkFile *bookmark, const xchar *uri, const xchar *description);

XLIB_AVAILABLE_IN_ALL
xchar *x_bookmark_file_get_description(XBookmarkFile *bookmark, const xchar *uri, XError **error) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_ALL
void x_bookmark_file_set_mime_type(XBookmarkFile *bookmark, const xchar *uri, const xchar *mime_type);

XLIB_AVAILABLE_IN_ALL
xchar *x_bookmark_file_get_mime_type(XBookmarkFile  *bookmark, const xchar *uri, XError **error) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_ALL
void x_bookmark_file_set_groups(XBookmarkFile *bookmark, const xchar *uri, const xchar **groups, xsize length);

XLIB_AVAILABLE_IN_ALL
void x_bookmark_file_add_group(XBookmarkFile *bookmark, const xchar *uri, const xchar *group);

XLIB_AVAILABLE_IN_ALL
xboolean x_bookmark_file_has_group(XBookmarkFile *bookmark, const xchar *uri, const xchar *group, XError **error);

XLIB_AVAILABLE_IN_ALL
xchar **x_bookmark_file_get_groups(XBookmarkFile *bookmark, const xchar *uri, xsize *length, XError **error);

XLIB_AVAILABLE_IN_ALL
void x_bookmark_file_add_application(XBookmarkFile *bookmark, const xchar *uri, const xchar *name, const xchar *exec);

XLIB_AVAILABLE_IN_ALL
xboolean x_bookmark_file_has_application(XBookmarkFile *bookmark, const xchar *uri, const xchar *name, XError **error);

XLIB_AVAILABLE_IN_ALL
xchar **x_bookmark_file_get_applications(XBookmarkFile *bookmark, const xchar *uri, xsize *length, XError **error);

XLIB_DEPRECATED_IN_2_66_FOR(x_bookmark_file_set_application_info)
xboolean x_bookmark_file_set_app_info(XBookmarkFile *bookmark, const xchar *uri, const xchar *name, const xchar *exec, xint count, time_t stamp, XError **error);

XLIB_AVAILABLE_IN_2_66
xboolean x_bookmark_file_set_application_info(XBookmarkFile *bookmark, const char *uri, const char *name, const char *exec, int count, XDateTime *stamp, XError **error);

XLIB_DEPRECATED_IN_2_66_FOR(x_bookmark_file_get_application_info)
xboolean x_bookmark_file_get_app_info(XBookmarkFile *bookmark, const xchar *uri, const xchar *name, xchar **exec, xuint *count, time_t *stamp, XError **error);

XLIB_AVAILABLE_IN_2_66
xboolean x_bookmark_file_get_application_info(XBookmarkFile *bookmark, const char *uri, const char *name, char **exec, unsigned int *count, XDateTime **stamp, XError **error);

XLIB_AVAILABLE_IN_ALL
void x_bookmark_file_set_is_private(XBookmarkFile *bookmark, const xchar *uri, xboolean is_private);

XLIB_AVAILABLE_IN_ALL
xboolean x_bookmark_file_get_is_private(XBookmarkFile *bookmark, const xchar *uri, XError **error);

XLIB_AVAILABLE_IN_ALL
void x_bookmark_file_set_icon(XBookmarkFile *bookmark, const xchar *uri, const xchar *href, const xchar *mime_type);

XLIB_AVAILABLE_IN_ALL
xboolean x_bookmark_file_get_icon(XBookmarkFile *bookmark, const xchar *uri, xchar **href, xchar **mime_type, XError **error);

XLIB_DEPRECATED_IN_2_66_FOR(x_bookmark_file_set_added_date_time)
void x_bookmark_file_set_added(XBookmarkFile  *bookmark, const xchar *uri, time_t added);

XLIB_AVAILABLE_IN_2_66
void x_bookmark_file_set_added_date_time(XBookmarkFile *bookmark, const char *uri, XDateTime *added);

XLIB_DEPRECATED_IN_2_66_FOR(x_bookmark_file_get_added_date_time)
time_t x_bookmark_file_get_added(XBookmarkFile *bookmark, const xchar *uri, XError **error);

XLIB_AVAILABLE_IN_2_66
XDateTime *x_bookmark_file_get_added_date_time(XBookmarkFile *bookmark, const char *uri, XError **error);

XLIB_DEPRECATED_IN_2_66_FOR(x_bookmark_file_set_modified_date_time)
void x_bookmark_file_set_modified(XBookmarkFile  *bookmark, const xchar *uri, time_t modified);

XLIB_AVAILABLE_IN_2_66
void x_bookmark_file_set_modified_date_time(XBookmarkFile *bookmark, const char *uri, XDateTime *modified);

XLIB_DEPRECATED_IN_2_66_FOR(x_bookmark_file_get_modified_date_time)
time_t x_bookmark_file_get_modified(XBookmarkFile *bookmark, const xchar *uri, XError **error);

XLIB_AVAILABLE_IN_2_66
XDateTime *x_bookmark_file_get_modified_date_time(XBookmarkFile *bookmark, const char *uri, XError **error);

XLIB_DEPRECATED_IN_2_66_FOR(x_bookmark_file_set_visited_date_time)
void x_bookmark_file_set_visited(XBookmarkFile *bookmark, const xchar *uri, time_t visited);

XLIB_AVAILABLE_IN_2_66
void x_bookmark_file_set_visited_date_time(XBookmarkFile *bookmark, const char *uri, XDateTime *visited);

XLIB_DEPRECATED_IN_2_66_FOR(x_bookmark_file_get_visited_date_time)
time_t x_bookmark_file_get_visited(XBookmarkFile *bookmark, const xchar *uri, XError **error);

XLIB_AVAILABLE_IN_2_66
XDateTime *x_bookmark_file_get_visited_date_time(XBookmarkFile *bookmark, const char *uri, XError **error);

XLIB_AVAILABLE_IN_ALL
xboolean x_bookmark_file_has_item(XBookmarkFile *bookmark, const xchar *uri);

XLIB_AVAILABLE_IN_ALL
xint x_bookmark_file_get_size(XBookmarkFile *bookmark);

XLIB_AVAILABLE_IN_ALL
xchar **x_bookmark_file_get_uris(XBookmarkFile *bookmark, xsize *length);

XLIB_AVAILABLE_IN_ALL
xboolean x_bookmark_file_remove_group(XBookmarkFile *bookmark, const xchar *uri, const xchar *group, XError **error);

XLIB_AVAILABLE_IN_ALL
xboolean x_bookmark_file_remove_application(XBookmarkFile *bookmark, const xchar *uri, const xchar *name, XError **error);

XLIB_AVAILABLE_IN_ALL
xboolean x_bookmark_file_remove_item(XBookmarkFile *bookmark, const xchar *uri, XError **error);

XLIB_AVAILABLE_IN_ALL
xboolean x_bookmark_file_move_item(XBookmarkFile *bookmark, const xchar *old_uri, const xchar *new_uri, XError **error);

X_END_DECLS

#endif
