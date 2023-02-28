#include <fcntl.h>
#include <utime.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <xlib/xlib/config.h>
#include <xlib/xlib/xlibconfig.h>
#include <xlib/xlib/xstdio.h>
#include <xlib/xlib/xfileutils.h>

int x_access(const xchar *filename, int mode)
{
    return access(filename, mode);
}

int x_chmod(const xchar *filename, int mode)
{
    return chmod(filename, mode);
}

int x_open(const xchar *filename, int flags, int mode)
{
    int fd;

    do {
        fd = open(filename, flags, mode);
    } while (X_UNLIKELY(fd == -1 && errno == EINTR));

    return fd;
}

int x_creat(const xchar *filename, int mode)
{
    return creat(filename, mode);
}

int x_rename(const xchar *oldfilename, const xchar *newfilename)
{
    return rename(oldfilename, newfilename);
}

int x_mkdir(const xchar *filename, int mode)
{
    return mkdir(filename, mode);
}

int x_chdir(const xchar *path)
{
    return chdir(path);
}

int x_stat(const xchar *filename, XStatBuf *buf)
{
    return stat(filename, buf);
}

int x_lstat(const xchar *filename, XStatBuf *buf)
{
    return lstat(filename, buf);
}

int x_unlink(const xchar *filename)
{
    return unlink(filename);
}

int x_remove(const xchar *filename)
{
    return remove(filename);
}

int x_rmdir(const xchar *filename)
{
    return rmdir(filename);
}

FILE *x_fopen(const xchar *filename, const xchar *mode)
{
    return fopen(filename, mode);
}

FILE *x_freopen(const xchar *filename, const xchar *mode, FILE *stream)
{
    return freopen(filename, mode, stream);
}

xint x_fsync(xint fd)
{
#if defined(HAVE_FSYNC) || defined(HAVE_FCNTL_F_FULLFSYNC)
    int retval;

    do {
#ifdef HAVE_FCNTL_F_FULLFSYNC
        retval = fcntl(fd, F_FULLFSYNC, 0);
#else
        retval = fsync(fd);
#endif
    } while (X_UNLIKELY(retval < 0 && errno == EINTR));

    return retval;
#else
    return 0;
#endif
}

int x_utime (const xchar *filename, struct utimbuf *utb)
{
    return utime(filename, utb);
}

xboolean x_close(xint fd, XError **error)
{
    int res;

    res = close(fd);
    if (res == -1) {
        int errsv = errno;
        if (errsv == EINTR) {
            return TRUE;
        }

        if (error) {
            x_set_error_literal(error, X_FILE_ERROR, x_file_error_from_errno(errsv), x_strerror(errsv));
        }

        if (errsv == EBADF) {
            if (fd >= 0) {
                x_critical("x_close(fd:%d) failed with EBADF. The tracking of file descriptors got messed up", fd);
            } else {
                x_critical("x_close(fd:%d) failed with EBADF. This is not a valid file descriptor", fd);
            }
        }

        errno = errsv;
        return FALSE;
    }

    return TRUE;
}
