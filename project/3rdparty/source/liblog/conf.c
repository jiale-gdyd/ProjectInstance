#include <time.h>
#include <errno.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "rule.h"
#include "conf.h"
#include "format.h"
#include "fmacros.h"
#include "rotater.h"
#include "logc_defs.h"
#include "level_list.h"

#define STATS_FILE                                  lstat

#define LIBLOG_CONF_DEFAULT_FORMAT                  "default = \"%D %V [%p:%F:%L] %m%n\""
#define LIBLOG_CONF_DEFAULT_RULE                    "*.*        >stdout"
#define LIBLOG_CONF_DEFAULT_BUF_SIZE_MIN            (1024)
#define LIBLOG_CONF_DEFAULT_BUF_SIZE_MAX            (2 * 1024 * 1024)
#define LIBLOG_CONF_DEFAULT_FILE_PERMS              (0600)
#define LIBLOG_CONF_DEFAULT_RELOAD_CONF_PERIOD      (0)
#define LIBLOG_CONF_DEFAULT_FSYNC_PERIOD            (0)
#define LIBLOG_CONF_BACKUP_ROTATE_LOCK_FILE         "liblog.lock"

enum {
    NO_CFG,
    FILE_CFG,
    IN_MEMORY_CFG
};

static int liblog_conf_build_with_string(struct liblog_conf *a_conf, const char *conf_string);

void liblog_conf_profile(struct liblog_conf *a_conf, int flag)
{
    int i;
    struct liblog_rule *a_rule;
    struct liblog_format *a_format;

    logc_assert(a_conf, );

    logc_profile(flag, "-conf[%p]-", a_conf);
    logc_profile(flag, "--global--");
    logc_profile(flag, "---file[%s],mtime[%s]---", a_conf->file, a_conf->mtime);
    logc_profile(flag, "---in-memory conf[%s]---", a_conf->cfg_ptr);
    logc_profile(flag, "---strict init[%d]---", a_conf->strict_init);
    logc_profile(flag, "---buffer min[%ld]---", a_conf->buf_size_min);
    logc_profile(flag, "---buffer max[%ld]---", a_conf->buf_size_max);

    if (a_conf->default_format) {
        logc_profile(flag, "---default_format---");
        liblog_format_profile(a_conf->default_format, flag);
    }

    logc_profile(flag, "---file perms[0%o]---", a_conf->file_perms);
    logc_profile(flag, "---reload conf period[%ld]---", a_conf->reload_conf_period);
    logc_profile(flag, "---fsync period[%ld]---", a_conf->fsync_period);

    logc_profile(flag, "---rotate lock file[%s]---", a_conf->rotate_lock_file);
    if (a_conf->rotater) {
        liblog_rotater_profile(a_conf->rotater, flag);
    }

    if (a_conf->levels) {
        liblog_level_list_profile(a_conf->levels, flag);
    }

    if (a_conf->formats) {
        logc_profile(flag, "--format list[%p]--", a_conf->formats);
        logc_arraylist_foreach(a_conf->formats, i, a_format) {
            liblog_format_profile((struct liblog_format *)a_format, flag);
        }
    }

    if (a_conf->rules) {
        logc_profile(flag, "--rule_list[%p]--", a_conf->rules);
        logc_arraylist_foreach(a_conf->rules, i, a_rule) {
            liblog_rule_profile((struct liblog_rule *)a_rule, flag);
        }
    }
}

void liblog_conf_del(struct liblog_conf *a_conf)
{
    logc_assert(a_conf, );

    if (a_conf->rotater) {
        liblog_rotater_del(a_conf->rotater);
    }

    if (a_conf->levels) {
        liblog_level_list_del(a_conf->levels);
    }

    if (a_conf->default_format) {
        liblog_format_del(a_conf->default_format);
    }

    if (a_conf->formats) {
        logc_arraylist_del(a_conf->formats);
    }

    if (a_conf->rules) {
        logc_arraylist_del(a_conf->rules);
    }

    free(a_conf);
}

static int liblog_conf_parse_line(struct liblog_conf * a_conf, char *line, int *section)
{
    int nscan, nread;
    struct liblog_rule *a_rule = NULL;
    char name[LOG_MAXLEN_CFG_LINE + 1];
    char value[LOG_MAXLEN_CFG_LINE + 1];
    char word_1[LOG_MAXLEN_CFG_LINE + 1];
    char word_2[LOG_MAXLEN_CFG_LINE + 1];
    char word_3[LOG_MAXLEN_CFG_LINE + 1];
    struct liblog_format *a_format = NULL;

    if (strlen(line) > LOG_MAXLEN_CFG_LINE) {
        logc_error("line_len[%ld] > LOG_MAXLEN_CFG_LINE[%ld], may cause overflow", strlen(line), LOG_MAXLEN_CFG_LINE);
        return -1;
    }

    if (line[0] == '[') {
        int last_section = *section;
        nscan = sscanf(line, "[ %[^] \t]", name);
        if (LOG_STRCMP(name, ==, "global")) {
            *section = 1;
        } else if (LOG_STRCMP(name, ==, "levels")) {
            *section = 2;
        } else if (LOG_STRCMP(name, ==, "formats")) {
            *section = 3;
        } else if (LOG_STRCMP(name, ==, "rules")) {
            *section = 4;
        } else {
            logc_error("wrong section name[%s]", name);
            return -1;
        }

        if (last_section >= *section) {
            logc_error("wrong sequence of section, must follow global->levels->formats->rules");
            return -1;
        }

        if (*section == 4) {
            if ((a_conf->reload_conf_period != 0) && (a_conf->fsync_period >= a_conf->reload_conf_period)) {
                logc_warn("fsync_period[%ld] >= reload_conf_period[%ld], set fsync_period to zero");
                a_conf->fsync_period = 0;
            }

            a_conf->rotater = liblog_rotater_new(a_conf->rotate_lock_file);
            if (!a_conf->rotater) {
                logc_error("liblog_rotater_new fail");
                return -1;
            }

            a_conf->default_format = liblog_format_new(a_conf->default_format_line, &(a_conf->time_cache_count));
            if (!a_conf->default_format) {
                logc_error("liblog_format_new fail");
                return -1;
            }
        }

        return 0;
    }

    switch (*section) {
        case 1:
            memset(name, 0x00, sizeof(name));
            memset(value, 0x00, sizeof(value));
            nscan = sscanf(line, " %[^=]= %s ", name, value);
            if (nscan != 2) {
                logc_error("sscanf [%s] fail, name or value is null", line);
                return -1;
            }

            memset(word_1, 0x00, sizeof(word_1));
            memset(word_2, 0x00, sizeof(word_2));
            memset(word_3, 0x00, sizeof(word_3));
            nread = 0;
            nscan = sscanf(name, "%s%n%s%s", word_1, &nread, word_2, word_3);

            if (LOG_STRCMP(word_1, ==, "strict") && LOG_STRCMP(word_2, ==, "init")) {
                if (LOG_STRICMP(value, ==, "false") && !getenv("LOG_STRICT_INIT")) {
                    a_conf->strict_init = 0;
                } else {
                    a_conf->strict_init = 1;
                }
            } else if (LOG_STRCMP(word_1, ==, "log") && LOG_STRCMP(word_2, ==, "level")) {
                strcpy(a_conf->log_level, value);
            }  else if (LOG_STRCMP(word_1, ==, "buffer") && LOG_STRCMP(word_2, ==, "min")) {
                a_conf->buf_size_min = logc_parse_byte_size(value);
            } else if (LOG_STRCMP(word_1, ==, "buffer") && LOG_STRCMP(word_2, ==, "max")) {
                a_conf->buf_size_max = logc_parse_byte_size(value);
            } else if (LOG_STRCMP(word_1, ==, "file") && LOG_STRCMP(word_2, ==, "perms")) {
                sscanf(value, "%o", &(a_conf->file_perms));
            } else if (LOG_STRCMP(word_1, ==, "rotate") && LOG_STRCMP(word_2, ==, "lock") && LOG_STRCMP(word_3, ==, "file")) {
                if (LOG_STRCMP(value, ==, "self")) {
                    strcpy(a_conf->rotate_lock_file, a_conf->file);
                } else {
                    strcpy(a_conf->rotate_lock_file, value);
                }
            } else if (LOG_STRCMP(word_1, ==, "default") && LOG_STRCMP(word_2, ==, "format")) {
                strcpy(a_conf->default_format_line, line + nread);
            } else if (LOG_STRCMP(word_1, ==, "reload") && LOG_STRCMP(word_2, ==, "conf") && LOG_STRCMP(word_3, ==, "period")) {
                a_conf->reload_conf_period = logc_parse_byte_size(value);
            } else if (LOG_STRCMP(word_1, ==, "fsync") && LOG_STRCMP(word_2, ==, "period")) {
                a_conf->fsync_period = logc_parse_byte_size(value);
            } else {
                logc_error("name[%s] is not any one of global options", name);
                if (a_conf->strict_init) {
                    return -1;
                }
            }
            break;

        case 2:
            if (liblog_level_list_set(a_conf->levels, line)) {
                logc_error("liblog_level_list_set fail");
                if (a_conf->strict_init) {
                    return -1;
                }
            }
            break;

        case 3:
            a_format = liblog_format_new(line, &(a_conf->time_cache_count));
            if (!a_format) {
                logc_error("liblog_format_new fail [%s]", line);
                if (a_conf->strict_init) {
                    return -1;
                } else {
                    break;
                }
            }

            if (logc_arraylist_add(a_conf->formats, a_format)) {
                liblog_format_del(a_format);
                logc_error("logc_arraylist_add fail");
                return -1;
            }
            break;

        case 4:
            a_rule = liblog_rule_new(line, a_conf->levels, a_conf->default_format, a_conf->formats, a_conf->file_perms, a_conf->fsync_period, &(a_conf->time_cache_count));
            if (!a_rule) {
                logc_error("liblog_rule_new fail [%s]", line);
                if (a_conf->strict_init) {
                    return -1;
                } else {
                    break;
                }
            }
            if (logc_arraylist_add(a_conf->rules, a_rule)) {
                liblog_rule_del(a_rule);
                logc_error("logc_arraylist_add fail");
                return -1;
            }
            break;

        default:
            logc_error("not in any section");
            return -1;
    }

    return 0;
}

char *sgets(char *s, int size, char **string)
{
    if (*string == NULL) {
        return NULL;
    }

    char *nlp = strchr(*string, '\n');
    char *fstring = *string;

    if (nlp == NULL) {
        if (strlen(fstring) > 0) {
            nlp = fstring + strlen(fstring);
        } else {
            return NULL;
        }
    }

    int ss =  (int)(nlp + 1 -  fstring);
    if (size > ss) {
        size = ss;
    }

    memcpy(s, *string, size);
    s[size] = 0;
    if (strlen(*string) == strlen(s)) {
        *string = NULL;
    } else {
        *string += size;
    }

    return s;
}

static int liblog_conf_build_with_string(struct liblog_conf *a_conf, const char *conf_string)
{
    int i = 0;
    int rc = 0;
    char *p = NULL;
    int section = 0;
    size_t line_len;
    int line_no = 0;
    char *pline = NULL;
    int in_quotation = 0;
    char line[LOG_MAXLEN_CFG_LINE + 1];
    char *conf_string_l = (char *)conf_string;

    if (a_conf == NULL) {
        return -1;
    }

    pline = line;
    memset(&line, 0x00, sizeof(line));

    while (sgets(pline, LOG_MAXLEN_CFG_LINE, &conf_string_l) != NULL) {
        ++line_no;
        line_len = strlen(pline);
        if (pline[line_len - 1] == '\n') {
            pline[line_len - 1] = '\0';
        }

        p = pline;
        while (*p && isspace((int)*p)) {
            ++p;
        }

        if ((*p == '\0') || (*p == '#')) {
            continue;
        }

        for (i = 0; p[i] != '\0'; ++i) {
            pline[i] = p[i];
        }
        pline[i] = '\0';

        for (p = pline + strlen(pline) - 1; isspace((int)*p); --p);

        if (*p == '\\') {
            if ((p - line) > LOG_MAXLEN_CFG_LINE - 30) {
                pline = line;
            } else {
                for (p--; isspace((int)*p); --p);
                p++;
                *p = 0;
                pline = p;
                continue;
            }
        } else {
            memmove(line, pline, strlen(pline)+1);
        }

        *++p = '\0';

        in_quotation = 0;
        for (p = line; *p != '\0'; p++) {
            if (*p == '"') {
                in_quotation ^= 1;
                continue;
            }

            if ((*p == '#') && !in_quotation) {
                *p = '\0';
                break;
            }
        }

        rc = liblog_conf_parse_line(a_conf, line, &section);
        if (rc < 0) {
            logc_error("parse configure file:[%s], line_no:[%ld] fail", a_conf->file, line_no);
            logc_error("line[%s]", line);
            goto exit;
        } else if (rc > 0) {
            logc_warn("parse configure file:[%s], line_no:[%ld] fail", a_conf->file, line_no);
            logc_warn("line:[%s]", line);
            logc_warn("as strict init is set to false, ignore and go on");
            rc = 0;
            continue;
        }
    }

exit:
    return rc;
}

static int liblog_conf_build_with_file(struct liblog_conf *a_conf)
{
    size_t line_len;
    FILE *fp = NULL;
    int i = 0, rc = 0;
    struct tm local_time;
    struct log_stat a_stat;
    char *pline = NULL, *p = NULL;
    char line[LOG_MAXLEN_CFG_LINE + 1];
    int line_no = 0, in_quotation = 0, section = 0;

    if (STATS_FILE(a_conf->file, &a_stat)) {
        logc_error("lstat conf file[%s] fail, errno[%d]", a_conf->file, errno);
        return -1;
    }

    localtime_r(&(a_stat.st_mtime), &local_time);
    strftime(a_conf->mtime, sizeof(a_conf->mtime), "%Y-%m-%d %H:%M:%S", &local_time);

    if ((fp = fopen(a_conf->file, "r")) == NULL) {
        logc_error("open configure file[%s] fail", a_conf->file);
        return -1;
    }

    a_conf->log_level[0] = '\0';

    pline = line;
    memset(&line, 0x00, sizeof(line));
    while (fgets((char *)pline, sizeof(line) - (pline - line), fp) != NULL) {
        ++line_no;
        line_len = strlen(pline);
        if (line_len == 0) {
            continue;
        }

        if (pline[line_len - 1] == '\n') {
            pline[line_len - 1] = '\0';
        }

        p = pline;
        while (*p && isspace((int)*p)) {
            ++p;
        }

        if ((*p == '\0') || (*p == '#')) {
            continue;
        }

        for (i = 0; p[i] != '\0'; ++i) {
            pline[i] = p[i];
        }
        pline[i] = '\0';

        for (p = pline + strlen(pline) - 1; isspace((int)*p); --p);

        if (*p == '\\') {
            if ((p - line) > (LOG_MAXLEN_CFG_LINE - 30)) {
                pline = line;
            } else {
                for (p--; (p >= line) && isspace((int)*p); --p);
                p++;
                *p = 0;
                pline = p;
                continue;
            }
        } else {
            pline = line;
        }
        *++p = '\0';

        in_quotation = 0;
        for (p = line; *p != '\0'; p++) {
            if (*p == '"') {
                in_quotation ^= 1;
                continue;
            }

            if ((*p == '#') && !in_quotation) {
                *p = '\0';
                break;
            }
        }

        rc = liblog_conf_parse_line(a_conf, line, &section);
        if (rc < 0) {
            logc_error("parse configure file[%s]line_no[%ld] fail", a_conf->file, line_no);
            logc_error("line[%s]", line);
            goto exit;
        } else if (rc > 0) {
            logc_warn("parse configure file[%s]line_no[%ld] fail", a_conf->file, line_no);
            logc_warn("line[%s]", line);
            logc_warn("as strict init is set to false, ignore and go on");
            rc = 0;
            continue;
        }
    }

    a_conf->level = liblog_level_list_atoi(a_conf->levels, a_conf->log_level);

exit:
    fclose(fp);
    return rc;
}

struct liblog_conf *liblog_conf_new_from_string(const char *config_string)
{
    struct liblog_conf *a_conf = NULL;

    a_conf = calloc(1, sizeof(struct liblog_conf));
    if (!a_conf) {
        logc_error("calloc fail, errno[%d]", errno);
        return NULL;
    }

    memset(a_conf->file, 0x00, sizeof(a_conf->file));

    a_conf->levels = liblog_level_list_new();
    if (!a_conf->levels) {
        logc_error("liblog_level_list_new fail");
        goto err;
    }

    a_conf->formats = logc_arraylist_new((arraylist_del_func)liblog_format_del);
    if (!a_conf->formats) {
        logc_error("libc_arraylist_new fail");
        goto err;
    }

    a_conf->rules = logc_arraylist_new((arraylist_del_func)liblog_rule_del);
    if (!a_conf->rules) {
        logc_error("libc_arraylist_new fail");
        goto err;
    }

    strcpy(a_conf->rotate_lock_file, "liblog-rotate.lock");
    a_conf->strict_init = 1;
    a_conf->buf_size_min = LIBLOG_CONF_DEFAULT_BUF_SIZE_MIN;
    a_conf->buf_size_max = LIBLOG_CONF_DEFAULT_BUF_SIZE_MAX;
    strcpy(a_conf->default_format_line, LIBLOG_CONF_DEFAULT_FORMAT);
    a_conf->file_perms = LIBLOG_CONF_DEFAULT_FILE_PERMS;
    a_conf->reload_conf_period = LIBLOG_CONF_DEFAULT_RELOAD_CONF_PERIOD;
    a_conf->fsync_period = LIBLOG_CONF_DEFAULT_FSYNC_PERIOD;

    a_conf->default_format = liblog_format_new(a_conf->default_format_line, &(a_conf->time_cache_count));
    if (!a_conf->default_format) {
        logc_error("liblog_format_new fail");
        goto err;
    }

    a_conf->rotater = liblog_rotater_new(a_conf->rotate_lock_file);
    if (!a_conf->rotater) {
        logc_error("liblog_rotater_new fail");
        goto err;
    }

    if (liblog_conf_build_with_string(a_conf, config_string)) {
        logc_error("liblog_conf_build_with_string fail");
        goto err;
    }

    liblog_conf_profile(a_conf, LOGC_DEBUG);
    return a_conf;

err:
    liblog_conf_del(a_conf);
    return NULL;
}

static int liblog_conf_build_without_file(struct liblog_conf *a_conf)
{
    struct liblog_rule *default_rule;

    a_conf->default_format = liblog_format_new(a_conf->default_format_line, &(a_conf->time_cache_count));
    if (!a_conf->default_format) {
        logc_error("liblog_format_new fail");
        return -1;
    }

    a_conf->rotater = liblog_rotater_new(a_conf->rotate_lock_file);
    if (!a_conf->rotater) {
        logc_error("liblog_rotater_new fail");
        return -1;
    }

    default_rule = liblog_rule_new(LIBLOG_CONF_DEFAULT_RULE, a_conf->levels, a_conf->default_format, a_conf->formats, a_conf->file_perms, a_conf->fsync_period, &(a_conf->time_cache_count));
    if (!default_rule) {
        logc_error("zlog_rule_new fail");
        return -1;
    }

    if (logc_arraylist_add(a_conf->rules, default_rule)) {
        liblog_rule_del(default_rule);
        logc_error("logc_arraylist_add fail");

        return -1;
    }

    return 0;
}

static int liblog_conf_build_in_memory(struct liblog_conf *a_conf)
{
    int rc = 0;
    int section = 0;
    char *pline = NULL;
    char line[LOG_MAXLEN_CFG_LINE + 1];

    pline = line;
    memset(line, 0x00, sizeof(line));
    pline = strtok((char *)a_conf->cfg_ptr, "\n");

    while (pline != NULL) {
        rc = liblog_conf_parse_line(a_conf, pline, &section);
        if (rc < 0) {
            logc_error("parse in-memory configurations[%s] line [%s] fail", a_conf->cfg_ptr, pline);
            break;
        } else if (rc > 0) {
            logc_error("parse in-memory configurations[%s] line [%s] fail", a_conf->cfg_ptr, pline);
            logc_warn("as strict init is set to false, ignore and go on");

            rc = 0;
            continue;
        }

        pline = strtok(NULL, "\n");
    }

    return rc;
}

struct liblog_conf *liblog_conf_new(const char *confpath)
{
    int nwrite = 0;
    int cfg_source = NO_CFG;
    struct liblog_conf *a_conf = NULL;

    a_conf = (struct liblog_conf *)calloc(1, sizeof(struct liblog_conf));
    if (!a_conf) {
        logc_error("calloc fail, errno[%d]", errno);
        return NULL;
    }

    if (confpath && (confpath[0] != '\0') && (confpath[0] != '[')) {
        nwrite = snprintf(a_conf->file, sizeof(a_conf->file), "%s", confpath);
        cfg_source = FILE_CFG;
    } else if (getenv("LOG_CONF_PATH") != NULL) {
        nwrite = snprintf(a_conf->file, sizeof(a_conf->file), "%s", getenv("LOG_CONF_PATH"));
        cfg_source = FILE_CFG;
    } else if (confpath && (confpath[0] == '[')) {
        memset(a_conf->file, 0x00, sizeof(a_conf->file));
        nwrite = snprintf(a_conf->cfg_ptr, sizeof(a_conf->cfg_ptr), "%s", confpath);
        cfg_source = IN_MEMORY_CFG;
        if ((nwrite < 0) || (nwrite >= (int)sizeof(a_conf->file))) {
            logc_error("not enough space for configurations, nwrite=[%d], errno[%d]", nwrite, errno);
            goto err;
        }
    } else {
        memset(a_conf->file, 0x00, sizeof(a_conf->file));
        cfg_source = NO_CFG;
    }

    if (((nwrite < 0) || (nwrite >= (int)sizeof(a_conf->file))) && (cfg_source == FILE_CFG)) {
        logc_error("not enough space for path name, nwrite=[%d], errno[%d]", nwrite, errno);
        goto err;
    }

    a_conf->strict_init = 1;
    a_conf->buf_size_min = LIBLOG_CONF_DEFAULT_BUF_SIZE_MIN;
    a_conf->buf_size_max = LIBLOG_CONF_DEFAULT_BUF_SIZE_MAX;
    if (cfg_source == FILE_CFG) {
        strcpy(a_conf->rotate_lock_file, a_conf->file);
    } else {
        strcpy(a_conf->rotate_lock_file, LIBLOG_CONF_BACKUP_ROTATE_LOCK_FILE);
    }

    strcpy(a_conf->default_format_line, LIBLOG_CONF_DEFAULT_FORMAT);
    a_conf->file_perms = LIBLOG_CONF_DEFAULT_FILE_PERMS;
    a_conf->reload_conf_period = LIBLOG_CONF_DEFAULT_RELOAD_CONF_PERIOD;
    a_conf->fsync_period = LIBLOG_CONF_DEFAULT_FSYNC_PERIOD;

    a_conf->levels = liblog_level_list_new();
    if (!a_conf->levels) {
        logc_error("liblog_level_list_new fail");
        goto err;
    }

    a_conf->formats = logc_arraylist_new((arraylist_del_func)liblog_format_del);
    if (!a_conf->formats) {
        logc_error("logc_arraylist_new fail");
        goto err;
    }

    a_conf->rules = logc_arraylist_new((arraylist_del_func)liblog_rule_del);
    if (!a_conf->rules) {
        logc_error("init rule_list fail");
        goto err;
    }

    if (cfg_source == FILE_CFG) {
        if (liblog_conf_build_with_file(a_conf)) {
            logc_error("liblog_conf_build_with_file fail");
            goto err;
        }
    } else if (cfg_source == IN_MEMORY_CFG) {
        if (liblog_conf_build_in_memory(a_conf)) {
            logc_error("liblog_conf_build_in_memory fail");
            goto err;
        }
    } else {
        if (liblog_conf_build_without_file(a_conf)) {
            logc_error("liblog_conf_build_without_file fail");
            goto err;
        }
    }

    liblog_conf_profile(a_conf, LOGC_DEBUG);
    return a_conf;

err:
    liblog_conf_del(a_conf);
    return NULL;
}
