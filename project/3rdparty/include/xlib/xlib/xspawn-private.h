#include <errno.h>

#include "config.h"
#include "xspawn.h"
#include "xlibintl.h"

static inline xint _x_spawn_exec_err_to_x_error(xint en)
{
    switch (en) {
#ifdef EACCES
        case EACCES:
            return X_SPAWN_ERROR_ACCES;
#endif

#ifdef EPERM
        case EPERM:
            return X_SPAWN_ERROR_PERM;
#endif

#ifdef E2BIG
        case E2BIG:
            return X_SPAWN_ERROR_TOO_BIG;
#endif

#ifdef ENOEXEC
        case ENOEXEC:
            return X_SPAWN_ERROR_NOEXEC;
#endif

#ifdef ENAMETOOLONG
        case ENAMETOOLONG:
            return X_SPAWN_ERROR_NAMETOOLONG;
#endif

#ifdef ENOENT
        case ENOENT:
            return X_SPAWN_ERROR_NOENT;
#endif

#ifdef ENOMEM
        case ENOMEM:
            return X_SPAWN_ERROR_NOMEM;
#endif

#ifdef ENOTDIR
        case ENOTDIR:
            return X_SPAWN_ERROR_NOTDIR;
#endif

#ifdef ELOOP
        case ELOOP:
            return X_SPAWN_ERROR_LOOP;
#endif

#ifdef ETXTBUSY
        case ETXTBUSY:
            return X_SPAWN_ERROR_TXTBUSY;
#endif

#ifdef EIO
        case EIO:
            return X_SPAWN_ERROR_IO;
#endif

#ifdef ENFILE
        case ENFILE:
            return X_SPAWN_ERROR_NFILE;
#endif

#ifdef EMFILE
        case EMFILE:
            return X_SPAWN_ERROR_MFILE;
#endif

#ifdef EINVAL
        case EINVAL:
            return X_SPAWN_ERROR_INVAL;
#endif

#ifdef EISDIR
        case EISDIR:
            return X_SPAWN_ERROR_ISDIR;
#endif

#ifdef ELIBBAD
        case ELIBBAD:
            return X_SPAWN_ERROR_LIBBAD;
#endif
        default:
            return X_SPAWN_ERROR_FAILED;
    }
}

static inline xboolean _x_spawn_invalid_source_fd(xint fd, const xint *source_fds, xsize n_fds, XError **error)
{
    xsize i;

    for (i = 0; i < n_fds; i++) {
        if (fd == source_fds[i]) {
            x_set_error(error, X_SPAWN_ERROR, X_SPAWN_ERROR_INVAL, _("Invalid source FDs argument"));
            return TRUE;
        }
    }

    return FALSE;
}
