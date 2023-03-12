#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "logc_defs.h"

size_t logc_parse_byte_size(char *astring)
{
    int c, m;
    long res;
    size_t sz;
    char *p, *q;

    logc_assert(astring, 0);

    for (p = q = astring; *p != '\0'; p++) {
        if (isspace(*p)) {
            continue;
        } else {
            *q = *p;
            q++;
        }
    }
    *q = '\0';

    sz = strlen(astring);
    res = strtol(astring, (char **)NULL, 10);
    if (res <= 0) {
        return 0;
    }

    if ((astring[sz - 1] == 'B') || (astring[sz - 1] == 'b')) {
        c = astring[sz - 2];
        m = 1024;
    } else {
        c = astring[sz - 1];
        m = 1000;
    }

    switch (c) {
        case 'K':
        case 'k':
            res *= m;
            break;

        case 'M':
        case 'm':
            res *= m * m;
            break;

        case 'G':
        case 'g':
            res *= m * m * m;
            break;

        default:
            if (!isdigit(c)) {
                logc_error("Wrong suffix parsing " "size in bytes for string [%s], ignoring suffix", astring);
            }
            break;
    }

    return res;
}

int logc_str_replace_env(char *str, size_t str_size)
{
    char *p, *q;
    char fmt[LOG_MAXLEN_CFG_LINE + 1];
    char env_key[LOG_MAXLEN_CFG_LINE + 1];
    char env_value[LOG_MAXLEN_CFG_LINE + 1];
    int str_len, nscan, nread, env_value_len;

    str_len = strlen(str);
    q = str;

    do {
        p = strchr(q, '%');
        if (!p) {
            break;
        }

        memset(fmt, 0x00, sizeof(fmt));
        memset(env_key, 0x00, sizeof(env_key));
        memset(env_value, 0x00, sizeof(env_value));

        nread = 0;
        nscan = sscanf(p + 1, "%[.0-9-]%n", fmt + 1, &nread);
        if (nscan == 1) {
            fmt[0] = '%';
            fmt[nread + 1] = 's';
        } else {
            nread = 0;
            strcpy(fmt, "%s");
        }

        q = p + 1 + nread;
        nscan = sscanf(q, "E(%[^)])%n", env_key, &nread);
        if (nscan == 0) {
            continue;
        }

        q += nread;
        if (*(q - 1) != ')') {
            logc_error("in string[%s] can't find match )", p);
            return -1;
        }

        env_value_len = snprintf(env_value, sizeof(env_value), fmt, getenv(env_key));
        if ((env_value_len < 0) || (env_value_len >= (int)sizeof(env_value))) {
            logc_error("snprintf fail, errno[%d], evn_value_len[%d]", errno, env_value_len);
            return -1;
        }

        str_len = str_len - (q - p) + env_value_len;
        if (str_len > (int)(str_size - 1)) {
            logc_error("repalce env_value[%s] cause overlap", env_value);
            return -1;
        }

        memmove(p + env_value_len, q, strlen(q) + 1);
        memcpy(p, env_value, env_value_len);
    } while (1);

    return 0;
}
