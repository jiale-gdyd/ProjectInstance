#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "logc_defs.h"
#include "category_table.h"

void liblog_category_table_profile(struct logc_hashtable *categories, int flag)
{
    struct liblog_category *a_category;
    struct logc_hashtable_entry *a_entry;

    logc_assert(categories, );
    logc_profile(flag, "-category_table[%p]-", categories);
    logc_hashtable_foreach(categories, a_entry) {
        a_category = (struct liblog_category *)a_entry->value;
        liblog_category_profile(a_category, flag);
    }
}

void liblog_category_table_del(struct logc_hashtable *categories)
{
    logc_assert(categories, );
    logc_hashtable_del(categories);
    logc_debug("liblog_category_table_del[%p]", categories);
}

struct logc_hashtable *liblog_category_table_new(void)
{
    struct logc_hashtable *categories;

    categories = (struct logc_hashtable *)logc_hashtable_new(20, (hashtable_hash_func)logc_hashtable_str_hash, (hashtable_equal_func)logc_hashtable_str_equal, NULL, (hashtable_del_func)liblog_category_del);
    if (!categories) {
        logc_error("logc_hashtable_new fail");
        return NULL;
    } else {
        liblog_category_table_profile(categories, LOGC_DEBUG);
        return categories;
    }
}

int liblog_category_table_update_rules(struct logc_hashtable *categories, struct logc_arraylist *new_rules)
{
    struct liblog_category *a_category;
    struct logc_hashtable_entry *a_entry;

    logc_assert(categories, -1);

    logc_hashtable_foreach(categories, a_entry) {
        a_category = (struct liblog_category *)a_entry->value;
        if (liblog_category_update_rules(a_category, new_rules)) {
            logc_error("liblog_category_update_rules fail, try rollback");
            return -1;
        }
    }

    return 0;
}

void liblog_category_table_commit_rules(struct logc_hashtable *categories)
{
    struct liblog_category *a_category;
    struct logc_hashtable_entry *a_entry;

    logc_assert(categories, );

    logc_hashtable_foreach(categories, a_entry) {
        a_category = (struct liblog_category *)a_entry->value;
        liblog_category_commit_rules(a_category);
    }
}

void liblog_category_table_rollback_rules(struct logc_hashtable *categories)
{
    struct liblog_category *a_category;
    struct logc_hashtable_entry *a_entry;

    logc_assert(categories, );

    logc_hashtable_foreach(categories, a_entry) {
        a_category = (struct liblog_category *)a_entry->value;
        liblog_category_rollback_rules(a_category);
    }
}

struct liblog_category *liblog_category_table_fetch_category(struct logc_hashtable *categories, const char *category_name, struct logc_arraylist *rules)
{
    struct liblog_category *a_category;

    logc_assert(categories, NULL);

    a_category = (struct liblog_category *)logc_hashtable_get(categories, category_name);
    if (a_category) {
        return a_category;
    }

    a_category = liblog_category_new(category_name, rules);
    if (!a_category) {
        logc_error("liblog_category_new fail");
        return NULL;
    }

    if (logc_hashtable_put(categories, a_category->name, a_category)) {
        logc_error("logc_hashtable_put fail");
        goto err;
    }

    return a_category;

err:
    liblog_category_del(a_category);
    return NULL;
}
