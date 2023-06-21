#include "acl/lib_acl/StdAfx.h"

#ifndef ACL_PREPARE_COMPILE

#include "acl/lib_acl/stdlib/acl_define.h"

#ifdef  ACL_WINDOWS
#include <io.h>
#include <stdarg.h>
#include <errno.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <time.h>

#endif

#ifdef	ACL_UNIX
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <signal.h>
#endif

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "acl/lib_acl/stdlib/acl_msg.h"
#include "acl/lib_acl/stdlib/acl_vstream.h"
#include "acl/lib_acl/stdlib/acl_sys_patch.h"

#endif

#if defined(ACL_UNIX)

ACL_FILE_HANDLE acl_file_open(const char *filepath, int flags, int mode)
{
    return open(filepath, flags, mode);
}

int acl_file_close(ACL_FILE_HANDLE fh)
{
    return close(fh);
}

acl_off_t acl_lseek(ACL_FILE_HANDLE fh, acl_off_t offset, int whence)
{
#if	defined(ACL_LINUX) || defined(ACL_SUNOS5)
# if    defined(MINGW)
    return lseek(fh, offset, whence);
# else
    return lseek64(fh, offset, whence);
# endif
#else
    return lseek(fh, offset, whence);
#endif
}

int acl_file_read(ACL_FILE_HANDLE fh, void *buf, size_t size,
    int timeout acl_unused, ACL_VSTREAM *fp, void *arg acl_unused)
{
    if (fp != NULL && fp->read_ready) {
        fp->read_ready = 0;
    }
    return (int) read(fh, buf, size);
}

int acl_file_write(ACL_FILE_HANDLE fh, const void *buf, size_t size,
    int timeout acl_unused, ACL_VSTREAM *fp acl_unused,
    void *arg acl_unused)
{
    return (int) write(fh, buf, size);
}

int acl_file_writev(ACL_FILE_HANDLE fh, const struct iovec *vector, int count,
    int timeout acl_unused, ACL_VSTREAM *fp acl_unused,
    void *arg acl_unused)
{
    return (int) writev(fh, vector, count);
}

int acl_file_fflush(ACL_FILE_HANDLE fh, ACL_VSTREAM *fp acl_unused,
    void *arg acl_unused)
{
    return fsync(fh);
}

acl_int64 acl_file_size(const char *filename)
{
    struct acl_stat sbuf;

    if (acl_stat(filename, &sbuf) == -1) {
        return -1;
    }
    return sbuf.st_size;
}

acl_int64 acl_file_fsize(ACL_FILE_HANDLE fh, ACL_VSTREAM *fp acl_unused,
    void *arg acl_unused)
{
    struct acl_stat sbuf;

    if (acl_fstat(fh, &sbuf) == -1) {
        return -1;
    }
    return sbuf.st_size;
}

#else
# error "unknown OS type"
#endif
