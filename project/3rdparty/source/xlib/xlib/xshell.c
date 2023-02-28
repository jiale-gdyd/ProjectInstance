#include <string.h>
#include <xlib/xlib/config.h>
#include <xlib/xlib/xshell.h>
#include <xlib/xlib/xslist.h>
#include <xlib/xlib/xthread.h>
#include <xlib/xlib/xstring.h>
#include <xlib/xlib/xlibintl.h>
#include <xlib/xlib/xstrfuncs.h>
#include <xlib/xlib/xtestutils.h>

X_DEFINE_QUARK(x-shell-error-quark, x_shell_error)

static xboolean unquote_string_inplace(xchar *str, xchar **end, XError **err)
{
    xchar *s;
    xchar *dest;
    xchar quote_char;

    x_return_val_if_fail(end != NULL, FALSE);
    x_return_val_if_fail(err == NULL || *err == NULL, FALSE);
    x_return_val_if_fail(str != NULL, FALSE);

    dest = s = str;
    quote_char = *s;

    if (!(*s == '"' || *s == '\'')) {
        x_set_error_literal(err, X_SHELL_ERROR, X_SHELL_ERROR_BAD_QUOTING, _("Quoted text doesn’t begin with a quotation mark"));
        *end = str;
        return FALSE;
    }

    ++s;
    if (quote_char == '"') {
        while (*s) {
            x_assert(s > dest);

            switch (*s) {
                case '"':
                    *dest = '\0';
                    ++s;
                    *end = s;
                    return TRUE;
                    break;

                case '\\':
                    ++s;
                    switch (*s) {
                        case '"':
                        case '\\':
                        case '`':
                        case '$':
                        case '\n':
                            *dest = *s;
                            ++s;
                            ++dest;
                            break;

                        default:
                            *dest = '\\';
                            ++dest;
                            break;
                    }
                    break;

                default:
                    *dest = *s;
                    ++dest;
                    ++s;
                    break;
            }

            x_assert(s > dest);
        }
    } else {
        while (*s) {
            x_assert(s > dest);

            if (*s == '\'') {
                *dest = '\0';
                ++s;
                *end = s;
                return TRUE;
            } else {
                *dest = *s;
                ++dest;
                ++s;
            }

            x_assert(s > dest);
        }
    }

    *dest = '\0';
    x_set_error_literal(err, X_SHELL_ERROR, X_SHELL_ERROR_BAD_QUOTING, _("Unmatched quotation mark in command line or other shell-quoted text"));
    *end = s;

    return FALSE;
}

xchar *x_shell_quote(const xchar *unquoted_string)
{
    XString *dest;
    const xchar *p;

    x_return_val_if_fail(unquoted_string != NULL, NULL);
    dest = x_string_new("'");

    p = unquoted_string;
    while (*p) {
        if (*p == '\'') {
            x_string_append(dest, "'\\''");
        } else {
            x_string_append_c(dest, *p);
        }

        ++p;
    }

    x_string_append_c(dest, '\'');
    return x_string_free(dest, FALSE);
}

xchar *x_shell_unquote(const xchar *quoted_string, XError **error)
{
    xchar *end;
    xchar *start;
    xchar *unquoted;
    XString *retval;

    x_return_val_if_fail(quoted_string != NULL, NULL);
    unquoted = x_strdup(quoted_string);

    start = unquoted;
    end = unquoted;
    retval = x_string_new(NULL);

    while (*start) {
        while (*start && !(*start == '"' || *start == '\'')) {
            if (*start == '\\') {
                ++start;
                if (*start) {
                    if (*start != '\n') {
                        x_string_append_c(retval, *start);
                    }

                    ++start;
                }
            } else {
                x_string_append_c(retval, *start);
                ++start;
            }
        }

        if (*start) {
            if (!unquote_string_inplace(start, &end, error)) {
                goto error;
            } else {
                x_string_append(retval, start);
                start = end;
            }
        }
    }

    x_free(unquoted);
    return x_string_free(retval, FALSE);

error:
    x_assert(error == NULL || *error != NULL);

    x_free(unquoted);
    x_string_free(retval, TRUE);
    return NULL;
}

static inline void ensure_token(XString **token)
{
    if (*token == NULL) {
        *token = x_string_new(NULL);
    }
}

static void delimit_token(XString **token, XSList **retval)
{
    if (*token == NULL) {
        return;
    }

    *retval = x_slist_prepend(*retval, x_string_free(*token, FALSE));
    *token = NULL;
}

static XSList *tokenize_command_line(const xchar *command_line, XError **error)
{
    const xchar *p;
    xboolean quoted;
    xchar current_quote;
    XSList *retval = NULL;
    XString *current_token = NULL;

    current_quote = '\0';
    quoted = FALSE;
    p = command_line;

    while (*p) {
        if (current_quote == '\\') {
            if (*p == '\n') {

            } else {
                ensure_token(&current_token);
                x_string_append_c(current_token, '\\');
                x_string_append_c(current_token, *p);
            }

            current_quote = '\0';
        } else if (current_quote == '#') {
            while (*p && *p != '\n') {
                ++p;
            }

            current_quote = '\0';
            if (*p == '\0') {
                break;
            }
        } else if (current_quote) {
            if (*p == current_quote && !(current_quote == '"' && quoted)) {
                current_quote = '\0';
            }

            ensure_token(&current_token);
            x_string_append_c(current_token, *p);
        } else {
            switch (*p) {
                case '\n':
                    delimit_token(&current_token, &retval);
                    break;

                case ' ':
                case '\t':
                    if (current_token && current_token->len > 0) {
                        delimit_token(&current_token, &retval);
                    }
                    break;

                case '\'':
                case '"':
                    ensure_token(&current_token);
                    x_string_append_c(current_token, *p);

                X_GNUC_FALLTHROUGH;
                case '\\':
                    current_quote = *p;
                    break;

                case '#':
                    if (p == command_line) {
                        current_quote = *p;
                        break;
                    }

                    switch (*(p-1)) {
                        case ' ':
                        case '\n':
                        case '\0':
                            current_quote = *p;
                            break;

                        default:
                            ensure_token(&current_token);
                            x_string_append_c(current_token, *p);
                            break;
                    }
                    break;

                default:
                    ensure_token(&current_token);
                    x_string_append_c(current_token, *p);
                    break;
            }
        }

        if (*p != '\\') {
            quoted = FALSE;
        } else {
            quoted = !quoted;
        }

        ++p;
    }

    delimit_token(&current_token, &retval);

    if (current_quote) {
        if (current_quote == '\\') {
            x_set_error(error, X_SHELL_ERROR, X_SHELL_ERROR_BAD_QUOTING, _("Text ended just after a “\\” character. (The text was “%s”)"), command_line);
        } else {
            x_set_error(error, X_SHELL_ERROR, X_SHELL_ERROR_BAD_QUOTING, _("Text ended before matching quote was found for %c. (The text was “%s”)"), current_quote, command_line);
        }

        goto error;
    }

    if (retval == NULL) {
        x_set_error_literal(error, X_SHELL_ERROR, X_SHELL_ERROR_EMPTY_STRING, _("Text was empty (or contained only whitespace)"));
        goto error;
    }

    retval = x_slist_reverse(retval);
    return retval;

error:
    x_assert(error == NULL || *error != NULL);
    x_slist_free_full(retval, x_free);

    return NULL;
}

xboolean x_shell_parse_argv(const xchar *command_line, xint *argcp, xchar ***argvp, XError **error)
{
    xint i;
    xint argc = 0;
    XSList *tmp_list;
    xchar **argv = NULL;
    XSList *tokens = NULL;

    x_return_val_if_fail(command_line != NULL, FALSE);

    tokens = tokenize_command_line(command_line, error);
    if (tokens == NULL) {
        return FALSE;
    }

    argc = x_slist_length(tokens);
    argv = x_new0(xchar *, argc + 1);

    i = 0;
    tmp_list = tokens;

    while (tmp_list) {
        argv[i] = (xchar *)x_shell_unquote((const xchar *)tmp_list->data, error);
        if (argv[i] == NULL) {
            goto failed;
        }

        tmp_list = x_slist_next(tmp_list);
        ++i;
    }

    x_slist_free_full(tokens, x_free);

    x_assert(argc > 0);
    x_assert(argv != NULL && argv[0] != NULL);

    if (argcp) {
        *argcp = argc;
    }

    if (argvp) {
        *argvp = argv;
    } else {
        x_strfreev(argv);
    }

    return TRUE;

failed:
    x_assert(error == NULL || *error != NULL);
    x_strfreev(argv);
    x_slist_free_full(tokens, x_free);

    return FALSE;
}
