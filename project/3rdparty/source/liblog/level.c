#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <syslog.h>

#include "level.h"
#include "logc_defs.h"

void liblog_level_profile(struct liblog_level *a_level, int flag)
{
    logc_assert(a_level, );
    logc_profile(flag, "---level[%p][%d,%s,%s,%d,%d]---", a_level, a_level->int_level, a_level->str_uppercase, a_level->str_lowercase, (int) a_level->str_len, a_level->syslog_level);
}

void liblog_level_del(struct liblog_level *a_level)
{
    logc_assert(a_level, );
    logc_debug("liblog_level_del[%p]", a_level);
    free(a_level);
}

static int syslog_level_atoi(char *str)
{
    logc_assert(str, -187);

    if (LOG_STRICMP(str, ==, "LOG_EMERG")) {
        return LOG_EMERG;
    }

    if (LOG_STRICMP(str, ==, "LOG_ALERT")) {
        return LOG_ALERT;
    }

    if (LOG_STRICMP(str, ==, "LOG_CRIT")) {
        return LOG_CRIT;
    }

    if (LOG_STRICMP(str, ==, "LOG_ERR")) {
        return LOG_ERR;
    }

    if (LOG_STRICMP(str, ==, "LOG_WARNING")) {
        return LOG_WARNING;
    }

    if (LOG_STRICMP(str, ==, "LOG_NOTICE")) {
        return LOG_NOTICE;
    }

    if (LOG_STRICMP(str, ==, "LOG_INFO")) {
        return LOG_INFO;
    }

    if (LOG_STRICMP(str, ==, "LOG_DEBUG")) {
        return LOG_DEBUG;
    }

    logc_error("wrong syslog level[%s]", str);
    return -187;
}

struct liblog_level *liblog_level_new(char *line)
{
    int i, l = 0, nscan;
    char sl[LOG_MAXLEN_CFG_LINE + 1];
    char str[LOG_MAXLEN_CFG_LINE + 1];
    struct liblog_level *a_level = NULL;

    logc_assert(line, NULL);

    memset(sl, 0x00, sizeof(sl));
    memset(str, 0x00, sizeof(str));

    nscan = sscanf(line, " %[^= \t] = %d ,%s", str, &l, sl);
    if (nscan < 2) {
        logc_error("level[%s], syntax wrong", line);
        return NULL;
    }

    if ((l < 0) || (l > 255)) {
        logc_error("l[%d] not in [0,255], wrong", l);
        return NULL;
    }

    if (str[0] == '\0') {
        logc_error("str[0] == 0");
        return NULL;
    }

    a_level = (struct liblog_level *)calloc(1, sizeof(struct liblog_level));
    if (!a_level) {
        logc_error("calloc fail, errno[%d]", errno);
        return NULL;
    }

    a_level->int_level = l;

    if (sl[0] == '\0') {
        a_level->syslog_level = LOG_DEBUG;
    } else {
        a_level->syslog_level = syslog_level_atoi(sl);
        if (a_level->syslog_level == -187) {
            logc_error("syslog_level_atoi fail");
            goto err;
        }
    }

    for (i = 0; (i < (int)sizeof(a_level->str_uppercase) - 1) && (str[i] != '\0'); i++) {
        (a_level->str_uppercase)[i] = toupper(str[i]);
        (a_level->str_lowercase)[i] = tolower(str[i]);
    }

    if (str[i] != '\0') {
        logc_error("not enough space for str, str[%s] > %d", str, i);
        goto err;
    } else {
        (a_level->str_uppercase)[i] = '\0';
        (a_level->str_lowercase)[i] = '\0';
    }

    a_level->str_len = i;
    return a_level;

err:
    logc_error("line[%s]", line);
    liblog_level_del(a_level);

    return NULL;
}
