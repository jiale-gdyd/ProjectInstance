#include <pwd.h>
#include <grp.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/stat.h>

#include "config.h"
#include "misc_mosq.h"
#include "logging_mosq.h"

FILE *mosquitto__fopen(const char *path, const char *mode, bool restrict_read)
{
    FILE *fptr;
    struct stat statbuf;

    if (restrict_read) {
        mode_t old_mask;

        old_mask = umask(0077);
        int open_flags = O_NOFOLLOW;

        for (size_t i = 0; i < strlen(mode); i++) {
            if (mode[i] == 'r') {
                open_flags |= O_RDONLY;
            } else if (mode[i] == 'w') {
                open_flags |= O_WRONLY;
                open_flags |= (O_TRUNC | O_CREAT | O_EXCL);
            } else if (mode[i] == 'a') {
                open_flags |= O_WRONLY;
                open_flags |= (O_APPEND | O_CREAT);
            } else if(mode[i] == 't') {
            } else if(mode[i] == 'b') {
            } else if(mode[i] == '+') {
                open_flags |= O_RDWR;
            }
        }

        int fd = open(path, open_flags, 0600);
        if (fd < 0) {
            return NULL;
        }

        fptr = fdopen(fd, mode);
        umask(old_mask);
    } else {
        fptr = fopen(path, mode);
    }

    if (!fptr) {
        return NULL;
    }

    if (fstat(fileno(fptr), &statbuf) < 0) {
        fclose(fptr);
        return NULL;
    }

    if (restrict_read) {
        if (statbuf.st_mode & S_IRWXO){
            fprintf(stderr, "Warning: File %s has world readable permissions. Future versions will refuse to load this file.\nTo fix this, use `chmod 0700 %s`.", path, path);
        }

        if (statbuf.st_uid != getuid()) {
            char buf[4096];
            struct passwd pw, *result;

            getpwuid_r(getuid(), &pw, buf, sizeof(buf), &result);
            if (result) {
                fprintf(stderr, "Warning: File %s owner is not %s. Future versions will refuse to load this file. To fix this, use `chown %s %s`.", path, result->pw_name, result->pw_name, path);
            }
        }

        if (statbuf.st_gid != getgid()) {
            char buf[4096];
            struct group grp, *result;

            getgrgid_r(getgid(), &grp, buf, sizeof(buf), &result);
            if (result) {
                fprintf(stderr, "Warning: File %s group is not %s. Future versions will refuse to load this file.", path, result->gr_name);
            }
        }
    }

    if (!S_ISREG(statbuf.st_mode)) {
        fclose(fptr);
        return NULL;
    }

    return fptr;
}

char *misc__trimblanks(char *str)
{
    char *endptr;

    if (str == NULL) {
        return NULL;
    }

    while (isspace(str[0])) {
        str++;
    }

    endptr = &str[strlen(str) - 1];
    while ((endptr > str) && isspace(endptr[0])) {
        endptr[0] = '\0';
        endptr--;
    }

    return str;
}

char *fgets_extending(char **buf, int *buflen, FILE *stream)
{
    char *rc;
    size_t len;
    char endchar;
    char *newbuf;
    int offset = 0;

    if ((stream == NULL) || (buf == NULL) || (buflen == NULL) || (*buflen < 1)) {
        return NULL;
    }

    do {
        rc = fgets(&((*buf)[offset]), (*buflen) - offset, stream);
        if (feof(stream) || (rc == NULL)) {
            return rc;
        }

        len = strlen(*buf);
        if (len == 0) {
            return rc;
        }
    
        endchar = (*buf)[len - 1];
        if (endchar == '\n') {
            return rc;
        }

        if ((int)(len + 1) < *buflen) {
            return NULL;
        }

        // 找不到EOL字符，因此扩展缓冲区
        offset = (*buflen) - 1;
        *buflen += 1000;
    
        newbuf = (char *)realloc(*buf, (size_t)*buflen);
        if (!newbuf) {
            return NULL;
        }
        *buf = newbuf;
    } while (1);
}

#define INVOKE_LOG_FN(format, ...)                              \
    do {                                                        \
        if (log_fn) {                                           \
            int tmp_err_no = errno;                             \
            char msg[2 * PATH_MAX];                             \
            snprintf(msg, sizeof(msg), (format), __VA_ARGS__);  \
            msg[sizeof(msg) - 1] = '\0';                        \
            (*log_fn)(msg);                                     \
            errno = tmp_err_no;                                 \
        }                                                       \
    } while (0)

int mosquitto_write_file(const char *target_path, bool restrict_read, int (*write_fn)(FILE *fptr, void *user_data), void *user_data, void (*log_fn)(const char *msg))
{
    int rc = 0;
    FILE *fptr = NULL;
    char tmp_file_path[PATH_MAX];

    if (!target_path || !write_fn){
        return MOSQ_ERR_INVAL;
    }

    rc = snprintf(tmp_file_path, PATH_MAX, "%s.new", target_path);
    if ((rc < 0) || (rc >= PATH_MAX)) {
        return MOSQ_ERR_INVAL;
    }

    /**
     * 如果系统在此文件末尾的重命名操作期间断电，则文件系统可能会在上电后留下一个如下所示的目录:
     *
     * 24094 -rw-r--r--    2 root     root          4099 May 30 16:27 mosquitto.db
     * 24094 -rw-r--r--    2 root     root          4099 May 30 16:27 mosquitto.db.new
     *
     * 24094表明mosquitto.db.new硬链接到与mosquitto.db相同的文件。如果fopen(outfile, "wb")被天真地调用，那么mosquitto.db将被截断并且数据库可能损坏。
     * 任何现有的mosquitto.db.new文件必须在打开之前删除，以保证它没有硬链接到mosquitto.db
     */
    if (unlink(tmp_file_path) && (errno != ENOENT)) {
        INVOKE_LOG_FN("unable to remove stale tmp file %s, error %s", tmp_file_path, strerror(errno));
        return MOSQ_ERR_INVAL;
    }

    fptr = mosquitto__fopen(tmp_file_path, "wb", restrict_read);
    if (fptr == NULL) {
        INVOKE_LOG_FN("unable to open %s for writing, error %s", tmp_file_path, strerror(errno));
        return MOSQ_ERR_INVAL;
    }

    if ((rc = (*write_fn)(fptr, user_data)) != MOSQ_ERR_SUCCESS){
        goto error;
    }

    rc = MOSQ_ERR_ERRNO;

    /**
     * 关闭文件并不能保证将内容写入磁盘。需要刷新以将数据从应用程序发送到操作系统缓冲区，然后使用fsync将数据从操作系统缓冲区传递到磁盘(以及磁盘硬件许可)
     * "成功关闭并不能保证数据已成功保存到磁盘，因为内核延迟写入。文件系统在流关闭时刷新缓冲区并不常见。如果您需要确保数据是物理的 存储，使用fsync(2)。(此时将取决于磁盘硬件)"。
     * 这保证了新状态文件在其内容有效之前不会覆盖旧状态文件。
     */
    if ((fflush(fptr) != 0) && (errno != EINTR)) {
        INVOKE_LOG_FN("unable to flush %s, error %s", tmp_file_path, strerror(errno));
        goto error;
    }

    if ((fsync(fileno(fptr)) != 0) && (errno != EINTR)) {
        INVOKE_LOG_FN("unable to sync %s to disk, error %s", tmp_file_path, strerror(errno));
        goto error;
    }

    if (fclose(fptr) != 0) {
        INVOKE_LOG_FN("unable to close %s on disk, error %s", tmp_file_path, strerror(errno));
        fptr = NULL;
        goto error;
    }
    fptr = NULL;

    if (rename(tmp_file_path, target_path) != 0) {
        INVOKE_LOG_FN("unable to replace %s by tmp file  %s, error %s", target_path, tmp_file_path, strerror(errno));
        goto error;
    }

    return MOSQ_ERR_SUCCESS;

error:
    if (fptr) {
        fclose(fptr);
        unlink(tmp_file_path);
    }

    return MOSQ_ERR_ERRNO;
}
