#ifndef __X_FILEUTILS_H__
#define __X_FILEUTILS_H__

#include "xlibconfig.h"
#include "xerror.h"
#include "xtypes.h"

X_BEGIN_DECLS

#define X_FILE_ERROR                x_file_error_quark()
#define X_IS_DIR_SEPARATOR(c)       ((c) == X_DIR_SEPARATOR)

typedef enum {
    X_FILE_ERROR_EXIST,
    X_FILE_ERROR_ISDIR,
    X_FILE_ERROR_ACCES,
    X_FILE_ERROR_NAMETOOLONG,
    X_FILE_ERROR_NOENT,
    X_FILE_ERROR_NOTDIR,
    X_FILE_ERROR_NXIO,
    X_FILE_ERROR_NODEV,
    X_FILE_ERROR_ROFS,
    X_FILE_ERROR_TXTBSY,
    X_FILE_ERROR_FAULT,
    X_FILE_ERROR_LOOP,
    X_FILE_ERROR_NOSPC,
    X_FILE_ERROR_NOMEM,
    X_FILE_ERROR_MFILE,
    X_FILE_ERROR_NFILE,
    X_FILE_ERROR_BADF,
    X_FILE_ERROR_INVAL,
    X_FILE_ERROR_PIPE,
    X_FILE_ERROR_AGAIN,
    X_FILE_ERROR_INTR,
    X_FILE_ERROR_IO,
    X_FILE_ERROR_PERM,
    X_FILE_ERROR_NOSYS,
    X_FILE_ERROR_FAILED
} XFileError;

typedef enum {
    X_FILE_TEST_IS_REGULAR    = 1 << 0,
    X_FILE_TEST_IS_SYMLINK    = 1 << 1,
    X_FILE_TEST_IS_DIR        = 1 << 2,
    X_FILE_TEST_IS_EXECUTABLE = 1 << 3,
    X_FILE_TEST_EXISTS        = 1 << 4
} XFileTest;

typedef enum {
    X_FILE_SET_CONTENTS_NONE = 0,
    X_FILE_SET_CONTENTS_CONSISTENT = 1 << 0,
    X_FILE_SET_CONTENTS_DURABLE = 1 << 1,
    X_FILE_SET_CONTENTS_ONLY_EXISTING = 1 << 2
} XFileSetContentsFlags
XLIB_AVAILABLE_ENUMERATOR_IN_2_66;

XLIB_AVAILABLE_IN_ALL
XQuark x_file_error_quark(void);

XLIB_AVAILABLE_IN_ALL
XFileError x_file_error_from_errno(xint err_no);

XLIB_AVAILABLE_IN_ALL
xboolean x_file_test(const xchar *filename, XFileTest test);

XLIB_AVAILABLE_IN_ALL
xboolean x_file_get_contents(const xchar *filename, xchar **contents, xsize *length, XError **error);

XLIB_AVAILABLE_IN_ALL
xboolean x_file_set_contents(const xchar *filename, const xchar *contents, xssize length, XError **error);

X_GNUC_BEGIN_IGNORE_DEPRECATIONS
XLIB_AVAILABLE_IN_2_66
xboolean x_file_set_contents_full(const xchar *filename, const xchar *contents, xssize length, XFileSetContentsFlags flags, int mode, XError **error);
X_GNUC_END_IGNORE_DEPRECATIONS

XLIB_AVAILABLE_IN_ALL
xchar *x_file_read_link(const xchar *filename, XError **error);

XLIB_AVAILABLE_IN_2_30
xchar *x_mkdtemp(xchar *tmpl);

XLIB_AVAILABLE_IN_2_30
xchar *x_mkdtemp_full(xchar *tmpl, xint mode);

XLIB_AVAILABLE_IN_ALL
xint x_mkstemp(xchar *tmpl);

XLIB_AVAILABLE_IN_ALL
xint x_mkstemp_full(xchar *tmpl, xint flags, xint mode);

XLIB_AVAILABLE_IN_ALL
xint x_file_open_tmp(const xchar *tmpl, xchar **name_used, XError **error);

XLIB_AVAILABLE_IN_2_30
xchar *x_dir_make_tmp(const xchar *tmpl, XError **error);

XLIB_AVAILABLE_IN_ALL
xchar *x_build_path(const xchar *separator, const xchar *first_element, ...) X_GNUC_MALLOC X_GNUC_NULL_TERMINATED;

XLIB_AVAILABLE_IN_ALL
xchar *x_build_pathv(const xchar *separator, xchar **args) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_ALL
xchar *x_build_filename(const xchar *first_element, ...) X_GNUC_MALLOC X_GNUC_NULL_TERMINATED;

XLIB_AVAILABLE_IN_ALL
xchar *x_build_filenamev(xchar **args) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_2_56
xchar *x_build_filename_valist(const xchar *first_element, va_list *args) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_ALL
xint x_mkdir_with_parents(const xchar *pathname, xint mode);

XLIB_AVAILABLE_IN_ALL
xboolean x_path_is_absolute(const xchar *file_name);
XLIB_AVAILABLE_IN_ALL
const xchar *x_path_skip_root(const xchar *file_name);

XLIB_DEPRECATED_FOR(x_path_get_basename)
const xchar *x_basename(const xchar *file_name);

#define x_dirname   x_path_get_dirname XLIB_DEPRECATED_MACRO_IN_2_26_FOR(x_path_get_dirname)

XLIB_AVAILABLE_IN_ALL
xchar *x_get_current_dir(void);

XLIB_AVAILABLE_IN_ALL
xchar *x_path_get_basename(const xchar *file_name) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_ALL
xchar *x_path_get_dirname(const xchar *file_name) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_2_58
xchar *x_canonicalize_filename(const xchar *filename, const xchar *relative_to) X_GNUC_MALLOC;

X_END_DECLS

#endif
