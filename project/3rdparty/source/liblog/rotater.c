#include <glob.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "rotater.h"
#include "logc_defs.h"

#define ROLLING         (1)
#define SEQUENCE        (2)

struct liblog_file {
    int  index;
    char path[LOG_MAXLEN_PATH + 1];
};

void liblog_rotater_profile(struct liblog_rotater *a_rotater, int flag)
{
    logc_assert(a_rotater,);
    logc_profile(flag, "--rotater[%p][%p,%s,%d][%s,%s,%s,%ld,%ld,%d,%d,%d]--",
        a_rotater, &(a_rotater->lock_mutex),
        a_rotater->lock_file, a_rotater->lock_fd,
        a_rotater->base_path, a_rotater->archive_path, a_rotater->glob_path,
        (long)a_rotater->num_start_len, (long)a_rotater->num_end_len,
        a_rotater->num_width, a_rotater->mv_type, a_rotater->max_count);

    if (a_rotater->files) {
        int i;
        struct liblog_file *a_file;
        
        logc_arraylist_foreach(a_rotater->files, i, a_file) {
            logc_profile(flag, "[%s,%d]->", a_file->path, a_file->index);
        }
    }
}

void liblog_rotater_del(struct liblog_rotater *a_rotater)
{
    logc_assert(a_rotater, );

    if (a_rotater->lock_fd != INVALID_LOCK_FD) {
        if (!unlock_file(a_rotater->lock_fd)) {
            logc_error("close fail, errno[%d]", errno);
        }

        a_rotater->lock_fd = INVALID_LOCK_FD;
    }

    if (pthread_mutex_destroy(&(a_rotater->lock_mutex))) {
        logc_error("pthread_mutex_destroy fail, errno[%d]", errno);
    }

    logc_debug("liblog_rotater_del[%p]", a_rotater);
    free(a_rotater);
}

struct liblog_rotater *liblog_rotater_new(char *lock_file)
{
    struct liblog_rotater *a_rotater;

    logc_assert(lock_file, NULL);

    a_rotater = (struct liblog_rotater *)calloc(1, sizeof(struct liblog_rotater));
    if (!a_rotater) {
        logc_error("calloc fail, errno[%d]", errno);
        liblog_rotater_del(a_rotater);
        return NULL;
    }

    if (pthread_mutex_init(&(a_rotater->lock_mutex), NULL)) {
        logc_error("pthread_mutex_init fail, errno[%d]", errno);
        liblog_rotater_del(a_rotater);
        return NULL;
    }

    a_rotater->lock_fd = INVALID_LOCK_FD;
    a_rotater->lock_file = lock_file;

    return a_rotater;
}

static void liblog_file_del(struct liblog_file *a_file)
{
    logc_debug("del onefile[%p]", a_file);
    logc_debug("a_file->path[%s]", a_file->path);
    free(a_file);
}

static struct liblog_file *liblog_file_check_new(struct liblog_rotater *a_rotater, const char *path)
{
    int nwrite, nread;
    struct liblog_file *a_file;

    if (LOG_STRCMP(a_rotater->base_path, ==, path)) {
        return NULL;
    }

    if ((path)[strlen(path) - 1] == '/') {
        return NULL;
    }

    a_file = (struct liblog_file *)calloc(1, sizeof(struct liblog_file));
    if (!a_file) {
        logc_error("calloc fail, errno[%d]", errno);
        return NULL;
    }

    nwrite = snprintf(a_file->path, sizeof(a_file->path), "%s", path);
    if ((nwrite < 0) || (nwrite >= (int)sizeof(a_file->path))) {
        logc_error("snprintf fail or overflow, nwrite=[%d], errno[%d]", nwrite, errno);
        goto err;
    }

    nread = 0;
    sscanf(a_file->path + a_rotater->num_start_len, "%d%n", &(a_file->index), &(nread));

    if (a_rotater->num_width != 0) {
        if (nread < a_rotater->num_width) {
            logc_error("aa.1.log is not expect, need aa.01.log");
            goto err;
        }
    }

    return a_file;

err:
    free(a_file);
    return NULL;
}

static int liblog_file_cmp(struct liblog_file *a_file1, struct liblog_file *a_file2)
{
    return (a_file1->index > a_file2->index);
}

static int liblog_rotater_add_archive_files(struct liblog_rotater *a_rotater)
{
    int rc = 0;
    char **pathv;
    size_t pathc;
    glob_t glob_buf;
    struct liblog_file *a_file;

    a_rotater->files = logc_arraylist_new((arraylist_del_func)liblog_file_del);
    if (!a_rotater->files) {
        logc_error("libc_arraylist_new fail");
        return -1;
    }

    rc = glob(a_rotater->glob_path, GLOB_ERR | GLOB_MARK | GLOB_NOSORT, NULL, &glob_buf);
    if (rc == GLOB_NOMATCH) {
        goto exit;
    } else if (rc) {
        logc_error("glob err, rc=[%d], errno[%d]", rc, errno);
        return -1;
    }

    pathv = glob_buf.gl_pathv;
    pathc = glob_buf.gl_pathc;

    for (; pathc-- > 0; pathv++) {
        a_file = liblog_file_check_new(a_rotater, *pathv);
        if (!a_file) {
            logc_warn("not the expect pattern file");
            continue;
        }

        rc = logc_arraylist_sortadd(a_rotater->files, (arraylist_cmp_func)liblog_file_cmp, a_file);
        if (rc) {
            logc_error("libc_arraylist_sortadd faile");
            goto err;
        }
    }

exit:
    globfree(&glob_buf);
    return 0;

err:
    globfree(&glob_buf);
    return -1;
}

static int liblog_rotater_seq_files(struct liblog_rotater *a_rotater)
{
    int i, j;
    int rc = 0;
    int nwrite = 0;
    struct liblog_file *a_file;
    char new_path[LOG_MAXLEN_PATH + 1];

    logc_arraylist_foreach(a_rotater->files, i, a_file) {
        if ((a_rotater->max_count > 0) && (i < (logc_arraylist_len(a_rotater->files) - a_rotater->max_count))) {
            rc = unlink(a_file->path);
            if (rc) {
                logc_error("unlink[%s] fail, errno[%d]", a_file->path, errno);
                return -1;
            }

            continue;
        }
    }

    if (logc_arraylist_len(a_rotater->files) > 0) {
        a_file = (struct liblog_file *)logc_arraylist_get(a_rotater->files, logc_arraylist_len(a_rotater->files) - 1);
        if (!a_file) {
            logc_error("logc_arraylist_get fail");
            return -1;
        }

        j = logc_max(logc_arraylist_len(a_rotater->files) - 1, a_file->index) + 1;
    } else {
        j = 0;
    }

    memset(new_path, 0x00, sizeof(new_path));
    nwrite = snprintf(new_path, sizeof(new_path), "%.*s%0*d%s", (int)a_rotater->num_start_len, a_rotater->glob_path, a_rotater->num_width, j, a_rotater->glob_path + a_rotater->num_end_len);
    if ((nwrite < 0) || (nwrite >= (int)sizeof(new_path))) {
        logc_error("nwrite[%d], overflow or errno[%d]", nwrite, errno);
        return -1;
    }

    if (rename(a_rotater->base_path, new_path)) {
        logc_error("rename[%s]->[%s] fail, errno[%d]", a_rotater->base_path, new_path, errno);
        return -1;
    }

    return 0;
}

static int liblog_rotater_roll_files(struct liblog_rotater *a_rotater)
{
    int i;
    int rc = 0;
    int nwrite;
    struct liblog_file *a_file;
    char new_path[LOG_MAXLEN_PATH + 1];

    for (i = logc_arraylist_len(a_rotater->files) - 1; i > -1; i--) {
        a_file = (struct liblog_file *)logc_arraylist_get(a_rotater->files, i);
        if (!a_file) {
            logc_error("logc_arraylist_get fail");
            return -1;
        }

        if ((a_rotater->max_count > 0) && (i >= (a_rotater->max_count - 1))) {
            rc = unlink(a_file->path);
            if (rc) {
                logc_error("unlink[%s] fail, errno[%d]", a_file->path, errno);
                return -1;
            }

            continue;
        }

        memset(new_path, 0x00, sizeof(new_path));
        nwrite = snprintf(new_path, sizeof(new_path), "%.*s%0*d%s", (int)a_rotater->num_start_len, a_rotater->glob_path, a_rotater->num_width, i + 1, a_rotater->glob_path + a_rotater->num_end_len);
        if ((nwrite < 0) || (nwrite >= (int)sizeof(new_path))) {
            logc_error("nwrite[%d], overflow or errno[%d]", nwrite, errno);
            return -1;
        }

        if (rename(a_file->path, new_path)) {
            logc_error("rename[%s]->[%s] fail, errno[%d]", a_file->path, new_path, errno);
            return -1;
        }
    }

    memset(new_path, 0x00, sizeof(new_path));
    nwrite = snprintf(new_path, sizeof(new_path), "%.*s%0*d%s", (int)a_rotater->num_start_len, a_rotater->glob_path, a_rotater->num_width, 0, a_rotater->glob_path + a_rotater->num_end_len);
    if ((nwrite < 0) || (nwrite >= (int)sizeof(new_path))) {
        logc_error("nwrite[%d], overflow or errno[%d]", nwrite, errno);
        return -1;
    }

    if (rename(a_rotater->base_path, new_path)) {
        logc_error("rename[%s]->[%s] fail, errno[%d]", a_rotater->base_path, new_path, errno);
        return -1;
    }

    return 0;
}

static int liblog_rotater_parse_archive_path(struct liblog_rotater *a_rotater)
{
    char *p;
    int nread;
    int nwrite;
    size_t len;

    if (a_rotater->archive_path[0] == '\0') {
        nwrite = snprintf(a_rotater->glob_path, sizeof(a_rotater->glob_path), "%s.*", a_rotater->base_path);
        if ((nwrite < 0) || (nwrite > (int)sizeof(a_rotater->glob_path))) {
            logc_error("nwrite[%d], overflow or errno[%d]", nwrite, errno);
            return -1;
        }

        a_rotater->mv_type = ROLLING;
        a_rotater->num_width = 0;
        a_rotater->num_start_len = strlen(a_rotater->base_path) + 1;
        a_rotater->num_end_len = strlen(a_rotater->base_path) + 2;

        return 0;
    } else {
        p = strchr(a_rotater->archive_path, '#');
        if (!p) {
            logc_error("no # in archive_path[%s]", a_rotater->archive_path);
            return -1;
        }

        nread = 0;
        sscanf(p, "#%d%n", &(a_rotater->num_width), &nread);
        if (nread == 0) {
            nread = 1;
        }

        if (*(p + nread) == 'r') {
            a_rotater->mv_type = ROLLING;
        } else if (*(p + nread) == 's') {
            a_rotater->mv_type = SEQUENCE;
        } else {
            logc_error("#r or #s not found");
            return -1;
        }

        len = p - a_rotater->archive_path;
        if (len > (sizeof(a_rotater->glob_path) - 1)) {
            logc_error("sizeof glob_path not enough, len[%ld]", (long)len);
            return -1;
        }

        memcpy(a_rotater->glob_path, a_rotater->archive_path, len);

        nwrite = snprintf(a_rotater->glob_path + len, sizeof(a_rotater->glob_path) - len, "*%s", p + nread + 1);
        if ((nwrite < 0) || (nwrite > (int)(sizeof(a_rotater->glob_path) - len))) {
            logc_error("nwrite[%d], overflow or errno[%d]", nwrite, errno);
            return -1;
        }

        a_rotater->num_start_len = len;
        a_rotater->num_end_len = len + 1;
    }

    return 0;
}

static void liblog_rotater_clean(struct liblog_rotater *a_rotater)
{
    a_rotater->base_path = NULL;
    a_rotater->archive_path = NULL;
    a_rotater->max_count = 0;
    a_rotater->mv_type = 0;
    a_rotater->num_width = 0;
    a_rotater->num_start_len = 0;
    a_rotater->num_end_len = 0;
    memset(a_rotater->glob_path, 0x00, sizeof(a_rotater->glob_path));

    if (a_rotater->files) {
        logc_arraylist_del(a_rotater->files);
    }
    a_rotater->files = NULL;
}

static int liblog_rotater_lsmv(struct liblog_rotater *a_rotater, char *base_path, char *archive_path, int archive_max_count)
{
    int rc = 0;

    a_rotater->base_path = base_path;
    a_rotater->archive_path = archive_path;
    a_rotater->max_count = archive_max_count;

    rc = liblog_rotater_parse_archive_path(a_rotater);
    if (rc) {
        logc_error("liblog_rotater_parse_archive_path fail");
        goto err;
    }

    rc = liblog_rotater_add_archive_files(a_rotater);
    if (rc) {
        logc_error("liblog_rotater_parse_archive_files fail");
        goto err;
    }

    if (a_rotater->mv_type == ROLLING) {
        rc = liblog_rotater_roll_files(a_rotater);
        if (rc) {
            logc_error("liblog_rotater_roll_files fail");
            goto err;
        }
    } else if (a_rotater->mv_type == SEQUENCE) {
        rc = liblog_rotater_seq_files(a_rotater);
        if (rc) {
            logc_error("liblog_rotater_seq_files fail");
            goto err;
        }
    }

    liblog_rotater_clean(a_rotater);
    return 0;

err:
    liblog_rotater_clean(a_rotater);
    return -1;
}

static int liblog_rotater_trylock(struct liblog_rotater *a_rotater)
{
    int rc;

    rc = pthread_mutex_trylock(&(a_rotater->lock_mutex));
    if (rc == EBUSY) {
        logc_warn("pthread_mutex_trylock fail, as lock_mutex is locked by other threads");
        return -1;
    } else if (rc != 0) {
        logc_error("pthread_mutex_trylock fail, rc[%d]", rc);
        return -1;
    }

    a_rotater->lock_fd = lock_file(a_rotater->lock_file);
    if (a_rotater->lock_fd == INVALID_LOCK_FD) {
        return -1;
    }

    return 0;
}

static int liblog_rotater_unlock(struct liblog_rotater *a_rotater)
{
    int rc = 0;

    if (!unlock_file(a_rotater->lock_fd)) {
        rc = -1;
    } else {
        a_rotater->lock_fd = INVALID_LOCK_FD;
    }

    if (pthread_mutex_unlock(&(a_rotater->lock_mutex))) {
        rc = -1;
        logc_error("pthread_mutex_unlock fail, errno[%d]", errno);
    }

    return rc;
}

int liblog_rotater_rotate(struct liblog_rotater *a_rotater, char *base_path, size_t msg_len, char *archive_path, long archive_max_size, int archive_max_count)
{
    int rc = 0;
    struct log_stat info;

    logc_assert(base_path, -1);

    if (liblog_rotater_trylock(a_rotater)) {
        logc_warn("liblog_rotater_trylock fail, maybe lock by other process or threads");
        return 0;
    }

    if (stat(base_path, &info)) {
        rc = -1;
        logc_error("stat [%s] fail, errno[%d]", base_path, errno);

        goto exit;
    }

    if ((info.st_size + msg_len) <= (size_t)archive_max_size) {
        rc = 0;
        goto exit;
    }

    rc = liblog_rotater_lsmv(a_rotater, base_path, archive_path, archive_max_count);
    if (rc) {
        logc_error("liblog_rotater_lsmv [%s] fail, return", base_path);
        rc = -1;
    }

exit:
    if (liblog_rotater_unlock(a_rotater)) {
        logc_error("liblog_rotater_unlock fail");
    }

    return rc;
}
