#include <stdlib.h>
#include <string.h>

#include <xlib/xlib/config.h>
#include <xlib/xlib/xmem.h>
#include <xlib/xlib/xtypes.h>
#include <xlib/xlib/xenviron.h>
#include <xlib/xlib/xstrfuncs.h>
#include <xlib/xlib/xmessages.h>
#include <xlib/xlib/xthreadprivate.h>

extern char **environ;

static xboolean x_environ_matches(const xchar *env, const xchar *variable, xsize len)
{
    return strncmp(env, variable, len) == 0 && env[len] == '=';
}

static xint x_environ_find (xchar **envp, const xchar *variable)
{
    xint i;
    xsize len;

    if (envp == NULL) {
        return -1;
    }

    len = strlen(variable);
    for (i = 0; envp[i]; i++) {
        if (x_environ_matches(envp[i], variable, len)) {
            return i;
        }
    }

    return -1;
}

const xchar *x_environ_getenv(xchar **envp, const xchar *variable)
{
    xint index;

    x_return_val_if_fail(variable != NULL, NULL);

    index = x_environ_find(envp, variable);
    if (index != -1) {
        return envp[index] + strlen(variable) + 1;
    } else {
        return NULL;
    }
}

xchar **x_environ_setenv(xchar **envp, const xchar *variable, const xchar *value, xboolean overwrite)
{
    xint index;

    x_return_val_if_fail(variable != NULL, NULL);
    x_return_val_if_fail(strchr(variable, '=') == NULL, NULL);
    x_return_val_if_fail(value != NULL, NULL);

    index = x_environ_find(envp, variable);
    if (index != -1) {
        if (overwrite) {
            x_free(envp[index]);
            envp[index] = x_strdup_printf("%s=%s", variable, value);
        }
    } else {
        xint length;

        length = envp ? x_strv_length(envp) : 0;
        envp = x_renew(xchar *, envp, length + 2);
        envp[length] = x_strdup_printf("%s=%s", variable, value);
        envp[length + 1] = NULL;
    }

    return envp;
}

static xchar **x_environ_unsetenv_internal(xchar **envp, const xchar *variable, xboolean free_value)
{
    xsize len;
    xchar **e, **f;

    len = strlen(variable);
    e = f = envp;

    while (*e != NULL) {
        if (!x_environ_matches(*e, variable, len)) {
            *f = *e;
            f++;
        } else {
            if (free_value) {
                x_free(*e);
            }
        }

        e++;
    }

    *f = NULL;
    return envp;
}

xchar **x_environ_unsetenv(xchar **envp, const xchar *variable)
{
    x_return_val_if_fail(variable != NULL, NULL);
    x_return_val_if_fail(strchr(variable, '=') == NULL, NULL);

    if (envp == NULL) {
        return NULL;
    }

    return x_environ_unsetenv_internal(envp, variable, TRUE);
}

const xchar *x_getenv(const xchar *variable)
{
    x_return_val_if_fail(variable != NULL, NULL);
    return getenv(variable);
}

xboolean x_setenv(const xchar *variable, const xchar *value, xboolean overwrite)
{
    xint result;

    x_return_val_if_fail(variable != NULL, FALSE);
    x_return_val_if_fail(strchr(variable, '=') == NULL, FALSE);
    x_return_val_if_fail(value != NULL, FALSE);

    if (x_thread_n_created() > 0) {
        x_debug("setenv()/putenv() are not thread-safe and should not be used after threads are created");
    }

    result = setenv(variable, value, overwrite);
    return result == 0;
}

void x_unsetenv(const xchar *variable)
{
    x_return_if_fail(variable != NULL);
    x_return_if_fail(strchr(variable, '=') == NULL);

    if (x_thread_n_created() > 0) {
        x_debug("unsetenv() is not thread-safe and should not be used after threads are created");
    }

    unsetenv(variable);
}

xchar **x_listenv(void)
{
    xint len, i, j;
    xchar **result, *eq;

    len = x_strv_length(environ);
    result = x_new0(xchar *, len + 1);

    j = 0;
    for (i = 0; i < len; i++) {
        eq = strchr(environ[i], '=');
        if (eq) {
            result[j++] = x_strndup(environ[i], eq - environ[i]);
        }
    }

    result[j] = NULL;
    return result;
}

xchar **x_get_environ(void)
{
    return x_strdupv(environ);
}
