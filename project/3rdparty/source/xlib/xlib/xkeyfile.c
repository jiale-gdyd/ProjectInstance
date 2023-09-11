#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <locale.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifndef O_CLOEXEC
#define O_CLOEXEC       0
#endif

#include <xlib/xlib/config.h>
#include <xlib/xlib/xmem.h>
#include <xlib/xlib/xhash.h>
#include <xlib/xlib/xlist.h>
#include <xlib/xlib/xslist.h>
#include <xlib/xlib/xutils.h>
#include <xlib/xlib/xstdio.h>
#include <xlib/xlib/xstring.h>
#include <xlib/xlib/xkeyfile.h>
#include <xlib/xlib/xconvert.h>
#include <xlib/xlib/xdataset.h>
#include <xlib/xlib/xlibintl.h>
#include <xlib/xlib/xmessages.h>
#include <xlib/xlib/xstrfuncs.h>
#include <xlib/xlib/xfileutils.h>

typedef struct _XKeyFileGroup XKeyFileGroup;

struct _XKeyFile {
    XList         *groups;
    XHashTable    *group_hash;
    XKeyFileGroup *start_group;
    XKeyFileGroup *current_group;
    XString       *parse_buffer;
    xchar         list_separator;
    XKeyFileFlags flags;
    xboolean      checked_locales;
    xchar         **locales;
    xint          ref_count;
};

typedef struct _XKeyFileKeyValuePair XKeyFileKeyValuePair;

struct _XKeyFileGroup {
    const xchar          *name;
    XList                *key_value_pairs;
    XHashTable           *lookup_map;
};

struct _XKeyFileKeyValuePair {
    xchar *key;
    xchar *value;
};

static xint find_file_in_data_dirs(const xchar *file, const xchar **data_dirs, xchar **output_file, XError **error);

static xboolean x_key_file_load_from_fd(XKeyFile *key_file, xint fd, XKeyFileFlags flags, XError **error);

static XList *x_key_file_lookup_group_node(XKeyFile *key_file, const xchar *group_name);
static XKeyFileGroup *x_key_file_lookup_group(XKeyFile *key_file, const xchar *group_name);

static XList *x_key_file_lookup_key_value_pair_node(XKeyFile *key_file, XKeyFileGroup *group, const xchar *key);
static XKeyFileKeyValuePair *x_key_file_lookup_key_value_pair(XKeyFile *key_file, XKeyFileGroup *group, const xchar *key);

static void x_key_file_remove_group_node(XKeyFile *key_file, XList *group_node);
static void x_key_file_remove_key_value_pair_node(XKeyFile *key_file, XKeyFileGroup *group, XList *pair_node);

static void x_key_file_add_key_value_pair(XKeyFile *key_file, XKeyFileGroup *group, XKeyFileKeyValuePair *pair, XList *sibling);
static void x_key_file_add_key(XKeyFile *key_file, XKeyFileGroup *group, const xchar *key, const xchar *value);

static void x_key_file_add_group(XKeyFile *key_file, const xchar *group_name, xboolean created);
static xboolean x_key_file_is_group_name(const xchar *name);
static xboolean x_key_file_is_key_name(const xchar *name, xsize len);

static void x_key_file_key_value_pair_free(XKeyFileKeyValuePair *pair);
static xboolean x_key_file_line_is_comment(const xchar *line);
static xboolean x_key_file_line_is_group(const xchar *line);
static xboolean x_key_file_line_is_key_value_pair(const xchar *line);

static xchar *x_key_file_parse_value_as_string(XKeyFile *key_file, const xchar *value, XSList **separators, XError **error);
static xchar *x_key_file_parse_string_as_value(XKeyFile *key_file, const xchar *string, xboolean escape_separator);
static xint x_key_file_parse_value_as_integer(XKeyFile *key_file, const xchar *value, XError **error);
static xchar *x_key_file_parse_integer_as_value(XKeyFile *key_file, xint value);
static xdouble x_key_file_parse_value_as_double(XKeyFile *key_file, const xchar *value, XError **error);
static xboolean x_key_file_parse_value_as_boolean(XKeyFile *key_file, const xchar *value, XError **error);
static const xchar *x_key_file_parse_boolean_as_value(XKeyFile *key_file, xboolean value);
static xchar *x_key_file_parse_value_as_comment(XKeyFile *key_file, const xchar *value, xboolean is_final_line);
static xchar *x_key_file_parse_comment_as_value (XKeyFile *key_file, const xchar *comment);
static void x_key_file_parse_key_value_pair(XKeyFile *key_file, const xchar *line, xsize length, XError **error);
static void x_key_file_parse_comment(XKeyFile *key_file, const xchar *line, xsize length, XError **error);
static void x_key_file_parse_group(XKeyFile *key_file, const xchar *line, xsize length, XError **error);
static const xchar *key_get_locale(const xchar *key, xsize *len_out);
static void x_key_file_parse_data(XKeyFile *key_file, const xchar *data, xsize length, XError **error);
static void x_key_file_flush_parse_buffer(XKeyFile *key_file, XError **error);

X_DEFINE_QUARK(x-key-file-error-quark, x_key_file_error)

static void x_key_file_init(XKeyFile *key_file)
{
    key_file->current_group = x_new0(XKeyFileGroup, 1);
    key_file->groups = x_list_prepend(NULL, key_file->current_group);
    key_file->group_hash = NULL;
    key_file->start_group = NULL;
    key_file->parse_buffer = NULL;
    key_file->list_separator = ';';
    key_file->flags = (XKeyFileFlags)0;
}

static void x_key_file_clear(XKeyFile *key_file)
{
    XList *tmp, *group_node;

    if (key_file->locales)  {
        x_strfreev(key_file->locales);
        key_file->locales = NULL;
    }
    key_file->checked_locales = FALSE;

    if (key_file->parse_buffer) {
        x_string_free(key_file->parse_buffer, TRUE);
        key_file->parse_buffer = NULL;
    }

    tmp = key_file->groups;
    while (tmp != NULL) {
        group_node = tmp;
        tmp = tmp->next;
        x_key_file_remove_group_node(key_file, group_node);
    }

    if (key_file->group_hash != NULL) {
        x_hash_table_destroy(key_file->group_hash);
        key_file->group_hash = NULL;
    }

    x_warn_if_fail(key_file->groups == NULL);
}

XKeyFile *x_key_file_new(void)
{
    XKeyFile *key_file;

    key_file = x_new0(XKeyFile, 1);
    key_file->ref_count = 1;
    x_key_file_init(key_file);

    return key_file;
}

void x_key_file_set_list_separator(XKeyFile *key_file, xchar separator)
{
    x_return_if_fail(key_file != NULL);
    key_file->list_separator = separator;
}

static xint find_file_in_data_dirs(const xchar *file, const xchar **dirs, xchar **output_file, XError **error)
{
    xint fd;
    xchar *path;
    const xchar **data_dirs, *data_dir;

    path = NULL;
    fd = -1;

    if (dirs == NULL) {
        return fd;
    }

    data_dirs = dirs;
    while (data_dirs && (data_dir = *data_dirs) && fd == -1) {
        xchar *sub_dir;
        const xchar *candidate_file;

        candidate_file = file;
        sub_dir = x_strdup("");
        while (candidate_file != NULL && fd == -1) {
            xchar *p;

            path = x_build_filename(data_dir, sub_dir, candidate_file, NULL);
            fd = x_open(path, O_RDONLY | O_CLOEXEC, 0);
            if (fd == -1) {
                x_free(path);
                path = NULL;
            }

            candidate_file = strchr(candidate_file, '-');
            if (candidate_file == NULL) {
                break;
            }

            candidate_file++;

            x_free(sub_dir);
            sub_dir = x_strndup(file, candidate_file - file - 1);

            for (p = sub_dir; *p != '\0'; p++) {
                if (*p == '-') {
                    *p = X_DIR_SEPARATOR;
                }
            }
        }

        x_free(sub_dir);
        data_dirs++;
    }

    if (fd == -1) {
        x_set_error_literal(error, X_KEY_FILE_ERROR, X_KEY_FILE_ERROR_NOT_FOUND, _("Valid key file could not be found in search dirs"));
    }

    if (output_file != NULL && fd != -1) {
        *output_file = x_strdup(path);
    }
    x_free(path);

    return fd;
}

static xboolean x_key_file_load_from_fd(XKeyFile *key_file, xint fd, XKeyFileFlags flags, XError **error)
{
    xssize bytes_read;
    struct stat stat_buf;
    xchar read_buf[4096];
    xchar list_separator;
    XError *key_file_error = NULL;

    if (fstat(fd, &stat_buf) < 0) {
        int errsv = errno;
        x_set_error_literal(error, X_FILE_ERROR, x_file_error_from_errno(errsv), x_strerror(errsv));
        return FALSE;
    }

    if (!S_ISREG(stat_buf.st_mode)) {
        x_set_error_literal(error, X_KEY_FILE_ERROR, X_KEY_FILE_ERROR_PARSE, _("Not a regular file"));
        return FALSE;
    }

    list_separator = key_file->list_separator;
    x_key_file_clear(key_file);
    x_key_file_init(key_file);
    key_file->list_separator = list_separator;
    key_file->flags = flags;

    do {
        int errsv;

        bytes_read = read(fd, read_buf, 4096);
        errsv = errno;

        if (bytes_read == 0) {
            break;
        }

        if (bytes_read < 0) {
            if (errsv == EINTR || errsv == EAGAIN) {
                continue;
            }

            x_set_error_literal(error, X_FILE_ERROR, x_file_error_from_errno(errsv), x_strerror(errsv));
            return FALSE;
        }

        x_key_file_parse_data(key_file, read_buf, bytes_read, &key_file_error);
    } while (!key_file_error);

    if (key_file_error) {
        x_propagate_error(error, key_file_error);
        return FALSE;
    }

    x_key_file_flush_parse_buffer(key_file, &key_file_error);

    if (key_file_error) {
        x_propagate_error(error, key_file_error);
        return FALSE;
    }

    return TRUE;
}

xboolean x_key_file_load_from_file(XKeyFile *key_file, const xchar *file, XKeyFileFlags flags, XError **error)
{
    xint fd;
    int errsv;
    XError *key_file_error = NULL;

    x_return_val_if_fail(key_file != NULL, FALSE);
    x_return_val_if_fail(file != NULL, FALSE);

    fd = x_open(file, O_RDONLY | O_CLOEXEC, 0);
    errsv = errno;

    if (fd == -1) {
        x_set_error_literal(error, X_FILE_ERROR, x_file_error_from_errno(errsv), x_strerror(errsv));
        return FALSE;
    }

    x_key_file_load_from_fd(key_file, fd, flags, &key_file_error);
    close(fd);

    if (key_file_error) {
        x_propagate_error(error, key_file_error);
        return FALSE;
    }

    return TRUE;
}

xboolean x_key_file_load_from_data(XKeyFile *key_file, const xchar *data, xsize length, XKeyFileFlags flags, XError **error)
{
    xchar list_separator;
    XError *key_file_error = NULL;

    x_return_val_if_fail(key_file != NULL, FALSE);
    x_return_val_if_fail(data != NULL || length == 0, FALSE);

    if (length == (xsize)-1) {
        length = strlen(data);
    }

    list_separator = key_file->list_separator;
    x_key_file_clear(key_file);
    x_key_file_init(key_file);
    key_file->list_separator = list_separator;
    key_file->flags = flags;

    x_key_file_parse_data(key_file, data, length, &key_file_error);
    if (key_file_error) {
        x_propagate_error(error, key_file_error);
        return FALSE;
    }

    x_key_file_flush_parse_buffer(key_file, &key_file_error);
    if (key_file_error) {
        x_propagate_error(error, key_file_error);
        return FALSE;
    }

    return TRUE;
}

xboolean x_key_file_load_from_bytes(XKeyFile *key_file, XBytes *bytes, XKeyFileFlags flags, XError **error)
{
    xsize size;
    const xuchar *data;

    x_return_val_if_fail(key_file != NULL, FALSE);
    x_return_val_if_fail(bytes != NULL, FALSE);

    data = (const xuchar *)x_bytes_get_data(bytes, &size);
    return x_key_file_load_from_data(key_file, (const xchar *)data, size, flags, error);
}

xboolean x_key_file_load_from_dirs(XKeyFile *key_file, const xchar *file, const xchar **search_dirs, xchar **full_path, XKeyFileFlags flags, XError **error)
{
    xint fd;
    xchar *output_path;
    xboolean found_file;
    const xchar **data_dirs;
    XError *key_file_error = NULL;

    x_return_val_if_fail(key_file != NULL, FALSE);
    x_return_val_if_fail(!x_path_is_absolute(file), FALSE);
    x_return_val_if_fail(search_dirs != NULL, FALSE);

    found_file = FALSE;
    data_dirs = search_dirs;
    output_path = NULL;

    while (*data_dirs != NULL && !found_file) {
        x_free(output_path);
        output_path = NULL;

        fd = find_file_in_data_dirs(file, data_dirs, &output_path, &key_file_error);
        if (fd == -1) {
            if (key_file_error) {
                x_propagate_error(error, key_file_error);
            }

            break;
        }

        found_file = x_key_file_load_from_fd(key_file, fd, flags, &key_file_error);
        close(fd);

        if (key_file_error) {
            x_propagate_error(error, key_file_error);
            break;
        }
    }

    if (found_file && full_path) {
        *full_path = output_path;
    } else {
        x_free(output_path);
    }

    return found_file;
}

xboolean x_key_file_load_from_data_dirs(XKeyFile *key_file, const xchar *file, xchar **full_path, XKeyFileFlags flags, XError **error)
{
    xsize i, j;
    xboolean found_file;
    xchar **all_data_dirs;
    const xchar *user_data_dir;
    const xchar *const *system_data_dirs;

    x_return_val_if_fail(key_file != NULL, FALSE);
    x_return_val_if_fail(!x_path_is_absolute(file), FALSE);

    user_data_dir = x_get_user_data_dir();
    system_data_dirs = x_get_system_data_dirs();
    all_data_dirs = x_new(xchar *, x_strv_length((xchar **)system_data_dirs) + 2);

    i = 0;
    all_data_dirs[i++] = x_strdup(user_data_dir);

    j = 0;
    while (system_data_dirs[j] != NULL) {
        all_data_dirs[i++] = x_strdup(system_data_dirs[j++]);
    }
    all_data_dirs[i] = NULL;

    found_file = x_key_file_load_from_dirs(key_file, file, (const xchar **)all_data_dirs, full_path, flags, error);
    x_strfreev(all_data_dirs);

    return found_file;
}

XKeyFile *x_key_file_ref(XKeyFile *key_file)
{
    x_return_val_if_fail(key_file != NULL, NULL);
    x_atomic_int_inc(&key_file->ref_count);

    return key_file;
}

void x_key_file_free(XKeyFile *key_file)
{
    x_return_if_fail(key_file != NULL);

    x_key_file_clear(key_file);
    if (x_atomic_int_dec_and_test(&key_file->ref_count)) {
        x_free_sized(key_file, sizeof(XKeyFile));
    } else {
        x_key_file_init(key_file);
    }
}

void x_key_file_unref(XKeyFile *key_file)
{
    x_return_if_fail(key_file != NULL);

    if (x_atomic_int_dec_and_test(&key_file->ref_count)) {
        x_key_file_clear(key_file);
        x_free_sized(key_file, sizeof(XKeyFile));
    }
}

static xboolean x_key_file_locale_is_interesting(XKeyFile *key_file, const xchar *locale, xsize locale_len)
{
    xsize i;

    if (key_file->flags & X_KEY_FILE_KEEP_TRANSLATIONS) {
        return TRUE;
    }

    if (!key_file->checked_locales) {
        x_assert(key_file->locales == NULL);
        key_file->locales = x_strdupv((xchar **)x_get_language_names());
        key_file->checked_locales = TRUE;
    }

    for (i = 0; key_file->locales[i] != NULL; i++) {
        if (x_ascii_strncasecmp(key_file->locales[i], locale, locale_len) == 0 && key_file->locales[i][locale_len] == '\0') {
            return TRUE;
        }
    }

    return FALSE;
}

static void x_key_file_parse_line(XKeyFile *key_file, const xchar *line, xsize length, XError **error)
{
    const xchar *line_start;
    XError *parse_error = NULL;

    x_return_if_fail(key_file != NULL);
    x_return_if_fail(line != NULL);

    line_start = line;
    while (x_ascii_isspace(*line_start)) {
        line_start++;
    }

    if (x_key_file_line_is_comment(line_start)) {
        x_key_file_parse_comment(key_file, line, length, &parse_error);
    } else if (x_key_file_line_is_group(line_start)) {
        x_key_file_parse_group(key_file, line_start, length - (line_start - line), &parse_error);
    } else if (x_key_file_line_is_key_value_pair(line_start)) {
        x_key_file_parse_key_value_pair(key_file, line_start, length - (line_start - line), &parse_error);
    } else {
        xchar *line_utf8 = x_utf8_make_valid(line, length);
        x_set_error(error, X_KEY_FILE_ERROR, X_KEY_FILE_ERROR_PARSE, _("Key file contains line “%s” which is not a key-value pair, group, or comment"), line_utf8);
        x_free(line_utf8);

        return;
    }

    if (parse_error) {
        x_propagate_error(error, parse_error);
    }
}

static void x_key_file_parse_comment(XKeyFile *key_file, const xchar *line, xsize length, XError **error)
{
    XKeyFileKeyValuePair *pair;

    if (!(key_file->flags & X_KEY_FILE_KEEP_COMMENTS)) {
        return;
    }

    x_warn_if_fail(key_file->current_group != NULL);

    pair = x_new(XKeyFileKeyValuePair, 1);
    pair->key = NULL;
    pair->value = x_strndup(line, length);
    key_file->current_group->key_value_pairs = x_list_prepend(key_file->current_group->key_value_pairs, pair);
}

static void x_key_file_parse_group(XKeyFile *key_file, const xchar *line, xsize length, XError **error)
{
    xchar *group_name;
    const xchar *group_name_start, *group_name_end;

    group_name_start = line + 1;
    group_name_end = line + length - 1;

    while (*group_name_end != ']') {
        group_name_end--;
    }

    group_name = x_strndup(group_name_start,  group_name_end - group_name_start);

    if (!x_key_file_is_group_name(group_name)) {
        x_set_error(error, X_KEY_FILE_ERROR, X_KEY_FILE_ERROR_PARSE, _("Invalid group name: %s"), group_name);
        x_free(group_name);
        return;
    }

    x_key_file_add_group(key_file, group_name, FALSE);
    x_free(group_name);
}

static void x_key_file_parse_key_value_pair(XKeyFile *key_file, const xchar *line, xsize length, XError **error)
{
    xsize locale_len;
    const xchar *locale;
    xsize key_len, value_len;
    xchar *key, *key_end, *value_start;

    if (key_file->current_group == NULL || key_file->current_group->name == NULL) {
        x_set_error_literal(error, X_KEY_FILE_ERROR, X_KEY_FILE_ERROR_GROUP_NOT_FOUND, _("Key file does not start with a group"));
        return;
    }

    key_end = value_start = (xchar *)strchr(line, '=');
    x_warn_if_fail(key_end != NULL);

    key_end--;
    value_start++;

    while (x_ascii_isspace(*key_end)) {
        key_end--;
    }

    key_len = key_end - line + 2;
    x_warn_if_fail(key_len <= length);

    if (!x_key_file_is_key_name(line, key_len - 1)) {
        x_set_error(error, X_KEY_FILE_ERROR, X_KEY_FILE_ERROR_PARSE, _("Invalid key name: %.*s"), (int) key_len - 1, line);
        return; 
    }

    key = x_strndup(line, key_len - 1);
    while (x_ascii_isspace(*value_start)) {
        value_start++;
    }

    value_len = line + length - value_start;
    x_warn_if_fail(key_file->start_group != NULL);

    x_assert(key_file->current_group != NULL);
    x_assert(key_file->current_group->name != NULL);

    if (key_file->start_group == key_file->current_group && strcmp (key, "Encoding") == 0) {
        if (value_len != strlen("UTF-8") || x_ascii_strncasecmp(value_start, "UTF-8", value_len) != 0) {
            xchar *value_utf8 = x_utf8_make_valid(value_start, value_len);
            x_set_error(error, X_KEY_FILE_ERROR, X_KEY_FILE_ERROR_UNKNOWN_ENCODING, _("Key file contains unsupported encoding “%s”"), value_utf8);
            x_free(value_utf8);
            x_free(key);
            return;
        }
    }

    locale = key_get_locale(key, &locale_len);
    if (locale == NULL || x_key_file_locale_is_interesting(key_file, locale, locale_len)) {
        XKeyFileKeyValuePair *pair;

        pair = x_new(XKeyFileKeyValuePair, 1);
        pair->key = x_steal_pointer(&key);
        pair->value = x_strndup(value_start, value_len);

        x_key_file_add_key_value_pair(key_file, key_file->current_group, pair, key_file->current_group->key_value_pairs);
    }

    x_free(key);
}

static const xchar *key_get_locale(const xchar *key, xsize *len_out)
{
    xsize locale_len;
    const xchar *locale;

    locale = x_strrstr(key, "[");
    if (locale != NULL) {
        locale_len = strlen(locale);
    } else {
        locale_len = 0;
    }

    if (locale_len > 2) {
        locale++;
        locale_len -= 2;
    } else {
        locale = NULL;
        locale_len = 0;
    }

    *len_out = locale_len;
    return locale;
}

static void x_key_file_parse_data(XKeyFile *key_file, const xchar *data, xsize length, XError **error)
{
    xsize i;
    XError *parse_error;

    x_return_if_fail(key_file != NULL);
    x_return_if_fail(data != NULL || length == 0);

    parse_error = NULL;

    if (!key_file->parse_buffer) {
        key_file->parse_buffer = x_string_sized_new(128);
    }

    i = 0;
    while (i < length) {
        if (data[i] == '\n') {
            if (key_file->parse_buffer->len > 0 && (key_file->parse_buffer->str[key_file->parse_buffer->len - 1] == '\r')) {
                x_string_erase(key_file->parse_buffer, key_file->parse_buffer->len - 1, 1);
            }

            if (key_file->parse_buffer->len > 0) {
                x_key_file_flush_parse_buffer(key_file, &parse_error);
            } else {
                x_key_file_parse_comment(key_file, "", 1, &parse_error);
            }

            if (parse_error) {
                x_propagate_error(error, parse_error);
                return;
            }

            i++;
        } else {
            xsize line_length;
            const xchar *end_of_line;
            const xchar *start_of_line;

            start_of_line = data + i;
            end_of_line = (const xchar *)memchr(start_of_line, '\n', length - i);

            if (end_of_line == NULL) {
                end_of_line = data + length;
            }
            line_length = end_of_line - start_of_line;

            x_string_append_len(key_file->parse_buffer, start_of_line, line_length);
            i += line_length;
        }
    }
}

static void x_key_file_flush_parse_buffer(XKeyFile *key_file, XError **error)
{
    XError *file_error = NULL;

    x_return_if_fail(key_file != NULL);

    if (!key_file->parse_buffer) {
        return;
    }

    file_error = NULL;
    if (key_file->parse_buffer->len > 0) {
        x_key_file_parse_line(key_file, key_file->parse_buffer->str, key_file->parse_buffer->len, &file_error);
        x_string_erase(key_file->parse_buffer, 0, -1);

        if (file_error) {
            x_propagate_error(error, file_error);
            return;
        }
    }
}

xchar *x_key_file_to_data(XKeyFile *key_file, xsize *length, XError **error)
{
    XString *data_string;
    XList *group_node, *key_file_node;

    x_return_val_if_fail(key_file != NULL, NULL);

    data_string = x_string_new(NULL);

    for (group_node = x_list_last(key_file->groups); group_node != NULL; group_node = group_node->prev) {
        XKeyFileGroup *group;

        group = (XKeyFileGroup *)group_node->data;
        if (group->name != NULL) {
            x_string_append_printf(data_string, "[%s]\n", group->name);
        }

        for (key_file_node = x_list_last(group->key_value_pairs); key_file_node != NULL; key_file_node = key_file_node->prev) {
            XKeyFileKeyValuePair *pair;

            pair = (XKeyFileKeyValuePair *)key_file_node->data;
            if (pair->key != NULL) {
                x_string_append_printf(data_string, "%s=%s\n", pair->key, pair->value);
            } else {
                x_string_append_printf(data_string, "%s\n", pair->value);
            }
        }
    }

    if (length) {
        *length = data_string->len;
    }

    return x_string_free(data_string, FALSE);
}

xchar **x_key_file_get_keys(XKeyFile *key_file, const xchar *group_name, xsize *length, XError **error)
{
    XList *tmp;
    xchar **keys;
    xsize i, num_keys;
    XKeyFileGroup *group;

    x_return_val_if_fail(key_file != NULL, NULL);
    x_return_val_if_fail(group_name != NULL, NULL);

    group = x_key_file_lookup_group(key_file, group_name);
    if (!group) {
        x_set_error(error, X_KEY_FILE_ERROR, X_KEY_FILE_ERROR_GROUP_NOT_FOUND, _("Key file does not have group “%s”"), group_name);
        return NULL;
    }

    num_keys = 0;
    for (tmp = group->key_value_pairs; tmp; tmp = tmp->next) {
        XKeyFileKeyValuePair *pair;

        pair = (XKeyFileKeyValuePair *)tmp->data;
        if (pair->key) {
            num_keys++;
        }
    }

    keys = x_new(xchar *, num_keys + 1);

    i = num_keys - 1;
    for (tmp = group->key_value_pairs; tmp; tmp = tmp->next) {
        XKeyFileKeyValuePair *pair;

        pair = (XKeyFileKeyValuePair *)tmp->data;
        if (pair->key) {
            keys[i] = x_strdup(pair->key);
            i--;
        }
    }

    keys[num_keys] = NULL;
    if (length) {
        *length = num_keys;
    }

    return keys;
}

xchar *x_key_file_get_start_group(XKeyFile *key_file)
{
    x_return_val_if_fail(key_file != NULL, NULL);

    if (key_file->start_group) {
        return x_strdup(key_file->start_group->name);
    }

    return NULL;
}

xchar **x_key_file_get_groups(XKeyFile *key_file, xsize *length)
{
    xchar **groups;
    XList *group_node;
    xsize i, num_groups;

    x_return_val_if_fail(key_file != NULL, NULL);
    num_groups = x_list_length(key_file->groups);
    x_return_val_if_fail(num_groups > 0, NULL);
    group_node = x_list_last(key_file->groups);
    x_return_val_if_fail(((XKeyFileGroup *) group_node->data)->name == NULL, NULL);

    groups = x_new(xchar *, num_groups);

    i = 0;
    for (group_node = group_node->prev; group_node != NULL; group_node = group_node->prev) {
        XKeyFileGroup *group;

        group = (XKeyFileGroup *)group_node->data;
        x_warn_if_fail(group->name != NULL);

        groups[i++] = x_strdup(group->name);
    }
    groups[i] = NULL;

    if (length) {
        *length = i;
    }

    return groups;
}

static void set_not_found_key_error(const char *group_name, const char *key, XError **error)
{
    x_set_error(error, X_KEY_FILE_ERROR, X_KEY_FILE_ERROR_KEY_NOT_FOUND, _("Key file does not have key “%s” in group “%s”"), key, group_name);
}

xchar *x_key_file_get_value(XKeyFile *key_file, const xchar *group_name, const xchar *key, XError **error)
{
    xchar *value = NULL;
    XKeyFileGroup *group;
    XKeyFileKeyValuePair *pair;

    x_return_val_if_fail(key_file != NULL, NULL);
    x_return_val_if_fail(group_name != NULL, NULL);
    x_return_val_if_fail(key != NULL, NULL);

    group = x_key_file_lookup_group(key_file, group_name);

    if (!group) {
        x_set_error(error, X_KEY_FILE_ERROR, X_KEY_FILE_ERROR_GROUP_NOT_FOUND, _("Key file does not have group “%s”"), group_name);
        return NULL;
    }

    pair = x_key_file_lookup_key_value_pair(key_file, group, key);
    if (pair) {
        value = x_strdup(pair->value);
    } else {
        set_not_found_key_error(group_name, key, error);
    }

    return value;
}

void x_key_file_set_value(XKeyFile *key_file, const xchar *group_name, const xchar *key, const xchar *value)
{
    XKeyFileGroup *group;
    XKeyFileKeyValuePair *pair;

    x_return_if_fail(key_file != NULL);
    x_return_if_fail(group_name != NULL && x_key_file_is_group_name(group_name));
    x_return_if_fail(key != NULL && x_key_file_is_key_name(key, strlen(key)));
    x_return_if_fail(value != NULL);

    group = x_key_file_lookup_group(key_file, group_name);
    if (!group) {
        x_key_file_add_group(key_file, group_name, TRUE);
        group = (XKeyFileGroup *)key_file->groups->data;
        x_key_file_add_key(key_file, group, key, value);
    } else {
        pair = x_key_file_lookup_key_value_pair(key_file, group, key);
        if (!pair) {
            x_key_file_add_key(key_file, group, key, value);
        } else {
            x_free(pair->value);
            pair->value = x_strdup(value);
        }
    }
}

xchar *x_key_file_get_string(XKeyFile *key_file, const xchar *group_name, const xchar *key, XError **error)
{
    XError *key_file_error;
    xchar *value, *string_value;

    x_return_val_if_fail(key_file != NULL, NULL);
    x_return_val_if_fail(group_name != NULL, NULL);
    x_return_val_if_fail(key != NULL, NULL);

    key_file_error = NULL;
    value = x_key_file_get_value(key_file, group_name, key, &key_file_error);

    if (key_file_error) {
        x_propagate_error(error, key_file_error);
        return NULL;
    }

    if (!x_utf8_validate(value, -1, NULL)) {
        xchar *value_utf8 = x_utf8_make_valid(value, -1);
        x_set_error(error, X_KEY_FILE_ERROR, X_KEY_FILE_ERROR_UNKNOWN_ENCODING, _("Key file contains key “%s” with value “%s” which is not UTF-8"), key, value_utf8);
        x_free(value_utf8);
        x_free(value);

        return NULL;
    }

    string_value = x_key_file_parse_value_as_string(key_file, value, NULL, &key_file_error);
    x_free(value);

    if (key_file_error) {
        if (x_error_matches(key_file_error, X_KEY_FILE_ERROR, X_KEY_FILE_ERROR_INVALID_VALUE)) {
            x_set_error(error, X_KEY_FILE_ERROR, X_KEY_FILE_ERROR_INVALID_VALUE, _("Key file contains key “%s” which has a value that cannot be interpreted."), key);
            x_error_free(key_file_error);
        } else {
            x_propagate_error(error, key_file_error);
        }
    }

    return string_value;
}

void x_key_file_set_string(XKeyFile *key_file, const xchar *group_name, const xchar *key, const xchar *string)
{
    xchar *value;

    x_return_if_fail(key_file != NULL);
    x_return_if_fail(string != NULL);

    value = x_key_file_parse_string_as_value(key_file, string, FALSE);
    x_key_file_set_value(key_file, group_name, key, value);
    x_free(value);
}

xchar **x_key_file_get_string_list(XKeyFile *key_file, const xchar *group_name, const xchar *key, xsize *length, XError **error)
{
    xint i, len;
    XSList *p, *pieces = NULL;
    XError *key_file_error = NULL;
    xchar *value, *string_value, **values;

    x_return_val_if_fail(key_file != NULL, NULL);
    x_return_val_if_fail(group_name != NULL, NULL);
    x_return_val_if_fail(key != NULL, NULL);

    if (length) {
        *length = 0;
    }

    value = x_key_file_get_value(key_file, group_name, key, &key_file_error);

    if (key_file_error) {
        x_propagate_error(error, key_file_error);
        return NULL;
    }

    if (!x_utf8_validate(value, -1, NULL)) {
        xchar *value_utf8 = x_utf8_make_valid(value, -1);
        x_set_error(error, X_KEY_FILE_ERROR, X_KEY_FILE_ERROR_UNKNOWN_ENCODING, _("Key file contains key “%s” with value “%s” which is not UTF-8"), key, value_utf8);
        x_free(value_utf8);
        x_free(value);

        return NULL;
    }

    string_value = x_key_file_parse_value_as_string(key_file, value, &pieces, &key_file_error);
    x_free(value);
    x_free(string_value);

    if (key_file_error) {
        if (x_error_matches(key_file_error, X_KEY_FILE_ERROR, X_KEY_FILE_ERROR_INVALID_VALUE)) {
            x_set_error(error, X_KEY_FILE_ERROR, X_KEY_FILE_ERROR_INVALID_VALUE, _("Key file contains key “%s” which has a value that cannot be interpreted."), key);
            x_error_free(key_file_error);
        } else {
            x_propagate_error(error, key_file_error);
        }

        x_slist_free_full(pieces, x_free);
        return NULL;
    }

    len = x_slist_length(pieces);
    values = x_new(xchar *, len + 1);
    for (p = pieces, i = 0; p; p = p->next) {
        values[i++] = (xchar *)p->data;
    }
    values[len] = NULL;

    x_slist_free(pieces);

    if (length) {
        *length = len;
    }

    return values;
}

void x_key_file_set_string_list(XKeyFile *key_file, const xchar *group_name, const xchar *key, const xchar *const list[], xsize length)
{
    xsize i;
    XString *value_list;

    x_return_if_fail(key_file != NULL);
    x_return_if_fail(list != NULL || length == 0);

    value_list = x_string_sized_new(length * 128);
    for (i = 0; i < length && list[i] != NULL; i++) {
        xchar *value;

        value = x_key_file_parse_string_as_value(key_file, list[i], TRUE);
        x_string_append(value_list, value);
        x_string_append_c(value_list, key_file->list_separator);

        x_free(value);
    }

    x_key_file_set_value(key_file, group_name, key, value_list->str);
    x_string_free(value_list, TRUE);
}

void x_key_file_set_locale_string(XKeyFile *key_file, const xchar *group_name, const xchar *key, const xchar *locale, const xchar *string)
{
    xchar *full_key, *value;

    x_return_if_fail(key_file != NULL);
    x_return_if_fail(key != NULL);
    x_return_if_fail(locale != NULL);
    x_return_if_fail(string != NULL);

    value = x_key_file_parse_string_as_value(key_file, string, FALSE);
    full_key = x_strdup_printf("%s[%s]", key, locale);
    x_key_file_set_value(key_file, group_name, full_key, value);
    x_free(full_key);
    x_free(value);
}

xchar *x_key_file_get_locale_string(XKeyFile *key_file, const xchar *group_name, const xchar *key, const xchar *locale, XError **error)
{
    xint i;
    xchar **languages;
    XError *key_file_error;
    xboolean free_languages = FALSE;
    xchar *candidate_key, *translated_value;

    x_return_val_if_fail(key_file != NULL, NULL);
    x_return_val_if_fail(group_name != NULL, NULL);
    x_return_val_if_fail(key != NULL, NULL);

    candidate_key = NULL;
    translated_value = NULL;
    key_file_error = NULL;

    if (locale) {
        languages = x_get_locale_variants(locale);
        free_languages = TRUE;
    } else {
        languages = (xchar **)x_get_language_names();
        free_languages = FALSE;
    }

    for (i = 0; languages[i]; i++) {
        candidate_key = x_strdup_printf("%s[%s]", key, languages[i]);
        translated_value = x_key_file_get_string(key_file, group_name, candidate_key, NULL);
        x_free(candidate_key);

        if (translated_value) {
            break;
        }
    }

    if (!translated_value) {
        translated_value = x_key_file_get_string(key_file, group_name, key, &key_file_error);
        if (!translated_value) {
            x_propagate_error(error, key_file_error);
        }
    }

    if (free_languages) {
        x_strfreev(languages);
    }

    return translated_value;
}

xchar *x_key_file_get_locale_for_key(XKeyFile *key_file, const xchar *group_name, const xchar *key, const xchar *locale)
{
    xsize i;
    xchar *result = NULL;
    const xchar *const *languages;
    xchar **languages_allocated = NULL;

    x_return_val_if_fail(key_file != NULL, NULL);
    x_return_val_if_fail(group_name != NULL, NULL);
    x_return_val_if_fail(key != NULL, NULL);

    if (locale != NULL) {
        languages_allocated = x_get_locale_variants(locale);
        languages = (const xchar *const *)languages_allocated;
    } else {
        languages = x_get_language_names();
    }

    for (i = 0; languages[i] != NULL; i++) {
        xchar *candidate_key, *translated_value;

        candidate_key = x_strdup_printf("%s[%s]", key, languages[i]);
        translated_value = x_key_file_get_string(key_file, group_name, candidate_key, NULL);
        x_free(translated_value);
        x_free(candidate_key);

        if (translated_value != NULL) {
            break;
        }
    }

    result = x_strdup(languages[i]);
    x_strfreev(languages_allocated);

    return result;
}

xchar **x_key_file_get_locale_string_list(XKeyFile *key_file, const xchar *group_name, const xchar *key, const xchar *locale, xsize *length, XError **error)
{
    xsize len;
    XError *key_file_error;
    xchar **values, *value;
    char list_separator[2];

    x_return_val_if_fail(key_file != NULL, NULL);
    x_return_val_if_fail(group_name != NULL, NULL);
    x_return_val_if_fail(key != NULL, NULL);

    key_file_error = NULL;

    value = x_key_file_get_locale_string(key_file, group_name,  key, locale, &key_file_error);
    if (key_file_error) {
        x_propagate_error(error, key_file_error);
    }

    if (!value) {
        if (length) {
            *length = 0;
        }

        return NULL;
    }

    len = strlen(value);
    if (value[len - 1] == key_file->list_separator) {
        value[len - 1] = '\0';
    }

    list_separator[0] = key_file->list_separator;
    list_separator[1] = '\0';
    values = x_strsplit(value, list_separator, 0);

    x_free(value);

    if (length) {
        *length = x_strv_length(values);
    }

    return values;
}

void x_key_file_set_locale_string_list(XKeyFile *key_file, const xchar *group_name, const xchar *key, const xchar *locale, const xchar *const list[], xsize length)
{
    xsize i;
    xchar *full_key;
    XString *value_list;

    x_return_if_fail(key_file != NULL);
    x_return_if_fail(key != NULL);
    x_return_if_fail(locale != NULL);
    x_return_if_fail(length != 0);

    value_list = x_string_sized_new(length * 128);
    for (i = 0; i < length && list[i] != NULL; i++) {
        xchar *value;
        
        value = x_key_file_parse_string_as_value(key_file, list[i], TRUE);
        x_string_append(value_list, value);
        x_string_append_c(value_list, key_file->list_separator);

        x_free(value);
    }

    full_key = x_strdup_printf("%s[%s]", key, locale);
    x_key_file_set_value(key_file, group_name, full_key, value_list->str);
    x_free(full_key);
    x_string_free(value_list, TRUE);
}

xboolean x_key_file_get_boolean(XKeyFile *key_file, const xchar *group_name, const xchar *key, XError **error)
{
    xchar *value;
    xboolean bool_value;
    XError *key_file_error = NULL;

    x_return_val_if_fail(key_file != NULL, FALSE);
    x_return_val_if_fail(group_name != NULL, FALSE);
    x_return_val_if_fail(key != NULL, FALSE);

    value = x_key_file_get_value(key_file, group_name, key, &key_file_error);
    if (!value) {
        x_propagate_error(error, key_file_error);
        return FALSE;
    }

    bool_value = x_key_file_parse_value_as_boolean(key_file, value, &key_file_error);
    x_free(value);

    if (key_file_error) {
        if (x_error_matches(key_file_error, X_KEY_FILE_ERROR, X_KEY_FILE_ERROR_INVALID_VALUE)) {
            x_set_error(error, X_KEY_FILE_ERROR, X_KEY_FILE_ERROR_INVALID_VALUE, _("Key file contains key “%s” which has a value that cannot be interpreted."), key);
            x_error_free(key_file_error);
        } else {
            x_propagate_error(error, key_file_error);
        }
    }

    return bool_value;
}

void x_key_file_set_boolean(XKeyFile *key_file, const xchar *group_name, const xchar *key, xboolean value)
{
    const xchar *result;

    x_return_if_fail(key_file != NULL);

    result = x_key_file_parse_boolean_as_value(key_file, value);
    x_key_file_set_value(key_file, group_name, key, result);
}

xboolean *x_key_file_get_boolean_list(XKeyFile *key_file, const xchar *group_name, const xchar *key, xsize *length, XError **error)
{
    xchar **values;
    xsize i, num_bools;
    xboolean *bool_values;
    XError *key_file_error;

    x_return_val_if_fail(key_file != NULL, NULL);
    x_return_val_if_fail(group_name != NULL, NULL);
    x_return_val_if_fail(key != NULL, NULL);

    if (length) {
        *length = 0;
    }

    key_file_error = NULL;
    values = x_key_file_get_string_list(key_file, group_name, key, &num_bools, &key_file_error);

    if (key_file_error) {
        x_propagate_error(error, key_file_error);
    }

    if (!values) {
        return NULL;
    }

    bool_values = x_new(xboolean, num_bools);
    for (i = 0; i < num_bools; i++) {
        bool_values[i] = x_key_file_parse_value_as_boolean(key_file, values[i], &key_file_error);
        if (key_file_error) {
            x_propagate_error(error, key_file_error);
            x_strfreev(values);
            x_free(bool_values);

            return NULL;
        }
    }
    x_strfreev(values);

    if (length) {
        *length = num_bools;
    }

    return bool_values;
}

void x_key_file_set_boolean_list(XKeyFile *key_file, const xchar *group_name, const xchar *key, xboolean list[], xsize length)
{
    xsize i;
    XString *value_list;

    x_return_if_fail(key_file != NULL);
    x_return_if_fail(list != NULL);

    value_list = x_string_sized_new(length * 8);
    for (i = 0; i < length; i++) {
        const xchar *value;

        value = x_key_file_parse_boolean_as_value(key_file, list[i]);

        x_string_append(value_list, value);
        x_string_append_c(value_list, key_file->list_separator);
    }

    x_key_file_set_value(key_file, group_name, key, value_list->str);
    x_string_free(value_list, TRUE);
}

xint x_key_file_get_integer(XKeyFile *key_file, const xchar *group_name, const xchar *key, XError **error)
{
    xchar *value;
    xint int_value;
    XError *key_file_error;

    x_return_val_if_fail(key_file != NULL, -1);
    x_return_val_if_fail(group_name != NULL, -1);
    x_return_val_if_fail(key != NULL, -1);

    key_file_error = NULL;

    value = x_key_file_get_value(key_file, group_name, key, &key_file_error);
    if (key_file_error) {
        x_propagate_error(error, key_file_error);
        return 0;
    }

    int_value = x_key_file_parse_value_as_integer(key_file, value, &key_file_error);
    x_free(value);

    if (key_file_error) {
        if (x_error_matches(key_file_error, X_KEY_FILE_ERROR, X_KEY_FILE_ERROR_INVALID_VALUE)) {
            x_set_error(error, X_KEY_FILE_ERROR, X_KEY_FILE_ERROR_INVALID_VALUE, _("Key file contains key “%s” in group “%s” which has a value that cannot be interpreted."), key, group_name);
            x_error_free(key_file_error);
        } else {
            x_propagate_error(error, key_file_error);
        }
    }

    return int_value;
}

void x_key_file_set_integer(XKeyFile *key_file, const xchar *group_name, const xchar *key, xint value)
{
    xchar *result;

    x_return_if_fail(key_file != NULL);

    result = x_key_file_parse_integer_as_value(key_file, value);
    x_key_file_set_value(key_file, group_name, key, result);
    x_free(result);
}

xint64 x_key_file_get_int64(XKeyFile *key_file, const xchar *group_name, const xchar *key, XError **error)
{
    xint64 v;
    xchar *s, *end;

    x_return_val_if_fail(key_file != NULL, -1);
    x_return_val_if_fail(group_name != NULL, -1);
    x_return_val_if_fail(key != NULL, -1);

    s = x_key_file_get_value(key_file, group_name, key, error);
    if (s == NULL) {
        return 0;
    }

    v = x_ascii_strtoll(s, &end, 10);
    if (*s == '\0' || *end != '\0') {
        x_set_error(error, X_KEY_FILE_ERROR, X_KEY_FILE_ERROR_INVALID_VALUE, _("Key “%s” in group “%s” has value “%s” where %s was expected"), key, group_name, s, "int64");
        x_free(s);
        return 0;
    }

    x_free(s);
    return v;
}

void x_key_file_set_int64(XKeyFile *key_file, const xchar *group_name, const xchar *key, xint64 value)
{
    xchar *result;

    x_return_if_fail(key_file != NULL);

    result = x_strdup_printf("%" X_XINT64_FORMAT, value);
    x_key_file_set_value(key_file, group_name, key, result);
    x_free(result);
}

xuint64 x_key_file_get_uint64(XKeyFile *key_file, const xchar *group_name, const xchar *key, XError **error)
{
    xuint64 v;
    xchar *s, *end;

    x_return_val_if_fail(key_file != NULL, -1);
    x_return_val_if_fail(group_name != NULL, -1);
    x_return_val_if_fail(key != NULL, -1);

    s = x_key_file_get_value(key_file, group_name, key, error);
    if (s == NULL) {
        return 0;
    }

    v = x_ascii_strtoull(s, &end, 10);
    if (*s == '\0' || *end != '\0') {
        x_set_error(error, X_KEY_FILE_ERROR, X_KEY_FILE_ERROR_INVALID_VALUE, _("Key “%s” in group “%s” has value “%s” where %s was expected"), key, group_name, s, "uint64");
        x_free(s);
        return 0;
    }

    x_free(s);
    return v;
}

void x_key_file_set_uint64(XKeyFile *key_file, const xchar *group_name, const xchar *key, xuint64 value)
{
    xchar *result;

    x_return_if_fail(key_file != NULL);

    result = x_strdup_printf("%" X_XUINT64_FORMAT, value);
    x_key_file_set_value(key_file, group_name, key, result);
    x_free(result);
}

xint *x_key_file_get_integer_list(XKeyFile *key_file, const xchar *group_name, const xchar *key, xsize *length, XError **error)
{
    xchar **values;
    xint *int_values;
    xsize i, num_ints;
    XError *key_file_error = NULL;

    x_return_val_if_fail(key_file != NULL, NULL);
    x_return_val_if_fail(group_name != NULL, NULL);
    x_return_val_if_fail(key != NULL, NULL);

    if (length) {
        *length = 0;
    }

    values = x_key_file_get_string_list(key_file, group_name, key, &num_ints, &key_file_error);
    if (key_file_error) {
        x_propagate_error(error, key_file_error);
    }

    if (!values) {
        return NULL;
    }

    int_values = x_new(xint, num_ints);
    for (i = 0; i < num_ints; i++) {
        int_values[i] = x_key_file_parse_value_as_integer(key_file, values[i], &key_file_error);
        if (key_file_error) {
            x_propagate_error(error, key_file_error);
            x_strfreev(values);
            x_free(int_values);

            return NULL;
        }
    }
    x_strfreev(values);

    if (length) {
        *length = num_ints;
    }

    return int_values;
}

void x_key_file_set_integer_list(XKeyFile *key_file, const xchar *group_name, const xchar *key, xint list[], xsize length)
{
    xsize i;
    XString *values;

    x_return_if_fail(key_file != NULL);
    x_return_if_fail(list != NULL);

    values = x_string_sized_new(length * 16);
    for (i = 0; i < length; i++) {
        xchar *value;

        value = x_key_file_parse_integer_as_value(key_file, list[i]);
        x_string_append(values, value);
        x_string_append_c(values, key_file->list_separator);

        x_free(value);
    }

    x_key_file_set_value(key_file, group_name, key, values->str);
    x_string_free(values, TRUE);
}

xdouble x_key_file_get_double(XKeyFile *key_file, const xchar *group_name, const xchar *key, XError **error)
{
    xchar *value;
    xdouble double_value;
    XError *key_file_error;

    x_return_val_if_fail(key_file != NULL, -1);
    x_return_val_if_fail(group_name != NULL, -1);
    x_return_val_if_fail(key != NULL, -1);

    key_file_error = NULL;

    value = x_key_file_get_value(key_file, group_name, key, &key_file_error);
    if (key_file_error) {
        x_propagate_error(error, key_file_error);
        return 0;
    }

    double_value = x_key_file_parse_value_as_double(key_file, value, &key_file_error);
    x_free(value);

    if (key_file_error) {
        if (x_error_matches(key_file_error, X_KEY_FILE_ERROR, X_KEY_FILE_ERROR_INVALID_VALUE)) {
            x_set_error(error, X_KEY_FILE_ERROR, X_KEY_FILE_ERROR_INVALID_VALUE, _("Key file contains key “%s” in group “%s” which has a value that cannot be interpreted."), key, group_name);
            x_error_free(key_file_error);
        } else {
            x_propagate_error(error, key_file_error);
        }
    }

    return double_value;
}

void x_key_file_set_double(XKeyFile *key_file, const xchar *group_name, const xchar *key, xdouble value)
{
    xchar result[X_ASCII_DTOSTR_BUF_SIZE];

    x_return_if_fail(key_file != NULL);

    x_ascii_dtostr(result, sizeof(result), value);
    x_key_file_set_value(key_file, group_name, key, result);
}

xdouble *x_key_file_get_double_list(XKeyFile *key_file, const xchar *group_name, const xchar *key, xsize *length, XError **error)
{
    xchar **values;
    xsize i, num_doubles;
    xdouble *double_values;
    XError *key_file_error = NULL;

    x_return_val_if_fail(key_file != NULL, NULL);
    x_return_val_if_fail(group_name != NULL, NULL);
    x_return_val_if_fail(key != NULL, NULL);

    if (length) {
        *length = 0;
    }

    values = x_key_file_get_string_list(key_file, group_name, key, &num_doubles, &key_file_error);
    if (key_file_error) {
        x_propagate_error(error, key_file_error);
    }

    if (!values) {
        return NULL;
    }

    double_values = x_new(xdouble, num_doubles);
    for (i = 0; i < num_doubles; i++) {
        double_values[i] = x_key_file_parse_value_as_double(key_file, values[i], &key_file_error);
        if (key_file_error) {
            x_propagate_error(error, key_file_error);
            x_strfreev(values);
            x_free(double_values);

            return NULL;
        }
    }
    x_strfreev(values);

    if (length) {
        *length = num_doubles;
    }

    return double_values;
}

void x_key_file_set_double_list(XKeyFile *key_file, const xchar *group_name, const xchar *key, xdouble list[], xsize length)
{
    xsize i;
    XString *values;

    x_return_if_fail(key_file != NULL);
    x_return_if_fail(list != NULL);

    values = x_string_sized_new(length * 16);
    for (i = 0; i < length; i++) {
        xchar result[X_ASCII_DTOSTR_BUF_SIZE];

        x_ascii_dtostr(result, sizeof(result), list[i]);

        x_string_append(values, result);
        x_string_append_c(values, key_file->list_separator);
    }

    x_key_file_set_value(key_file, group_name, key, values->str);
    x_string_free(values, TRUE);
}

static xboolean x_key_file_set_key_comment(XKeyFile *key_file, const xchar *group_name, const xchar *key, const xchar *comment, XError **error)
{
    XKeyFileGroup *group;
    XKeyFileKeyValuePair *pair;
    XList *key_node, *comment_node, *tmp;
    
    group = x_key_file_lookup_group(key_file, group_name);
    if (!group) {
        x_set_error(error, X_KEY_FILE_ERROR, X_KEY_FILE_ERROR_GROUP_NOT_FOUND, _("Key file does not have group “%s”"), group_name ? group_name : "(null)");
        return FALSE;
    }

    key_node = x_key_file_lookup_key_value_pair_node(key_file, group, key);
    if (key_node == NULL) {
        set_not_found_key_error(group->name, key, error);
        return FALSE;
    }

    tmp = key_node->next;
    while (tmp != NULL) {
        pair = (XKeyFileKeyValuePair *)tmp->data;
        if (pair->key != NULL) {
            break;
        }

        comment_node = tmp;
        tmp = tmp->next;
        x_key_file_remove_key_value_pair_node(key_file, group, comment_node); 
    }

    if (comment == NULL) {
        return TRUE;
    }

    pair = x_new(XKeyFileKeyValuePair, 1);
    pair->key = NULL;
    pair->value = x_key_file_parse_comment_as_value(key_file, comment);
    
    key_node = x_list_insert(key_node, pair, 1);
    (void)key_node;

    return TRUE;
}

static xboolean x_key_file_set_top_comment(XKeyFile *key_file, const xchar *comment, XError **error)
{
    XList *group_node;
    XKeyFileGroup *group;
    XKeyFileKeyValuePair *pair;

    x_warn_if_fail(key_file->groups != NULL);
    group_node = x_list_last(key_file->groups);
    group = (XKeyFileGroup *)group_node->data;
    x_warn_if_fail(group->name == NULL);

    x_list_free_full(group->key_value_pairs, (XDestroyNotify)x_key_file_key_value_pair_free);
    group->key_value_pairs = NULL;

    if (comment == NULL) {
        return TRUE;
    }

    pair = x_new(XKeyFileKeyValuePair, 1);
    pair->key = NULL;
    pair->value = x_key_file_parse_comment_as_value(key_file, comment);

    group->key_value_pairs = x_list_prepend(group->key_value_pairs, pair);
    return TRUE;
}

static xboolean x_key_file_set_group_comment(XKeyFile *key_file, const xchar *group_name, const xchar *comment, XError **error)
{
    XList *group_node;
    XKeyFileGroup *group;
    XKeyFileKeyValuePair *pair;
    
    x_return_val_if_fail(group_name != NULL && x_key_file_is_group_name(group_name), FALSE);

    group = x_key_file_lookup_group(key_file, group_name);
    if (!group) {
        x_set_error(error, X_KEY_FILE_ERROR, X_KEY_FILE_ERROR_GROUP_NOT_FOUND, _("Key file does not have group “%s”"), group_name);
        return FALSE;
    }

    if (group == key_file->start_group) {
        return x_key_file_set_top_comment(key_file, comment, error);
    }

    group_node = x_key_file_lookup_group_node(key_file, group_name);
    group = group_node->next->data;
    for (XList *lp = group->key_value_pairs; lp != NULL; ) {
        XList *lnext = lp->next;
        pair = lp->data;
        if (pair->key != NULL) {
            break;
        }

        x_key_file_remove_key_value_pair_node(key_file, group, lp);
        lp = lnext;
    }

    if (comment == NULL) {
        return TRUE;
    }

    pair = x_new(XKeyFileKeyValuePair, 1);
    pair->key = NULL;
    pair->value = x_key_file_parse_comment_as_value(key_file, comment);
    group->key_value_pairs = x_list_prepend(group->key_value_pairs, pair);

    return TRUE;
}

xboolean x_key_file_set_comment(XKeyFile *key_file, const xchar *group_name, const xchar *key, const xchar *comment, XError **error)
{
    x_return_val_if_fail(key_file != NULL, FALSE);

    if (group_name != NULL && key != NULL)  {
        if (!x_key_file_set_key_comment(key_file, group_name, key, comment, error)) {
            return FALSE;
        }
    } else if (group_name != NULL)  {
        if (!x_key_file_set_group_comment(key_file, group_name, comment, error)) {
            return FALSE;
        }
    } else {
        if (!x_key_file_set_top_comment (key_file, comment, error)) {
            return FALSE;
        }
    }

    return TRUE;
}

static xchar *x_key_file_get_key_comment(XKeyFile *key_file, const xchar *group_name, const xchar *key, XError **error)
{
    xchar *comment;
    XString *string;
    XKeyFileGroup *group;
    XList *key_node, *tmp;
    XKeyFileKeyValuePair *pair;

    x_return_val_if_fail(group_name != NULL && x_key_file_is_group_name(group_name), NULL);

    group = x_key_file_lookup_group(key_file, group_name);
    if (!group) {
        x_set_error(error, X_KEY_FILE_ERROR, X_KEY_FILE_ERROR_GROUP_NOT_FOUND, _("Key file does not have group “%s”"), group_name ? group_name : "(null)");
        return NULL;
    }

    key_node = x_key_file_lookup_key_value_pair_node(key_file, group, key);
    if (key_node == NULL) {
        set_not_found_key_error(group->name, key, error);
        return NULL;
    }

    string = NULL;
    tmp = key_node->next;
    if (!key_node->next) {
        return NULL;
    }

    pair = (XKeyFileKeyValuePair *)tmp->data;
    if (pair->key != NULL) {
        return NULL;
    }

    while (tmp->next) {
        pair = (XKeyFileKeyValuePair *)tmp->next->data;
        if (pair->key != NULL) {
            break;
        }

        tmp = tmp->next;
    }

    while (tmp != key_node) {
        pair = (XKeyFileKeyValuePair *)tmp->data;
        if (string == NULL) {
            string = x_string_sized_new(512);
        }

        comment = x_key_file_parse_value_as_comment(key_file, pair->value, (tmp->prev == key_node));
        x_string_append(string, comment);
        x_free(comment);

        tmp = tmp->prev;
    }

    if (string != NULL) {
        comment = x_string_free_and_steal(x_steal_pointer(&string));
    } else {
        comment = NULL;
    }

    return comment;
}

static xchar *get_group_comment(XKeyFile *key_file, XKeyFileGroup *group, XError **error)
{
    XList *tmp;
    xchar *comment;
    XString *string;

    string = NULL;

    tmp = group->key_value_pairs;
    while (tmp) {
        XKeyFileKeyValuePair *pair;

        pair = (XKeyFileKeyValuePair *)tmp->data;
        if (pair->key != NULL) {
            tmp = tmp->prev;
            break;
        }

        if (tmp->next == NULL) {
            break;
        }

        tmp = tmp->next;
    }

    while (tmp != NULL) {
        XKeyFileKeyValuePair *pair;

        pair = (XKeyFileKeyValuePair *)tmp->data;
        if (string == NULL) {
            string = x_string_sized_new(512);
        }

        comment = x_key_file_parse_value_as_comment(key_file, pair->value, (tmp->prev == NULL));
        x_string_append(string, comment);
        x_free(comment);

        tmp = tmp->prev;
    }

    if (string != NULL) {
        return x_string_free(string, FALSE);
    }

    return NULL;
}

static xchar *x_key_file_get_group_comment(XKeyFile *key_file, const xchar *group_name, XError **error)
{
    XList *group_node;
    XKeyFileGroup *group;

    group = x_key_file_lookup_group(key_file, group_name);
    if (!group) {
        x_set_error(error, X_KEY_FILE_ERROR, X_KEY_FILE_ERROR_GROUP_NOT_FOUND, _("Key file does not have group “%s”"), group_name ? group_name : "(null)");
        return NULL;
    }

    group_node = x_key_file_lookup_group_node(key_file, group_name);
    group_node = group_node->next;
    group = (XKeyFileGroup *)group_node->data;

    return get_group_comment(key_file, group, error);
}

static xchar *x_key_file_get_top_comment(XKeyFile *key_file, XError **error)
{
    XList *group_node;
    XKeyFileGroup *group;

    x_warn_if_fail(key_file->groups != NULL);
    group_node = x_list_last(key_file->groups);
    group = (XKeyFileGroup *)group_node->data;
    x_warn_if_fail(group->name == NULL);

    return get_group_comment(key_file, group, error);
}

xchar *x_key_file_get_comment(XKeyFile *key_file, const xchar *group_name, const xchar *key, XError **error)
{
    x_return_val_if_fail(key_file != NULL, NULL);

    if (group_name != NULL && key != NULL) {
        return x_key_file_get_key_comment(key_file, group_name, key, error);
    } else if (group_name != NULL) {
        return x_key_file_get_group_comment(key_file, group_name, error);
    } else {
        return x_key_file_get_top_comment(key_file, error);
    }
}

xboolean x_key_file_remove_comment(XKeyFile *key_file, const xchar *group_name, const xchar *key, XError **error)
{
    x_return_val_if_fail(key_file != NULL, FALSE);

    if (group_name != NULL && key != NULL) {
        return x_key_file_set_key_comment(key_file, group_name, key, NULL, error);
    } else if (group_name != NULL) {
        return x_key_file_set_group_comment(key_file, group_name, NULL, error);
    } else {
        return x_key_file_set_top_comment(key_file, NULL, error);
    }
}

xboolean x_key_file_has_group(XKeyFile *key_file, const xchar *group_name)
{
    x_return_val_if_fail(key_file != NULL, FALSE);
    x_return_val_if_fail(group_name != NULL, FALSE);

    return x_key_file_lookup_group(key_file, group_name) != NULL;
}

static xboolean x_key_file_has_key_full(XKeyFile *key_file, const xchar *group_name, const xchar *key, xboolean *has_key, XError **error)
{
    XKeyFileGroup *group;
    XKeyFileKeyValuePair *pair;

    x_return_val_if_fail(key_file != NULL, FALSE);
    x_return_val_if_fail(group_name != NULL, FALSE);
    x_return_val_if_fail(key != NULL, FALSE);

    group = x_key_file_lookup_group(key_file, group_name);
    if (!group) {
        x_set_error(error, X_KEY_FILE_ERROR, X_KEY_FILE_ERROR_GROUP_NOT_FOUND, _("Key file does not have group “%s”"), group_name);
        return FALSE;
    }

    pair = x_key_file_lookup_key_value_pair(key_file, group, key);

    if (has_key) {
        *has_key = pair != NULL;
    }

    return TRUE;
}

xboolean x_key_file_has_key(XKeyFile *key_file, const xchar *group_name, const xchar *key, XError **error)
{
    xboolean has_key;
    XError *temp_error = NULL;

    if (x_key_file_has_key_full(key_file, group_name, key, &has_key, &temp_error)) {
        return has_key;
    } else {
        x_propagate_error(error, temp_error);
        return FALSE;
    }
}

static void x_key_file_add_group(XKeyFile *key_file, const xchar *group_name, xboolean created)
{
    XKeyFileGroup *group;

    x_return_if_fail(key_file != NULL);
    x_return_if_fail(group_name != NULL && x_key_file_is_group_name(group_name));

    group = x_key_file_lookup_group(key_file, group_name);
    if (group != NULL) {
        key_file->current_group = group;
        return;
    }

    group = x_new0(XKeyFileGroup, 1);
    group->name = x_strdup(group_name);
    group->lookup_map = x_hash_table_new(x_str_hash, x_str_equal);
    key_file->groups = x_list_prepend(key_file->groups, group);
    key_file->current_group = group;

    if (key_file->start_group == NULL) {
        key_file->start_group = group;
    } else if (!(key_file->flags & X_KEY_FILE_KEEP_COMMENTS) || created) {
        XKeyFileGroup *next_group = key_file->groups->next->data;
        XKeyFileKeyValuePair *pair;
        if (next_group->key_value_pairs != NULL) {
            pair = next_group->key_value_pairs->data;
        }

        if (next_group->key_value_pairs == NULL || ((pair->key != NULL) && !x_strstr_len(pair->value, -1, "\n"))) {
            XKeyFileKeyValuePair *pair = x_new(XKeyFileKeyValuePair, 1);
            pair->key = NULL;
            pair->value = x_strdup("");
            next_group->key_value_pairs = x_list_prepend(next_group->key_value_pairs, pair);
        }
    }

    if (!key_file->group_hash) {
        key_file->group_hash = x_hash_table_new(x_str_hash, x_str_equal);
    }

    x_hash_table_insert(key_file->group_hash, (xpointer)group->name, group);
}

static void x_key_file_key_value_pair_free(XKeyFileKeyValuePair *pair)
{
    if (pair != NULL) {
        x_free(pair->key);
        x_free(pair->value);
        x_free_sized(pair, sizeof(XKeyFileKeyValuePair));
    }
}

static void x_key_file_remove_key_value_pair_node(XKeyFile *key_file, XKeyFileGroup *group, XList *pair_node)
{
    XKeyFileKeyValuePair *pair;

    pair = (XKeyFileKeyValuePair *)pair_node->data;
    group->key_value_pairs = x_list_remove_link(group->key_value_pairs, pair_node);

    x_warn_if_fail(pair->value != NULL);
    x_key_file_key_value_pair_free(pair);

    x_list_free_1(pair_node);
}

static void x_key_file_remove_group_node(XKeyFile *key_file, XList *group_node)
{
    XList *tmp;
    XKeyFileGroup *group;

    group = (XKeyFileGroup *) group_node->data;
    if (group->name) {
        x_assert(key_file->group_hash);
        x_hash_table_remove(key_file->group_hash, group->name);
    }

    if (key_file->current_group == group) {
        if (key_file->groups) {
            key_file->current_group = (XKeyFileGroup *)key_file->groups->data;
        } else {
            key_file->current_group = NULL;
        }
    }

    if (key_file->start_group == group) {
        tmp = x_list_last(key_file->groups);
        while (tmp != NULL) {
            if (tmp != group_node && ((XKeyFileGroup *)tmp->data)->name != NULL) {
                break;
            }

            tmp = tmp->prev;
        }

        if (tmp) {
            key_file->start_group = (XKeyFileGroup *)tmp->data;
        } else {
            key_file->start_group = NULL;
        }
    }

    key_file->groups = x_list_remove_link(key_file->groups, group_node);
    tmp = group->key_value_pairs;

    while (tmp != NULL) {
        XList *pair_node;

        pair_node = tmp;
        tmp = tmp->next;
        x_key_file_remove_key_value_pair_node(key_file, group, pair_node);
    }

    x_warn_if_fail(group->key_value_pairs == NULL);

    if (group->lookup_map) {
        x_hash_table_destroy(group->lookup_map);
        group->lookup_map = NULL;
    }

    x_free((xchar *)group->name);
    x_free_sized(group, sizeof(XKeyFileGroup));
    x_list_free_1(group_node);
}

xboolean x_key_file_remove_group(XKeyFile *key_file, const xchar *group_name, XError **error)
{
    XList *group_node;

    x_return_val_if_fail(key_file != NULL, FALSE);
    x_return_val_if_fail(group_name != NULL, FALSE);

    group_node = x_key_file_lookup_group_node(key_file, group_name);
    if (!group_node) {
        x_set_error(error, X_KEY_FILE_ERROR, X_KEY_FILE_ERROR_GROUP_NOT_FOUND, _("Key file does not have group “%s”"), group_name);
        return FALSE;
    }

    x_key_file_remove_group_node(key_file, group_node);
    return TRUE;
}

static void x_key_file_add_key_value_pair(XKeyFile *key_file, XKeyFileGroup *group, XKeyFileKeyValuePair *pair, XList *sibling)
{
    x_hash_table_replace(group->lookup_map, pair->key, pair);
    group->key_value_pairs = x_list_insert_before(group->key_value_pairs, sibling, pair);
}

static void x_key_file_add_key(XKeyFile *key_file, XKeyFileGroup *group, const xchar *key, const xchar *value)
{
    XList *lp;
    XKeyFileKeyValuePair *pair;

    pair = x_new(XKeyFileKeyValuePair, 1);
    pair->key = x_strdup(key);
    pair->value = x_strdup(value);

    lp = group->key_value_pairs;
    while ((lp != NULL) && (((XKeyFileKeyValuePair *)lp->data)->key == NULL)) {
        lp = lp->next;
    }

    x_key_file_add_key_value_pair(key_file, group, pair, lp);
}

xboolean x_key_file_remove_key(XKeyFile *key_file, const xchar *group_name, const xchar *key, XError **error)
{
    XKeyFileGroup *group;
    XKeyFileKeyValuePair *pair;

    x_return_val_if_fail(key_file != NULL, FALSE);
    x_return_val_if_fail(group_name != NULL, FALSE);
    x_return_val_if_fail(key != NULL, FALSE);

    pair = NULL;

    group = x_key_file_lookup_group(key_file, group_name);
    if (!group) {
        x_set_error(error, X_KEY_FILE_ERROR, X_KEY_FILE_ERROR_GROUP_NOT_FOUND, _("Key file does not have group “%s”"), group_name);
        return FALSE;
    }

    pair = x_key_file_lookup_key_value_pair(key_file, group, key);
    if (!pair) {
        set_not_found_key_error(group->name, key, error);
        return FALSE;
    }

    group->key_value_pairs = x_list_remove(group->key_value_pairs, pair);
    x_hash_table_remove(group->lookup_map, pair->key);
    x_key_file_key_value_pair_free(pair);

    return TRUE;
}

static XList *x_key_file_lookup_group_node(XKeyFile *key_file, const xchar *group_name)
{
    XKeyFileGroup *group;

    group = x_key_file_lookup_group(key_file, group_name);
    if (group == NULL) {
        return NULL;
    }

    return x_list_find(key_file->groups, group);
}

static XKeyFileGroup *x_key_file_lookup_group(XKeyFile *key_file, const xchar *group_name)
{
    if (!key_file->group_hash) {
        return NULL;
    }

    return (XKeyFileGroup *)x_hash_table_lookup(key_file->group_hash, group_name);
}

static XList *x_key_file_lookup_key_value_pair_node(XKeyFile *key_file, XKeyFileGroup *group, const xchar *key)
{
    XList *key_node;

    for (key_node = group->key_value_pairs; key_node != NULL; key_node = key_node->next) {
        XKeyFileKeyValuePair *pair;

        pair = (XKeyFileKeyValuePair *)key_node->data;
        if (pair->key && strcmp(pair->key, key) == 0) {
            break;
        }
    }

    return key_node;
}

static XKeyFileKeyValuePair *x_key_file_lookup_key_value_pair(XKeyFile *key_file, XKeyFileGroup *group, const xchar *key)
{
    return (XKeyFileKeyValuePair *)x_hash_table_lookup(group->lookup_map, key);
}

static xboolean x_key_file_line_is_comment(const xchar *line)
{
    return (*line == '#' || *line == '\0' || *line == '\n');
}

static xboolean x_key_file_is_group_name(const xchar *name)
{
    const xchar *p, *q;

    x_assert(name != NULL);

    p = q = name;
    while (*q && *q != ']' && *q != '[' && !x_ascii_iscntrl(*q)) {
        q = x_utf8_find_next_char(q, NULL);
    }

    if (*q != '\0' || q == p) {
        return FALSE;
    }

    return TRUE;
}

static xboolean x_key_file_is_key_name(const xchar *name, xsize len)
{
    const xchar *p, *q, *end;

    x_assert(name != NULL);

    p = q = name;
    end = name + len;

    while (q < end && *q && *q != '=' && *q != '[' && *q != ']') {
        q = x_utf8_find_next_char(q, end);
        if (q == NULL) {
            q = end;
        }
    }

    if (q == p) {
        return FALSE;
    }

    if (*p == ' ' || q[-1] == ' ') {
        return FALSE;
    }

    if (*q == '[') {
        q++;
        while (q < end && *q != '\0' && (x_unichar_isalnum(x_utf8_get_char_validated(q, end - q)) || *q == '-' || *q == '_' || *q == '.' || *q == '@')) {
            q = x_utf8_find_next_char(q, end);
            if (q == NULL) {
                q = end;
                break;
            }
        }

        if (*q != ']') {
            return FALSE;
        }

        q++;
    }

    if (q < end) {
        return FALSE;
    }

    return TRUE;
}

static xboolean x_key_file_line_is_group(const xchar *line)
{
    const xchar *p;

    p = line;
    if (*p != '[') {
        return FALSE;
    }

    p++;
    while (*p && *p != ']') {
        p = x_utf8_find_next_char(p, NULL);
    }

    if (*p != ']') {
        return FALSE;
    }

    p = x_utf8_find_next_char(p, NULL);
    while (*p == ' ' || *p == '\t') {
        p = x_utf8_find_next_char(p, NULL);
    }

    if (*p) {
        return FALSE;
    }

    return TRUE;
}

static xboolean x_key_file_line_is_key_value_pair(const xchar *line)
{
    const xchar *p;

    p = x_utf8_strchr(line, -1, '=');
    if (!p) {
        return FALSE;
    }

    if (*p == line[0]) {
        return FALSE;
    }

    return TRUE;
}

static xchar *x_key_file_parse_value_as_string(XKeyFile *key_file, const xchar *value, XSList **pieces, XError **error)
{
    const xchar *p;
    XSList *tmp_pieces = NULL;
    xchar *string_value, *q0, *q;

    x_assert(pieces == NULL || *pieces == NULL);

    string_value = x_new(xchar, strlen(value) + 1);

    p = value;
    q0 = q = string_value;

    while (*p) {
        if (*p == '\\') {
            p++;
            switch (*p) {
                case 's':
                    *q = ' ';
                    break;

                case 'n':
                    *q = '\n';
                    break;

                case 't':
                    *q = '\t';
                    break;

                case 'r':
                    *q = '\r';
                    break;

                case '\\':
                    *q = '\\';
                    break;

                case '\0':
                    x_clear_error(error);
                    x_set_error_literal(error, X_KEY_FILE_ERROR, X_KEY_FILE_ERROR_INVALID_VALUE, _("Key file contains escape character at end of line"));
                    goto error;

                default:
                    if (pieces && *p == key_file->list_separator) {
                        *q = key_file->list_separator;
                    } else {
                        *q++ = '\\';
                        *q = *p;

                        if (*error == NULL) {
                            xchar sequence[3];

                            sequence[0] = '\\';
                            sequence[1] = *p;
                            sequence[2] = '\0';

                            x_set_error(error, X_KEY_FILE_ERROR, X_KEY_FILE_ERROR_INVALID_VALUE, _("Key file contains invalid escape sequence “%s”"), sequence);
                        }
                    }
                    break;
            }
        } else {
            *q = *p;
            if (pieces && (*p == key_file->list_separator)) {
                tmp_pieces = x_slist_prepend(tmp_pieces, x_strndup(q0, q - q0));
                q0 = q + 1;
            }
        }

        if (*p == '\0') {
            break;
        }

        q++;
        p++;
    }

    *q = '\0';
    if (pieces) {
        if (q0 < q) {
            tmp_pieces = x_slist_prepend(tmp_pieces, x_strndup(q0, q - q0));
        }

        *pieces = x_slist_reverse(tmp_pieces);
    }

    return string_value;

error:
    x_free(string_value);
    x_slist_free_full(tmp_pieces, x_free);

    return NULL;
}

static xchar *x_key_file_parse_string_as_value(XKeyFile *key_file, const xchar *string, xboolean escape_separator)
{
    xsize length;
    const xchar *p;
    xchar *value, *q;
    xboolean parsing_leading_space;

    length = strlen(string) + 1;
    value = x_new(xchar, 2 * length);

    p = string;
    q = value;
    parsing_leading_space = TRUE;

    while (p < (string + length - 1)) {
        xchar escaped_character[3] = { '\\', 0, 0 };

        switch (*p) {
            case ' ':
                if (parsing_leading_space) {
                    escaped_character[1] = 's';
                    strcpy(q, escaped_character);
                    q += 2;
                } else {
                    *q = *p;
                    q++;
                }
                break;

            case '\t':
                if (parsing_leading_space) {
                    escaped_character[1] = 't';
                    strcpy(q, escaped_character);
                    q += 2;
                } else {
                    *q = *p;
                    q++;
                }
                break;

            case '\n':
                escaped_character[1] = 'n';
                strcpy(q, escaped_character);
                q += 2;
                break;

            case '\r':
                escaped_character[1] = 'r';
                strcpy(q, escaped_character);
                q += 2;
                break;

            case '\\':
                escaped_character[1] = '\\';
                strcpy(q, escaped_character);
                q += 2;
                parsing_leading_space = FALSE;
                break;

            default:
                if (escape_separator && *p == key_file->list_separator) {
                    escaped_character[1] = key_file->list_separator;
                    strcpy(q, escaped_character);
                    q += 2;
                    parsing_leading_space = TRUE;
                } else  {
                    *q = *p;
                    q++;
                    parsing_leading_space = FALSE;
                }
                break;
        }

        p++;
    }
    *q = '\0';

    return value;
}

static xint x_key_file_parse_value_as_integer(XKeyFile *key_file, const xchar *value, XError **error)
{
    int errsv;
    xchar *eof_int;
    xint int_value;
    xlong long_value;

    errno = 0;
    long_value = strtol(value, &eof_int, 10);
    errsv = errno;

    if (*value == '\0' || (*eof_int != '\0' && !x_ascii_isspace(*eof_int))) {
        xchar *value_utf8 = x_utf8_make_valid(value, -1);
        x_set_error(error, X_KEY_FILE_ERROR, X_KEY_FILE_ERROR_INVALID_VALUE, _("Value “%s” cannot be interpreted as a number."), value_utf8);
        x_free(value_utf8);

        return 0;
    }

    int_value = long_value;
    if (int_value != long_value || errsv == ERANGE) {
        xchar *value_utf8 = x_utf8_make_valid(value, -1);
        x_set_error(error, X_KEY_FILE_ERROR, X_KEY_FILE_ERROR_INVALID_VALUE, _("Integer value “%s” out of range"),  value_utf8);
        x_free(value_utf8);

        return 0;
    }

    return int_value;
}

static xchar *x_key_file_parse_integer_as_value(XKeyFile *key_file, xint value)
{
    return x_strdup_printf("%d", value);
}

static xdouble x_key_file_parse_value_as_double(XKeyFile *key_file, const xchar *value, XError **error)
{
    xchar *end_of_valid_d;
    xdouble double_value = 0;

    double_value = x_ascii_strtod(value, &end_of_valid_d);

    if (*end_of_valid_d != '\0' || end_of_valid_d == value) {
        xchar *value_utf8 = x_utf8_make_valid(value, -1);
        x_set_error(error, X_KEY_FILE_ERROR, X_KEY_FILE_ERROR_INVALID_VALUE, _("Value “%s” cannot be interpreted as a float number."), value_utf8);
        x_free(value_utf8);

        double_value = 0;
    }

    return double_value;
}

static xint strcmp_sized(const xchar *s1, size_t len1, const xchar *s2)
{
    size_t len2 = strlen(s2);
    return strncmp(s1, s2, MAX(len1, len2));
}

static xboolean x_key_file_parse_value_as_boolean(XKeyFile *key_file, const xchar *value, XError **error)
{
    xchar *value_utf8;
    xint i, length = 0;

    for (i = 0; value[i]; i++) {
        if (!x_ascii_isspace(value[i])) {
            length = i + 1;
        }
    }

    if (strcmp_sized(value, length, "true") == 0 || strcmp_sized(value, length, "1") == 0) {
        return TRUE;
    } else if (strcmp_sized(value, length, "false") == 0 || strcmp_sized(value, length, "0") == 0) {
        return FALSE;
    }

    value_utf8 = x_utf8_make_valid(value, -1);
    x_set_error(error, X_KEY_FILE_ERROR, X_KEY_FILE_ERROR_INVALID_VALUE, _("Value “%s” cannot be interpreted as a boolean."), value_utf8);
    x_free(value_utf8);

    return FALSE;
}

static const xchar *x_key_file_parse_boolean_as_value(XKeyFile *key_file, xboolean value)
{
    if (value) {
        return "true";
    } else {
        return "false";
    }
}

static xchar *x_key_file_parse_value_as_comment(XKeyFile *key_file, const xchar *value, xboolean is_final_line)
{
    xsize i;
    xchar **lines;
    XString *string;

    string = x_string_sized_new(512);
    lines = x_strsplit(value, "\n", 0);

    for (i = 0; lines[i] != NULL; i++) {
        const xchar *line = lines[i];
        if (i != 0) {
            x_string_append_c(string, '\n');
        }

        if (line[0] == '#') {
            line++;
        }
        x_string_append(string, line);
    }
    x_strfreev(lines);

    if (!is_final_line) {
        x_string_append_c(string, '\n');
    }

    return x_string_free(string, FALSE);
}

static xchar *x_key_file_parse_comment_as_value(XKeyFile *key_file, const xchar *comment)
{
    xsize i;
    xchar **lines;
    XString *string;

    string = x_string_sized_new(512);
    lines = x_strsplit(comment, "\n", 0);

    for (i = 0; lines[i] != NULL; i++) {
        x_string_append_printf(string, "#%s%s", lines[i],  lines[i + 1] == NULL? "" : "\n");
    }
    x_strfreev(lines);

    return x_string_free(string, FALSE);
}

xboolean x_key_file_save_to_file(XKeyFile *key_file, const xchar *filename, XError **error)
{
    xsize length;
    xchar *contents;
    xboolean success;

    x_return_val_if_fail(key_file != NULL, FALSE);
    x_return_val_if_fail(filename != NULL, FALSE);
    x_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    contents = x_key_file_to_data(key_file, &length, NULL);
    x_assert(contents != NULL);

    success = x_file_set_contents(filename, contents, length, error);
    x_free(contents);

    return success;
}
