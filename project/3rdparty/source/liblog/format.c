#include <errno.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "spec.h"
#include "thread.h"
#include "format.h"
#include "logc_defs.h"

void liblog_format_profile(struct liblog_format *a_format, int flag)
{
    logc_assert(a_format, );
    logc_profile(flag, "---format[%p][%s = %s(%p)]---", a_format, a_format->name, a_format->pattern, a_format->pattern_specs);
}

void liblog_format_del(struct liblog_format *a_format)
{
    logc_assert(a_format, );
    if (a_format->pattern_specs) {
        logc_arraylist_del(a_format->pattern_specs);
    }

    logc_debug("liblog_format_del[%p]", a_format);
    free(a_format);
}

struct liblog_format *liblog_format_new(char *line, int *time_cache_count)
{
    char *p, *q;
    int nscan = 0, nread = 0;
    const char *p_start, *p_end;
    struct liblog_spec *a_spec;
    struct liblog_format *a_format = NULL;

    logc_assert(line, NULL);

    a_format = (struct liblog_format *)calloc(1, sizeof(struct liblog_format));
    if (!a_format) {
        logc_error("calloc fail, errno[%d]", errno);
        return NULL;
    }

    memset(a_format->name, 0x00, sizeof(a_format->name));

    nread = 0;
    nscan = sscanf(line, " %[^= \t] = %n", a_format->name, &nread);
    if (nscan != 1) {
        logc_error("format[%s], syntax wrong", line);
        goto err;
    }

    if (*(line + nread) != '"') {
        logc_error("the 1st char of pattern is not \", line + nread[%s]", line + nread);
        goto err;
    }

    for (p = a_format->name; *p != '\0'; p++) {
        if ((!isalnum(*p)) && (*p != '_')) {
            logc_error("a_format->name[%s] character is not in [a-Z][0-9][_]", a_format->name);
            goto err;
        }
    }

    p_start = line + nread + 1;
    p_end = strrchr(p_start, '"');
    if (!p_end) {
        logc_error("there is no \" at end of pattern, line[%s]", line);
        goto err;
    }

    if ((size_t)(p_end - p_start) > (sizeof(a_format->pattern) - 1)) {
        logc_error("pattern is too long");
        goto err;
    }

    memset(a_format->pattern, 0x00, sizeof(a_format->pattern));
    memcpy(a_format->pattern, p_start, p_end - p_start);

    if (logc_str_replace_env(a_format->pattern, sizeof(a_format->pattern))) {
        logc_error("logc_str_replace_env fail");
        goto err;
    }

    a_format->pattern_specs = logc_arraylist_new((arraylist_del_func)liblog_spec_del);
    if (!(a_format->pattern_specs)) {
        logc_error("logc_arraylist_new fail");
        goto err;
    }

    for (p = a_format->pattern; *p != '\0'; p = q) {
        a_spec = liblog_spec_new(p, &q, time_cache_count);
        if (!a_spec) {
            logc_error("liblog_spec_new fail");
            goto err;
        }

        if (logc_arraylist_add(a_format->pattern_specs, a_spec)) {
            liblog_spec_del(a_spec);
            logc_error("logc_arraylist_add fail");

            goto err;
        }
    }

    liblog_format_profile(a_format, LOGC_DEBUG);
    return a_format;

err:
    liblog_format_del(a_format);
    return NULL;
}

int liblog_format_gen_msg(struct liblog_format *a_format, struct liblog_thread *a_thread)
{
    int i;
    struct liblog_spec *a_spec;

    liblog_buf_restart(a_thread->msg_buf);
    logc_arraylist_foreach(a_format->pattern_specs, i, a_spec) {
        if (liblog_spec_gen_msg(a_spec, a_thread) == 0) {
            continue;
        } else {
            return -1;
        }
    }

    return 0;
}
