#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <xlib/xlib/config.h>
#include <xlib/xlib/xdir.h>
#include <xlib/xlib/xlibintl.h>
#include <xlib/xlib/xconvert.h>
#include <xlib/xlib/xstrfuncs.h>
#include <xlib/xlib/xtestutils.h>
#include <xlib/xlib/xfileutils.h>
#include <xlib/xlib/xlib-private.h>

struct _XDir {
    DIR *dirp;
};

XDir *x_dir_open_with_errno(const xchar *path, xuint flags)
{
    XDir dir;

    x_return_val_if_fail(path != NULL, NULL);

    dir.dirp = opendir(path);
    if (dir.dirp == NULL) {
        return NULL;
    }

    return (XDir *)x_memdup2(&dir, sizeof dir);
}

XDir *x_dir_open(const xchar *path, xuint flags, XError **error)
{
    XDir *dir;
    xint saved_errno;

    dir = x_dir_open_with_errno(path, flags);
    if (dir == NULL) {
        xchar *utf8_path;

        saved_errno = errno;
        utf8_path = x_filename_to_utf8(path, -1, NULL, NULL, NULL);

        x_set_error(error, X_FILE_ERROR, x_file_error_from_errno(saved_errno), _("Error opening directory “%s”: %s"), utf8_path, x_strerror(saved_errno));
        x_free(utf8_path);
    }

    return dir;
}

XDir *x_dir_new_from_dirp(xpointer dirp)
{
    XDir *dir;

    x_return_val_if_fail(dirp != NULL, NULL);

    dir = x_new(XDir, 1);
    dir->dirp = (DIR *)dirp;

    return dir;
}

const xchar *x_dir_read_name(XDir *dir)
{
    struct dirent *entry;

    x_return_val_if_fail(dir != NULL, NULL);

    entry = readdir(dir->dirp);
    while (entry && (0 == strcmp(entry->d_name, ".") || 0 == strcmp(entry->d_name, ".."))) {
        entry = readdir(dir->dirp);
    }

    if (entry) {
        return entry->d_name;
    } else {
        return NULL;
    }
}

void x_dir_rewind(XDir *dir)
{
    x_return_if_fail(dir != NULL);
    rewinddir(dir->dirp);
}

void x_dir_close(XDir *dir)
{
    x_return_if_fail(dir != NULL);
    closedir(dir->dirp);
    x_free(dir);
}
