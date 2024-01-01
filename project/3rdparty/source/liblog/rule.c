#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "buf.h"
#include "rule.h"
#include "conf.h"
#include "spec.h"
#include "color.h"
#include "format.h"
#include "thread.h"
#include "fmacros.h"
#include "rotater.h"
#include "logc_defs.h"
#include "level_list.h"

#include <liblog/liblog.h>

void liblog_rule_profile(struct liblog_rule *a_rule, int flag)
{
    int i;
    struct liblog_spec *a_spec;

    logc_assert(a_rule, );
    logc_profile(flag, "---rule:[%p][%s%c%d]-[%d,%d][%s,%p,%d:%ld*%d~%s][%d][%d][%s:%s:%p];[%p]---",
        a_rule,

        a_rule->category,
        a_rule->compare_char,
        a_rule->level,

        a_rule->file_perms,
        a_rule->file_open_flags,

        a_rule->file_path,
        a_rule->dynamic_specs,
        a_rule->static_fd,

        a_rule->archive_max_size,
        a_rule->archive_max_count,
        a_rule->archive_path,

        a_rule->pipe_fd,

        a_rule->syslog_facility,

        a_rule->record_name,
        a_rule->record_path,
        a_rule->record_func,
        a_rule->format);

    if (a_rule->dynamic_specs) {
        logc_arraylist_foreach(a_rule->dynamic_specs, i, a_spec) {
            liblog_spec_profile(a_spec, flag);
        }
    }
}

static int liblog_rule_output_static_file_single(struct liblog_rule *a_rule, struct liblog_thread *a_thread)
{
    struct stat stb;
    int do_file_reload = 0;
    int redo_inode_stat = 0;

    if (liblog_format_gen_msg(a_rule->format, a_thread)) {
        logc_error("liblog_format_gen_msg fail");
        return -1;
    }

    if (stat(a_rule->file_path, &stb)) {
        if (errno == ENOENT) {
            logc_error("stat fail on [%s], errno[%d]", a_rule->file_path, errno);
            return -1;
        } else {
            do_file_reload = 1;
            redo_inode_stat = 1;
        }
    } else {
        do_file_reload = ((stb.st_ino != a_rule->static_ino) || (stb.st_dev != a_rule->static_dev));
    }

    if (do_file_reload) {
        close(a_rule->static_fd);
        a_rule->static_fd = open(a_rule->file_path, O_WRONLY | O_APPEND | O_CREAT | a_rule->file_open_flags, a_rule->file_perms);
        if (a_rule->static_fd < 0) {
            logc_error("open file[%s] fail, errno[%d]", a_rule->file_path, errno);
            return -1;
        }

        if (redo_inode_stat) {
            if (stat(a_rule->file_path, &stb)) {
                logc_error("stat fail on new file[%s], errno[%d]", a_rule->file_path, errno);
                return -1;
            }
        }

        a_rule->static_dev = stb.st_dev;
        a_rule->static_ino = stb.st_ino;
    }

    if (write(a_rule->static_fd, liblog_buf_str(a_thread->msg_buf), liblog_buf_len(a_thread->msg_buf)) < 0) {
        logc_error("write fail, errno[%d]", errno);
        return -1;
    }

    if (a_rule->fsync_period && (++a_rule->fsync_count >= a_rule->fsync_period)) {
        a_rule->fsync_count = 0;
        if (fsync(a_rule->static_fd)) {
            logc_error("fsync[%d] fail, errno[%d]", a_rule->static_fd, errno);
        }
    }

    return 0;
}

static char *liblog_rule_gen_archive_path(struct liblog_rule *a_rule, struct liblog_thread *a_thread)
{
    int i;
    struct liblog_spec *a_spec;

    if (!a_rule->archive_specs) {
        return a_rule->archive_path;
    }

    liblog_buf_restart(a_thread->archive_path_buf);

    logc_arraylist_foreach(a_rule->archive_specs, i, a_spec) {
        if (liblog_spec_gen_archive_path(a_spec, a_thread)) {
            logc_error("liblog_spec_gen_archive_path fail");
            return NULL;
        }
    }

    liblog_buf_seal(a_thread->archive_path_buf);
    return liblog_buf_str(a_thread->archive_path_buf);
}

static int liblog_rule_output_static_file_rotate(struct liblog_rule *a_rule, struct liblog_thread *a_thread)
{
    int fd;
    size_t len;
    struct log_stat info;

    if (liblog_format_gen_msg(a_rule->format, a_thread)) {
        logc_error("liblog_format_gen_msg fail");
        return -1;
    }

    fd = open(a_rule->file_path, a_rule->file_open_flags | O_WRONLY | O_APPEND | O_CREAT, a_rule->file_perms);
    if (fd < 0) {
        logc_error("open file[%s] fail, errno[%d]", a_rule->file_path, errno);
        return -1;
    }

    len = liblog_buf_len(a_thread->msg_buf);
    if (write(fd, liblog_buf_str(a_thread->msg_buf), len) < 0) {
        logc_error("write fail, errno[%d]", errno);
        close(fd);

        return -1;
    }

    if (a_rule->fsync_period && (++a_rule->fsync_count >= a_rule->fsync_period)) {
        a_rule->fsync_count = 0;
        if (fsync(fd)) {
            logc_error("fsync[%d] fail, errno[%d]", fd, errno);
        }
    }

    if (close(fd) < 0) {
        logc_error("close fail, maybe cause by write, errno[%d]", errno);
        return -1;
    }

    if (len > (size_t)a_rule->archive_max_size) {
        logc_debug("one msg's len[%ld] > archive_max_size[%ld], no rotate", (long)len, (long)a_rule->archive_max_size);
        return 0;
    }

    if (stat(a_rule->file_path, &info)) {
        logc_warn("stat [%s] fail, errno[%d], maybe in rotating", a_rule->file_path, errno);
        return 0;
    }

    if ((long)(info.st_size + len) < a_rule->archive_max_size) {
        return 0;
    }

    if (liblog_rotater_rotate(liblog_env_conf->rotater, a_rule->file_path, len, liblog_rule_gen_archive_path(a_rule, a_thread), a_rule->archive_max_size, a_rule->archive_max_count)) {
        logc_error("liblog_rotater_rotate fail");
        return -1;
    }

    return 0;
}

#define liblog_rule_gen_path(a_rule, a_thread) do {                 \
    int i;                                                          \
    struct liblog_spec *a_spec;                                     \
                                                                    \
    liblog_buf_restart(a_thread->path_buf);                         \
                                                                    \
    logc_arraylist_foreach(a_rule->dynamic_specs, i, a_spec) {      \
        if (liblog_spec_gen_path(a_spec, a_thread)) {               \
            logc_error("liblog_spec_gen_path fail");                \
            return -1;                                              \
        }                                                           \
    }                                                               \
                                                                    \
    liblog_buf_seal(a_thread->path_buf);                            \
} while(0)

static int liblog_rule_output_dynamic_file_single(struct liblog_rule *a_rule, struct liblog_thread *a_thread)
{
    int fd;

    liblog_rule_gen_path(a_rule, a_thread);

    if (liblog_format_gen_msg(a_rule->format, a_thread)) {
        logc_error("liblog_format_gen_msg fail");
        return -1;
    }

    fd = open(liblog_buf_str(a_thread->path_buf), a_rule->file_open_flags | O_WRONLY | O_APPEND | O_CREAT, a_rule->file_perms);
    if (fd < 0) {
        logc_error("open file[%s] fail, errno[%d]", liblog_buf_str(a_thread->path_buf), errno);
        return -1;
    }

    if (write(fd, liblog_buf_str(a_thread->msg_buf), liblog_buf_len(a_thread->msg_buf)) < 0) {
        logc_error("write fail, errno[%d]", errno);
        close(fd);

        return -1;
    }

    if (a_rule->fsync_period && (++a_rule->fsync_count >= a_rule->fsync_period)) {
        a_rule->fsync_count = 0;
        if (fsync(fd)) {
            logc_error("fsync[%d] fail, errno[%d]", fd, errno);
        }
    }

    if (close(fd) < 0) {
        logc_error("close fail, maybe cause by write, errno[%d]", errno);
        return -1;
    }

    return 0;
}

static int liblog_rule_output_dynamic_file_rotate(struct liblog_rule *a_rule, struct liblog_thread *a_thread)
{
    int fd;
    char *path;
    size_t len;
    struct log_stat info;

    liblog_rule_gen_path(a_rule, a_thread);

    if (liblog_format_gen_msg(a_rule->format, a_thread)) {
        logc_error("liblog_format_gen_msg fail");
        return -1;
    }

    path = liblog_buf_str(a_thread->path_buf);
    fd = open(path, a_rule->file_open_flags | O_WRONLY | O_APPEND | O_CREAT, a_rule->file_perms);
    if (fd < 0) {
        logc_error("open file[%s] fail, errno[%d]", liblog_buf_str(a_thread->path_buf), errno);
        return -1;
    }

    len = liblog_buf_len(a_thread->msg_buf);
    if (write(fd, liblog_buf_str(a_thread->msg_buf), len) < 0) {
        logc_error("write fail, errno[%d]", errno);
        close(fd);

        return -1;
    }

    if (a_rule->fsync_period && (++a_rule->fsync_count >= a_rule->fsync_period)) {
        a_rule->fsync_count = 0;
        if (fsync(fd)) {
            logc_error("fsync[%d] fail, errno[%d]", fd, errno);
        }
    }

    if (close(fd) < 0) {
        logc_error("write fail, maybe cause by write, errno[%d]", errno);
        return -1;
    }

    if (len > (size_t)a_rule->archive_max_size) {
        logc_debug("one msg's len[%ld] > archive_max_size[%ld], no rotate", (long)len, (long)a_rule->archive_max_size);
        return 0;
    }

    if (stat(path, &info)) {
        logc_warn("stat [%s] fail, errno[%d], maybe in rotating", path, errno);
        return 0;
    }

    if ((long)(info.st_size + len) < a_rule->archive_max_size) {
        return 0;
    }

    if (liblog_rotater_rotate(liblog_env_conf->rotater, path, len, liblog_rule_gen_archive_path(a_rule, a_thread), a_rule->archive_max_size, a_rule->archive_max_count)) {
        logc_error("liblog_rotater_rotate fail");
        return -1;
    }

    return 0;
}

static int liblog_rule_output_pipe(struct liblog_rule *a_rule, struct liblog_thread *a_thread)
{
    if (liblog_format_gen_msg(a_rule->format, a_thread)) {
        logc_error("liblog_format_gen_msg fail");
        return -1;
    }

    if (write(a_rule->pipe_fd, liblog_buf_str(a_thread->msg_buf), liblog_buf_len(a_thread->msg_buf)) < 0) {
        logc_error("write fail, errno[%d]", errno);
        return -1;
    }

    return 0;
}

static int liblog_rule_output_syslog(struct liblog_rule *a_rule, struct liblog_thread *a_thread)
{
    struct liblog_level *a_level;

    if (liblog_format_gen_msg(a_rule->format, a_thread)) {
        logc_error("liblog_format_gen_msg fail");
        return -1;
    }

    a_level = liblog_level_list_get(liblog_env_conf->levels, a_thread->event->level);
    liblog_buf_seal(a_thread->msg_buf);
    syslog(a_rule->syslog_facility | a_level->syslog_level, "%s", liblog_buf_str(a_thread->msg_buf));

    return 0;
}

static int liblog_rule_output_static_record(struct liblog_rule *a_rule, struct liblog_thread *a_thread)
{
    struct liblog_msg msg;

    if (!a_rule->record_func) {
        logc_error("user defined record function for [%s] not set, no output", a_rule->record_name);
        return -1;
    }

    if (liblog_format_gen_msg(a_rule->format, a_thread)) {
        logc_error("liblog_format_gen_msg fail");
        return -1;
    }

    liblog_buf_seal(a_thread->msg_buf);

    msg.buf = liblog_buf_str(a_thread->msg_buf);
    msg.len = liblog_buf_len(a_thread->msg_buf);
    msg.path = a_rule->record_path;

    if (a_rule->record_func(&msg)) {
        logc_error("a_rule->record fail");
        return -1;
    }

    return 0;
}

static int liblog_rule_output_dynamic_record(struct liblog_rule *a_rule, struct liblog_thread *a_thread)
{
    struct liblog_msg msg;

    if (!a_rule->record_func) {
        logc_error("user defined record function for [%s] not set, no output", a_rule->record_name);
        return -1;
    }

    liblog_rule_gen_path(a_rule, a_thread);

    if (liblog_format_gen_msg(a_rule->format, a_thread)) {
        logc_error("liblog_format_gen_msg fail");
        return -1;
    }

    liblog_buf_seal(a_thread->msg_buf);

    msg.buf = liblog_buf_str(a_thread->msg_buf);
    msg.len = liblog_buf_len(a_thread->msg_buf);
    msg.path = liblog_buf_str(a_thread->path_buf);

    if (a_rule->record_func(&msg)) {
        logc_error("a_rule->record fail");
        return -1;
    }

    return 0;
}

static int liblog_rule_output_stdout(struct liblog_rule *a_rule, struct liblog_thread *a_thread)
{
    if (liblog_format_gen_msg(a_rule->format, a_thread)) {
        logc_error("zlog_format_gen_msg fail");
        return -1;
    }

#if (COLOR_OUTPUT_TERMINAL_ENABLE == 1)
    size_t str_len = 0;
    char buf[LOG_MAXLEN_PATH] = {0};
    char buffer[LOG_MAXLEN_PATH * 2 + 1] = {0};

    memset(buf, 0x00, sizeof(buf));
    sprintf(buf, "%s", liblog_buf_str(a_thread->msg_buf));
    buf[liblog_buf_len(a_thread->msg_buf)] = 0;
    
    if (a_thread->event->level == LIBLOG_LEVEL_DEBUG) {
        sprintf(buffer, "%s%s%s", LOG_COLOR_DEBUG, buf, LOG_COLOR_RESET);
    } else if (a_thread->event->level == LIBLOG_LEVEL_INFO) {
        sprintf(buffer, "%s%s%s", LOG_COLOR_INFO, buf, LOG_COLOR_RESET);
    } else if (a_thread->event->level == LIBLOG_LEVEL_NOTICE) {
        sprintf(buffer, "%s%s%s", LOG_COLOR_NOTICE, buf, LOG_COLOR_RESET);
    } else if (a_thread->event->level == LIBLOG_LEVEL_WARN) {
        sprintf(buffer, "%s%s%s", LOG_COLOR_WARN, buf, LOG_COLOR_RESET);
    } else if (a_thread->event->level == LIBLOG_LEVEL_ERROR) {
        sprintf(buffer, "%s%s%s", LOG_COLOR_ERROR, buf, LOG_COLOR_RESET);
    } else if (a_thread->event->level == LIBLOG_LEVEL_FATAL) {
        sprintf(buffer, "%s%s%s", LOG_COLOR_FATAL, buf, LOG_COLOR_RESET);
    } else {
        sprintf(buffer, "%s%s%s", LOG_COLOR_NORMAL, buf, LOG_COLOR_RESET);
    }

    str_len = strlen(buffer);
    if (write(STDOUT_FILENO, buffer, str_len) < 0) {
        logc_error("write fail, errno[%d]", errno);
        return -1;
    }
#else
    if (write(STDOUT_FILENO, liblog_buf_str(a_thread->msg_buf), liblog_buf_len(a_thread->msg_buf)) < 0) {
        logc_error("write fail, errno[%d]", errno);
        return -1;
    }
#endif

    return 0;
}

static int liblog_rule_output_stderr(struct liblog_rule *a_rule, struct liblog_thread *a_thread)
{
    if (liblog_format_gen_msg(a_rule->format, a_thread)) {
        logc_error("zlog_format_gen_msg fail");
        return -1;
    }

#if (COLOR_OUTPUT_TERMINAL_ENABLE == 1)
    size_t str_len = 0;
    char buf[LOG_MAXLEN_PATH] = {0};
    char buffer[LOG_MAXLEN_PATH * 2 + 1] = {0};

    memset(buf, 0x00, sizeof(buf));
    sprintf(buf, "%s", liblog_buf_str(a_thread->msg_buf));
    buf[liblog_buf_len(a_thread->msg_buf)] = 0;
    
    if (a_thread->event->level == LIBLOG_LEVEL_DEBUG) {
        sprintf(buffer, "%s%s%s", LOG_COLOR_DEBUG, buf, LOG_COLOR_RESET);
    } else if (a_thread->event->level == LIBLOG_LEVEL_INFO) {
        sprintf(buffer, "%s%s%s", LOG_COLOR_INFO, buf, LOG_COLOR_RESET);
    } else if (a_thread->event->level == LIBLOG_LEVEL_NOTICE) {
        sprintf(buffer, "%s%s%s", LOG_COLOR_NOTICE, buf, LOG_COLOR_RESET);
    } else if (a_thread->event->level == LIBLOG_LEVEL_WARN) {
        sprintf(buffer, "%s%s%s", LOG_COLOR_WARN, buf, LOG_COLOR_RESET);
    } else if (a_thread->event->level == LIBLOG_LEVEL_ERROR) {
        sprintf(buffer, "%s%s%s", LOG_COLOR_ERROR, buf, LOG_COLOR_RESET);
    } else if (a_thread->event->level == LIBLOG_LEVEL_FATAL) {
        sprintf(buffer, "%s%s%s", LOG_COLOR_FATAL, buf, LOG_COLOR_RESET);
    } else {
        sprintf(buffer, "%s%s%s", LOG_COLOR_NORMAL, buf, LOG_COLOR_RESET);
    }

    str_len = strlen(buffer);
    if (write(STDERR_FILENO, buffer, str_len) < 0) {
        logc_error("write fail, errno[%d]", errno);
        return -1;
    }
#else
    if (write(STDERR_FILENO, liblog_buf_str(a_thread->msg_buf), liblog_buf_len(a_thread->msg_buf)) < 0) {
        logc_error("write fail, errno[%d]", errno);
        return -1;
    }
#endif

    return 0;
}

static int syslog_facility_atoi(char *facility)
{
    logc_assert(facility, -187);

    if (LOG_STRICMP(facility, ==, "LOG_LOCAL0")) {
        return LOG_LOCAL0;
    }

    if (LOG_STRICMP(facility, ==, "LOG_LOCAL1")) {
        return LOG_LOCAL1;
    }

    if (LOG_STRICMP(facility, ==, "LOG_LOCAL2")) {
        return LOG_LOCAL2;
    }

    if (LOG_STRICMP(facility, ==, "LOG_LOCAL3")) {
        return LOG_LOCAL3;
    }

    if (LOG_STRICMP(facility, ==, "LOG_LOCAL4")) {
        return LOG_LOCAL4;
    }

    if (LOG_STRICMP(facility, ==, "LOG_LOCAL5")) {
        return LOG_LOCAL5;
    }

    if (LOG_STRICMP(facility, ==, "LOG_LOCAL6")) {
        return LOG_LOCAL6;
    }

    if (LOG_STRICMP(facility, ==, "LOG_LOCAL7")) {
        return LOG_LOCAL7;
    }

    if (LOG_STRICMP(facility, ==, "LOG_USER")) {
        return LOG_USER;
    }

    if (LOG_STRICMP(facility, ==, "LOG_AUTHPRIV")) {
        return LOG_AUTHPRIV;
    }

    if (LOG_STRICMP(facility, ==, "LOG_CRON")) {
        return LOG_CRON;
    }

    if (LOG_STRICMP(facility, ==, "LOG_DAEMON")) {
        return LOG_DAEMON;
    }

    if (LOG_STRICMP(facility, ==, "LOG_FTP")) {
        return LOG_FTP;
    }

    if (LOG_STRICMP(facility, ==, "LOG_KERN")) {
        return LOG_KERN;
    }

    if (LOG_STRICMP(facility, ==, "LOG_LPR")) {
        return LOG_LPR;
    }

    if (LOG_STRICMP(facility, ==, "LOG_MAIL")) {
        return LOG_MAIL;
    }

    if (LOG_STRICMP(facility, ==, "LOG_NEWS")) {
        return LOG_NEWS;
    }

    if (LOG_STRICMP(facility, ==, "LOG_SYSLOG")) {
        return LOG_SYSLOG;
    }

    return LOG_AUTHPRIV;

    logc_error("wrong syslog facility[%s], must in LOG_LOCAL[0-7] or LOG_USER", facility);
    return -187;
}

static int liblog_rule_parse_path(char *path_start, char *path_str, size_t path_size, struct logc_arraylist **path_specs, int *time_cache_count)
{
    size_t len;
    char *p, *q;
    struct liblog_spec *a_spec;
    struct logc_arraylist *specs;

    p = path_start + 1;

    q = strrchr(p, '"');
    if (!q) {
        logc_error("matching \" not found in conf line[%s]", path_start);
        return -1;
    }

    len = q - p;
    if (len > (path_size - 1)) {
        logc_error("file_path too long %ld > %ld", len, path_size - 1);
        return -1;
    }

    memcpy(path_str, p, len);

    if (logc_str_replace_env(path_str, path_size)) {
        logc_error("libc_str_replace_env fail");
        return -1;
    }

    if (strchr(path_str, '%') == NULL) {
        return 0;
    }

    specs = logc_arraylist_new((arraylist_del_func)liblog_spec_del);
    if (!path_specs) {
        logc_error("libc_arraylist_new fail");
        return -1;
    }

    for (p = path_str; *p != '\0'; p = q) {
        a_spec = liblog_spec_new(p, &q, time_cache_count);
        if (!a_spec) {
            logc_error("liblog_spec_new fail");
            goto err;
        }

        if (logc_arraylist_add(specs, a_spec)) {
            logc_error("libc_arraylist_add fail");
            goto err;
        }
    }

    *path_specs = specs;
    return 0;

err:
    if (specs) {
        logc_arraylist_del(specs);
    }

    if (a_spec) {
        liblog_spec_del(a_spec);
    }

    return -1;
}

struct liblog_rule *liblog_rule_new(char *line, struct logc_arraylist *levels, struct liblog_format *default_format, struct logc_arraylist *formats, unsigned int file_perms, size_t fsync_period, int *time_cache_count)
{
    int rc = 0;
    size_t len;
    char *p, *q;
    char *action;
    char *file_limit;
    int nscan = 0, nread = 0;
    struct liblog_rule *a_rule;
    char level[LOG_MAXLEN_CFG_LINE + 1];
    char output[LOG_MAXLEN_CFG_LINE + 1];
    char selector[LOG_MAXLEN_CFG_LINE + 1];
    char category[LOG_MAXLEN_CFG_LINE + 1];
    char file_path[LOG_MAXLEN_CFG_LINE + 1];
    char format_name[LOG_MAXLEN_CFG_LINE + 1];
    char archive_max_size[LOG_MAXLEN_CFG_LINE + 1];

    logc_assert(line, NULL);
    logc_assert(default_format, NULL);
    logc_assert(formats, NULL);

    a_rule = (struct liblog_rule *)calloc(1, sizeof(struct liblog_rule));
    if (!a_rule) {
        logc_error("calloc fail, errno[%d]", errno);
        return NULL;
    }

    a_rule->file_perms = file_perms;
    a_rule->fsync_period = fsync_period;

    memset(&selector, 0x00, sizeof(selector));
    nscan = sscanf(line, "%s %n", selector, &nread);
    if (nscan != 1) {
        logc_error("sscanf [%s] fail, selector", line);
        goto err;
    }
    action = line + nread;

    memset(level, 0x00, sizeof(level));
    memset(category, 0x00, sizeof(category));

    nscan = sscanf(selector, " %[^.].%s", category, level);
    if (nscan != 2) {
        logc_error("sscanf [%s] fail, category or level is null", selector);
        goto err;
    }

    for (p = category; *p != '\0'; p++) {
        if ((!isalnum(*p)) && (*p != '_') && (*p != '-') && (*p != '*') && (*p != '!')) {
            logc_error("category name[%s] character is not in [a-Z][0-9][_!*-]", category);
            goto err;
        }
    }

    strcpy(a_rule->category, category);

    switch (level[0]) {
        case '=':
            a_rule->compare_char = '=';
            p = level + 1;
            break;

        case '!':
            a_rule->compare_char = '!';
            p = level + 1;
            break;

        case '*':
            a_rule->compare_char = '*';
            p = level;
            break;

        default:
            a_rule->compare_char = '.';
            p = level;
            break;
    }

    switch (a_rule->compare_char) {
        case '=':
            memset(a_rule->level_bitmap, 0x00, sizeof(a_rule->level_bitmap));
            a_rule->level_bitmap[a_rule->level / 8] |= (1 << (7 - a_rule->level % 8));
            break;

        case '!':
            memset(a_rule->level_bitmap, 0xFF, sizeof(a_rule->level_bitmap));
            a_rule->level_bitmap[a_rule->level / 8] &= ~(1 << (7 - a_rule->level % 8));
            break;

        case '*':
            memset(a_rule->level_bitmap, 0xFF, sizeof(a_rule->level_bitmap));
            break;

        case '.':
            memset(a_rule->level_bitmap, 0x00, sizeof(a_rule->level_bitmap));
            a_rule->level_bitmap[a_rule->level / 8] |= ~(0xFF << (8 - a_rule->level % 8));
            memset(a_rule->level_bitmap + a_rule->level / 8 + 1, 0xFF, sizeof(a_rule->level_bitmap) -  a_rule->level / 8 - 1);
            break;
    }

    a_rule->level = liblog_level_list_atoi(levels, p);

    memset(output, 0x00, sizeof(output));
    memset(format_name, 0x00, sizeof(format_name));

    nscan = sscanf(action, " %[^;];%s", output, format_name);
    if (nscan < 1) {
        logc_error("sscanf [%s] fail", action);
        goto err;
    }

    if (LOG_STRCMP(format_name, ==, "")) {
        logc_debug("no format specified, use default");
        a_rule->format = default_format;
    } else {
        int i;
        int find_flag = 0;
        struct liblog_format *a_format;

        logc_arraylist_foreach(formats, i, a_format) {
            if (liblog_format_has_name(a_format, format_name)) {
                a_rule->format = a_format;
                find_flag = 1;
                break;
            }
        }

        if (!find_flag) {
            logc_error("in conf file can't find format[%s], pls check", format_name);
            goto err;
        }
    }

    memset(file_path, 0x00, sizeof(file_path));
    nscan = sscanf(output, " %[^,],", file_path);
    if (nscan < 1) {
        logc_error("sscanf [%s] fail", action);
        goto err;
    }

    file_limit = strchr(output, ',');
    if (file_limit) {
        file_limit++;
        while (isspace(*file_limit) ) {
            file_limit++;
        }
    }

    p = NULL;
    switch (file_path[0]) {
        case '-' :
            if (file_path[1] != '"') {
                logc_error(" - must set before a file output");
                goto err;
            }

            a_rule->fsync_period = 0;
            p = file_path + 1;
            a_rule->file_open_flags = O_SYNC;

        case '"' :
            if (!p) {
                p = file_path;
            }

            rc = liblog_rule_parse_path(p, a_rule->file_path, sizeof(a_rule->file_path), &(a_rule->dynamic_specs), time_cache_count);
            if (rc) {
                logc_error("zlog_rule_parse_path fail");
                goto err;
            }

            if (file_limit) {
                memset(archive_max_size, 0x00, sizeof(archive_max_size));
                nscan = sscanf(file_limit, " %[0-9GgMmKkBb] * %d ~", archive_max_size, &(a_rule->archive_max_count));
                if (nscan) {
                    a_rule->archive_max_size = logc_parse_byte_size(archive_max_size);
                }

                p = strchr(file_limit, '"');
                if (p) {
                    rc = liblog_rule_parse_path(p, a_rule->archive_path, sizeof(a_rule->file_path), &(a_rule->archive_specs), time_cache_count);
                    if (rc) {
                        logc_error("liblog_rule_parse_path fail");
                        goto err;
                    }

                    p = strchr(a_rule->archive_path, '#');
                    if ((p == NULL) || ((strchr(p, 'r') == NULL) && (strchr(p, 's') == NULL))) {
                        logc_error("archive_path must contain #r or #s");
                        goto err;
                    }
                }
            }

            if (a_rule->dynamic_specs) {
                if (a_rule->archive_max_size <= 0) {
                    a_rule->output = liblog_rule_output_dynamic_file_single;
                } else {
                    a_rule->output = liblog_rule_output_dynamic_file_rotate;
                }
            } else {
                struct stat stb;

                if (a_rule->archive_max_size <= 0) {
                    a_rule->output = liblog_rule_output_static_file_single;
                } else {
                    a_rule->output = liblog_rule_output_static_file_rotate;
                }

                a_rule->static_fd = open(a_rule->file_path, O_WRONLY | O_APPEND | O_CREAT | a_rule->file_open_flags, a_rule->file_perms);
                if (a_rule->static_fd < 0) {
                    logc_error("open file[%s] fail, errno[%d]", a_rule->file_path, errno);
                    goto err;
                }

                if (fstat(a_rule->static_fd, &stb)) {
                    logc_error("fstat [%s] fail, errno[%d], failing to open static_fd", a_rule->file_path, errno);
                    goto err;
                }

                if (a_rule->archive_max_size > 0) {
                    close(a_rule->static_fd);
                    a_rule->static_fd = -1;
                }

                a_rule->static_dev = stb.st_dev;
                a_rule->static_ino = stb.st_ino;
            }
            break;

        case '|' :
            a_rule->pipe_fp = popen(output + 1, "w");
            if (!a_rule->pipe_fp) {
                logc_error("popen fail, errno[%d]", errno);
                goto err;
            }

            a_rule->pipe_fd = fileno(a_rule->pipe_fp);
            if (a_rule->pipe_fd < 0 ) {
                logc_error("fileno fail, errno[%d]", errno);
                goto err;
            }
            a_rule->output = liblog_rule_output_pipe;
            break;

        case '>' :
            if (LOG_STRNCMP(file_path + 1, ==, "syslog", 6)) {
                a_rule->syslog_facility = syslog_facility_atoi(file_limit);
                if (a_rule->syslog_facility == -187) {
                    logc_error("-187 get");
                    goto err;
                }
                
                a_rule->output = liblog_rule_output_syslog;
                openlog(NULL, LOG_NDELAY | LOG_NOWAIT | LOG_PID, LOG_USER);
            } else if (LOG_STRNCMP(file_path + 1, ==, "stdout", 6)) {
                a_rule->output = liblog_rule_output_stdout;
            } else if (LOG_STRNCMP(file_path + 1, ==, "stderr", 6)) {
                a_rule->output = liblog_rule_output_stderr;
            } else {
                logc_error("[%s]the string after is not syslog, stdout or stderr", output);
                goto err;
            }
            break;

        case '$' :
            sscanf(file_path + 1, "%s", a_rule->record_name);
            if (file_limit) {
                p = strchr(file_limit, '"');
                if (!p) {
                    logc_error("record_path not start with \", [%s]", file_limit);
                    goto err;
                }
                p++;

                q = strrchr(p, '"');
                if (!q) {
                    logc_error("matching \" not found in conf line[%s]", p);
                    goto err;
                }

                len = q - p;
                if (len > (sizeof(a_rule->record_path) - 1)) {
                    logc_error("record_path too long %ld > %ld", len, sizeof(a_rule->record_path) - 1);
                    goto err;
                }

                memcpy(a_rule->record_path, p, len);
            }

            rc = logc_str_replace_env(a_rule->record_path, sizeof(a_rule->record_path));
            if (rc) {
                logc_error("logc_str_replace_env fail");
                goto err;
            }

            if (strchr(a_rule->record_path, '%') == NULL) {
                a_rule->output = liblog_rule_output_static_record;
            } else {
                struct liblog_spec *a_spec;

                a_rule->output = liblog_rule_output_dynamic_record;

                a_rule->dynamic_specs = logc_arraylist_new((arraylist_del_func)liblog_spec_del);
                if (!(a_rule->dynamic_specs)) {
                    logc_error("logc_arraylist_new fail");
                    goto err;
                }
                
                for (p = a_rule->record_path; *p != '\0'; p = q) {
                    a_spec = liblog_spec_new(p, &q, time_cache_count);
                    if (!a_spec) {
                        logc_error("liblog_spec_new fail");
                        goto err;
                    }

                    rc = logc_arraylist_add(a_rule->dynamic_specs, a_spec);
                    if (rc) {
                        liblog_spec_del(a_spec);
                        logc_error("logc_arraylist_add fail");
                        goto err;
                    }
                }
            }
            break;

        default :
            logc_error("the 1st char[%c] of file_path[%s] is wrong", file_path[0], file_path);
            goto err;
    }

    return a_rule;

err:
    liblog_rule_del(a_rule);
    return NULL;
}

void liblog_rule_del(struct liblog_rule *a_rule)
{
    logc_assert(a_rule, );

    if (a_rule->dynamic_specs) {
        logc_arraylist_del(a_rule->dynamic_specs);
        a_rule->dynamic_specs = NULL;
    }

    if (a_rule->static_fd > 0) {
        if (close(a_rule->static_fd)) {
            logc_error("close fail, maybe cause by write, errno[%d]", errno);
        }
    }

    if (a_rule->pipe_fp) {
        if (pclose(a_rule->pipe_fp) == -1) {
            logc_error("pclose fail, errno[%d]", errno);
        }
    }

    if (a_rule->archive_specs) {
        logc_arraylist_del(a_rule->archive_specs);
        a_rule->archive_specs = NULL;
    }

    logc_debug("liblog_rule_del[%p]", a_rule);
    free(a_rule);
}

int liblog_rule_output(struct liblog_rule *a_rule, struct liblog_thread *a_thread)
{
    switch (a_rule->compare_char) {
        case '*' :
            return a_rule->output(a_rule, a_thread);
            break;

        case '.' :
            if (a_thread->event->level >= a_rule->level) {
                return a_rule->output(a_rule, a_thread);
            } else {
                return 0;
            }
            break;

        case '=' :
            if (a_thread->event->level == a_rule->level) {
                return a_rule->output(a_rule, a_thread);
            } else {
                return 0;
            }
            break;

        case '!' :
            if (a_thread->event->level != a_rule->level) {
                return a_rule->output(a_rule, a_thread);
            } else {
                return 0;
            }
            break;
    }

    return 0;
}

int liblog_rule_is_wastebin(struct liblog_rule *a_rule)
{
    logc_assert(a_rule, -1);

    if (LOG_STRCMP(a_rule->category, ==, "!")) {
        return 1;
    }

    return 0;
}

int liblog_rule_match_category(struct liblog_rule *a_rule, char *category)
{
    logc_assert(a_rule, -1);
    logc_assert(category, -1);

    if (LOG_STRCMP(a_rule->category, ==, "*")) {
        return 1;
    } else if (LOG_STRCMP(a_rule->category, ==, category)) {
        return 1;
    } else {
        size_t len;
        len = strlen(a_rule->category);

        if (a_rule->category[len - 1] == '_') {
            if (strlen(category) == len - 1) {
                len--;
            }

            if (LOG_STRNCMP(a_rule->category, ==, category, len)) {
                return 1;
            }
        }
    }

    return 0;
}

int liblog_rule_set_record(struct liblog_rule *a_rule, struct logc_hashtable *records)
{
    struct liblog_record *a_record;

    if ((a_rule->output != liblog_rule_output_static_record) && (a_rule->output != liblog_rule_output_dynamic_record)) {
        return 0;
    }

    a_record = (struct liblog_record *)logc_hashtable_get(records, a_rule->record_name);
    if (a_record) {
        a_rule->record_func = a_record->output;
    }

    return 0;
}
