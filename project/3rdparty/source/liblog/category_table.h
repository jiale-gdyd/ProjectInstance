#ifndef CATEGORY_TABLE_H
#define CATEGORY_TABLE_H

#include "category.h"
#include "logc_defs.h"

struct logc_hashtable *liblog_category_table_new(void);
void liblog_category_table_del(struct logc_hashtable *categories);
void liblog_category_table_profile(struct logc_hashtable *categories, int flag);

void liblog_category_table_commit_rules(struct logc_hashtable *categories);
void liblog_category_table_rollback_rules(struct logc_hashtable *categories);
int liblog_category_table_update_rules(struct logc_hashtable *categories, struct logc_arraylist *new_rules);

struct liblog_category *liblog_category_table_fetch_category(struct logc_hashtable *categories, const char *category_name, struct logc_arraylist *rules);

#endif
