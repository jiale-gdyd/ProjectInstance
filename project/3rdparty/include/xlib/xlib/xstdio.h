#ifndef __X_STDIO_H__
#define __X_STDIO_H__

#include <errno.h>
#include <sys/stat.h>

#include "config.h"
#include "xtypes.h"
#include "xprintf.h"

X_BEGIN_DECLS

struct utimbuf;
typedef struct stat XStatBuf;

XLIB_AVAILABLE_IN_ALL
int x_access(const xchar *filename, int mode);

XLIB_AVAILABLE_IN_ALL
int x_chmod(const xchar *filename, int mode);

XLIB_AVAILABLE_IN_ALL
int x_open(const xchar *filename, int flags, int mode);

XLIB_AVAILABLE_IN_ALL
int x_creat(const xchar *filename, int mode);

XLIB_AVAILABLE_IN_ALL
int x_rename(const xchar *oldfilename, const xchar *newfilename);

XLIB_AVAILABLE_IN_ALL
int x_mkdir(const xchar *filename, int mode);

XLIB_AVAILABLE_IN_ALL
int x_chdir(const xchar *path);

XLIB_AVAILABLE_IN_ALL
int x_stat(const xchar *filename, XStatBuf *buf);

XLIB_AVAILABLE_IN_ALL
int x_lstat(const xchar *filename, XStatBuf *buf);

XLIB_AVAILABLE_IN_ALL
int x_unlink(const xchar *filename);

XLIB_AVAILABLE_IN_ALL
int x_remove(const xchar *filename);

XLIB_AVAILABLE_IN_ALL
int x_rmdir(const xchar *filename);

XLIB_AVAILABLE_IN_ALL
FILE *x_fopen(const xchar *filename, const xchar *mode);

XLIB_AVAILABLE_IN_ALL
FILE *x_freopen(const xchar *filename, const xchar *mode, FILE *stream);

XLIB_AVAILABLE_IN_2_64
xint x_fsync(xint fd);

XLIB_AVAILABLE_IN_ALL
int x_utime(const xchar *filename, struct utimbuf *utb);

XLIB_AVAILABLE_IN_2_36
xboolean x_close(xint fd, XError **error);

XLIB_AVAILABLE_STATIC_INLINE_IN_2_76
static inline xboolean x_clear_fd(int *fd_ptr, XError **error)
{
    int fd = *fd_ptr;

    *fd_ptr = -1;
    if (fd < 0) {
        return TRUE;
    }

X_GNUC_BEGIN_IGNORE_DEPRECATIONS
    return x_close(fd, error);
X_GNUC_END_IGNORE_DEPRECATIONS
}

#ifdef x_autofree
static inline void _x_clear_fd_ignore_error(int *fd_ptr)
{
    int errsv = errno;

    X_GNUC_BEGIN_IGNORE_DEPRECATIONS
    if (!x_clear_fd(fd_ptr, NULL)) {

    }
    X_GNUC_END_IGNORE_DEPRECATIONS

    errno = errsv;
}

#define x_autofd    _XLIB_CLEANUP(_x_clear_fd_ignore_error) XLIB_AVAILABLE_MACRO_IN_2_76
#endif

X_END_DECLS

#endif
