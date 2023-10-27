#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <xlib/xlib/config.h>
#include <xlib/xlib/xlibconfig.h>

#ifndef S_ISLNK
#define S_ISLNK(x)          0
#endif

#ifndef O_BINARY
#define O_BINARY            0
#endif

#ifndef O_CLOEXEC
#define O_CLOEXEC           0
#endif

#include <xlib/xlib/xstdio.h>
#include <xlib/xlib/xlibintl.h>
#include <xlib/xlib/xfileutils.h>

int x_mkdir_with_parents(const xchar *pathname, int mode)
{
    xchar *fn, *p;

    if (pathname == NULL || *pathname == '\0') {
        errno = EINVAL;
        return -1;
    }

    if (x_mkdir(pathname, mode) == 0) {
        return 0;
    } else if (errno == EEXIST) {
        if (!x_file_test(pathname, X_FILE_TEST_IS_DIR)) {
            errno = ENOTDIR;
            return -1;
        }

        return 0;
    }

    fn = x_strdup(pathname);
    if (x_path_is_absolute(fn)) {
        p = (xchar *)x_path_skip_root(fn);
    } else {
        p = fn;
    }

    do {
        while (*p && !X_IS_DIR_SEPARATOR(*p)) {
            p++;
        }

        if (!*p) {
            p = NULL;
        } else {
            *p = '\0';
        }

        if (!x_file_test(fn, X_FILE_TEST_EXISTS)) {
            if (x_mkdir(fn, mode) == -1 && errno != EEXIST) {
                int errno_save = errno;
                if (errno != ENOENT || !p) {
                    x_free(fn);
                    errno = errno_save;
                    return -1;
                }
            }
        } else if (!x_file_test(fn, X_FILE_TEST_IS_DIR)) {
            x_free(fn);
            errno = ENOTDIR;
            return -1;
        }

        if (p) {
            *p++ = X_DIR_SEPARATOR;
            while (*p && X_IS_DIR_SEPARATOR(*p)) {
                p++;
            }
        }
    } while (p);

    x_free(fn);
    return 0;
}

xboolean x_file_test(const xchar *filename, XFileTest test)
{
    x_return_val_if_fail(filename != NULL, FALSE);

    if ((test & X_FILE_TEST_EXISTS) && (access(filename, F_OK) == 0)) {
        return TRUE;
    }

    if ((test & X_FILE_TEST_IS_EXECUTABLE) && (access(filename, X_OK) == 0)) {
        if (getuid () != 0)
        return TRUE;
    } else {
        test = (XFileTest)(test & ~X_FILE_TEST_IS_EXECUTABLE);
    }

    if (test & X_FILE_TEST_IS_SYMLINK) {
        struct stat s;

        if ((lstat(filename, &s) == 0) && S_ISLNK(s.st_mode)) {
            return TRUE;
        }
    }

    if (test & (X_FILE_TEST_IS_REGULAR | X_FILE_TEST_IS_DIR | X_FILE_TEST_IS_EXECUTABLE)) {
        struct stat s;

        if (stat(filename, &s) == 0) {
            if ((test & X_FILE_TEST_IS_REGULAR) && S_ISREG(s.st_mode)) {
                return TRUE;
            }

            if ((test & X_FILE_TEST_IS_DIR) && S_ISDIR(s.st_mode)) {
                return TRUE;
            }

            if ((test & X_FILE_TEST_IS_EXECUTABLE) && ((s.st_mode & S_IXOTH) || (s.st_mode & S_IXUSR) || (s.st_mode & S_IXGRP))) {
                return TRUE;
            }
        }
    }

    return FALSE;
}

X_DEFINE_QUARK(x-file-error-quark, x_file_error)

XFileError x_file_error_from_errno(xint err_no)
{
    switch (err_no) {
#ifdef EEXIST
        case EEXIST:
            return X_FILE_ERROR_EXIST;
#endif

#ifdef EISDIR
        case EISDIR:
            return X_FILE_ERROR_ISDIR;
#endif

#ifdef EACCES
        case EACCES:
            return X_FILE_ERROR_ACCES;
#endif

#ifdef ENAMETOOLONG
        case ENAMETOOLONG:
            return X_FILE_ERROR_NAMETOOLONG;
#endif

#ifdef ENOENT
        case ENOENT:
            return X_FILE_ERROR_NOENT;
#endif

#ifdef ENOTDIR
        case ENOTDIR:
            return X_FILE_ERROR_NOTDIR;
#endif

#ifdef ENXIO
        case ENXIO:
            return X_FILE_ERROR_NXIO;
#endif

#ifdef ENODEV
        case ENODEV:
            return X_FILE_ERROR_NODEV;
#endif

#ifdef EROFS
        case EROFS:
            return X_FILE_ERROR_ROFS;
#endif

#ifdef ETXTBSY
        case ETXTBSY:
            return X_FILE_ERROR_TXTBSY;
#endif

#ifdef EFAULT
        case EFAULT:
            return X_FILE_ERROR_FAULT;
#endif

#ifdef ELOOP
        case ELOOP:
            return X_FILE_ERROR_LOOP;
#endif

#ifdef ENOSPC
        case ENOSPC:
            return X_FILE_ERROR_NOSPC;
#endif

#ifdef ENOMEM
        case ENOMEM:
            return X_FILE_ERROR_NOMEM;
#endif

#ifdef EMFILE
        case EMFILE:
            return X_FILE_ERROR_MFILE;
#endif

#ifdef ENFILE
        case ENFILE:
            return X_FILE_ERROR_NFILE;
#endif

#ifdef EBADF
        case EBADF:
            return X_FILE_ERROR_BADF;
#endif

#ifdef EINVAL
        case EINVAL:
            return X_FILE_ERROR_INVAL;
#endif

#ifdef EPIPE
        case EPIPE:
            return X_FILE_ERROR_PIPE;
#endif

#ifdef EAGAIN
        case EAGAIN:
            return X_FILE_ERROR_AGAIN;
#endif

#ifdef EINTR
        case EINTR:
            return X_FILE_ERROR_INTR;
#endif

#ifdef EIO
        case EIO:
            return X_FILE_ERROR_IO;
#endif

#ifdef EPERM
        case EPERM:
            return X_FILE_ERROR_PERM;
#endif

#ifdef ENOSYS
        case ENOSYS:
            return X_FILE_ERROR_NOSYS;
#endif

        default:
            return X_FILE_ERROR_FAILED;
    }
}

static char *format_error_message(const xchar *filename, const xchar *format_string, int saved_errno) X_GNUC_FORMAT(2);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"

static char *format_error_message(const xchar *filename, const xchar *format_string, int saved_errno)
{
    xchar *msg;
    xchar *display_name;

    display_name = x_filename_display_name(filename);
    msg = x_strdup_printf(format_string, display_name, x_strerror(saved_errno));
    x_free(display_name);

    return msg;
}

#pragma GCC diagnostic pop

static void set_file_error(XError **error, const xchar *filename, const xchar *format_string, int saved_errno)
{
    char *msg = format_error_message(filename, format_string, saved_errno);

    x_set_error_literal(error, X_FILE_ERROR, x_file_error_from_errno(saved_errno), msg);
    x_free(msg);
}

static xboolean get_contents_stdio(const xchar *filename, FILE *f, xchar **contents, xsize *length, XError **error)
{
    xchar *tmp;
    xsize bytes;
    xchar buf[4096];
    xchar *str = NULL;
    xsize total_bytes = 0;
    xchar *display_filename;
    xsize total_allocated = 0;

    x_assert(f != NULL);

    while (!feof(f)) {
        xint save_errno;

        bytes = fread(buf, 1, sizeof(buf), f);
        save_errno = errno;

        if (total_bytes > X_MAXSIZE - bytes) {
            goto file_too_large;
        }

        while (total_bytes + bytes >= total_allocated) {
            if (str) {
                if (total_allocated > X_MAXSIZE / 2) {
                    goto file_too_large;
                }

                total_allocated *= 2;
            } else {
                total_allocated = MIN(bytes + 1, sizeof (buf));
            }

            tmp = (xchar *)x_try_realloc(str, total_allocated);
            if (tmp == NULL) {
                display_filename = x_filename_display_name(filename);
                x_set_error(error, X_FILE_ERROR, X_FILE_ERROR_NOMEM, x_dngettext(GETTEXT_PACKAGE, "Could not allocate %lu byte to read file “%s”", "Could not allocate %lu bytes to read file “%s”", (xulong)total_allocated), (xulong)total_allocated, display_filename);
                x_free(display_filename);

                goto error;
            }

            str = tmp;
        }

        if (ferror(f)) {
            display_filename = x_filename_display_name(filename);
            x_set_error(error, X_FILE_ERROR, x_file_error_from_errno(save_errno), _("Error reading file “%s”: %s"), display_filename, x_strerror(save_errno));
            x_free(display_filename);

            goto error;
        }

        x_assert(str != NULL);
        memcpy(str + total_bytes, buf, bytes);

        total_bytes += bytes;
    }

    fclose(f);

    if (total_allocated == 0) {
        str = x_new(xchar, 1);
        total_bytes = 0;
    }

    str[total_bytes] = '\0';
    if (length) {
        *length = total_bytes;
    }
    *contents = str;

    return TRUE;

file_too_large:
    display_filename = x_filename_display_name(filename);
    x_set_error(error, X_FILE_ERROR, X_FILE_ERROR_FAILED, _("File “%s” is too large"), display_filename);
    x_free(display_filename);

error:
    x_free(str);
    fclose(f);
    return FALSE;
}

static xboolean get_contents_regfile(const xchar *filename, struct stat *stat_buf, xint fd, xchar **contents, xsize *length, XError **error)
{
    xchar *buf;
    xsize size;
    xsize bytes_read;
    xsize alloc_size;
    xchar *display_filename;
    
    size = stat_buf->st_size;

    alloc_size = size + 1;
    buf = (xchar *)x_try_malloc(alloc_size);

    if (buf == NULL) {
        display_filename = x_filename_display_name(filename);
        x_set_error(error, X_FILE_ERROR, X_FILE_ERROR_NOMEM, x_dngettext(GETTEXT_PACKAGE, "Could not allocate %lu byte to read file “%s”", "Could not allocate %lu bytes to read file “%s”", (xulong)alloc_size), (xulong)alloc_size,  display_filename);
        x_free(display_filename);
        goto error;
    }

    bytes_read = 0;
    while (bytes_read < size) {
        xssize rc;

        rc = read(fd, buf + bytes_read, size - bytes_read);
        if (rc < 0) {
            if (errno != EINTR) {
                int save_errno = errno;

                x_free(buf);
                display_filename = x_filename_display_name(filename);
                x_set_error(error, X_FILE_ERROR, x_file_error_from_errno(save_errno), _("Failed to read from file “%s”: %s"), display_filename, 
                x_strerror(save_errno));
                x_free(display_filename);

                goto error;
            }
        } else if (rc == 0) {
            break;
        } else {
            bytes_read += rc;
        }
    }

    buf[bytes_read] = '\0';
    if (length) {
        *length = bytes_read;
    }
    *contents = buf;

    close(fd);
    return TRUE;

error:
    close(fd);
    return FALSE;
}

static xboolean get_contents_posix(const xchar *filename, xchar **contents, xsize *length, XError **error)
{
    xint fd;
    struct stat stat_buf;

    fd = open(filename, O_RDONLY | O_BINARY | O_CLOEXEC);
    if (fd < 0) {
        int saved_errno = errno;
        if (error) {
            set_file_error(error, filename, _("Failed to open file “%s”: %s"), saved_errno);
        }

        return FALSE;
    }

    if (fstat(fd, &stat_buf) < 0) {
        int saved_errno = errno;
        if (error) {
            set_file_error(error, filename, _("Failed to get attributes of file “%s”: fstat() failed: %s"), saved_errno);
        }

        close(fd);
        return FALSE;
    }

    if (stat_buf.st_size > 0 && S_ISREG(stat_buf.st_mode)) {
        xboolean retval = get_contents_regfile(filename, &stat_buf, fd, contents, length, error);
        return retval;
    } else {
        FILE *f;
        xboolean retval;

        f = fdopen(fd, "r");
        if (f == NULL) {
            int saved_errno = errno;
            if (error) {
                set_file_error(error, filename, _("Failed to open file “%s”: fdopen() failed: %s"), saved_errno);
            }

            return FALSE;
        }

        retval = get_contents_stdio (filename, f, contents, length, error);
        return retval;
    }
}

xboolean x_file_get_contents(const xchar *filename, xchar **contents, xsize *length, XError **error)
{  
    x_return_val_if_fail(filename != NULL, FALSE);
    x_return_val_if_fail(contents != NULL, FALSE);

    *contents = NULL;
    if (length) {
        *length = 0;
    }

    return get_contents_posix(filename, contents, length, error);
}

static xboolean rename_file(const char *old_name, const char *new_name, xboolean do_fsync, XError **err)
{
    errno = 0;
    if (x_rename(old_name, new_name) == -1) {
        int save_errno = errno;
        xchar *display_old_name = x_filename_display_name(old_name);
        xchar *display_new_name = x_filename_display_name(new_name);

        x_set_error(err, X_FILE_ERROR, x_file_error_from_errno(save_errno), _("Failed to rename file “%s” to “%s”: x_rename() failed: %s"), display_old_name, display_new_name, x_strerror(save_errno));

        x_free(display_old_name);
        x_free(display_new_name);
        
        return FALSE;
    }

    if (do_fsync) {
        xchar *dir = x_path_get_dirname(new_name);
        int dir_fd = x_open(dir, O_RDONLY | O_CLOEXEC, 0);

        if (dir_fd >= 0) {
            x_fsync(dir_fd);
            x_close(dir_fd, NULL);
        }

        x_free(dir);
    }

    return TRUE;
}

static xboolean fd_should_be_fsynced(int fd, const xchar *test_file, XFileSetContentsFlags flags)
{
    struct stat statbuf;

    if ((flags & (X_FILE_SET_CONTENTS_CONSISTENT | X_FILE_SET_CONTENTS_DURABLE)) && (flags & X_FILE_SET_CONTENTS_ONLY_EXISTING)) {
        errno = 0;
        if (x_lstat(test_file, &statbuf) == 0) {
            return (statbuf.st_size > 0);
        } else if (errno == ENOENT) {
            return FALSE;
        } else {
            return TRUE;
        }
    } else {
        return (flags & (X_FILE_SET_CONTENTS_CONSISTENT | X_FILE_SET_CONTENTS_DURABLE));
    }
}

static xboolean truncate_file(int fd, off_t length, const char  *dest_file, XError **error)
{
    while (ftruncate(fd, length) < 0) {
        int saved_errno = errno;
        if (saved_errno == EINTR) {
            continue;
        }

        if (error != NULL) {
            set_file_error(error, dest_file, _("Failed to write file “%s”: ftruncate() failed: %s"), saved_errno);
        }

        return FALSE;
    }

    return TRUE;
}

static xboolean write_to_file(const xchar *contents, xsize length, int fd, const xchar *dest_file, xboolean do_fsync, XError **err)
{
    if (length > 0) {
        (void)fallocate(fd, 0, 0, length);
    }

    while (length > 0) {
        xssize s;

        s = write(fd, contents, MIN(length, (xsize)X_MAXSSIZE));
        if (s < 0) {
            int saved_errno = errno;
            if (saved_errno == EINTR) {
                continue;
            }

            if (err) {
                set_file_error(err, dest_file, _("Failed to write file “%s”: write() failed: %s"), saved_errno);
            }
            close(fd);

            return FALSE;
        }

        x_assert((xsize) s <= length);

        contents += s;
        length -= s;
    }

    errno = 0;
    if (do_fsync && x_fsync(fd) != 0) {
        int saved_errno = errno;
        if (err) {
            set_file_error(err, dest_file, _("Failed to write file “%s”: fsync() failed: %s"), saved_errno);
        }
        close(fd);

        return FALSE;
    }

    errno = 0;
    if (!x_close(fd, err)) {
        return FALSE;
    }

    return TRUE;
}

xboolean x_file_set_contents(const xchar  *filename, const xchar *contents, xssize length, XError **error)
{
    return x_file_set_contents_full(filename, contents, length, (XFileSetContentsFlags)(X_FILE_SET_CONTENTS_CONSISTENT | X_FILE_SET_CONTENTS_ONLY_EXISTING), 0666, error);
}

xboolean x_file_set_contents_full(const xchar *filename, const xchar *contents, xssize length, XFileSetContentsFlags flags, int mode, XError **error)
{
    x_return_val_if_fail(filename != NULL, FALSE);
    x_return_val_if_fail(error == NULL || *error == NULL, FALSE);
    x_return_val_if_fail(contents != NULL || length == 0, FALSE);
    x_return_val_if_fail(length >= -1, FALSE);

    if (length < 0) {
        length = strlen(contents);
    }

    if (flags & X_FILE_SET_CONTENTS_CONSISTENT) {
        int fd;
        xboolean retval;
        xboolean do_fsync;
        xchar *tmp_filename = NULL;
        XError *rename_error = NULL;

        tmp_filename = x_strdup_printf("%s.XXXXXX", filename);
        errno = 0;

        fd = x_mkstemp_full(tmp_filename, O_RDWR | O_BINARY | O_CLOEXEC, mode);
        if (fd == -1) {
            int saved_errno = errno;
            if (error) {
                set_file_error(error, tmp_filename, _("Failed to create file “%s”: %s"), saved_errno);
            }

            retval = FALSE;
            goto consistent_out;
        }

        do_fsync = fd_should_be_fsynced(fd, filename, flags);
        if (!write_to_file(contents, length, x_steal_fd(&fd), tmp_filename, do_fsync, error)) {
            x_unlink(tmp_filename);
            retval = FALSE;
            goto consistent_out;
        }

        if (!rename_file(tmp_filename, filename, do_fsync, &rename_error)) {
            x_unlink(tmp_filename);
            x_propagate_error(error, rename_error);
            retval = FALSE;

            goto consistent_out;
        }

        retval = TRUE;

consistent_out:
        x_free(tmp_filename);
        return retval;
    } else {
        int direct_fd;
        int open_flags;
        xboolean do_fsync;

        open_flags = O_RDWR | O_BINARY | O_CREAT | O_CLOEXEC;
#ifdef O_NOFOLLOW
        open_flags |= O_NOFOLLOW;
#endif
        errno = 0;
        direct_fd = x_open(filename, open_flags, mode);

        if (direct_fd < 0) {
            int saved_errno = errno;

#ifdef O_NOFOLLOW
#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__DragonFly__)
            if (saved_errno == EMLINK)
#elif defined(__NetBSD__)
            if (saved_errno == EFTYPE)
#else
            if (saved_errno == ELOOP)
#endif
                return x_file_set_contents_full(filename, contents, length, (XFileSetContentsFlags)(flags | X_FILE_SET_CONTENTS_CONSISTENT), mode, error);
#endif
            if (error) {
                set_file_error(error, filename, _("Failed to open file “%s”: %s"), saved_errno);
            }

            return FALSE;
        }

        do_fsync = fd_should_be_fsynced(direct_fd, filename, flags);
        if (!truncate_file(direct_fd, 0, filename, error)) {
            return FALSE;
        }

        if (!write_to_file(contents, length, x_steal_fd(&direct_fd), filename, do_fsync, error)) {
            return FALSE;
        }
    }

    return TRUE;
}

typedef xint (*XTmpFileCallback)(const xchar *, xint, xint);

static xint get_tmp_file (xchar *tmpl, XTmpFileCallback f, int flags, int mode)
{
    xint64 value;
    char *XXXXXX;
    xint64 now_us;
    int count, fd;
    static int counter = 0;
    static const char letters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    static const int NLETTERS = sizeof(letters) - 1;

    x_return_val_if_fail(tmpl != NULL, -1);

    XXXXXX = x_strrstr(tmpl, "XXXXXX");

    if (!XXXXXX || strncmp(XXXXXX, "XXXXXX", 6)) {
        errno = EINVAL;
        return -1;
    }

    now_us = x_get_real_time();
    value = ((now_us % X_USEC_PER_SEC) ^ (now_us / X_USEC_PER_SEC)) + counter++;

    for (count = 0; count < 100; value += 7777, ++count) {
        xint64 v = value;

        XXXXXX[0] = letters[v % NLETTERS];
        v /= NLETTERS;
        XXXXXX[1] = letters[v % NLETTERS];
        v /= NLETTERS;
        XXXXXX[2] = letters[v % NLETTERS];
        v /= NLETTERS;
        XXXXXX[3] = letters[v % NLETTERS];
        v /= NLETTERS;
        XXXXXX[4] = letters[v % NLETTERS];
        v /= NLETTERS;
        XXXXXX[5] = letters[v % NLETTERS];

        fd = f(tmpl, flags, mode);
        if (fd >= 0) {
            return fd;
        } else if (errno != EEXIST) {
            return -1;
        }
    }

    errno = EEXIST;
    return -1;
}

static xint wrap_g_mkdir(const xchar *filename, int flags X_GNUC_UNUSED, int mode)
{
    return x_mkdir(filename, mode);
}

static xint wrap_g_open(const xchar *filename, int flags, int mode)
{
    return x_open(filename, flags, mode);
}

xchar *x_mkdtemp_full(xchar *tmpl, xint mode)
{
    if (get_tmp_file(tmpl, wrap_g_mkdir, 0, mode) == -1) {
        return NULL;
    } else {
        return tmpl;
    }
}

xchar *x_mkdtemp(xchar *tmpl)
{
    return x_mkdtemp_full(tmpl, 0700);
}

xint x_mkstemp_full(xchar *tmpl, xint flags, xint mode)
{
    return get_tmp_file(tmpl, wrap_g_open, flags | O_CREAT | O_EXCL, mode);
}

xint x_mkstemp(xchar *tmpl)
{
    return x_mkstemp_full(tmpl, O_RDWR | O_BINARY | O_CLOEXEC, 0600);
}

static xint x_get_tmp_name(const xchar *tmpl, xchar **name_used, XTmpFileCallback f, xint flags, xint mode, XError **error)
{
    int retval;
    const char *sep;
    const char *slash;
    const char *tmpdir;
    char *fulltemplate;

    if (tmpl == NULL) {
        tmpl = ".XXXXXX";
    }

    if ((slash = strchr(tmpl, X_DIR_SEPARATOR)) != NULL) {
        xchar *display_tmpl = x_filename_display_name(tmpl);
        char c[2];

        c[0] = *slash;
        c[1] = '\0';

        x_set_error(error, X_FILE_ERROR, X_FILE_ERROR_FAILED, _("Template “%s” invalid, should not contain a “%s”"), display_tmpl, c);
        x_free(display_tmpl);

        return -1;
    }

    if (strstr(tmpl, "XXXXXX") == NULL) {
        xchar *display_tmpl = x_filename_display_name(tmpl);
        x_set_error(error, X_FILE_ERROR, X_FILE_ERROR_FAILED, _("Template “%s” doesn’t contain XXXXXX"), display_tmpl);
        x_free(display_tmpl);
        return -1;
    }

    tmpdir = x_get_tmp_dir();

    if (X_IS_DIR_SEPARATOR(tmpdir[strlen(tmpdir) - 1])) {
        sep = "";
    } else {
        sep = X_DIR_SEPARATOR_S;
    }

    fulltemplate = x_strconcat(tmpdir, sep, tmpl, NULL);

    retval = get_tmp_file(fulltemplate, f, flags, mode);
    if (retval == -1) {
        int saved_errno = errno;
        if (error) {
            set_file_error(error, fulltemplate, _("Failed to create file “%s”: %s"), saved_errno);
        }

        x_free(fulltemplate);
        return -1;
    }

    *name_used = fulltemplate;
    return retval;
}

xint x_file_open_tmp(const xchar *tmpl, xchar **name_used, XError **error)
{
    xint result;
    xchar *fulltemplate;

    x_return_val_if_fail(error == NULL || *error == NULL, -1);

    result = x_get_tmp_name(tmpl, &fulltemplate, wrap_g_open, O_CREAT | O_EXCL | O_RDWR | O_BINARY | O_CLOEXEC, 0600, error);
    if (result != -1) {
        if (name_used) {
            *name_used = fulltemplate;
        } else {
            x_free(fulltemplate);
        }
    }

    return result;
}

xchar *x_dir_make_tmp(const xchar *tmpl, XError **error)
{
    xchar *fulltemplate;

    x_return_val_if_fail(error == NULL || *error == NULL, NULL);

    if (x_get_tmp_name(tmpl, &fulltemplate, wrap_g_mkdir, 0, 0700, error) == -1) {
        return NULL;
    } else {
        return fulltemplate;
    }
}

static xchar *x_build_path_va(const xchar *separator, const xchar *first_element, va_list *args, xchar **str_array)
{
    xint i = 0;
    XString *result;
    xboolean is_first = TRUE;
    const xchar *next_element;
    xboolean have_leading = FALSE;
    const xchar *last_trailing = NULL;
    const xchar *single_element = NULL;
    xint separator_len = strlen(separator);

    result = x_string_new(NULL);
    if (str_array) {
        next_element = str_array[i++];
    } else {
        next_element = first_element;
    }

    while (TRUE) {
        const xchar *end;
        const xchar *start;
        const xchar *element;

        if (next_element) {
            element = next_element;
            if (str_array) {
                next_element = str_array[i++];
            } else {
                next_element = va_arg(*args, xchar *);
            }
        } else {
            break;
        }

        if (!*element) {
            continue;
        }
        
        start = element;
        if (separator_len) {
            while (strncmp(start, separator, separator_len) == 0) {
                start += separator_len;
            }
        }

        end = start + strlen(start);
        if (separator_len) {
            while (end >= start + separator_len && strncmp(end - separator_len, separator, separator_len) == 0) {
                end -= separator_len;
            }

            last_trailing = end;
            while (last_trailing >= element + separator_len && strncmp(last_trailing - separator_len, separator, separator_len) == 0) {
                last_trailing -= separator_len;
            }

            if (!have_leading) {
                if (last_trailing <= start) {
                    single_element = element;
                }

                x_string_append_len(result, element, start - element);
                have_leading = TRUE;
            } else {
                single_element = NULL;
            }
        }

        if (end == start) {
            continue;
        }

        if (!is_first) {
            x_string_append(result, separator);
        }

        x_string_append_len(result, start, end - start);
        is_first = FALSE;
    }

    if (single_element) {
        x_string_free(result, TRUE);
        return x_strdup(single_element);
    } else {
        if (last_trailing) {
            x_string_append(result, last_trailing);
        }

        return x_string_free(result, FALSE);
    }
}

xchar *x_build_pathv(const xchar *separator, xchar **args)
{
    if (!args) {
        return NULL;
    }

    return x_build_path_va(separator, NULL, NULL, args);
}

xchar *x_build_path(const xchar *separator, const xchar *first_element, ...)
{
    xchar *str;
    va_list args;

    x_return_val_if_fail(separator != NULL, NULL);

    va_start(args, first_element);
    str = x_build_path_va(separator, first_element, &args, NULL);
    va_end(args);

    return str;
}

static xchar *x_build_filename_va(const xchar  *first_argument, va_list *args, xchar **str_array)
{
    xchar *str;
    str = x_build_path_va(X_DIR_SEPARATOR_S, first_argument, args, str_array);
    return str;
}

xchar *x_build_filename_valist(const xchar *first_element, va_list *args)
{
    x_return_val_if_fail(first_element != NULL, NULL);
    return x_build_filename_va(first_element, args, NULL);
}

xchar *x_build_filenamev(xchar **args)
{
    return x_build_filename_va(NULL, NULL, args);
}

xchar *x_build_filename(const xchar *first_element, ...)
{
    xchar *str;
    va_list args;

    va_start(args, first_element);
    str = x_build_filename_va(first_element, &args, NULL);
    va_end(args);

    return str;
}

xchar *x_file_read_link(const xchar *filename, XError **error)
{
#if defined(HAVE_READLINK)
    size_t size;
    xchar *buffer;
    xssize read_size;

    x_return_val_if_fail(filename != NULL, NULL);
    x_return_val_if_fail(error == NULL || *error == NULL, NULL);

    size = 256;
    buffer = (xchar *)x_malloc(size);

    while (TRUE) {
        read_size = readlink(filename, buffer, size);
        if (read_size < 0) {
            int saved_errno = errno;
            if (error) {
                set_file_error(error, filename, _("Failed to read the symbolic link “%s”: %s"), saved_errno);
            }

            x_free(buffer);
            return NULL;
        }

        if ((size_t)read_size < size) {
            buffer[read_size] = 0;
            return buffer;
        }

        size *= 2;
        buffer = (xchar *)x_realloc(buffer, size);
    }
#else
    x_return_val_if_fail(filename != NULL, NULL);
    x_return_val_if_fail(error == NULL || *error == NULL, NULL);

    x_set_error_literal(error, X_FILE_ERROR, X_FILE_ERROR_INVAL, _("Symbolic links not supported"));

    return NULL;
#endif
}

xboolean x_path_is_absolute(const xchar *file_name)
{
    x_return_val_if_fail(file_name != NULL, FALSE);

    if (X_IS_DIR_SEPARATOR(file_name[0])) {
        return TRUE;
    }

    return FALSE;
}

const xchar *x_path_skip_root(const xchar *file_name)
{
    x_return_val_if_fail (file_name != NULL, NULL);

    if (X_IS_DIR_SEPARATOR(file_name[0])) {
        while (X_IS_DIR_SEPARATOR(file_name[0])) {
            file_name++;
        }

        return (xchar *)file_name;
    }

    return NULL;
}

const xchar *x_basename(const xchar *file_name)
{
    xchar *base;

    x_return_val_if_fail(file_name != NULL, NULL);

    base = (xchar *)strrchr(file_name, X_DIR_SEPARATOR);
    if (base) {
        return base + 1;
    }

    return (xchar *)file_name;
}

xchar *x_path_get_basename(const xchar *file_name)
{
    xsize len;
    xssize base;
    xchar *retval;
    xssize last_nonslash;

    x_return_val_if_fail(file_name != NULL, NULL);

    if (file_name[0] == '\0') {
        return x_strdup(".");
    }

    last_nonslash = strlen(file_name) - 1;

    while (last_nonslash >= 0 && X_IS_DIR_SEPARATOR(file_name[last_nonslash])) {
        last_nonslash--;
    }

    if (last_nonslash == -1) {
        return x_strdup(X_DIR_SEPARATOR_S);
    }

    base = last_nonslash;
    while (base >=0 && !X_IS_DIR_SEPARATOR(file_name[base])) {
        base--;
    }

    len = last_nonslash - base;
    retval = (xchar *)x_malloc(len + 1);
    memcpy(retval, file_name + (base + 1), len);
    retval[len] = '\0';

    return retval;
}

xchar *x_path_get_dirname(const xchar *file_name)
{
    xsize len;
    xchar *base;

    x_return_val_if_fail(file_name != NULL, NULL);

    base = (xchar *)strrchr((const char *)file_name, X_DIR_SEPARATOR);
    if (!base) {
        return x_strdup(".");
    }

    while (base > file_name && X_IS_DIR_SEPARATOR(*base)) {
        base--;
    }

    len = (xuint)1 + base - file_name;
    base = x_new(xchar, len + 1);
    memmove(base, file_name, len);
    base[len] = 0;

    return base;
}

xchar *x_canonicalize_filename(const xchar *filename, const xchar *relative_to)
{
    xchar *canon, *input, *output, *after_root, *output_start;

    x_return_val_if_fail(relative_to == NULL || x_path_is_absolute(relative_to), NULL);

    if (!x_path_is_absolute(filename)) {
        const xchar *cwd;
        xchar *cwd_allocated = NULL;

        if (relative_to != NULL) {
            cwd = relative_to;
        } else {
            cwd = cwd_allocated = x_get_current_dir();
        }

        canon = x_build_filename(cwd, filename, NULL);
        x_free(cwd_allocated);
    } else {
        canon = x_strdup(filename);
    }

    after_root = (char *)x_path_skip_root(canon);
    if (after_root == NULL) {
        x_free(canon);
        return x_build_filename(X_DIR_SEPARATOR_S, filename, NULL);
    }

    for (output = after_root - 1; (output >= canon) && X_IS_DIR_SEPARATOR(*output); output--) {
        *output = X_DIR_SEPARATOR;
    }

    output++;
    if (*output == X_DIR_SEPARATOR) {
        output++;
    }

    if (after_root - output == 1) {
        output++;
    }

    input = after_root;
    output_start = output;

    while (*input) {
        x_assert(input > canon && X_IS_DIR_SEPARATOR(input[-1]));
        x_assert(output > canon && X_IS_DIR_SEPARATOR(output[-1]));
        x_assert(input >= output);

        while (X_IS_DIR_SEPARATOR(input[0])) {
            input++;
        }

        if (input[0] == '.' && (input[1] == 0 || X_IS_DIR_SEPARATOR(input[1]))) {
            if (input[1] == 0) {
                break;
            }

            input += 2;
        } else if (input[0] == '.' && input[1] == '.' && (input[2] == 0 || X_IS_DIR_SEPARATOR(input[2]))) {
            if (output > output_start) {
                do {
                    output--;
                } while (!X_IS_DIR_SEPARATOR(output[-1]) && output > output_start);
            }

            if (input[2] == 0) {
                break;
            }

            input += 3;
        } else {
            while (*input && !X_IS_DIR_SEPARATOR(*input)) {
                *output++ = *input++;
            }

            if (input[0] == 0) {
                break;
            }

            input++;
            *output++ = X_DIR_SEPARATOR;
        }
    }

    if (output > output_start && X_IS_DIR_SEPARATOR(output[-1])) {
        output--;
    }

    *output = '\0';
    return canon;
}

#if defined(MAXPATHLEN)
#define X_PATH_LENGTH               MAXPATHLEN
#elif defined(PATH_MAX)
#define X_PATH_LENGTH               PATH_MAX
#elif defined(_PC_PATH_MAX)
#define X_PATH_LENGTH               sysconf(_PC_PATH_MAX)
#else
#define X_PATH_LENGTH               2048
#endif

xchar *x_get_current_dir(void)
{
    const xchar *pwd;
    xchar *dir = NULL;
    xchar *buffer = NULL;
    struct stat pwdbuf, dotbuf;
    static xsize buffer_size = 0;

    pwd = x_getenv("PWD");
    if (pwd != NULL && x_stat(".", &dotbuf) == 0 && x_stat(pwd, &pwdbuf) == 0 && dotbuf.st_dev == pwdbuf.st_dev && dotbuf.st_ino == pwdbuf.st_ino) {
        return x_strdup(pwd);
    }

    if (buffer_size == 0) {
        buffer_size = (X_PATH_LENGTH == -1) ? 2048 : X_PATH_LENGTH;
    }

    while (buffer_size < X_MAXSIZE / 2) {
        x_free(buffer);
        buffer = x_new(xchar, buffer_size);
        *buffer = 0;
        dir = getcwd(buffer, buffer_size);

        if (dir || errno != ERANGE) {
            break;
        }

        buffer_size *= 2;
    }

    x_assert(dir == NULL || strnlen(dir, buffer_size) < buffer_size);
    if (!dir || !*buffer) {
        x_assert(buffer_size >= 2);
        buffer[0] = X_DIR_SEPARATOR;
        buffer[1] = 0;
    }

    dir = x_strdup(buffer);
    x_free(buffer);

    return dir;
}
