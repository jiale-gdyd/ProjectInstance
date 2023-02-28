#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <stdarg.h>

#include <xlib/xlib/config.h>

#include <xlib/xlib/xmem.h>
#include <xlib/xlib/xhash.h>
#include <xlib/xlib/xlist.h>
#include <xlib/xlib/xmain.h>
#include <xlib/xlib/xshell.h>
#include <xlib/xlib/xslice.h>
#include <xlib/xlib/xstdio.h>
#include <xlib/xlib/xerror.h>
#include <xlib/xlib/xtimer.h>
#include <xlib/xlib/xutils.h>
#include <xlib/xlib/xstring.h>
#include <xlib/xlib/xmarkup.h>
#include <xlib/xlib/xconvert.h>
#include <xlib/xlib/xdataset.h>
#include <xlib/xlib/xlibintl.h>
#include <xlib/xlib/xdatetime.h>
#include <xlib/xlib/xmessages.h>
#include <xlib/xlib/xstrfuncs.h>
#include <xlib/xlib/xfileutils.h>
#include <xlib/xlib/xbookmarkfile.h>

#define XBEL_VERSION                        "1.0"
#define XBEL_DTD_NICK                       "xbel"
#define XBEL_DTD_SYSTEM                     "+//IDN python.org//DTD XML Bookmark Exchange Language 1.0//EN//XML"

#define XBEL_DTD_URI                        "http://www.python.org/topics/xml/dtds/xbel-1.0.dtd"

#define XBEL_ROOT_ELEMENT                   "xbel"
#define XBEL_FOLDER_ELEMENT                 "folder"
#define XBEL_BOOKMARK_ELEMENT               "bookmark"
#define XBEL_ALIAS_ELEMENT                  "alias"
#define XBEL_SEPARATOR_ELEMENT              "separator"
#define XBEL_TITLE_ELEMENT                  "title"
#define XBEL_DESC_ELEMENT                   "desc"
#define XBEL_INFO_ELEMENT                   "info"
#define XBEL_METADATA_ELEMENT               "metadata"

#define XBEL_VERSION_ATTRIBUTE              "version"
#define XBEL_FOLDED_ATTRIBUTE               "folded"
#define XBEL_OWNER_ATTRIBUTE                "owner"
#define XBEL_ADDED_ATTRIBUTE                "added"
#define XBEL_VISITED_ATTRIBUTE              "visited"
#define XBEL_MODIFIED_ATTRIBUTE             "modified"
#define XBEL_ID_ATTRIBUTE                   "id"
#define XBEL_HREF_ATTRIBUTE                 "href"
#define XBEL_REF_ATTRIBUTE                  "ref"

#define XBEL_YES_VALUE                      "yes"
#define XBEL_NO_VALUE                       "no"

#define BOOKMARK_METADATA_OWNER             "http://freedesktop.org"

#define BOOKMARK_NAMESPACE_NAME             "bookmark"
#define BOOKMARK_NAMESPACE_URI              "http://www.freedesktop.org/standards/desktop-bookmarks"

#define BOOKMARK_GROUPS_ELEMENT             "groups"
#define BOOKMARK_GROUP_ELEMENT              "group"
#define BOOKMARK_APPLICATIONS_ELEMENT       "applications"
#define BOOKMARK_APPLICATION_ELEMENT        "application"
#define BOOKMARK_ICON_ELEMENT               "icon"
#define BOOKMARK_PRIVATE_ELEMENT            "private"

#define BOOKMARK_NAME_ATTRIBUTE             "name"
#define BOOKMARK_EXEC_ATTRIBUTE             "exec"
#define BOOKMARK_COUNT_ATTRIBUTE            "count"
#define BOOKMARK_TIMESTAMP_ATTRIBUTE        "timestamp"
#define BOOKMARK_MODIFIED_ATTRIBUTE         "modified"
#define BOOKMARK_HREF_ATTRIBUTE             "href"
#define BOOKMARK_TYPE_ATTRIBUTE             "type"

#define MIME_NAMESPACE_NAME                 "mime"
#define MIME_NAMESPACE_URI                  "http://www.freedesktop.org/standards/shared-mime-info"
#define MIME_TYPE_ELEMENT                   "mime-type"
#define MIME_TYPE_ATTRIBUTE                 "type"

typedef struct _ParseData ParseData;
typedef struct _BookmarkItem BookmarkItem;
typedef struct _BookmarkAppInfo BookmarkAppInfo;
typedef struct _BookmarkMetadata BookmarkMetadata;

struct _BookmarkAppInfo {
    xchar     *name;
    xchar     *exec;
    xuint     count;
    XDateTime *stamp;
};

struct _BookmarkMetadata {
    xchar      *mime_type;
    XList      *groups;
    XList      *applications;
    XHashTable *apps_by_name;
    xchar      *icon_href;
    xchar      *icon_mime;
    xuint      is_private : 1;
};

struct _BookmarkItem {
    xchar            *uri;
    xchar            *title;
    xchar            *description;
    XDateTime        *added;
    XDateTime        *modified;
    XDateTime        *visited;
    BookmarkMetadata *metadata;
};

struct _XBookmarkFile {
    xchar      *title;
    xchar      *description;
    XList      *items;
    XHashTable *items_by_uri;
};

typedef enum {
    STATE_STARTED = 0,
    STATE_ROOT,
    STATE_BOOKMARK,
    STATE_TITLE,
    STATE_DESC,
    STATE_INFO,
    STATE_METADATA,
    STATE_APPLICATIONS,
    STATE_APPLICATION,
    STATE_GROUPS,
    STATE_GROUP,
    STATE_MIME,
    STATE_ICON,
    STATE_FINISHED
} ParserState;

static void x_bookmark_file_init(XBookmarkFile *bookmark);
static void x_bookmark_file_clear(XBookmarkFile *bookmark);
static xchar *x_bookmark_file_dump(XBookmarkFile  *bookmark, xsize *length, XError **error);
static xboolean x_bookmark_file_parse(XBookmarkFile *bookmark, const xchar *buffer, xsize length, XError **error);

static BookmarkItem *x_bookmark_file_lookup_item(XBookmarkFile  *bookmark, const xchar *uri);
static void x_bookmark_file_add_item(XBookmarkFile *bookmark, BookmarkItem *item, XError **error);
static xboolean timestamp_from_iso8601(const xchar *iso_date, XDateTime **out_date_time, XError **error);

static BookmarkAppInfo *bookmark_app_info_new(const xchar *name)
{
    BookmarkAppInfo *retval;
    x_warn_if_fail(name != NULL);

    retval = x_slice_new(BookmarkAppInfo);

    retval->name = x_strdup(name);
    retval->exec = NULL;
    retval->count = 0;
    retval->stamp = NULL;

    return retval;
}

static void bookmark_app_info_free(BookmarkAppInfo *app_info)
{
    if (!app_info) {
        return;
    }

    x_free(app_info->name);
    x_free(app_info->exec);
    x_clear_pointer(&app_info->stamp, x_date_time_unref);

    x_slice_free(BookmarkAppInfo, app_info);
}

static BookmarkAppInfo *bookmark_app_info_copy(BookmarkAppInfo *app_info)
{
    BookmarkAppInfo *copy;

    if (!app_info) {
        return NULL;
    }

    copy = bookmark_app_info_new(app_info->name);
    copy->count = app_info->count;
    copy->exec = x_strdup(app_info->exec);

    if (app_info->stamp) {
        copy->stamp = x_date_time_ref(app_info->stamp);
    }

    return copy;
}

static xchar *bookmark_app_info_dump(BookmarkAppInfo *app_info)
{
    xchar *retval;
    xchar *name, *exec, *modified, *count;

    x_warn_if_fail(app_info != NULL);

    if (app_info->count == 0) {
        return NULL;
    }

    name = x_markup_escape_text(app_info->name, -1);
    exec = x_markup_escape_text(app_info->exec, -1);
    count = x_strdup_printf("%u", app_info->count);

    if (app_info->stamp) {
        char *tmp;

        tmp = x_date_time_format_iso8601(app_info->stamp);
        modified = x_strconcat(" " BOOKMARK_MODIFIED_ATTRIBUTE "=\"", tmp, "\"", NULL);
        x_free(tmp);
    } else {
        modified = x_strdup("");
    }

    retval = x_strconcat("          "
                            "<" BOOKMARK_NAMESPACE_NAME ":" BOOKMARK_APPLICATION_ELEMENT
                            " " BOOKMARK_NAME_ATTRIBUTE "=\"", name, "\""
                            " " BOOKMARK_EXEC_ATTRIBUTE "=\"", exec, "\""
                            " " BOOKMARK_EXEC_ATTRIBUTE "=\"", exec, "\"",
                            modified,
                            " " BOOKMARK_COUNT_ATTRIBUTE "=\"", count, "\"/>\n",
                            NULL);
    x_free(name);
    x_free(exec);
    x_free(modified);
    x_free(count);

    return retval;
}

static BookmarkMetadata *bookmark_metadata_new(void)
{
    BookmarkMetadata *retval;

    retval = x_slice_new(BookmarkMetadata);
    retval->mime_type = NULL;
    retval->groups = NULL;
    retval->applications = NULL;
    retval->apps_by_name = x_hash_table_new_full(x_str_hash, x_str_equal, NULL, NULL);
    retval->is_private = FALSE;
    retval->icon_href = NULL;
    retval->icon_mime = NULL;

    return retval;
}

static void bookmark_metadata_free(BookmarkMetadata *metadata)
{
    if (!metadata) {
        return;
    }

    x_free(metadata->mime_type);

    x_list_free_full(metadata->groups, x_free);
    x_list_free_full(metadata->applications, (XDestroyNotify)bookmark_app_info_free);

    x_hash_table_destroy(metadata->apps_by_name);

    x_free(metadata->icon_href);
    x_free(metadata->icon_mime);

    x_slice_free(BookmarkMetadata, metadata);
}

static BookmarkMetadata *bookmark_metadata_copy(BookmarkMetadata *metadata)
{
    XList *l;
    BookmarkMetadata *copy;

    if (!metadata) {
        return NULL;
    }

    copy = bookmark_metadata_new();
    copy->is_private = metadata->is_private;
    copy->mime_type = x_strdup(metadata->mime_type);
    copy->icon_href = x_strdup(metadata->icon_href);
    copy->icon_mime = x_strdup(metadata->icon_mime);

    copy->groups = x_list_copy_deep(metadata->groups, (XCopyFunc)x_strdup, NULL);
    copy->applications = x_list_copy_deep(metadata->applications, (XCopyFunc)bookmark_app_info_copy, NULL);

    for (l = copy->applications; l; l = l->next) {
        BookmarkAppInfo *app_info = (BookmarkAppInfo *)l->data;
        x_hash_table_insert(copy->apps_by_name, app_info->name, app_info);
    }

    x_assert(x_hash_table_size(copy->apps_by_name) == x_hash_table_size(metadata->apps_by_name));

    return copy;
}

static xchar *bookmark_metadata_dump(BookmarkMetadata *metadata)
{
    xchar *buffer;
    XString *retval;

    if (!metadata->applications) {
        return NULL;
    }

    retval = x_string_sized_new(1024);

    x_string_append(retval, "      " "<" XBEL_METADATA_ELEMENT " " XBEL_OWNER_ATTRIBUTE "=\"" BOOKMARK_METADATA_OWNER "\">\n");

    if (metadata->mime_type) {
        buffer = x_strconcat("        " "<" MIME_NAMESPACE_NAME ":" MIME_TYPE_ELEMENT " " MIME_TYPE_ATTRIBUTE "=\"", metadata->mime_type, "\"/>\n", NULL);
        x_string_append(retval, buffer);
        x_free(buffer);
    }

    if (metadata->groups) {
        XList *l;

        x_string_append(retval, "        " "<" BOOKMARK_NAMESPACE_NAME ":" BOOKMARK_GROUPS_ELEMENT ">\n");

        for (l = x_list_last(metadata->groups); l != NULL; l = l->prev) {
            xchar *group_name;

            group_name = x_markup_escape_text((xchar *) l->data, -1);
            buffer = x_strconcat("          "
                        "<" BOOKMARK_NAMESPACE_NAME
                        ":" BOOKMARK_GROUP_ELEMENT ">",
                        group_name,
                        "</" BOOKMARK_NAMESPACE_NAME
                        ":"  BOOKMARK_GROUP_ELEMENT ">\n", NULL);
            x_string_append(retval, buffer);

            x_free(buffer);
            x_free(group_name);
        }

        x_string_append(retval, "        " "</" BOOKMARK_NAMESPACE_NAME ":" BOOKMARK_GROUPS_ELEMENT ">\n");
    }

    if (metadata->applications) {
        XList *l;

        x_string_append(retval, "        " "<" BOOKMARK_NAMESPACE_NAME ":" BOOKMARK_APPLICATIONS_ELEMENT ">\n");

        for (l = x_list_last(metadata->applications); l != NULL; l = l->prev) {
            xchar *app_data;
            BookmarkAppInfo *app_info = (BookmarkAppInfo *)l->data;

            x_warn_if_fail(app_info != NULL);

            app_data = bookmark_app_info_dump(app_info);
            if (app_data) {
                retval = x_string_append(retval, app_data);
                x_free(app_data);
            }
        }

        x_string_append(retval, "        " "</" BOOKMARK_NAMESPACE_NAME ":" BOOKMARK_APPLICATIONS_ELEMENT ">\n");
    }

    if (metadata->icon_href) {
        if (!metadata->icon_mime) {
            metadata->icon_mime = x_strdup("application/octet-stream");
        }

        buffer = x_strconcat("       "
                    "<" BOOKMARK_NAMESPACE_NAME
                    ":" BOOKMARK_ICON_ELEMENT
                    " " BOOKMARK_HREF_ATTRIBUTE "=\"", metadata->icon_href,
                    "\" " BOOKMARK_TYPE_ATTRIBUTE "=\"", metadata->icon_mime, "\"/>\n", NULL);
        x_string_append(retval, buffer);
        x_free(buffer);
    }

    if (metadata->is_private) {
        x_string_append(retval,
                "        "
                "<" BOOKMARK_NAMESPACE_NAME
                ":" BOOKMARK_PRIVATE_ELEMENT "/>\n");
    }
    x_string_append(retval, "      " "</" XBEL_METADATA_ELEMENT ">\n");

    return x_string_free(retval, FALSE);
}

static BookmarkItem *bookmark_item_new(const xchar *uri)
{
    BookmarkItem *item;

    x_warn_if_fail(uri != NULL);

    item = x_slice_new(BookmarkItem);
    item->uri = x_strdup(uri);
    item->title = NULL;
    item->description = NULL;
    item->added = NULL;
    item->modified = NULL;
    item->visited = NULL;
    item->metadata = NULL;

    return item;
}

static void bookmark_item_free(BookmarkItem *item)
{
    if (!item) {
        return;
    }

    x_free(item->uri);
    x_free(item->title);
    x_free(item->description);

    if (item->metadata) {
        bookmark_metadata_free(item->metadata);
    }

    x_clear_pointer(&item->added, x_date_time_unref);
    x_clear_pointer(&item->modified, x_date_time_unref);
    x_clear_pointer(&item->visited, x_date_time_unref);

    x_slice_free(BookmarkItem, item);
}

static BookmarkItem *bookmark_item_copy(BookmarkItem *item)
{
    BookmarkItem *copy;

    if (!item) {
        return NULL;
    }

    copy = bookmark_item_new(item->uri);

    copy->title = x_strdup(item->title);
    copy->description = x_strdup(item->description);

    copy->metadata = bookmark_metadata_copy(item->metadata);

    if (item->added) {
        copy->added = x_date_time_ref(item->added);
    }

    if (item->modified) {
        copy->modified = x_date_time_ref(item->modified);
    }

    if (item->visited) {
        copy->visited = x_date_time_ref(item->visited);
    }

    return copy;
}

static void bookmark_item_touch_modified(BookmarkItem *item)
{
    x_clear_pointer(&item->modified, x_date_time_unref);
    item->modified = x_date_time_new_now_utc();
}

static xchar *bookmark_item_dump(BookmarkItem *item)
{
    XString *retval;
    xchar *escaped_uri;

    if (!item->metadata || !item->metadata->applications) {
        x_warning("Item for URI '%s' has no registered applications: skipping.", item->uri);
        return NULL;
    }

    retval = x_string_sized_new(4096);

    x_string_append(retval, "  <" XBEL_BOOKMARK_ELEMENT " ");

    escaped_uri = x_markup_escape_text(item->uri, -1);

    x_string_append(retval, XBEL_HREF_ATTRIBUTE "=\"");
    x_string_append(retval, escaped_uri);
    x_string_append(retval , "\" ");

    x_free(escaped_uri);

    if (item->added) {
        char *added;

        added = x_date_time_format_iso8601(item->added);
        x_string_append(retval, XBEL_ADDED_ATTRIBUTE "=\"");
        x_string_append(retval, added);
        x_string_append(retval, "\" ");
        x_free(added);
    }

    if (item->modified) {
        char *modified;

        modified = x_date_time_format_iso8601(item->modified);
        x_string_append(retval, XBEL_MODIFIED_ATTRIBUTE "=\"");
        x_string_append(retval, modified);
        x_string_append(retval, "\" ");
        x_free(modified);
    }

    if (item->visited) {
        char *visited;

        visited = x_date_time_format_iso8601(item->visited);
        x_string_append(retval, XBEL_VISITED_ATTRIBUTE "=\"");
        x_string_append(retval, visited);
        x_string_append(retval, "\" ");
        x_free(visited);
    }

    if (retval->str[retval->len - 1] == ' ') {
        x_string_truncate (retval, retval->len - 1);
    }
    x_string_append(retval, ">\n");

    if (item->title) {
        xchar *escaped_title;

        escaped_title = x_markup_escape_text(item->title, -1);
        x_string_append(retval, "    " "<" XBEL_TITLE_ELEMENT ">");
        x_string_append(retval, escaped_title);
        x_string_append(retval, "</" XBEL_TITLE_ELEMENT ">\n");

        x_free(escaped_title);
    }

    if (item->description) {
        xchar *escaped_desc;

        escaped_desc = x_markup_escape_text(item->description, -1);
        x_string_append(retval, "    " "<" XBEL_DESC_ELEMENT ">");
        x_string_append(retval, escaped_desc);
        x_string_append(retval, "</" XBEL_DESC_ELEMENT ">\n");

        x_free(escaped_desc);
    }

    if (item->metadata) {
        xchar *metadata;

        metadata = bookmark_metadata_dump(item->metadata);
        if (metadata) {
            x_string_append(retval, "    " "<" XBEL_INFO_ELEMENT ">\n");
            x_string_append(retval, metadata);
            x_string_append(retval, "    " "</" XBEL_INFO_ELEMENT ">\n");
            x_free(metadata);
        }
    }

    x_string_append(retval, "  </" XBEL_BOOKMARK_ELEMENT ">\n");
    return x_string_free(retval, FALSE);
}

static BookmarkAppInfo *bookmark_item_lookup_app_info(BookmarkItem *item, const xchar *app_name)
{
    x_warn_if_fail(item != NULL && app_name != NULL);

    if (!item->metadata) {
        return NULL;
    }

    return (BookmarkAppInfo *)x_hash_table_lookup(item->metadata->apps_by_name, app_name);
}

static void x_bookmark_file_init(XBookmarkFile *bookmark)
{
    bookmark->title = NULL;
    bookmark->description = NULL;
    bookmark->items = NULL;
    bookmark->items_by_uri = x_hash_table_new_full(x_str_hash, x_str_equal, NULL, NULL);
}

static void x_bookmark_file_clear(XBookmarkFile *bookmark)
{
    x_free(bookmark->title);
    x_free(bookmark->description);

    x_list_free_full(bookmark->items, (XDestroyNotify)bookmark_item_free);
    bookmark->items = NULL;

    x_clear_pointer(&bookmark->items_by_uri, x_hash_table_unref);
}

struct _ParseData {
    ParserState   state;
    XHashTable    *namespaces;
    XBookmarkFile *bookmark_file;
    BookmarkItem  *current_item;
};

static ParseData *parse_data_new(void)
{
    ParseData *retval;

    retval = x_new(ParseData, 1);
    retval->state = STATE_STARTED;
    retval->namespaces = x_hash_table_new_full(x_str_hash, x_str_equal, (XDestroyNotify)x_free, (XDestroyNotify)x_free);
    retval->bookmark_file = NULL;
    retval->current_item = NULL;

    return retval;
}

static void parse_data_free(ParseData *parse_data)
{
    x_hash_table_destroy(parse_data->namespaces);
    x_free(parse_data);
}

#define IS_ATTRIBUTE(s, a)              ((0 == strcmp((s), (a))))

static void parse_bookmark_element(XMarkupParseContext *context, ParseData *parse_data, const xchar **attribute_names, const xchar **attribute_values, XError **error)
{
    xint i;
    const xchar *attr;
    XError *add_error;
    BookmarkItem *item;
    const xchar *uri, *added, *modified, *visited;

    x_warn_if_fail((parse_data != NULL) && (parse_data->state == STATE_BOOKMARK));

    i = 0;
    uri = added = modified = visited = NULL;

    for (attr = attribute_names[i]; attr != NULL; attr = attribute_names[++i]) {
        if (IS_ATTRIBUTE(attr, XBEL_HREF_ATTRIBUTE)) {
            uri = attribute_values[i];
        } else if (IS_ATTRIBUTE(attr, XBEL_ADDED_ATTRIBUTE)) {
            added = attribute_values[i];
        } else if (IS_ATTRIBUTE(attr, XBEL_MODIFIED_ATTRIBUTE)) {
            modified = attribute_values[i];
        } else if (IS_ATTRIBUTE(attr, XBEL_VISITED_ATTRIBUTE)) {
            visited = attribute_values[i];
        } else {
            x_set_error(error, X_MARKUP_ERROR, X_MARKUP_ERROR_UNKNOWN_ATTRIBUTE, _("Unexpected attribute “%s” for element “%s”"), attr, XBEL_BOOKMARK_ELEMENT);
            return;
        }
    }

    if (!uri) {
        x_set_error(error, X_MARKUP_ERROR, X_MARKUP_ERROR_INVALID_CONTENT, _("Attribute “%s” of element “%s” not found"), XBEL_HREF_ATTRIBUTE, XBEL_BOOKMARK_ELEMENT);
        return;
    }

    x_warn_if_fail(parse_data->current_item == NULL);

    item = bookmark_item_new(uri);

    if (added != NULL && !timestamp_from_iso8601(added, &item->added, error)) {
        bookmark_item_free(item);
        return;
    }

    if (modified != NULL && !timestamp_from_iso8601(modified, &item->modified, error)) {
        bookmark_item_free(item);
        return;
    }

    if (visited != NULL && !timestamp_from_iso8601(visited, &item->visited, error)) {
        bookmark_item_free(item);
        return;
    }

    add_error = NULL;
    x_bookmark_file_add_item(parse_data->bookmark_file, item, &add_error);
    if (add_error) {
        bookmark_item_free(item);
        x_propagate_error(error, add_error);

        return;
    }

    parse_data->current_item = item;
}

static void parse_application_element(XMarkupParseContext *context, ParseData *parse_data, const xchar **attribute_names, const xchar **attribute_values, XError **error)
{
    xint i;
    const xchar *attr;
    BookmarkItem *item;
    BookmarkAppInfo *ai;
    const xchar *name, *exec, *count, *stamp, *modified;

    x_warn_if_fail((parse_data != NULL) && (parse_data->state == STATE_APPLICATION));

    i = 0;
    name = exec = count = stamp = modified = NULL;
    for (attr = attribute_names[i]; attr != NULL; attr = attribute_names[++i]) {
        if (IS_ATTRIBUTE(attr, BOOKMARK_NAME_ATTRIBUTE)) {
            name = attribute_values[i];
        } else if (IS_ATTRIBUTE(attr, BOOKMARK_EXEC_ATTRIBUTE)) {
            exec = attribute_values[i];
        } else if (IS_ATTRIBUTE(attr, BOOKMARK_COUNT_ATTRIBUTE)) {
            count = attribute_values[i];
        } else if (IS_ATTRIBUTE(attr, BOOKMARK_TIMESTAMP_ATTRIBUTE)) {
            stamp = attribute_values[i];
        } else if (IS_ATTRIBUTE(attr, BOOKMARK_MODIFIED_ATTRIBUTE)) {
            modified = attribute_values[i];
        }
    }

    if (!name) {
        x_set_error(error, X_MARKUP_ERROR, X_MARKUP_ERROR_INVALID_CONTENT, _("Attribute “%s” of element “%s” not found"), BOOKMARK_NAME_ATTRIBUTE, BOOKMARK_APPLICATION_ELEMENT);
        return;
    }

    if (!exec) {
        x_set_error(error, X_MARKUP_ERROR, X_MARKUP_ERROR_INVALID_CONTENT, _("Attribute “%s” of element “%s” not found"), BOOKMARK_EXEC_ATTRIBUTE, BOOKMARK_APPLICATION_ELEMENT);
        return;
    }

    x_warn_if_fail(parse_data->current_item != NULL);
    item = parse_data->current_item;

    ai = bookmark_item_lookup_app_info(item, name);
    if (!ai) {
        ai = bookmark_app_info_new(name);

        if (!item->metadata) {
            item->metadata = bookmark_metadata_new();
        }

        item->metadata->applications = x_list_prepend(item->metadata->applications, ai);
        x_hash_table_replace(item->metadata->apps_by_name, ai->name, ai);
    }

    x_free(ai->exec);
    ai->exec = x_strdup(exec);

    if (count) {
        ai->count = atoi(count);
    } else {
        ai->count = 1;
    }

    x_clear_pointer(&ai->stamp, x_date_time_unref);
    if (modified != NULL) {
        if (!timestamp_from_iso8601(modified, &ai->stamp, error)) {
            return;
        }
    } else {
        if (stamp) {
            ai->stamp = x_date_time_new_from_unix_utc(atol(stamp));
        } else {
            ai->stamp = x_date_time_new_now_utc();
        }
    }
}

static void parse_mime_type_element(XMarkupParseContext *context, ParseData *parse_data, const xchar **attribute_names, const xchar **attribute_values, XError **error)
{
    xint i;
    const xchar *type;
    const xchar *attr;
    BookmarkItem *item;

    x_warn_if_fail((parse_data != NULL) && (parse_data->state == STATE_MIME));

    i = 0;
    type = NULL;

    for (attr = attribute_names[i]; attr != NULL; attr = attribute_names[++i]) {
        if (IS_ATTRIBUTE(attr, MIME_TYPE_ATTRIBUTE)) {
            type = attribute_values[i];
        }
    }

    if (!type) {
        type = "application/octet-stream";
    }

    x_warn_if_fail(parse_data->current_item != NULL);
    item = parse_data->current_item;

    if (!item->metadata) {
        item->metadata = bookmark_metadata_new();
    }

    x_free(item->metadata->mime_type);
    item->metadata->mime_type = x_strdup(type);
}

static void parse_icon_element(XMarkupParseContext *context, ParseData *parse_data, const xchar **attribute_names, const xchar **attribute_values, XError **error)
{
    xint i;
    const xchar *href;
    const xchar *type;
    const xchar *attr;
    BookmarkItem *item;

    x_warn_if_fail((parse_data != NULL) && (parse_data->state == STATE_ICON));

    i = 0;
    href = NULL;
    type = NULL;

    for (attr = attribute_names[i]; attr != NULL; attr = attribute_names[++i]) {
        if (IS_ATTRIBUTE(attr, BOOKMARK_HREF_ATTRIBUTE)) {
            href = attribute_values[i];
        } else if (IS_ATTRIBUTE(attr, BOOKMARK_TYPE_ATTRIBUTE)) {
            type = attribute_values[i];
        }
    }

    if (!href) {
        x_set_error(error, X_MARKUP_ERROR, X_MARKUP_ERROR_INVALID_CONTENT, _("Attribute “%s” of element “%s” not found"), BOOKMARK_HREF_ATTRIBUTE, BOOKMARK_ICON_ELEMENT);
        return;
    }

    if (!type) {
        type = "application/octet-stream";
    }

    x_warn_if_fail(parse_data->current_item != NULL);
    item = parse_data->current_item;

    if (!item->metadata) {
        item->metadata = bookmark_metadata_new();
    }

    x_free(item->metadata->icon_href);
    x_free(item->metadata->icon_mime);
    item->metadata->icon_href = x_strdup(href);
    item->metadata->icon_mime = x_strdup(type);
}

static void map_namespace_to_name(ParseData *parse_data, const xchar **attribute_names, const xchar **attribute_values)
{
    xint i;
    const xchar *attr;

    x_warn_if_fail(parse_data != NULL);

    if (!attribute_names || !attribute_names[0]) {
        return;
    }

    i = 0;
    for (attr = attribute_names[i]; attr; attr = attribute_names[++i]) {
        if (x_str_has_prefix(attr, "xmlns")) {
            xchar *p;
            xchar *namespace_name, *namespace_uri;

            p = x_utf8_strchr(attr, -1, ':');
            if (p) {
                p = x_utf8_next_char(p);
            } else {
                p = "default";
            }

            namespace_name = x_strdup(p);
            namespace_uri = x_strdup(attribute_values[i]);

            x_hash_table_replace(parse_data->namespaces, namespace_name, namespace_uri);
        }
    }
}

static xboolean is_element_full(ParseData *parse_data, const xchar *element_full, const xchar *ns, const xchar *element, const xchar sep)
{
    xboolean retval;
    xchar *ns_uri, *ns_name;
    const xchar *p, *element_name;

    x_warn_if_fail(parse_data != NULL);
    x_warn_if_fail(element_full != NULL);

    if (!element) {
        return FALSE;
    }

    if (!ns) {
        return (0 == strcmp(element_full, element));
    }

    p = x_utf8_strchr(element_full, -1, ':');
    if (p) {
        ns_name = x_strndup(element_full, p - element_full);
        element_name = x_utf8_next_char(p);
    } else {
        ns_name = x_strdup("default");
        element_name = element_full;
    }

    ns_uri = (xchar *)x_hash_table_lookup(parse_data->namespaces, ns_name);
    if (!ns_uri) {
        x_free(ns_name);
        return (0 == strcmp(element_full, element));
    }

    retval = (0 == strcmp(ns_uri, ns) && 0 == strcmp(element_name, element));
    x_free(ns_name);

    return retval;
}

#define IS_ELEMENT(p, s, e)             (is_element_full((p), (s), NULL, (e), '\0'))
#define IS_ELEMENT_NS(p, s, n, e)       (is_element_full((p), (s), (n), (e), '|'))

static const xchar *parser_state_to_element_name(ParserState state)
{
    switch (state) {
        case STATE_STARTED:
        case STATE_FINISHED:
            return "(top-level)";

        case STATE_ROOT:
            return XBEL_ROOT_ELEMENT;

        case STATE_BOOKMARK:
            return XBEL_BOOKMARK_ELEMENT;

        case STATE_TITLE:
            return XBEL_TITLE_ELEMENT;

        case STATE_DESC:
            return XBEL_DESC_ELEMENT;

        case STATE_INFO:
            return XBEL_INFO_ELEMENT;

        case STATE_METADATA:
            return XBEL_METADATA_ELEMENT;

        case STATE_APPLICATIONS:
            return BOOKMARK_APPLICATIONS_ELEMENT;

        case STATE_APPLICATION:
            return BOOKMARK_APPLICATION_ELEMENT;

        case STATE_GROUPS:
            return BOOKMARK_GROUPS_ELEMENT;

        case STATE_GROUP:
            return BOOKMARK_GROUP_ELEMENT;

        case STATE_MIME:
            return MIME_TYPE_ELEMENT;

        case STATE_ICON:
            return BOOKMARK_ICON_ELEMENT;

        default:
            x_assert_not_reached();
    }
}

static void start_element_raw_cb(XMarkupParseContext *context, const xchar *element_name, const xchar **attribute_names, const xchar **attribute_values, xpointer user_data, XError **error)
{
    ParseData *parse_data = (ParseData *)user_data;

    map_namespace_to_name(parse_data, attribute_names, attribute_values);

    switch (parse_data->state) {
        case STATE_STARTED:
            if (IS_ELEMENT(parse_data, element_name, XBEL_ROOT_ELEMENT)) {
                xint i;
                const xchar *attr;

                i = 0;
                for (attr = attribute_names[i]; attr; attr = attribute_names[++i]) {
                    if ((IS_ATTRIBUTE(attr, XBEL_VERSION_ATTRIBUTE)) && (0 == strcmp(attribute_values[i], XBEL_VERSION))) {
                        parse_data->state = STATE_ROOT;
                    }
                }
            } else {
                x_set_error(error, X_MARKUP_ERROR, X_MARKUP_ERROR_INVALID_CONTENT, _("Unexpected tag “%s”, tag “%s” expected"), element_name, XBEL_ROOT_ELEMENT);
            }
            break;

        case STATE_ROOT:
            if (IS_ELEMENT(parse_data, element_name, XBEL_TITLE_ELEMENT)) {
                parse_data->state = STATE_TITLE;
            } else if (IS_ELEMENT(parse_data, element_name, XBEL_DESC_ELEMENT)) {
                parse_data->state = STATE_DESC;
            } else if (IS_ELEMENT(parse_data, element_name, XBEL_BOOKMARK_ELEMENT)) {
                XError *inner_error = NULL;

                parse_data->state = STATE_BOOKMARK;
                parse_bookmark_element(context, parse_data, attribute_names, attribute_values, &inner_error);
                if (inner_error) {
                    x_propagate_error(error, inner_error);
                }
            } else {
                x_set_error(error, X_MARKUP_ERROR, X_MARKUP_ERROR_INVALID_CONTENT, _("Unexpected tag “%s” inside “%s”"), element_name, XBEL_ROOT_ELEMENT);
            }
            break;

        case STATE_BOOKMARK:
            if (IS_ELEMENT(parse_data, element_name, XBEL_TITLE_ELEMENT)) {
                parse_data->state = STATE_TITLE;
            } else if (IS_ELEMENT(parse_data, element_name, XBEL_DESC_ELEMENT)) {
                parse_data->state = STATE_DESC;
            } else if (IS_ELEMENT(parse_data, element_name, XBEL_INFO_ELEMENT)) {
                parse_data->state = STATE_INFO;
            } else {
                x_set_error(error, X_MARKUP_ERROR, X_MARKUP_ERROR_INVALID_CONTENT, _("Unexpected tag “%s” inside “%s”"), element_name, XBEL_BOOKMARK_ELEMENT);
            }
            break;

        case STATE_INFO:
            if (IS_ELEMENT(parse_data, element_name, XBEL_METADATA_ELEMENT)) {
                xint i;
                const xchar *attr;

                i = 0;
                for (attr = attribute_names[i]; attr; attr = attribute_names[++i]) {
                    if ((IS_ATTRIBUTE(attr, XBEL_OWNER_ATTRIBUTE)) && (0 == strcmp(attribute_values[i], BOOKMARK_METADATA_OWNER))) {
                        parse_data->state = STATE_METADATA;
                        if (!parse_data->current_item->metadata) {
                            parse_data->current_item->metadata = bookmark_metadata_new();
                        }
                    }
                }
            } else {
                x_set_error(error, X_MARKUP_ERROR, X_MARKUP_ERROR_INVALID_CONTENT, _("Unexpected tag “%s”, tag “%s” expected"), element_name, XBEL_METADATA_ELEMENT);
            }
            break;

        case STATE_METADATA:
            if (IS_ELEMENT_NS(parse_data, element_name, BOOKMARK_NAMESPACE_URI, BOOKMARK_APPLICATIONS_ELEMENT)) {
                parse_data->state = STATE_APPLICATIONS;
            } else if (IS_ELEMENT_NS(parse_data, element_name, BOOKMARK_NAMESPACE_URI, BOOKMARK_GROUPS_ELEMENT)) {
                parse_data->state = STATE_GROUPS;
            } else if (IS_ELEMENT_NS(parse_data, element_name, BOOKMARK_NAMESPACE_URI, BOOKMARK_PRIVATE_ELEMENT)) {
                parse_data->current_item->metadata->is_private = TRUE;
            } else if (IS_ELEMENT_NS(parse_data, element_name, BOOKMARK_NAMESPACE_URI, BOOKMARK_ICON_ELEMENT)) {
                XError *inner_error = NULL;

                parse_data->state = STATE_ICON;
                parse_icon_element(context, parse_data, attribute_names, attribute_values, &inner_error);
                if (inner_error) {
                    x_propagate_error(error, inner_error);
                }
            } else if (IS_ELEMENT_NS(parse_data, element_name, MIME_NAMESPACE_URI, MIME_TYPE_ELEMENT)) {
                XError *inner_error = NULL;

                parse_data->state = STATE_MIME;
                parse_mime_type_element(context, parse_data, attribute_names, attribute_values, &inner_error);
                if (inner_error) {
                    x_propagate_error(error, inner_error);
                }
            } else {
                x_set_error(error, X_MARKUP_ERROR, X_MARKUP_ERROR_UNKNOWN_ELEMENT, _("Unexpected tag “%s” inside “%s”"), element_name, XBEL_METADATA_ELEMENT);
            }
            break;

        case STATE_APPLICATIONS:
            if (IS_ELEMENT_NS(parse_data, element_name, BOOKMARK_NAMESPACE_URI, BOOKMARK_APPLICATION_ELEMENT)) {
                XError *inner_error = NULL;

                parse_data->state = STATE_APPLICATION;
                parse_application_element(context, parse_data, attribute_names, attribute_values, &inner_error);
                if (inner_error) {
                    x_propagate_error(error, inner_error);
                }
            } else {
                x_set_error(error, X_MARKUP_ERROR, X_MARKUP_ERROR_INVALID_CONTENT, _("Unexpected tag “%s”, tag “%s” expected"), element_name, BOOKMARK_APPLICATION_ELEMENT);
            }
            break;

        case STATE_GROUPS:
            if (IS_ELEMENT_NS(parse_data, element_name, BOOKMARK_NAMESPACE_URI, BOOKMARK_GROUP_ELEMENT)) {
                parse_data->state = STATE_GROUP;
            } else {
                x_set_error(error, X_MARKUP_ERROR, X_MARKUP_ERROR_INVALID_CONTENT, _("Unexpected tag “%s”, tag “%s” expected"), element_name, BOOKMARK_GROUP_ELEMENT);
            }
            break;

        case STATE_TITLE:
        case STATE_DESC:
        case STATE_APPLICATION:
        case STATE_GROUP:
        case STATE_MIME:
        case STATE_ICON:
        case STATE_FINISHED:
            x_set_error(error, X_MARKUP_ERROR, X_MARKUP_ERROR_INVALID_CONTENT, _("Unexpected tag “%s” inside “%s”"), element_name, parser_state_to_element_name(parse_data->state));
            break;

        default:
            x_assert_not_reached();
            break;
    }
}

static void end_element_raw_cb(XMarkupParseContext *context, const xchar *element_name, xpointer user_data, XError **error)
{
    ParseData *parse_data = (ParseData *)user_data;

    if (IS_ELEMENT(parse_data, element_name, XBEL_ROOT_ELEMENT)) {
        parse_data->state = STATE_FINISHED;
    } else if (IS_ELEMENT(parse_data, element_name, XBEL_BOOKMARK_ELEMENT)) {
        parse_data->current_item = NULL;
        parse_data->state = STATE_ROOT;
    } else if ((IS_ELEMENT(parse_data, element_name, XBEL_INFO_ELEMENT)) || (IS_ELEMENT(parse_data, element_name, XBEL_TITLE_ELEMENT)) || (IS_ELEMENT(parse_data, element_name, XBEL_DESC_ELEMENT))) {
        if (parse_data->current_item) {
            parse_data->state = STATE_BOOKMARK;
        } else {
            parse_data->state = STATE_ROOT;
        }
    } else if (IS_ELEMENT(parse_data, element_name, XBEL_METADATA_ELEMENT)) {
        parse_data->state = STATE_INFO;
    } else if (IS_ELEMENT_NS(parse_data, element_name, BOOKMARK_NAMESPACE_URI, BOOKMARK_APPLICATION_ELEMENT)) {
        parse_data->state = STATE_APPLICATIONS;
    } else if (IS_ELEMENT_NS(parse_data, element_name, BOOKMARK_NAMESPACE_URI, BOOKMARK_GROUP_ELEMENT)) {
        parse_data->state = STATE_GROUPS;
    } else if ((IS_ELEMENT_NS(parse_data, element_name, BOOKMARK_NAMESPACE_URI, BOOKMARK_APPLICATIONS_ELEMENT))
        || (IS_ELEMENT_NS(parse_data, element_name, BOOKMARK_NAMESPACE_URI, BOOKMARK_GROUPS_ELEMENT))
        || (IS_ELEMENT_NS(parse_data, element_name, BOOKMARK_NAMESPACE_URI, BOOKMARK_PRIVATE_ELEMENT))
        || (IS_ELEMENT_NS(parse_data, element_name, BOOKMARK_NAMESPACE_URI, BOOKMARK_ICON_ELEMENT))
        || (IS_ELEMENT_NS(parse_data, element_name, MIME_NAMESPACE_URI, MIME_TYPE_ELEMENT)))
    {
        parse_data->state = STATE_METADATA;
    }
}

static void text_raw_cb(XMarkupParseContext *context, const xchar *text, xsize length, xpointer user_data, XError **error)
{
    xchar *payload;
    ParseData *parse_data = (ParseData *)user_data;

    payload = x_strndup(text, length);

    switch (parse_data->state) {
        case STATE_TITLE:
            if (parse_data->current_item) {
                x_free(parse_data->current_item->title);
                parse_data->current_item->title = x_strdup(payload);
            } else {
                x_free(parse_data->bookmark_file->title);
                parse_data->bookmark_file->title = x_strdup(payload);
            }
            break;

        case STATE_DESC:
            if (parse_data->current_item) {
                x_free(parse_data->current_item->description);
                parse_data->current_item->description = x_strdup(payload);
            } else {
                x_free(parse_data->bookmark_file->description);
                parse_data->bookmark_file->description = x_strdup(payload);
            }
            break;

        case STATE_GROUP: {
            XList *groups;

            x_warn_if_fail(parse_data->current_item != NULL);

            if (!parse_data->current_item->metadata) {
                parse_data->current_item->metadata = bookmark_metadata_new();
            }
            groups = parse_data->current_item->metadata->groups;
            parse_data->current_item->metadata->groups = x_list_prepend(groups, x_strdup(payload));
        }
        break;

        case STATE_ROOT:
        case STATE_BOOKMARK:
        case STATE_INFO:
        case STATE_METADATA:
        case STATE_APPLICATIONS:
        case STATE_APPLICATION:
        case STATE_GROUPS:
        case STATE_MIME:
        case STATE_ICON:
        break;

        default:
            x_warn_if_reached();
            break;
    }

    x_free(payload);
}

static const XMarkupParser markup_parser = {
    start_element_raw_cb,
    end_element_raw_cb,
    text_raw_cb,
    NULL,
    NULL
};

static xboolean x_bookmark_file_parse(XBookmarkFile *bookmark, const xchar *buffer, xsize length, XError **error)
{
    xboolean retval;
    ParseData *parse_data;
    XMarkupParseContext *context;
    XError *parse_error, *end_error;

    x_warn_if_fail(bookmark != NULL);

    if (!buffer) {
        return FALSE;
    }

    parse_error = NULL;
    end_error = NULL;

    if (length == (xsize)-1) {
        length = strlen(buffer);
    }

    parse_data = parse_data_new();
    parse_data->bookmark_file = bookmark;

    context = x_markup_parse_context_new(&markup_parser, X_MARKUP_DEFAULT_FLAGS, parse_data, (XDestroyNotify)parse_data_free);
    retval = x_markup_parse_context_parse(context, buffer, length, &parse_error);
    if (!retval) {
        x_propagate_error(error, parse_error);
    } else {
        retval = x_markup_parse_context_end_parse(context, &end_error);
        if (!retval) {
            x_propagate_error(error, end_error);
        }
    }

    x_markup_parse_context_free(context);
    return retval;
}

static xchar *x_bookmark_file_dump(XBookmarkFile *bookmark, xsize *length, XError **error)
{
    XList *l;
    xchar *buffer;
    XString *retval;

    retval = x_string_sized_new(4096);

    x_string_append(retval,
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<" XBEL_ROOT_ELEMENT " " XBEL_VERSION_ATTRIBUTE "=\"" XBEL_VERSION "\"\n"
            "      xmlns:" BOOKMARK_NAMESPACE_NAME "=\"" BOOKMARK_NAMESPACE_URI "\"\n"
            "      xmlns:" MIME_NAMESPACE_NAME     "=\"" MIME_NAMESPACE_URI "\"\n>");

    if (bookmark->title) {
        xchar *escaped_title;

        escaped_title = x_markup_escape_text(bookmark->title, -1);
        buffer = x_strconcat("  "
                    "<" XBEL_TITLE_ELEMENT ">",
                    escaped_title,
                    "</" XBEL_TITLE_ELEMENT ">\n", NULL);
        x_string_append(retval, buffer);

        x_free(buffer);
        x_free(escaped_title);
    }

    if (bookmark->description) {
        xchar *escaped_desc;

        escaped_desc = x_markup_escape_text(bookmark->description, -1);
        buffer = x_strconcat("  "
                    "<" XBEL_DESC_ELEMENT ">",
                    escaped_desc,
                    "</" XBEL_DESC_ELEMENT ">\n", NULL);
        x_string_append(retval, buffer);

        x_free(buffer);
        x_free(escaped_desc);
       }

    if (!bookmark->items) {
        goto out;
    } else {
        retval = x_string_append(retval, "\n");
    }

    for (l = x_list_last(bookmark->items); l != NULL; l = l->prev) {
        xchar *item_dump;
        BookmarkItem *item = (BookmarkItem *)l->data;

        item_dump = bookmark_item_dump(item);
        if (!item_dump) {
            continue;
        }

        retval = x_string_append(retval, item_dump);
        x_free(item_dump);
    }

out:
    x_string_append(retval, "</" XBEL_ROOT_ELEMENT ">");
    if (length) {
        *length = retval->len;
    }

    return x_string_free(retval, FALSE);
}

static xboolean timestamp_from_iso8601(const xchar *iso_date, XDateTime **out_date_time, XError **error)
{
    XDateTime *dt = x_date_time_new_from_iso8601(iso_date, NULL);
    if (dt == NULL) {
        x_set_error(error, X_BOOKMARK_FILE_ERROR, X_BOOKMARK_FILE_ERROR_READ, _("Invalid date/time ‘%s’ in bookmark file"), iso_date);
        return FALSE;
    }

    *out_date_time = x_steal_pointer(&dt);
    return TRUE;
}

X_DEFINE_QUARK(x-bookmark-file-error-quark, x_bookmark_file_error)

XBookmarkFile *x_bookmark_file_new(void)
{
    XBookmarkFile *bookmark;

    bookmark = x_new(XBookmarkFile, 1);
    x_bookmark_file_init(bookmark);

    return bookmark;
}

XBookmarkFile *x_bookmark_file_copy(XBookmarkFile *bookmark)
{
    XList *l;
    XBookmarkFile *copy;

    x_return_val_if_fail(bookmark != NULL, NULL);

    copy = x_bookmark_file_new();
    copy->title = x_strdup(bookmark->title);
    copy->description = x_strdup(bookmark->description);
    copy->items = x_list_copy_deep(bookmark->items, (XCopyFunc)bookmark_item_copy, NULL);

    for (l = copy->items; l; l = l->next) {
        BookmarkItem *item = (BookmarkItem *)l->data;
        x_hash_table_insert(copy->items_by_uri, item->uri, item);
    }

    x_assert(x_hash_table_size(copy->items_by_uri) == x_hash_table_size(bookmark->items_by_uri));

    return copy;
}

void x_bookmark_file_free(XBookmarkFile *bookmark)
{
    if (!bookmark) {
        return;
    }

    x_bookmark_file_clear(bookmark);
    x_free(bookmark);
}

xboolean x_bookmark_file_load_from_data(XBookmarkFile *bookmark, const xchar *data, xsize length, XError **error)
{
    xboolean retval;
    XError *parse_error;

    x_return_val_if_fail(bookmark != NULL, FALSE);

    if (length == (xsize)-1) {
        length = strlen(data);
    }

    if (bookmark->items) {
        x_bookmark_file_clear(bookmark);
        x_bookmark_file_init(bookmark);
    }

    parse_error = NULL;
    retval = x_bookmark_file_parse(bookmark, data, length, &parse_error);
    if (!retval) {
        x_propagate_error(error, parse_error);
    }

    return retval;
}

xboolean x_bookmark_file_load_from_file(XBookmarkFile *bookmark, const xchar *filename, XError **error)
{
    xsize len;
    xboolean ret = FALSE;
    xchar *buffer = NULL;

    x_return_val_if_fail(bookmark != NULL, FALSE);
    x_return_val_if_fail(filename != NULL, FALSE);

    if (!x_file_get_contents(filename, &buffer, &len, error)) {
        goto out;
    }

    if (!x_bookmark_file_load_from_data(bookmark, buffer, len, error)) {
        goto out;
    }

    ret = TRUE;
out:
    x_free(buffer);
    return ret;
}

static xchar *find_file_in_data_dirs(const xchar *file, xchar ***dirs, XError **error)
{
    xchar **data_dirs, *data_dir, *path;

    path = NULL;
    if (dirs == NULL) {
        return NULL;
    }

    data_dirs = *dirs;
    path = NULL;

    while (data_dirs && (data_dir = *data_dirs) && !path) {
        xchar *candidate_file, *sub_dir;

        candidate_file = (xchar *)file;
        sub_dir = x_strdup("");

        while (candidate_file != NULL && !path) {
            xchar *p;

            path = x_build_filename(data_dir, sub_dir, candidate_file, NULL);
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

    *dirs = data_dirs;
    if (!path) {
        x_set_error_literal(error, X_BOOKMARK_FILE_ERROR, X_BOOKMARK_FILE_ERROR_FILE_NOT_FOUND, _("No valid bookmark file found in data dirs"));
        return NULL;
    }

    return path;
}

xboolean x_bookmark_file_load_from_data_dirs(XBookmarkFile *bookmark, const xchar *file, xchar **full_path, XError **error)
{
    xsize i, j;
    xchar *output_path;
    xboolean found_file;
    XError *file_error = NULL;
    const xchar *user_data_dir;
    xchar **all_data_dirs, **data_dirs;
    const xchar *const *system_data_dirs;

    x_return_val_if_fail(bookmark != NULL, FALSE);
    x_return_val_if_fail(!x_path_is_absolute(file), FALSE);

    user_data_dir = x_get_user_data_dir();
    system_data_dirs = x_get_system_data_dirs();
    all_data_dirs = x_new0(xchar *, x_strv_length ((xchar **)system_data_dirs) + 2);

    i = 0;
    all_data_dirs[i++] = x_strdup(user_data_dir);

    j = 0;
    while (system_data_dirs[j] != NULL) {
        all_data_dirs[i++] = x_strdup(system_data_dirs[j++]);
    }

    found_file = FALSE;
    data_dirs = all_data_dirs;
    output_path = NULL;

    while (*data_dirs != NULL && !found_file) {
        x_free(output_path);

        output_path = find_file_in_data_dirs(file, &data_dirs, &file_error);
        if (file_error) {
            x_propagate_error(error, file_error);
            break;
        }

        found_file = x_bookmark_file_load_from_file(bookmark, output_path, &file_error);
        if (file_error) {
            x_propagate_error(error, file_error);
            break;
        }
    }

    if (found_file && full_path) {
        *full_path = output_path;
    } else {
        x_free(output_path);
    }

    x_strfreev(all_data_dirs);
    return found_file;
}

xchar *x_bookmark_file_to_data(XBookmarkFile *bookmark, xsize *length, XError **error)
{
    xchar *retval;
    XError *write_error = NULL;

    x_return_val_if_fail(bookmark != NULL, NULL);

    retval = x_bookmark_file_dump(bookmark, length, &write_error);
    if (write_error) {
        x_propagate_error(error, write_error);
        return NULL;
    }

    return retval;
}

xboolean x_bookmark_file_to_file(XBookmarkFile *bookmark, const xchar *filename, XError **error)
{
    xsize len;
    xchar *data;
    xboolean retval;
    XError *data_error, *write_error;

    x_return_val_if_fail(bookmark != NULL, FALSE);
    x_return_val_if_fail(filename != NULL, FALSE);

    data_error = NULL;
    data = x_bookmark_file_to_data(bookmark, &len, &data_error);

    if (data_error) {
        x_propagate_error(error, data_error);
        return FALSE;
    }

    write_error = NULL;
    x_file_set_contents(filename, data, len, &write_error);
    if (write_error) {
        x_propagate_error(error, write_error);
        retval = FALSE;
    } else {
        retval = TRUE;
    }
    x_free(data);

    return retval;
}

static BookmarkItem *x_bookmark_file_lookup_item(XBookmarkFile *bookmark, const xchar *uri)
{
    x_warn_if_fail(bookmark != NULL && uri != NULL);
    return (BookmarkItem *)x_hash_table_lookup(bookmark->items_by_uri, uri);
}

static void x_bookmark_file_add_item(XBookmarkFile *bookmark, BookmarkItem *item, XError **error)
{
    x_warn_if_fail(bookmark != NULL);
    x_warn_if_fail(item != NULL);

    if (X_UNLIKELY(x_bookmark_file_has_item(bookmark, item->uri))) {
        x_set_error(error, X_BOOKMARK_FILE_ERROR, X_BOOKMARK_FILE_ERROR_INVALID_URI, _("A bookmark for URI “%s” already exists"), item->uri);
        return;
    }

    bookmark->items = x_list_prepend(bookmark->items, item);
    x_hash_table_replace(bookmark->items_by_uri, item->uri, item);

    if (item->added == NULL) {
        item->added = x_date_time_new_now_utc();
    }

    if (item->modified == NULL) {
        item->modified = x_date_time_new_now_utc();
    }

    if (item->visited == NULL) {
        item->visited = x_date_time_new_now_utc();
    }
}

xboolean x_bookmark_file_remove_item(XBookmarkFile *bookmark, const xchar *uri, XError **error)
{
    BookmarkItem *item;

    x_return_val_if_fail(bookmark != NULL, FALSE);
    x_return_val_if_fail(uri != NULL, FALSE);

    item = x_bookmark_file_lookup_item(bookmark, uri);
    if (!item) {
        x_set_error(error, X_BOOKMARK_FILE_ERROR, X_BOOKMARK_FILE_ERROR_URI_NOT_FOUND, _("No bookmark found for URI “%s”"), uri);
        return FALSE;
    }

    bookmark->items = x_list_remove(bookmark->items, item);
    x_hash_table_remove(bookmark->items_by_uri, item->uri);
    bookmark_item_free(item);

    return TRUE;
}

xboolean x_bookmark_file_has_item(XBookmarkFile *bookmark, const xchar *uri)
{
    x_return_val_if_fail(bookmark != NULL, FALSE);
    x_return_val_if_fail(uri != NULL, FALSE);

    return (NULL != x_hash_table_lookup(bookmark->items_by_uri, uri));
}

xchar **x_bookmark_file_get_uris(XBookmarkFile *bookmark, xsize *length)
{
    XList *l;
    xchar **uris;
    xsize i, n_items;

    x_return_val_if_fail(bookmark != NULL, NULL);

    n_items = x_list_length(bookmark->items);
    uris = x_new0(xchar *, n_items + 1);

    for (l = x_list_last(bookmark->items), i = 0; l != NULL; l = l->prev) {
        BookmarkItem *item = (BookmarkItem *)l->data;
        x_warn_if_fail(item != NULL);
        uris[i++] = x_strdup(item->uri);
    }
    uris[i] = NULL;

    if (length) {
        *length = i;
    }

    return uris;
}

void x_bookmark_file_set_title(XBookmarkFile *bookmark, const xchar *uri, const xchar *title)
{
    x_return_if_fail(bookmark != NULL);

    if (!uri) {
        x_free(bookmark->title);
        bookmark->title = x_strdup(title);
    } else {
        BookmarkItem *item;

        item = x_bookmark_file_lookup_item(bookmark, uri);
        if (!item) {
            item = bookmark_item_new(uri);
            x_bookmark_file_add_item(bookmark, item, NULL);
        }

        x_free(item->title);
        item->title = x_strdup(title);

        bookmark_item_touch_modified(item);
    }
}

xchar *x_bookmark_file_get_title(XBookmarkFile *bookmark, const xchar *uri, XError **error)
{
    BookmarkItem *item;

    x_return_val_if_fail(bookmark != NULL, NULL);

    if (!uri) {
        return x_strdup(bookmark->title);
    }

    item = x_bookmark_file_lookup_item(bookmark, uri);
    if (!item) {
        x_set_error(error, X_BOOKMARK_FILE_ERROR, X_BOOKMARK_FILE_ERROR_URI_NOT_FOUND, _("No bookmark found for URI “%s”"), uri);
        return NULL;
    }

    return x_strdup(item->title);
}

void x_bookmark_file_set_description(XBookmarkFile *bookmark, const xchar *uri, const xchar *description)
{
    x_return_if_fail(bookmark != NULL);

    if (!uri) {
        x_free(bookmark->description);
        bookmark->description = x_strdup(description);
    } else {
        BookmarkItem *item;

        item = x_bookmark_file_lookup_item(bookmark, uri);
        if (!item) {
            item = bookmark_item_new(uri);
            x_bookmark_file_add_item(bookmark, item, NULL);
        }

        x_free(item->description);
        item->description = x_strdup(description);

        bookmark_item_touch_modified(item);
    }
}

xchar *x_bookmark_file_get_description(XBookmarkFile *bookmark, const xchar *uri, XError **error)
{
    BookmarkItem *item;

    x_return_val_if_fail(bookmark != NULL, NULL);

    if (!uri) {
        return x_strdup(bookmark->description);
    }

    item = x_bookmark_file_lookup_item(bookmark, uri);
    if (!item) {
        x_set_error(error, X_BOOKMARK_FILE_ERROR, X_BOOKMARK_FILE_ERROR_URI_NOT_FOUND, _("No bookmark found for URI “%s”"), uri);
        return NULL;
    }

    return x_strdup(item->description);
}

void x_bookmark_file_set_mime_type(XBookmarkFile *bookmark, const xchar *uri, const xchar *mime_type)
{
    BookmarkItem *item;

    x_return_if_fail(bookmark != NULL);
    x_return_if_fail(uri != NULL);
    x_return_if_fail(mime_type != NULL);

    item = x_bookmark_file_lookup_item(bookmark, uri);
    if (!item) {
        item = bookmark_item_new(uri);
        x_bookmark_file_add_item(bookmark, item, NULL);
    }

    if (!item->metadata) {
        item->metadata = bookmark_metadata_new();
    }
    x_free(item->metadata->mime_type);

    item->metadata->mime_type = x_strdup(mime_type);
    bookmark_item_touch_modified(item);
}

xchar *x_bookmark_file_get_mime_type(XBookmarkFile *bookmark, const xchar *uri, XError **error)
{
    BookmarkItem *item;

    x_return_val_if_fail(bookmark != NULL, NULL);
    x_return_val_if_fail(uri != NULL, NULL);

    item = x_bookmark_file_lookup_item(bookmark, uri);
    if (!item) {
        x_set_error(error, X_BOOKMARK_FILE_ERROR, X_BOOKMARK_FILE_ERROR_URI_NOT_FOUND, _("No bookmark found for URI “%s”"), uri);
        return NULL;
    }

    if (!item->metadata) {
        x_set_error(error, X_BOOKMARK_FILE_ERROR, X_BOOKMARK_FILE_ERROR_INVALID_VALUE, _("No MIME type defined in the bookmark for URI “%s”"), uri);
        return NULL;
    }

    return x_strdup(item->metadata->mime_type);
}

void x_bookmark_file_set_is_private(XBookmarkFile *bookmark, const xchar *uri, xboolean is_private)
{
    BookmarkItem *item;

    x_return_if_fail(bookmark != NULL);
    x_return_if_fail(uri != NULL);

    item = x_bookmark_file_lookup_item(bookmark, uri);
    if (!item) {
        item = bookmark_item_new(uri);
        x_bookmark_file_add_item(bookmark, item, NULL);
    }

    if (!item->metadata) {
        item->metadata = bookmark_metadata_new();
    }

    item->metadata->is_private = (is_private == TRUE);
    bookmark_item_touch_modified(item);
}

xboolean x_bookmark_file_get_is_private(XBookmarkFile *bookmark, const xchar *uri, XError **error)
{
    BookmarkItem *item;

    x_return_val_if_fail(bookmark != NULL, FALSE);
    x_return_val_if_fail(uri != NULL, FALSE);

    item = x_bookmark_file_lookup_item(bookmark, uri);
    if (!item) {
        x_set_error(error, X_BOOKMARK_FILE_ERROR, X_BOOKMARK_FILE_ERROR_URI_NOT_FOUND, _("No bookmark found for URI “%s”"), uri);
        return FALSE;
    }

    if (!item->metadata) {
        x_set_error(error, X_BOOKMARK_FILE_ERROR, X_BOOKMARK_FILE_ERROR_INVALID_VALUE, _("No private flag has been defined in bookmark for URI “%s”"), uri);
        return FALSE;
    }

    return item->metadata->is_private;
}

void x_bookmark_file_set_added(XBookmarkFile *bookmark, const xchar *uri, time_t added)
{
    XDateTime *added_dt = (added != (time_t) -1) ? x_date_time_new_from_unix_utc(added) : x_date_time_new_now_utc();
    x_bookmark_file_set_added_date_time(bookmark, uri, added_dt);
    x_date_time_unref (added_dt);
}

void x_bookmark_file_set_added_date_time(XBookmarkFile *bookmark, const char *uri, XDateTime *added)
{
    BookmarkItem *item;

    x_return_if_fail(bookmark != NULL);
    x_return_if_fail(uri != NULL);
    x_return_if_fail(added != NULL);

    item = x_bookmark_file_lookup_item(bookmark, uri);
    if (!item) {
        item = bookmark_item_new(uri);
        x_bookmark_file_add_item(bookmark, item, NULL);
    }

    x_clear_pointer(&item->added, x_date_time_unref);
    item->added = x_date_time_ref(added);
    x_clear_pointer(&item->modified, x_date_time_unref);
    item->modified = x_date_time_ref(added);
}

time_t x_bookmark_file_get_added(XBookmarkFile *bookmark, const xchar *uri, XError **error)
{
    XDateTime *added = x_bookmark_file_get_added_date_time(bookmark, uri, error);
    return (added != NULL) ? x_date_time_to_unix(added) : (time_t) -1;
}

XDateTime *x_bookmark_file_get_added_date_time(XBookmarkFile *bookmark, const char *uri, XError **error)
{
    BookmarkItem *item;

    x_return_val_if_fail(bookmark != NULL, NULL);
    x_return_val_if_fail(uri != NULL, NULL);
    x_return_val_if_fail(error == NULL || *error == NULL, NULL);

    item = x_bookmark_file_lookup_item(bookmark, uri);
    if (!item) {
        x_set_error(error, X_BOOKMARK_FILE_ERROR, X_BOOKMARK_FILE_ERROR_URI_NOT_FOUND, _("No bookmark found for URI “%s”"), uri);
        return NULL;
    }

    return item->added;
}

void x_bookmark_file_set_modified(XBookmarkFile *bookmark, const xchar *uri, time_t modified)
{
    XDateTime *modified_dt = (modified != (time_t) -1) ? x_date_time_new_from_unix_utc(modified) : x_date_time_new_now_utc();
    x_bookmark_file_set_modified_date_time(bookmark, uri, modified_dt);
    x_date_time_unref(modified_dt);
}

void x_bookmark_file_set_modified_date_time(XBookmarkFile *bookmark, const char *uri, XDateTime *modified)
{
    BookmarkItem *item;

    x_return_if_fail(bookmark != NULL);
    x_return_if_fail(uri != NULL);
    x_return_if_fail(modified != NULL);

    item = x_bookmark_file_lookup_item(bookmark, uri);
    if (!item) {
        item = bookmark_item_new(uri);
        x_bookmark_file_add_item(bookmark, item, NULL);
    }

    x_clear_pointer(&item->modified, x_date_time_unref);
    item->modified = x_date_time_ref(modified);
}

time_t x_bookmark_file_get_modified(XBookmarkFile *bookmark, const xchar *uri, XError **error)
{
    XDateTime *modified = x_bookmark_file_get_modified_date_time(bookmark, uri, error);
    return (modified != NULL) ? x_date_time_to_unix(modified) : (time_t) -1;
}

XDateTime *x_bookmark_file_get_modified_date_time(XBookmarkFile *bookmark, const char *uri, XError **error)
{
    BookmarkItem *item;

    x_return_val_if_fail(bookmark != NULL, NULL);
    x_return_val_if_fail(uri != NULL, NULL);
    x_return_val_if_fail(error == NULL || *error == NULL, NULL);

    item = x_bookmark_file_lookup_item(bookmark, uri);
    if (!item) {
        x_set_error(error, X_BOOKMARK_FILE_ERROR, X_BOOKMARK_FILE_ERROR_URI_NOT_FOUND, _("No bookmark found for URI “%s”"), uri);
        return NULL;
    }

    return item->modified;
}

void x_bookmark_file_set_visited(XBookmarkFile *bookmark, const xchar *uri, time_t visited)
{
    XDateTime *visited_dt = (visited != (time_t) -1) ? x_date_time_new_from_unix_utc(visited) : x_date_time_new_now_utc();
    x_bookmark_file_set_visited_date_time(bookmark, uri, visited_dt);
    x_date_time_unref (visited_dt);
}

void x_bookmark_file_set_visited_date_time(XBookmarkFile *bookmark, const char *uri, XDateTime *visited)
{
    BookmarkItem *item;

    x_return_if_fail(bookmark != NULL);
    x_return_if_fail(uri != NULL);
    x_return_if_fail(visited != NULL);

    item = x_bookmark_file_lookup_item(bookmark, uri);
    if (!item) {
        item = bookmark_item_new(uri);
        x_bookmark_file_add_item(bookmark, item, NULL);
    }

    x_clear_pointer(&item->visited, x_date_time_unref);
    item->visited = x_date_time_ref(visited);
}

time_t x_bookmark_file_get_visited(XBookmarkFile *bookmark, const xchar *uri, XError **error)
{
    XDateTime *visited = x_bookmark_file_get_visited_date_time(bookmark, uri, error);
    return (visited != NULL) ? x_date_time_to_unix(visited) : (time_t) -1;
}

XDateTime *x_bookmark_file_get_visited_date_time(XBookmarkFile *bookmark, const char *uri, XError **error)
{
    BookmarkItem *item;

    x_return_val_if_fail(bookmark != NULL, NULL);
    x_return_val_if_fail(uri != NULL, NULL);
    x_return_val_if_fail(error == NULL || *error == NULL, NULL);

    item = x_bookmark_file_lookup_item(bookmark, uri);
    if (!item) {
        x_set_error(error, X_BOOKMARK_FILE_ERROR, X_BOOKMARK_FILE_ERROR_URI_NOT_FOUND, _("No bookmark found for URI “%s”"), uri);
        return NULL;
    }

    return item->visited;
}

xboolean x_bookmark_file_has_group(XBookmarkFile *bookmark, const xchar *uri, const xchar *group, XError **error)
{
    XList *l;
    BookmarkItem *item;

    x_return_val_if_fail(bookmark != NULL, FALSE);
    x_return_val_if_fail(uri != NULL, FALSE);

    item = x_bookmark_file_lookup_item(bookmark, uri);
    if (!item) {
        x_set_error(error, X_BOOKMARK_FILE_ERROR, X_BOOKMARK_FILE_ERROR_URI_NOT_FOUND, _("No bookmark found for URI “%s”"), uri);
        return FALSE;
    }

    if (!item->metadata) {
        return FALSE;
    }

    for (l = item->metadata->groups; l != NULL; l = l->next) {
        if (strcmp((const char *)l->data, group) == 0) {
            return TRUE;
        }
    }

    return FALSE;
}

void x_bookmark_file_add_group(XBookmarkFile *bookmark, const xchar *uri, const xchar *group)
{
    BookmarkItem *item;

    x_return_if_fail(bookmark != NULL);
    x_return_if_fail(uri != NULL);
    x_return_if_fail(group != NULL && group[0] != '\0');

    item = x_bookmark_file_lookup_item(bookmark, uri);
    if (!item) {
        item = bookmark_item_new(uri);
        x_bookmark_file_add_item(bookmark, item, NULL);
    }

    if (!item->metadata) {
        item->metadata = bookmark_metadata_new();
    }

    if (!x_bookmark_file_has_group(bookmark, uri, group, NULL)) {
        item->metadata->groups = x_list_prepend(item->metadata->groups, x_strdup(group));
        bookmark_item_touch_modified(item);
    }
}

xboolean x_bookmark_file_remove_group(XBookmarkFile *bookmark, const xchar *uri, const xchar *group, XError **error)
{
    XList *l;
    BookmarkItem *item;

    x_return_val_if_fail(bookmark != NULL, FALSE);
    x_return_val_if_fail(uri != NULL, FALSE);

    item = x_bookmark_file_lookup_item(bookmark, uri);
    if (!item) {
        x_set_error(error, X_BOOKMARK_FILE_ERROR, X_BOOKMARK_FILE_ERROR_URI_NOT_FOUND, _("No bookmark found for URI “%s”"), uri);
        return FALSE;
    }

    if (!item->metadata) {
        x_set_error(error, X_BOOKMARK_FILE_ERROR, X_BOOKMARK_FILE_ERROR_INVALID_VALUE, _("No groups set in bookmark for URI “%s”"), uri);
        return FALSE;
    }

    for (l = item->metadata->groups; l != NULL; l = l->next) {
        if (strcmp((const char *)l->data, group) == 0) {
            item->metadata->groups = x_list_remove_link(item->metadata->groups, l);
            x_free(l->data);
            x_list_free_1(l);

            bookmark_item_touch_modified(item);
            return TRUE;
        }
    }

    return FALSE;
}

void x_bookmark_file_set_groups(XBookmarkFile *bookmark, const xchar *uri, const xchar **groups, xsize length)
{
    xsize i;
    BookmarkItem *item;

    x_return_if_fail(bookmark != NULL);
    x_return_if_fail(uri != NULL);
    x_return_if_fail(groups != NULL);

    item = x_bookmark_file_lookup_item(bookmark, uri);
    if (!item) {
        item = bookmark_item_new(uri);
        x_bookmark_file_add_item(bookmark, item, NULL);
    }

    if (!item->metadata) {
        item->metadata = bookmark_metadata_new();
    }

    x_list_free_full(item->metadata->groups, x_free);
    item->metadata->groups = NULL;

    if (groups) {
        for (i = 0; i < length && groups[i] != NULL; i++) {
            item->metadata->groups = x_list_append(item->metadata->groups, x_strdup(groups[i]));
        }
    }

    bookmark_item_touch_modified(item);
}

xchar **x_bookmark_file_get_groups(XBookmarkFile *bookmark, const xchar *uri, xsize *length, XError **error)
{
    XList *l;
    xsize len, i;
    xchar **retval;
    BookmarkItem *item;

    x_return_val_if_fail(bookmark != NULL, NULL);
    x_return_val_if_fail(uri != NULL, NULL);

    item = x_bookmark_file_lookup_item(bookmark, uri);
    if (!item) {
        x_set_error(error, X_BOOKMARK_FILE_ERROR, X_BOOKMARK_FILE_ERROR_URI_NOT_FOUND, _("No bookmark found for URI “%s”"), uri);
        return NULL;
    }

    if (!item->metadata) {
        if (length) {
            *length = 0;
        }

        return NULL;
    }

    len = x_list_length(item->metadata->groups);
    retval = x_new0(xchar *, len + 1);
    for (l = x_list_last(item->metadata->groups), i = 0; l != NULL; l = l->prev) {
        xchar *group_name = (xchar *)l->data;

        x_warn_if_fail(group_name != NULL);
        retval[i++] = x_strdup(group_name);
    }
    retval[i] = NULL;

    if (length) {
        *length = len;
    }

    return retval;
}

void x_bookmark_file_add_application(XBookmarkFile *bookmark, const xchar *uri, const xchar *name, const xchar *exec)
{
    XDateTime *stamp;
    BookmarkItem *item;
    xchar *app_name, *app_exec;

    x_return_if_fail(bookmark != NULL);
    x_return_if_fail(uri != NULL);

    item = x_bookmark_file_lookup_item(bookmark, uri);
    if (!item) {
        item = bookmark_item_new(uri);
        x_bookmark_file_add_item(bookmark, item, NULL);
    }

    if (name && name[0] != '\0') {
        app_name = x_strdup(name);
    } else {
        app_name = x_strdup(x_get_application_name());
    }

    if (exec && exec[0] != '\0') {
        app_exec = x_strdup(exec);
    } else {
        app_exec = x_strjoin(" ", x_get_prgname(), "%u", NULL);
    }

    stamp = x_date_time_new_now_utc();
    x_bookmark_file_set_application_info(bookmark, uri, app_name, app_exec, -1, stamp, NULL);

    x_date_time_unref(stamp);
    x_free(app_exec);
    x_free(app_name);
}

xboolean x_bookmark_file_remove_application(XBookmarkFile *bookmark, const xchar *uri, const xchar *name, XError **error)
{
    xboolean retval;
    XError *set_error;

    x_return_val_if_fail(bookmark != NULL, FALSE);
    x_return_val_if_fail(uri != NULL, FALSE);
    x_return_val_if_fail(name != NULL, FALSE);

    set_error = NULL;
    retval = x_bookmark_file_set_application_info(bookmark, uri, name, "", 0, NULL, &set_error);
    if (set_error) {
        x_propagate_error(error, set_error);
        return FALSE;
    }

    return retval;
}

xboolean x_bookmark_file_has_application(XBookmarkFile  *bookmark, const xchar *uri, const xchar *name, XError **error)
{
    BookmarkItem *item;

    x_return_val_if_fail(bookmark != NULL, FALSE);
    x_return_val_if_fail(uri != NULL, FALSE);
    x_return_val_if_fail(name != NULL, FALSE);

    item = x_bookmark_file_lookup_item(bookmark, uri);
    if (!item) {
        x_set_error(error, X_BOOKMARK_FILE_ERROR, X_BOOKMARK_FILE_ERROR_URI_NOT_FOUND, _("No bookmark found for URI “%s”"), uri);
        return FALSE;
    }

    return (NULL != bookmark_item_lookup_app_info(item, name));
}

xboolean x_bookmark_file_set_app_info(XBookmarkFile *bookmark, const xchar *uri, const xchar *name, const xchar *exec, xint count, time_t stamp, XError **error)
{
    xboolean retval;
    XDateTime *stamp_dt = (stamp != (time_t) -1) ? x_date_time_new_from_unix_utc(stamp) : x_date_time_new_now_utc();

    retval = x_bookmark_file_set_application_info(bookmark, uri, name, exec, count, stamp_dt, error);
    x_date_time_unref(stamp_dt);

    return retval;
}

xboolean x_bookmark_file_set_application_info(XBookmarkFile *bookmark, const char *uri, const char *name, const char *exec, int count, XDateTime *stamp, XError **error)
{
    BookmarkItem *item;
    BookmarkAppInfo *ai;

    x_return_val_if_fail(bookmark != NULL, FALSE);
    x_return_val_if_fail(uri != NULL, FALSE);
    x_return_val_if_fail(name != NULL, FALSE);
    x_return_val_if_fail(exec != NULL, FALSE);
    x_return_val_if_fail(count == 0 || stamp != NULL, FALSE);
    x_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    item = x_bookmark_file_lookup_item(bookmark, uri);
    if (!item) {
        if (count == 0) {
            x_set_error(error, X_BOOKMARK_FILE_ERROR, X_BOOKMARK_FILE_ERROR_URI_NOT_FOUND, _("No bookmark found for URI “%s”"), uri);
            return FALSE;
        } else {
            item = bookmark_item_new(uri);
            x_bookmark_file_add_item(bookmark, item, NULL);
        }
    }

    if (!item->metadata) {
        item->metadata = bookmark_metadata_new();
    }

    ai = bookmark_item_lookup_app_info(item, name);
    if (!ai) {
        if (count == 0) {
            x_set_error(error, X_BOOKMARK_FILE_ERROR, X_BOOKMARK_FILE_ERROR_APP_NOT_REGISTERED, _("No application with name “%s” registered a bookmark for “%s”"), name, uri);
            return FALSE;
         } else {
            ai = bookmark_app_info_new (name);

            item->metadata->applications = x_list_prepend(item->metadata->applications, ai);
            x_hash_table_replace(item->metadata->apps_by_name, ai->name, ai);
        }
    }

    if (count == 0) {
        item->metadata->applications = x_list_remove(item->metadata->applications, ai);
        x_hash_table_remove(item->metadata->apps_by_name, ai->name);
        bookmark_app_info_free(ai);
        bookmark_item_touch_modified(item);

        return TRUE;
    } else if (count > 0) {
        ai->count = count;
    } else {
        ai->count += 1;
    }

    x_clear_pointer(&ai->stamp, x_date_time_unref);
    ai->stamp = x_date_time_ref(stamp);

    if (exec && exec[0] != '\0') {
        x_free(ai->exec);
        ai->exec = x_shell_quote(exec);
    }

    bookmark_item_touch_modified(item);
    return TRUE;
}

static xchar *expand_exec_line(const xchar *exec_fmt, const xchar *uri)
{
    xchar ch;
    XString *exec;

    exec = x_string_sized_new(512);
    while ((ch = *exec_fmt++) != '\0') {
        if (ch != '%') {
            exec = x_string_append_c(exec, ch);
            continue;
        }

        ch = *exec_fmt++;
        switch (ch) {
            case '\0':
            goto out;
            case 'U':
            case 'u':
                x_string_append(exec, uri);
                break;

        case 'F':
        case 'f': {
            xchar *file = x_filename_from_uri(uri, NULL, NULL);
            if (file) {
                x_string_append(exec, file);
                x_free(file);
            } else {
                x_string_free(exec, TRUE);
                return NULL;
            }
        }
        break;

        case '%':
        default:
            exec = x_string_append_c(exec, ch);
            break;
    }
}

out:
    return x_string_free(exec, FALSE);
}

xboolean x_bookmark_file_get_app_info(XBookmarkFile *bookmark, const xchar *uri, const xchar *name, xchar **exec, xuint *count, time_t *stamp, XError **error)
{
    xboolean retval;
    XDateTime *stamp_dt = NULL;

    retval = x_bookmark_file_get_application_info(bookmark, uri, name, exec, count, &stamp_dt, error);
    if (!retval) {
        return FALSE;
    }

    if (stamp != NULL) {
        *stamp = x_date_time_to_unix(stamp_dt);
    }

    return TRUE;
}

xboolean x_bookmark_file_get_application_info(XBookmarkFile *bookmark, const char *uri, const char *name, char **exec, unsigned int *count, XDateTime **stamp, XError **error)
{
    BookmarkItem *item;
    BookmarkAppInfo *ai;

    x_return_val_if_fail(bookmark != NULL, FALSE);
    x_return_val_if_fail(uri != NULL, FALSE);
    x_return_val_if_fail(name != NULL, FALSE);
    x_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    item = x_bookmark_file_lookup_item(bookmark, uri);
    if (!item) {
        x_set_error(error, X_BOOKMARK_FILE_ERROR, X_BOOKMARK_FILE_ERROR_URI_NOT_FOUND, _("No bookmark found for URI “%s”"), uri);
        return FALSE;
    }

    ai = bookmark_item_lookup_app_info(item, name);
    if (!ai) {
        x_set_error(error, X_BOOKMARK_FILE_ERROR, X_BOOKMARK_FILE_ERROR_APP_NOT_REGISTERED, _("No application with name “%s” registered a bookmark for “%s”"), name, uri);
        return FALSE;
    }

    if (exec) {
        xchar *command_line;
        XError *unquote_error = NULL;

        command_line = x_shell_unquote(ai->exec, &unquote_error);
        if (unquote_error) {
            x_propagate_error(error, unquote_error);
            return FALSE;
        }

        *exec = expand_exec_line (command_line, uri);
        if (!*exec) {
            x_set_error(error, X_BOOKMARK_FILE_ERROR, X_BOOKMARK_FILE_ERROR_INVALID_URI, _("Failed to expand exec line “%s” with URI “%s”"), ai->exec, uri);
            x_free(command_line);

            return FALSE;
        } else {
            x_free(command_line);
        }
    }

    if (count) {
        *count = ai->count;
    }

    if (stamp) {
        *stamp = ai->stamp;
    }

    return TRUE;
}

xchar **x_bookmark_file_get_applications(XBookmarkFile *bookmark, const xchar *uri, xsize *length, XError **error)
{
    XList *l;
    xchar **apps;
    xsize i, n_apps;
    BookmarkItem *item;

    x_return_val_if_fail(bookmark != NULL, NULL);
    x_return_val_if_fail(uri != NULL, NULL);

    item = x_bookmark_file_lookup_item(bookmark, uri);
    if (!item) {
        x_set_error(error, X_BOOKMARK_FILE_ERROR, X_BOOKMARK_FILE_ERROR_URI_NOT_FOUND, _("No bookmark found for URI “%s”"), uri);
        return NULL;
    }

    if (!item->metadata) {
        if (length) {
            *length = 0;
        }

        return NULL;
    }

    n_apps = x_list_length(item->metadata->applications);
    apps = x_new0(xchar *, n_apps + 1);

    for (l = x_list_last(item->metadata->applications), i = 0; l != NULL; l = l->prev) {
        BookmarkAppInfo *ai;
        ai = (BookmarkAppInfo *)l->data;

        x_warn_if_fail(ai != NULL);
        x_warn_if_fail(ai->name != NULL);

        apps[i++] = x_strdup(ai->name);
    }
    apps[i] = NULL;

    if (length) {
        *length = i;
    }

    return apps;
}

xint x_bookmark_file_get_size(XBookmarkFile *bookmark)
{
    x_return_val_if_fail(bookmark != NULL, 0);
    return x_list_length(bookmark->items);
}

xboolean x_bookmark_file_move_item(XBookmarkFile *bookmark, const xchar *old_uri, const xchar *new_uri, XError **error)
{
    BookmarkItem *item;

    x_return_val_if_fail(bookmark != NULL, FALSE);
    x_return_val_if_fail(old_uri != NULL, FALSE);

    item = x_bookmark_file_lookup_item(bookmark, old_uri);
    if (!item) {
        x_set_error(error, X_BOOKMARK_FILE_ERROR, X_BOOKMARK_FILE_ERROR_URI_NOT_FOUND, _("No bookmark found for URI “%s”"), old_uri);
        return FALSE;
    }

    if (new_uri && new_uri[0] != '\0') {
        if (x_strcmp0(old_uri, new_uri) == 0) {
            return TRUE;
        }

        if (x_bookmark_file_has_item(bookmark, new_uri)) {
            if (!x_bookmark_file_remove_item(bookmark, new_uri, error)) {
                return FALSE;
            }
        }

        x_hash_table_steal(bookmark->items_by_uri, item->uri);

        x_free(item->uri);
        item->uri = x_strdup(new_uri);
        bookmark_item_touch_modified(item);

        x_hash_table_replace(bookmark->items_by_uri, item->uri, item);
        return TRUE;
    } else {
        if (!x_bookmark_file_remove_item(bookmark, old_uri, error)) {
            return FALSE;
        }

        return TRUE;
    }
}

void x_bookmark_file_set_icon(XBookmarkFile *bookmark, const xchar *uri, const xchar *href, const xchar *mime_type)
{
    BookmarkItem *item;

    x_return_if_fail(bookmark != NULL);
    x_return_if_fail(uri != NULL);

    item = x_bookmark_file_lookup_item(bookmark, uri);
    if (!item) {
        item = bookmark_item_new(uri);
        x_bookmark_file_add_item(bookmark, item, NULL);
    }

    if (!item->metadata) {
        item->metadata = bookmark_metadata_new();
    }

    x_free(item->metadata->icon_href);
    x_free(item->metadata->icon_mime);

    item->metadata->icon_href = x_strdup(href);
    if (mime_type && mime_type[0] != '\0') {
        item->metadata->icon_mime = x_strdup(mime_type);
    } else {
        item->metadata->icon_mime = x_strdup("application/octet-stream");
    }

    bookmark_item_touch_modified(item);
}

xboolean x_bookmark_file_get_icon(XBookmarkFile *bookmark, const xchar *uri, xchar **href, xchar **mime_type, XError **error)
{
    BookmarkItem *item;

    x_return_val_if_fail(bookmark != NULL, FALSE);
    x_return_val_if_fail(uri != NULL, FALSE);

    item = x_bookmark_file_lookup_item(bookmark, uri);
    if (!item) {
        x_set_error(error, X_BOOKMARK_FILE_ERROR, X_BOOKMARK_FILE_ERROR_URI_NOT_FOUND, _("No bookmark found for URI “%s”"), uri);
        return FALSE;
    }

    if ((!item->metadata) || (!item->metadata->icon_href)) {
        return FALSE;
    }

    if (href) {
        *href = x_strdup(item->metadata->icon_href);
    }

    if (mime_type) {
        *mime_type = x_strdup(item->metadata->icon_mime);
    }

    return TRUE;
}
