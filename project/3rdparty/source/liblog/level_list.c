#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <syslog.h>

#include "level.h"
#include "logc_defs.h"
#include "level_list.h"

void liblog_level_list_profile(struct logc_arraylist *levels, int flag)
{
    int i;
    struct liblog_level *a_level;

    logc_assert(levels, );
    logc_profile(flag, "--level_list[%p]--", levels);
    logc_arraylist_foreach(levels, i, a_level) {
        if (a_level) {
            liblog_level_profile(a_level, flag);
        }
    }
}

void liblog_level_list_del(struct logc_arraylist *levels)
{
    logc_assert(levels, );
    logc_arraylist_del(levels);

    logc_debug("logc_level_list_del[%p]", levels);
}

static int liblog_level_list_set_default(struct logc_arraylist *levels)
{
    return liblog_level_list_set(levels, "* = 0, LOG_INFO")
        || liblog_level_list_set(levels, "DEBUG  = 20, LOG_DEBUG")
        || liblog_level_list_set(levels, "INFO   = 40, LOG_INFO")
        || liblog_level_list_set(levels, "NOTICE = 60, LOG_NOTICE")
        || liblog_level_list_set(levels, "WARN   = 80, LOG_WARNING")
        || liblog_level_list_set(levels, "ERROR  = 100, LOG_ERR")
        || liblog_level_list_set(levels, "FATAL  = 120, LOG_ALERT")
        || liblog_level_list_set(levels, "UNKNOW = 254, LOG_ERR")
        || liblog_level_list_set(levels, "! = 255, LOG_INFO");
}

struct logc_arraylist *liblog_level_list_new(void)
{
    struct logc_arraylist *levels;

    levels = logc_arraylist_new((arraylist_del_func)liblog_level_del);
    if (!levels) {
        logc_error("logc_arraylist_new fail");
        return NULL;
    }

    if (liblog_level_list_set_default(levels)) {
        logc_error("liblog_level_list_set_default fail");
        goto err;
    }

    return levels;

err:
    logc_arraylist_del(levels);
    return NULL;
}

int liblog_level_list_set(struct logc_arraylist *levels, char *line)
{
    struct liblog_level *a_level;

    a_level = liblog_level_new(line);
    if (!a_level) {
        logc_error("liblog_level_new fail");
        return -1;
    }

    if (logc_arraylist_set(levels, a_level->int_level, a_level)) {
        logc_error("logc_arraylist_set fail");
        goto err;
    }

    return 0;

err:
    logc_error("line[%s]", line);
    liblog_level_del(a_level);

    return -1;
}

struct liblog_level *liblog_level_list_get(struct logc_arraylist *levels, int l)
{
    struct liblog_level *a_level;

    a_level = (struct liblog_level *)logc_arraylist_get(levels, l);
    if (a_level) {
        return a_level;
    } else {
        logc_error("l[%d] not in (0, 254), or has no level defined, see configure file define, set to UNKNOW", l);
        return (struct liblog_level *)logc_arraylist_get(levels, 254);
    }
}

int liblog_level_list_atoi(struct logc_arraylist *levels, char *str)
{
    int i;
    struct liblog_level *a_level;

    if ((str == NULL) || (*str == '\0')) {
        logc_error("str is [%s], can't find level", str);
        return -1;
    }

    logc_arraylist_foreach(levels, i, a_level) {
        if (a_level && LOG_STRICMP(str, ==, (a_level)->str_uppercase)) {
            return i;
        }
    }

    logc_error("str[%s] can't found in level list", str);
    return -1;
}
