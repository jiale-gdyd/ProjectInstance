#ifndef LIBLOG_LOCKFILE_H
#define LIBLOG_LOCKFILE_H

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>

#define INVALID_LOCK_FD     -1

int lock_file(char *path);
bool unlock_file(int fd);

#endif