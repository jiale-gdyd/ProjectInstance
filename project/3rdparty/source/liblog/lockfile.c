#include "lockfile.h"
#include "logc_profile.h"

int lock_file(char *path)
{
    if (!path || (strlen(path) <= 0)) {
        return INVALID_LOCK_FD;
    }

    int fd = open(path, O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
    if (fd == INVALID_LOCK_FD) {
        logc_error("lock file error : %s ", strerror(errno));
    }

    return fd;
}

bool unlock_file(int fd)
{
    if (fd == INVALID_LOCK_FD) {
        return true;
    }

    bool ret = close(fd) == 0;
    if (ret == false) {
        logc_error("unlock file error : %s ", strerror(errno));
    }

    return ret;
}