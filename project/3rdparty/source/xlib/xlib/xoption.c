#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <xlib/xlib/config.h>
#include <xlib/xlib/xtypes.h>
#include <xlib/xlib/xoption.h>
#include <xlib/xlib/xprintf.h>
#include <xlib/xlib/xlibintl.h>
#include <xlib/xlib/xutilsprivate.h>

#define TRANSLATE(group, str)       (((group)->translate_func ? (* (group)->translate_func)((str), (group)->translate_data) : (str)))
#define NO_ARG(entry)               ((entry)->arg == X_OPTION_ARG_NONE || ((entry)->arg == X_OPTION_ARG_CALLBACK && ((entry)->flags & X_OPTION_FLAG_NO_ARG)))
#define OPTIONAL_ARG(entry)         ((entry)->arg == X_OPTION_ARG_CALLBACK && (entry)->flags & X_OPTION_FLAG_OPTIONAL_ARG)

typedef struct {
    XOptionArg arg_type;
    xpointer   arg_data;
    union {
        xboolean boolt;
        xint     integer;
        xchar    *str;
        xchar    **array;
        xdouble  dbl;
        xint64   int64;
    } prev;
    union {
        xchar    *str;
        struct {
            xint  len;
            xchar **data;
        } array;
    } allocated;
} Change;

typedef struct {
    xchar **ptr;
    xchar *value;
} PendingNull;

struct _XOptionContext {
    XList          *groups;
    xchar          *parameter_string;
    xchar          *summary;
    xchar          *description;
    XTranslateFunc translate_func;
    XDestroyNotify translate_notify;
    xpointer       translate_data;
    xuint          help_enabled   : 1;
    xuint          ignore_unknown : 1;
    xuint          strv_mode      : 1;
    xuint          strict_posix   : 1;
    XOptionGroup   *main_group;
    XList          *changes;
    XList          *pending_nulls;
};

struct _XOptionGroup {
    xchar            *name;
    xchar            *description;
    xchar            *help_description;
    xint             ref_count;
    XDestroyNotify   destroy_notify;
    xpointer         user_data;
    XTranslateFunc   translate_func;
    XDestroyNotify   translate_notify;
    xpointer         translate_data;
    XOptionEntry     *entries;
    xsize            n_entries;
    XOptionParseFunc pre_parse_func;
    XOptionParseFunc post_parse_func;
    XOptionErrorFunc error_func;
};

static void free_changes_list(XOptionContext *context, xboolean revert);
static void free_pending_nulls(XOptionContext *context, xboolean perform_nulls);

static int _x_unichar_get_width (xunichar c)
{
    if (X_UNLIKELY(x_unichar_iszerowidth(c))) {
        return 0;
    }

    if (x_unichar_iswide(c)) {
        return 2;
    }

    return 1;
}

static xlong _x_utf8_strwidth(const xchar *p)
{
    xlong len = 0;
    x_return_val_if_fail(p != NULL, 0);

    while (*p) {
        len += _x_unichar_get_width(x_utf8_get_char(p));
        p = x_utf8_next_char(p);
    }

    return len;
}

X_DEFINE_QUARK(x-option-context-error-quark, x_option_error)

XOptionContext *x_option_context_new(const xchar *parameter_string)
{
    XOptionContext *context;

    context = x_new0(XOptionContext, 1);

    if (parameter_string != NULL && *parameter_string == '\0') {
        parameter_string = NULL;
    }

    context->parameter_string = x_strdup(parameter_string);
    context->strict_posix = FALSE;
    context->help_enabled = TRUE;
    context->ignore_unknown = FALSE;

    return context;
}

void x_option_context_free(XOptionContext *context)
{
    x_return_if_fail(context != NULL);

    x_list_free_full(context->groups, (XDestroyNotify)x_option_group_unref);

    if (context->main_group) {
        x_option_group_unref(context->main_group);
    }

    free_changes_list(context, FALSE);
    free_pending_nulls(context, FALSE);

    x_free(context->parameter_string);
    x_free(context->summary);
    x_free(context->description);

    if (context->translate_notify) {
        (* context->translate_notify)(context->translate_data);
    }

    x_free(context);
}

void x_option_context_set_help_enabled(XOptionContext *context, xboolean help_enabled)
{
    x_return_if_fail(context != NULL);
    context->help_enabled = help_enabled;
}

xboolean x_option_context_get_help_enabled(XOptionContext *context)
{
    x_return_val_if_fail(context != NULL, FALSE);
    return context->help_enabled;
}

void x_option_context_set_ignore_unknown_options(XOptionContext *context, xboolean ignore_unknown)
{
    x_return_if_fail(context != NULL);
    context->ignore_unknown = ignore_unknown;
}

xboolean x_option_context_get_ignore_unknown_options(XOptionContext *context)
{
    x_return_val_if_fail(context != NULL, FALSE);
    return context->ignore_unknown;
}

void x_option_context_set_strict_posix(XOptionContext *context, xboolean strict_posix)
{
    x_return_if_fail(context != NULL);
    context->strict_posix = strict_posix;
}

xboolean x_option_context_get_strict_posix(XOptionContext *context)
{
    x_return_val_if_fail(context != NULL, FALSE);
    return context->strict_posix;
}

void x_option_context_add_group(XOptionContext *context, XOptionGroup *group)
{
    XList *list;

    x_return_if_fail(context != NULL);
    x_return_if_fail(group != NULL);
    x_return_if_fail(group->name != NULL);
    x_return_if_fail(group->description != NULL);
    x_return_if_fail(group->help_description != NULL);

    for (list = context->groups; list; list = list->next) {
        XOptionGroup *g = (XOptionGroup *)list->data;

        if ((group->name == NULL && g->name == NULL) || (group->name && g->name && strcmp (group->name, g->name) == 0)) {
            x_warning("A group named \"%s\" is already part of this XOptionContext", group->name);
        }
    }

    context->groups = x_list_append(context->groups, group);
}

void x_option_context_set_main_group(XOptionContext *context, XOptionGroup *group)
{
    x_return_if_fail(context != NULL);
    x_return_if_fail(group != NULL);

    if (context->main_group) {
        x_warning("This XOptionContext already has a main group");
        return;
    }

    context->main_group = group;
}

XOptionGroup *x_option_context_get_main_group(XOptionContext *context)
{
    x_return_val_if_fail(context != NULL, NULL);
    return context->main_group;
}

void x_option_context_add_main_entries(XOptionContext *context, const XOptionEntry *entries, const xchar *translation_domain)
{
    x_return_if_fail(context != NULL);
    x_return_if_fail(entries != NULL);

    if (!context->main_group) {
        context->main_group = x_option_group_new(NULL, NULL, NULL, NULL, NULL);
    }

    x_option_group_add_entries(context->main_group, entries);
    x_option_group_set_translation_domain(context->main_group, translation_domain);
}

static xint calculate_max_length(XOptionGroup *group, XHashTable *aliases)
{
    XOptionEntry *entry;
    const xchar *long_name;
    xsize i, len, max_length;

    max_length = 0;

    for (i = 0; i < group->n_entries; i++) {
        entry = &group->entries[i];

        if (entry->flags & X_OPTION_FLAG_HIDDEN) {
            continue;
        }

        long_name = (const xchar *)x_hash_table_lookup (aliases, &entry->long_name);
        if (!long_name) {
            long_name = entry->long_name;
        }
        len = _x_utf8_strwidth(long_name);

        if (entry->short_name) {
            len += 4;
        }

        if (!NO_ARG(entry) && entry->arg_description) {
            len += 1 + _x_utf8_strwidth(TRANSLATE(group, entry->arg_description));
        }

        max_length = MAX(max_length, len);
    }

    return max_length;
}

static void print_entry(XOptionGroup *group, xint max_length, const XOptionEntry *entry, XString *string, XHashTable *aliases)
{
    XString *str;
    const xchar *long_name;

    if (entry->flags & X_OPTION_FLAG_HIDDEN) {
        return;
    }

    if (entry->long_name[0] == 0) {
        return;
    }

    long_name = (const xchar *)x_hash_table_lookup(aliases, &entry->long_name);
    if (!long_name) {
        long_name = entry->long_name;
    }

    str = x_string_new(NULL);

    if (entry->short_name) {
        x_string_append_printf(str, "  -%c, --%s", entry->short_name, long_name);
    } else {
        x_string_append_printf(str, "  --%s", long_name);
    }

    if (entry->arg_description) {
        x_string_append_printf(str, "=%s", TRANSLATE(group, entry->arg_description));
    }

    x_string_append_printf(string, "%s%*s %s\n", str->str, (int) (max_length + 4 - _x_utf8_strwidth(str->str)), "", entry->description ? TRANSLATE(group, entry->description) : "");
    x_string_free (str, TRUE);
}

static xboolean group_has_visible_entries(XOptionContext *context, XOptionGroup *group, xboolean main_entries)
{
    xint i, l;
    XOptionEntry *entry;
    XOptionFlags reject_filter = X_OPTION_FLAG_HIDDEN;
    xboolean main_group = group == context->main_group;

    if (!main_entries) {
        reject_filter = (XOptionFlags)(reject_filter | X_OPTION_FLAG_IN_MAIN);
    }

    for (i = 0, l = (group ? group->n_entries : 0); i < l; i++) {
        entry = &group->entries[i];

        if (main_entries && !main_group && !(entry->flags & X_OPTION_FLAG_IN_MAIN)) {
            continue;
        }

        if (entry->long_name[0] == 0) {
            continue;
        }

        if (!(entry->flags & reject_filter)) {
            return TRUE;
        }
    }

    return FALSE;
}

static xboolean group_list_has_visible_entries(XOptionContext *context, XList *group_list, xboolean main_entries)
{
    while (group_list) {
        if (group_has_visible_entries(context, (XOptionGroup *)group_list->data, main_entries)) {
            return TRUE;
        }

        group_list = group_list->next;
    }

    return FALSE;
}

static xboolean context_has_h_entry(XOptionContext *context)
{
    xsize i;
    XList *list;

    if (context->main_group) {
        for (i = 0; i < context->main_group->n_entries; i++) {
            if (context->main_group->entries[i].short_name == 'h') {
                return TRUE;
            }
        }
    }

    for (list = context->groups; list != NULL; list = x_list_next(list)) {
        XOptionGroup *group;

        group = (XOptionGroup *)list->data;
        for (i = 0; i < group->n_entries; i++) {
            if (group->entries[i].short_name == 'h') {
                return TRUE;
            }
        }
    }

    return FALSE;
}

xchar *x_option_context_get_help(XOptionContext *context, xboolean main_help, XOptionGroup *group)
{
    xsize i;
    XList *list;
    xuchar token;
    XString *string;
    xboolean seen[256];
    XOptionEntry *entry;
    XHashTable *aliases;
    XHashTable *shadow_map;
    xint max_length = 0, len;
    const xchar *rest_description;

    x_return_val_if_fail(context != NULL, NULL);

    string = x_string_sized_new(1024);

    rest_description = NULL;
    if (context->main_group) {
        for (i = 0; i < context->main_group->n_entries; i++) {
            entry = &context->main_group->entries[i];
            if (entry->long_name[0] == 0) {
                rest_description = TRANSLATE(context->main_group, entry->arg_description);
                break;
            }
        }
    }

    x_string_append_printf(string, "%s\n  %s", _("Usage:"), x_get_prgname());
    if (context->help_enabled || (context->main_group && context->main_group->n_entries > 0) || context->groups != NULL) {
        x_string_append_printf(string, " %s", _("[OPTION…]"));
    }

    if (rest_description) {
        x_string_append(string, " ");
        x_string_append(string, rest_description);
    }

    if (context->parameter_string) {
        x_string_append(string, " ");
        x_string_append(string, TRANSLATE(context, context->parameter_string));
    }

    x_string_append(string, "\n\n");

    if (context->summary) {
        x_string_append(string, TRANSLATE(context, context->summary));
        x_string_append(string, "\n\n");
    }

    memset(seen, 0, sizeof(xboolean) * 256);
    shadow_map = x_hash_table_new(x_str_hash, x_str_equal);
    aliases = x_hash_table_new_full(NULL, NULL, NULL, x_free);

    if (context->main_group) {
        for (i = 0; i < context->main_group->n_entries; i++) {
            entry = &context->main_group->entries[i];
            x_hash_table_insert(shadow_map, (xpointer)entry->long_name, entry);

            if (seen[(xuchar)entry->short_name]) {
                entry->short_name = 0;
            } else {
                seen[(xuchar)entry->short_name] = TRUE;
            }
        }
    }

    list = context->groups;
    while (list != NULL) {
        XOptionGroup *g = (XOptionGroup *)list->data;

        for (i = 0; i < g->n_entries; i++) {
            entry = &g->entries[i];
            if (x_hash_table_lookup(shadow_map, entry->long_name) && !(entry->flags & X_OPTION_FLAG_NOALIAS)) {
                x_hash_table_insert(aliases, &entry->long_name, x_strdup_printf("%s-%s", g->name, entry->long_name));
            } else {
                x_hash_table_insert(shadow_map, (xpointer)entry->long_name, entry);
            }

            if (seen[(xuchar)entry->short_name] && !(entry->flags & X_OPTION_FLAG_NOALIAS)) {
                entry->short_name = 0;
            } else {
                seen[(xuchar)entry->short_name] = TRUE;
            }
        }

        list = list->next;
    }

    x_hash_table_destroy(shadow_map);
    list = context->groups;

    if (context->help_enabled) {
        max_length = _x_utf8_strwidth("-?, --help");
        if (list) {
            len = _x_utf8_strwidth("--help-all");
            max_length = MAX(max_length, len);
        }
    }

    if (context->main_group) {
        len = calculate_max_length(context->main_group, aliases);
        max_length = MAX(max_length, len);
    }

    while (list != NULL) {
        XOptionGroup *g = (XOptionGroup *)list->data;

        if (context->help_enabled) {
            len = _x_utf8_strwidth("--help-") + _x_utf8_strwidth(g->name);
            max_length = MAX(max_length, len);
        }

        len = calculate_max_length(g, aliases);
        max_length = MAX(max_length, len);

        list = list->next;
    }

    max_length += 4;
    if (!group && context->help_enabled) {
        list = context->groups;

        token = context_has_h_entry(context) ? '?' : 'h';
        x_string_append_printf(string, "%s\n  -%c, --%-*s %s\n", _("Help Options:"), token, max_length - 4, "help", _("Show help options"));

        if (list) {
            x_string_append_printf(string, "  --%-*s %s\n", max_length, "help-all", _("Show all help options"));
        }

        while (list) {
            XOptionGroup *g = (XOptionGroup *)list->data;

            if (group_has_visible_entries(context, g, FALSE)) {
                x_string_append_printf(string, "  --help-%-*s %s\n", max_length - 5, g->name, TRANSLATE(g, g->help_description));
            }

            list = list->next;
        }

        x_string_append(string, "\n");
    }

    if (group) {
        if (group_has_visible_entries(context, group, FALSE)) {
            x_string_append(string, TRANSLATE(group, group->description));
            x_string_append(string, "\n");
            for (i = 0; i < group->n_entries; i++) {
                print_entry(group, max_length, &group->entries[i], string, aliases);
            }
            x_string_append(string, "\n");
        }
    } else if (!main_help) {
        list = context->groups;

        while (list) {
            XOptionGroup *g = (XOptionGroup *)list->data;

            if (group_has_visible_entries(context, g, FALSE)) {
                x_string_append(string, g->description);
                x_string_append(string, "\n");
                for (i = 0; i < g->n_entries; i++) {
                    if (!(g->entries[i].flags & X_OPTION_FLAG_IN_MAIN)) {
                        print_entry(g, max_length, &g->entries[i], string, aliases);
                    }
                }

                x_string_append(string, "\n");
            }

            list = list->next;
        }
    }

    if ((main_help || !group) && (group_has_visible_entries(context, context->main_group, TRUE) || group_list_has_visible_entries(context, context->groups, TRUE))) {
        list = context->groups;

        if (context->help_enabled || list) {
            x_string_append(string,  _("Application Options:"));
        } else {
            x_string_append(string, _("Options:"));
        }
        x_string_append(string, "\n");

        if (context->main_group) {
            for (i = 0; i < context->main_group->n_entries; i++) {
                print_entry(context->main_group, max_length, &context->main_group->entries[i], string, aliases);
            }
        }

        while (list != NULL) {
            XOptionGroup *g = (XOptionGroup *)list->data;

            for (i = 0; i < g->n_entries; i++) {
                if (g->entries[i].flags & X_OPTION_FLAG_IN_MAIN) {
                    print_entry(g, max_length, &g->entries[i], string, aliases);
                }
            }

            list = list->next;
        }

        x_string_append(string, "\n");
    }

    if (context->description) {
        x_string_append(string, TRANSLATE(context, context->description));
        x_string_append(string, "\n");
    }

    x_hash_table_destroy(aliases);
    return x_string_free(string, FALSE);
}

X_NORETURN static void print_help(XOptionContext *context, xboolean main_help, XOptionGroup *group)
{
    xchar *help;

    help = x_option_context_get_help(context, main_help, group);
    x_print("%s", help);
    x_free(help);

    exit(0);
}

static xboolean parse_int(const xchar *arg_name, const xchar *arg, xint *result, XError **error)
{
    xlong tmp;
    xchar *end;

    errno = 0;
    tmp = strtol(arg, &end, 0);

    if (*arg == '\0' || *end != '\0') {
        x_set_error(error, X_OPTION_ERROR, X_OPTION_ERROR_BAD_VALUE, _("Cannot parse integer value “%s” for %s"), arg, arg_name);
        return FALSE;
    }

    *result = tmp;
    if (*result != tmp || errno == ERANGE) {
        x_set_error(error, X_OPTION_ERROR, X_OPTION_ERROR_BAD_VALUE, _("Integer value “%s” for %s out of range"), arg, arg_name);
        return FALSE;
    }

    return TRUE;
}

static xboolean parse_double(const xchar *arg_name, const xchar *arg, xdouble *result, XError **error)
{
    xchar *end;
    xdouble tmp;

    errno = 0;
    tmp = x_strtod(arg, &end);

    if (*arg == '\0' || *end != '\0') {
        x_set_error(error, X_OPTION_ERROR, X_OPTION_ERROR_BAD_VALUE, _("Cannot parse double value “%s” for %s"), arg, arg_name);
        return FALSE;
    }

    if (errno == ERANGE) {
        x_set_error(error, X_OPTION_ERROR, X_OPTION_ERROR_BAD_VALUE, _("Double value “%s” for %s out of range"), arg, arg_name);
        return FALSE;
    }

    *result = tmp;
    return TRUE;
}

static xboolean parse_int64(const xchar *arg_name, const xchar *arg, xint64 *result, XError **error)
{
    xchar *end;
    xint64 tmp;

    errno = 0;
    tmp = x_ascii_strtoll(arg, &end, 0);

    if (*arg == '\0' || *end != '\0') {
        x_set_error(error, X_OPTION_ERROR, X_OPTION_ERROR_BAD_VALUE, _("Cannot parse integer value “%s” for %s"), arg, arg_name);
        return FALSE;
    }

    if (errno == ERANGE) {
        x_set_error(error, X_OPTION_ERROR, X_OPTION_ERROR_BAD_VALUE, _("Integer value “%s” for %s out of range"), arg, arg_name);
        return FALSE;
    }

    *result = tmp;
    return TRUE;
}

static Change *get_change(XOptionContext *context, XOptionArg arg_type, xpointer arg_data)
{
    XList *list;
    Change *change = NULL;

    for (list = context->changes; list != NULL; list = list->next) {
        change = (Change *)list->data;
        if (change->arg_data == arg_data) {
            goto found;
        }
    }

    change = x_new0(Change, 1);
    change->arg_type = arg_type;
    change->arg_data = arg_data;

    context->changes = x_list_prepend(context->changes, change);

found:
    return change;
}

static void add_pending_null(XOptionContext *context, xchar **ptr, xchar *value)
{
    PendingNull *n;

    n = x_new0(PendingNull, 1);
    n->ptr = ptr;
    n->value = value;

    context->pending_nulls = x_list_prepend(context->pending_nulls, n);
}

static xboolean parse_arg(XOptionContext *context, XOptionGroup *group, XOptionEntry *entry, const xchar *value, const xchar *option_name, XError **error)
{
    Change *change;

    x_assert(value || OPTIONAL_ARG(entry) || NO_ARG(entry));

    switch (entry->arg) {
        case X_OPTION_ARG_NONE: {
            (void)get_change(context, X_OPTION_ARG_NONE, entry->arg_data);
            *(xboolean *)entry->arg_data = !(entry->flags & X_OPTION_FLAG_REVERSE);
            break;
        }

        case X_OPTION_ARG_STRING: {
            xchar *data;

            data = x_locale_to_utf8(value, -1, NULL, NULL, error);
            if (!data) {
                return FALSE;
            }

            change = get_change(context, X_OPTION_ARG_STRING, entry->arg_data);
            if (!change->allocated.str) {
                change->prev.str = *(xchar **)entry->arg_data;
            } else {
                x_free(change->allocated.str);
            }

            change->allocated.str = data;
            *(xchar **)entry->arg_data = data;
            break;
        }

        case X_OPTION_ARG_STRING_ARRAY: {
            xchar *data;

            data = x_locale_to_utf8(value, -1, NULL, NULL, error);
            if (!data) {
                return FALSE;
            }

            change = get_change(context, X_OPTION_ARG_STRING_ARRAY, entry->arg_data);
            if (change->allocated.array.len == 0) {
                change->prev.array = *(xchar ***)entry->arg_data;
                change->allocated.array.data = x_new(xchar *, 2);
            } else {
                change->allocated.array.data = x_renew(xchar *, change->allocated.array.data, change->allocated.array.len + 2);
            }
            change->allocated.array.data[change->allocated.array.len] = data;
            change->allocated.array.data[change->allocated.array.len + 1] = NULL;
            change->allocated.array.len++;
            *(xchar ***)entry->arg_data = change->allocated.array.data;
            break;
        }

        case X_OPTION_ARG_FILENAME: {
            xchar *data;

            data = x_strdup(value);
            change = get_change(context, X_OPTION_ARG_FILENAME, entry->arg_data);

            if (!change->allocated.str) {
                change->prev.str = *(xchar **)entry->arg_data;
            } else {
                x_free(change->allocated.str);
            }

            change->allocated.str = data;
            *(xchar **)entry->arg_data = data;
            break;
        }

        case X_OPTION_ARG_FILENAME_ARRAY: {
            xchar *data;

            data = x_strdup(value);
            change = get_change(context, X_OPTION_ARG_STRING_ARRAY, entry->arg_data);

            if (change->allocated.array.len == 0) {
                change->prev.array = *(xchar ***)entry->arg_data;
                change->allocated.array.data = x_new(xchar *, 2);
            } else {
                change->allocated.array.data = x_renew(xchar *, change->allocated.array.data, change->allocated.array.len + 2);
            }
            change->allocated.array.data[change->allocated.array.len] = data;
            change->allocated.array.data[change->allocated.array.len + 1] = NULL;

            change->allocated.array.len ++;
            *(xchar ***)entry->arg_data = change->allocated.array.data;
            break;
        }

        case X_OPTION_ARG_INT: {
            xint data;

            if (!parse_int(option_name, value, &data, error)) {
                return FALSE;
            }

            change = get_change(context, X_OPTION_ARG_INT, entry->arg_data);
            change->prev.integer = *(xint *)entry->arg_data;
            *(xint *)entry->arg_data = data;
            break;
        }

        case X_OPTION_ARG_CALLBACK: {
            xchar *data;
            xboolean retval;

            if (!value && entry->flags & X_OPTION_FLAG_OPTIONAL_ARG) {
                data = NULL;
            } else if (entry->flags & X_OPTION_FLAG_NO_ARG) {
                data = NULL;
            } else if (entry->flags & X_OPTION_FLAG_FILENAME) {
                data = x_strdup(value);
            } else {
                data = x_locale_to_utf8(value, -1, NULL, NULL, error);
            }

            if (!(entry->flags & (X_OPTION_FLAG_NO_ARG|X_OPTION_FLAG_OPTIONAL_ARG)) && !data) {
                return FALSE;
            }

            retval = (* (XOptionArgFunc)entry->arg_data) (option_name, data, group->user_data, error);

            if (!retval && error != NULL && *error == NULL) {
                x_set_error(error, X_OPTION_ERROR, X_OPTION_ERROR_FAILED, _("Error parsing option %s"), option_name);
            }
            x_free(data);

            return retval;
            break;
        }

        case X_OPTION_ARG_DOUBLE: {
            xdouble data;

            if (!parse_double(option_name, value, &data, error)) {
                return FALSE;
            }

            change = get_change(context, X_OPTION_ARG_DOUBLE, entry->arg_data);
            change->prev.dbl = *(xdouble *)entry->arg_data;
            *(xdouble *)entry->arg_data = data;
            break;
        }

        case X_OPTION_ARG_INT64: {
            xint64 data;

            if (!parse_int64(option_name, value, &data, error)) {
                return FALSE;
            }

            change = get_change(context, X_OPTION_ARG_INT64, entry->arg_data);
            change->prev.int64 = *(xint64 *)entry->arg_data;
            *(xint64 *)entry->arg_data = data;
            break;
        }

        default:
            x_assert_not_reached();
    }

    return TRUE;
}

static xboolean parse_short_option(XOptionContext *context, XOptionGroup *group, xint idx, xint *new_idx, xchar arg, xint *argc, xchar ***argv, XError **error, xboolean *parsed)
{
    xsize j;

    for (j = 0; j < group->n_entries; j++) {
        if (arg == group->entries[j].short_name) {
            xchar *option_name;
            xchar *value = NULL;

            option_name = x_strdup_printf("-%c", group->entries[j].short_name);
            if (NO_ARG(&group->entries[j])) {
                value = NULL;
            } else {
                if (*new_idx > idx) {
                    x_set_error(error, X_OPTION_ERROR, X_OPTION_ERROR_FAILED, _("Error parsing option %s"), option_name);
                    x_free(option_name);
                    return FALSE;
                }

                if (idx < *argc - 1) {
                    if (OPTIONAL_ARG(&group->entries[j]) && ((*argv)[idx + 1][0] == '-')) {
                        value = NULL;
                    } else {
                        value = (*argv)[idx + 1];
                        add_pending_null(context, &((*argv)[idx + 1]), NULL);
                        *new_idx = idx + 1;
                    }
                } else if (idx >= *argc - 1 && OPTIONAL_ARG(&group->entries[j])) {
                    value = NULL;
                } else {
                    x_set_error(error, X_OPTION_ERROR, X_OPTION_ERROR_BAD_VALUE, _("Missing argument for %s"), option_name);
                    x_free(option_name);
                    return FALSE;
                }
            }

            if (!parse_arg(context, group, &group->entries[j], value, option_name, error)) {
                x_free(option_name);
                return FALSE;
            }

            x_free(option_name);
            *parsed = TRUE;
        }
    }

    return TRUE;
}

static xboolean parse_long_option(XOptionContext *context, XOptionGroup *group, xint *idx, xchar *arg, xboolean aliased, xint *argc, xchar ***argv, XError **error, xboolean *parsed)
{
    xsize j;

    for (j = 0; j < group->n_entries; j++) {
        if (*idx >= *argc) {
            return TRUE;
        }

        if (aliased && (group->entries[j].flags & X_OPTION_FLAG_NOALIAS)) {
            continue;
        }

        if (NO_ARG(&group->entries[j]) && strcmp(arg, group->entries[j].long_name) == 0) {
            xboolean retval;
            xchar *option_name;

            option_name = x_strconcat("--", group->entries[j].long_name, NULL);
            retval = parse_arg(context, group, &group->entries[j], NULL, option_name, error);
            x_free(option_name);

            add_pending_null(context, &((*argv)[*idx]), NULL);
            *parsed = TRUE;

            return retval;
        } else {
            xint len = strlen(group->entries[j].long_name);

            if (strncmp(arg, group->entries[j].long_name, len) == 0 && (arg[len] == '=' || arg[len] == 0)) {
                xchar *value = NULL;
                xchar *option_name;

                add_pending_null(context, &((*argv)[*idx]), NULL);
                option_name = x_strconcat("--", group->entries[j].long_name, NULL);

                if (arg[len] == '=') {
                    value = arg + len + 1;
                } else if (*idx < *argc - 1) {
                    if (!OPTIONAL_ARG(&group->entries[j])) {
                        value = (*argv)[*idx + 1];
                        add_pending_null(context, &((*argv)[*idx + 1]), NULL);
                        (*idx)++;
                    } else {
                        if ((*argv)[*idx + 1][0] == '-') {
                            xboolean retval;
                            retval = parse_arg(context, group, &group->entries[j], NULL, option_name, error);
                            *parsed = TRUE;
                            x_free(option_name);
                            return retval;
                        } else {
                            value = (*argv)[*idx + 1];
                            add_pending_null(context, &((*argv)[*idx + 1]), NULL);
                            (*idx)++;
                        }
                    }
                } else if (*idx >= *argc - 1 && OPTIONAL_ARG(&group->entries[j])) {
                        xboolean retval;
                        retval = parse_arg(context, group, &group->entries[j], NULL, option_name, error);
                        *parsed = TRUE;
                        x_free(option_name);
                        return retval;
                } else {
                    x_set_error(error, X_OPTION_ERROR, X_OPTION_ERROR_BAD_VALUE, _("Missing argument for %s"), option_name);
                    x_free(option_name);
                    return FALSE;
                }

                if (!parse_arg(context, group, &group->entries[j], value, option_name, error)) {
                    x_free(option_name);
                    return FALSE;
                }

                x_free(option_name);
                *parsed = TRUE;
            }
        }
    }

    return TRUE;
}

static xboolean parse_remaining_arg(XOptionContext *context, XOptionGroup *group, xint *idx, xint *argc, xchar ***argv, XError **error, xboolean *parsed)
{
    xsize j;

    for (j = 0; j < group->n_entries; j++) {
        if (*idx >= *argc) {
            return TRUE;
        }

        if (group->entries[j].long_name[0]) {
            continue;
        }

        x_return_val_if_fail(group->entries[j].arg == X_OPTION_ARG_CALLBACK || group->entries[j].arg == X_OPTION_ARG_STRING_ARRAY || group->entries[j].arg == X_OPTION_ARG_FILENAME_ARRAY, FALSE);
        add_pending_null(context, &((*argv)[*idx]), NULL);

        if (!parse_arg(context, group, &group->entries[j], (*argv)[*idx], "", error)) {
            return FALSE;
        }

        *parsed = TRUE;
        return TRUE;
    }

    return TRUE;
}

static void free_changes_list(XOptionContext *context, xboolean revert)
{
    XList *list;

    for (list = context->changes; list != NULL; list = list->next) {
        Change *change = (Change *)list->data;

        if (revert) {
            switch (change->arg_type) {
                case X_OPTION_ARG_NONE:
                    *(xboolean *)change->arg_data = change->prev.boolt;
                    break;

                case X_OPTION_ARG_INT:
                    *(xint *)change->arg_data = change->prev.integer;
                    break;

                case X_OPTION_ARG_STRING:
                case X_OPTION_ARG_FILENAME:
                    x_free(change->allocated.str);
                    *(xchar **)change->arg_data = change->prev.str;
                    break;

                case X_OPTION_ARG_STRING_ARRAY:
                case X_OPTION_ARG_FILENAME_ARRAY:
                    x_strfreev(change->allocated.array.data);
                    *(xchar ***)change->arg_data = change->prev.array;
                    break;

                case X_OPTION_ARG_DOUBLE:
                    *(xdouble *)change->arg_data = change->prev.dbl;
                    break;

                case X_OPTION_ARG_INT64:
                    *(xint64 *)change->arg_data = change->prev.int64;
                    break;

                default:
                x_assert_not_reached();
            }
        }

        x_free(change);
    }

    x_list_free(context->changes);
    context->changes = NULL;
}

static void free_pending_nulls(XOptionContext *context, xboolean perform_nulls)
{
    XList *list;

    for (list = context->pending_nulls; list != NULL; list = list->next) {
        PendingNull *n = (PendingNull *)list->data;

        if (perform_nulls) {
            if (n->value) {
                *(n->ptr)[0] = '-';
                strcpy(*n->ptr + 1, n->value);
            } else {
                if (context->strv_mode) {
                    x_free(*n->ptr);
                }

                *n->ptr = NULL;
            }
        }

        x_free(n->value);
        x_free(n);
    }

    x_list_free(context->pending_nulls);
    context->pending_nulls = NULL;
}

static char *platform_get_argv0(void)
{
#ifdef HAVE_PROC_SELF_CMDLINE
    xsize len;
    char *cmdline;
    char *base_arg0;

    if (!x_file_get_contents("/proc/self/cmdline", &cmdline, &len, NULL)) {
        return NULL;
    }

    x_assert(memchr(cmdline, 0, len + 1));

    base_arg0 = x_path_get_basename(cmdline);
    x_free(cmdline);
    return base_arg0;
#endif

    return NULL;
}

xboolean x_option_context_parse(XOptionContext *context, xint *argc, xchar ***argv, XError **error)
{
    xint i, j, k;
    XList *list;

    x_return_val_if_fail(context != NULL, FALSE);

    if (!x_get_prgname()) {
        xchar *prgname;

        if (argc && argv && *argc) {
            prgname = x_path_get_basename((*argv)[0]);
        } else {
            prgname = platform_get_argv0();
        }

        x_set_prgname_once(prgname ? prgname : "<unknown>");
        x_free(prgname);
    }

    list = context->groups;
    while (list) {
        XOptionGroup *group = (XOptionGroup *)list->data;

        if (group->pre_parse_func) {
            if (!(* group->pre_parse_func)(context, group, group->user_data, error)) {
                goto fail;
            }
        }

        list = list->next;
    }

    if (context->main_group && context->main_group->pre_parse_func) {
        if (!(* context->main_group->pre_parse_func)(context, context->main_group, context->main_group->user_data, error)) {
            goto fail;
        }
    }

    if (argc && argv) {
        xint separator_pos = 0;
        xboolean has_unknown = FALSE;
        xboolean stop_parsing = FALSE;

        for (i = 1; i < *argc; i++) {
            xchar *arg, *dash;
            xboolean parsed = FALSE;

            if ((*argv)[i][0] == '-' && (*argv)[i][1] != '\0' && !stop_parsing) {
                if ((*argv)[i][1] == '-') {
                    arg = (*argv)[i] + 2;
                    if (*arg == 0) {
                        separator_pos = i;
                        stop_parsing = TRUE;
                        continue;
                    }

                    if (context->help_enabled) {
                        if (strcmp(arg, "help") == 0) {
                            print_help(context, TRUE, NULL);
                        } else if (strcmp(arg, "help-all") == 0) {
                            print_help(context, FALSE, NULL);
                        } else if (strncmp(arg, "help-", 5) == 0) {
                            list = context->groups;

                            while (list) {
                                XOptionGroup *group = (XOptionGroup *)list->data;

                                if (strcmp(arg + 5, group->name) == 0) {
                                    print_help(context, FALSE, group);
                                }

                                list = list->next;
                            }
                        }
                    }

                    if (context->main_group && !parse_long_option(context, context->main_group, &i, arg, FALSE, argc, argv, error, &parsed)) {
                        goto fail;
                    }

                    if (parsed) {
                        continue;
                    }

                    list = context->groups;
                    while (list) {
                        XOptionGroup *group = (XOptionGroup *)list->data;

                        if (!parse_long_option(context, group, &i, arg, FALSE, argc, argv, error, &parsed)) {
                            goto fail;
                        }

                        if (parsed) {
                            break;
                        }
                        list = list->next;
                    }

                    if (parsed) {
                        continue;
                    }

                    dash = strchr (arg, '-');
                    if (dash && arg < dash) {
                        list = context->groups;
                        while (list) {
                            XOptionGroup *group = (XOptionGroup *)list->data;

                            if (strncmp(group->name, arg, dash - arg) == 0) {
                                if (!parse_long_option(context, group, &i, dash + 1, TRUE, argc, argv, error, &parsed)) {
                                    goto fail;
                                }

                                if (parsed) {
                                    break;
                                }
                            }

                            list = list->next;
                        }
                    }

                    if (context->ignore_unknown) {
                        continue;
                    }
                } else {
                    xint new_i = i, arg_length;
                    xboolean *nulled_out = NULL;
                    xboolean has_h_entry = context_has_h_entry(context);

                    arg = (*argv)[i] + 1;
                    arg_length = strlen(arg);
                    nulled_out = x_newa0(xboolean, arg_length);
    
                    for (j = 0; j < arg_length; j++) {
                        if (context->help_enabled && (arg[j] == '?' || (arg[j] == 'h' && !has_h_entry))) {
                            print_help(context, TRUE, NULL);
                        }

                        parsed = FALSE;
                        if (context->main_group && !parse_short_option(context, context->main_group, i, &new_i, arg[j], argc, argv, error, &parsed)) {
                            goto fail;
                        }

                        if (!parsed) {
                            list = context->groups;
                            while (list) {
                                XOptionGroup *group = (XOptionGroup *)list->data;
                                if (!parse_short_option(context, group, i, &new_i, arg[j], argc, argv, error, &parsed)) {
                                    goto fail;
                                }

                                if (parsed) {
                                    break;
                                }
                                list = list->next;
                            }
                        }

                        if (context->ignore_unknown && parsed) {
                            nulled_out[j] = TRUE;
                        } else if (context->ignore_unknown) {
                            continue;
                        } else if (!parsed) {
                            break;
                        }
                    }

                    if (context->ignore_unknown) {
                        xint arg_index = 0;
                        xchar *new_arg = NULL;

                        for (j = 0; j < arg_length; j++) {
                            if (!nulled_out[j]) {
                                if (!new_arg) {
                                    new_arg = (xchar *)x_malloc(arg_length + 1);
                                }

                                new_arg[arg_index++] = arg[j];
                            }
                        }

                        if (new_arg) {
                            new_arg[arg_index] = '\0';
                        }

                        add_pending_null(context, &((*argv)[i]), new_arg);
                        i = new_i;
                    } else if (parsed) {
                        add_pending_null(context, &((*argv)[i]), NULL);
                        i = new_i;
                    }
                }

                if (!parsed) {
                    has_unknown = TRUE;
                }

                if (!parsed && !context->ignore_unknown) {
                    x_set_error(error, X_OPTION_ERROR, X_OPTION_ERROR_UNKNOWN_OPTION, _("Unknown option %s"), (*argv)[i]);
                    goto fail;
                }
            } else {
                if (context->strict_posix) {
                    stop_parsing = TRUE;
                }

                if (context->main_group && !parse_remaining_arg(context, context->main_group, &i, argc, argv, error, &parsed)) {
                    goto fail;
                }

                if (!parsed && (has_unknown || (*argv)[i][0] == '-')) {
                    separator_pos = 0;
                }
            }
        }

        if (separator_pos > 0) {
            add_pending_null(context, &((*argv)[separator_pos]), NULL);
        }
    }

    list = context->groups;
    while (list) {
        XOptionGroup *group = (XOptionGroup *)list->data;

        if (group->post_parse_func) {
            if (!(* group->post_parse_func)(context, group, group->user_data, error)) {
                goto fail;
            }
        }

        list = list->next;
    }

    if (context->main_group && context->main_group->post_parse_func) {
        if (!(* context->main_group->post_parse_func)(context, context->main_group, context->main_group->user_data, error)) {
            goto fail;
        }
    }

    if (argc && argv) {
        free_pending_nulls(context, TRUE);

        for (i = 1; i < *argc; i++) {
            for (k = i; k < *argc; k++) {
                if ((*argv)[k] != NULL) {
                    break;
                }
            }

            if (k > i) {
                k -= i;
                for (j = i + k; j < *argc; j++) {
                    (*argv)[j-k] = (*argv)[j];
                    (*argv)[j] = NULL;
                }

                *argc -= k;
            }
        }
    }

    return TRUE;

fail:
    list = context->groups;
    while (list) {
        XOptionGroup *group = (XOptionGroup *)list->data;

        if (group->error_func) {
            (*group->error_func)(context, group, group->user_data, error);
        }

        list = list->next;
    }

    if (context->main_group && context->main_group->error_func) {
        (* context->main_group->error_func)(context, context->main_group, context->main_group->user_data, error);
    }

    free_changes_list(context, TRUE);
    free_pending_nulls(context, FALSE);

    return FALSE;
}

XOptionGroup *x_option_group_new(const xchar *name, const xchar *description, const xchar *help_description, xpointer user_data, XDestroyNotify destroy)
{
    XOptionGroup *group;

    group = x_new0(XOptionGroup, 1);
    group->ref_count = 1;
    group->name = x_strdup(name);
    group->description = x_strdup(description);
    group->help_description = x_strdup(help_description);
    group->user_data = user_data;
    group->destroy_notify = destroy;

    return group;
}

void x_option_group_free(XOptionGroup *group)
{
    x_option_group_unref(group);
}

XOptionGroup *x_option_group_ref(XOptionGroup *group)
{
    x_return_val_if_fail(group != NULL, NULL);

    group->ref_count++;
    return group;
}

void x_option_group_unref(XOptionGroup *group)
{
    x_return_if_fail(group != NULL);

    if (--group->ref_count == 0) {
        x_free(group->name);
        x_free(group->description);
        x_free(group->help_description);

        x_free(group->entries);

        if (group->destroy_notify) {
            (* group->destroy_notify)(group->user_data);
        }

        if (group->translate_notify) {
            (* group->translate_notify)(group->translate_data);
        }

        x_free(group);
    }
}

void x_option_group_add_entries(XOptionGroup *group, const XOptionEntry *entries)
{
    xsize i, n_entries;

    x_return_if_fail(group != NULL);
    x_return_if_fail(entries != NULL);

    for (n_entries = 0; entries[n_entries].long_name != NULL; n_entries++);

    x_return_if_fail(n_entries <= X_MAXSIZE - group->n_entries);

    group->entries = x_renew(XOptionEntry, group->entries, group->n_entries + n_entries);

    if (n_entries != 0) {
        memcpy(group->entries + group->n_entries, entries, sizeof(XOptionEntry) * n_entries);
    }

    for (i = group->n_entries; i < group->n_entries + n_entries; i++) {
        xchar c = group->entries[i].short_name;

        if (c == '-' || (c != 0 && !x_ascii_isprint(c))) {
            x_warning(X_STRLOC ": ignoring invalid short option '%c' (%d) in entry %s:%s", c, c, group->name, group->entries[i].long_name);
            group->entries[i].short_name = '\0';
        }

        if (group->entries[i].arg != X_OPTION_ARG_NONE && (group->entries[i].flags & X_OPTION_FLAG_REVERSE) != 0) {
            x_warning(X_STRLOC ": ignoring reverse flag on option of arg-type %d in entry %s:%s", group->entries[i].arg, group->name, group->entries[i].long_name);
            group->entries[i].flags &= ~X_OPTION_FLAG_REVERSE;
        }

        if (group->entries[i].arg != X_OPTION_ARG_CALLBACK && (group->entries[i].flags & (X_OPTION_FLAG_NO_ARG|X_OPTION_FLAG_OPTIONAL_ARG|X_OPTION_FLAG_FILENAME)) != 0) {
            x_warning(X_STRLOC ": ignoring no-arg, optional-arg or filename flags (%d) on option of arg-type %d in entry %s:%s", group->entries[i].flags, group->entries[i].arg, group->name, group->entries[i].long_name);
            group->entries[i].flags &= ~(X_OPTION_FLAG_NO_ARG|X_OPTION_FLAG_OPTIONAL_ARG|X_OPTION_FLAG_FILENAME);
        }
    }

    group->n_entries += n_entries;
}

void x_option_group_set_parse_hooks(XOptionGroup *group, XOptionParseFunc pre_parse_func, XOptionParseFunc post_parse_func)
{
    x_return_if_fail(group != NULL);

    group->pre_parse_func = pre_parse_func;
    group->post_parse_func = post_parse_func;
}

void x_option_group_set_error_hook(XOptionGroup *group, XOptionErrorFunc error_func)
{
    x_return_if_fail(group != NULL);
    group->error_func = error_func;
}

void x_option_group_set_translate_func(XOptionGroup *group, XTranslateFunc func, xpointer data, XDestroyNotify destroy_notify)
{
    x_return_if_fail(group != NULL);

    if (group->translate_notify) {
        group->translate_notify(group->translate_data);
    }

    group->translate_func = func;
    group->translate_data = data;
    group->translate_notify = destroy_notify;
}

static const xchar *dgettext_swapped(const xchar *msgid, const xchar *domainname)
{
    return x_dgettext(domainname, msgid);
}

void x_option_group_set_translation_domain(XOptionGroup *group, const xchar *domain)
{
    x_return_if_fail(group != NULL);
    x_option_group_set_translate_func(group, (XTranslateFunc)dgettext_swapped, x_strdup(domain), x_free);
}

void x_option_context_set_translate_func(XOptionContext *context, XTranslateFunc func, xpointer data, XDestroyNotify destroy_notify)
{
    x_return_if_fail(context != NULL);

    if (context->translate_notify) {
        context->translate_notify(context->translate_data);
    }

    context->translate_func = func;
    context->translate_data = data;
    context->translate_notify = destroy_notify;
}

void x_option_context_set_translation_domain(XOptionContext *context, const xchar *domain)
{
    x_return_if_fail(context != NULL);
    x_option_context_set_translate_func(context, (XTranslateFunc)dgettext_swapped, x_strdup(domain), x_free);
}

void x_option_context_set_summary(XOptionContext *context, const xchar *summary)
{
    x_return_if_fail(context != NULL);

    x_free(context->summary);
    context->summary = x_strdup(summary);
}

const xchar *x_option_context_get_summary(XOptionContext *context)
{
    x_return_val_if_fail(context != NULL, NULL);
    return context->summary;
}

void x_option_context_set_description(XOptionContext *context, const xchar *description)
{
    x_return_if_fail(context != NULL);

    x_free(context->description);
    context->description = x_strdup(description);
}

const xchar *x_option_context_get_description(XOptionContext *context)
{
    x_return_val_if_fail(context != NULL, NULL);
    return context->description;
}

xboolean x_option_context_parse_strv(XOptionContext *context, xchar ***arguments, XError **error)
{
    xint argc;
    xboolean success;

    x_return_val_if_fail(context != NULL, FALSE);

    context->strv_mode = TRUE;
    argc = arguments && *arguments ? x_strv_length(*arguments) : 0;
    success = x_option_context_parse(context, &argc, arguments, error);
    context->strv_mode = FALSE;

    return success;
}
