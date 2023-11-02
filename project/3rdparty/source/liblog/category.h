#ifndef CATEGORY_H
#define CATEGORY_H

#include "thread.h"
#include "logc_defs.h"

struct liblog_category {
    char                  name[LOG_MAXLEN_PATH + 1];
    size_t                name_len;
    unsigned char         level_bitmap[32];
    unsigned char         level_bitmap_backup[32];
    struct logc_arraylist *fit_rules;
    struct logc_arraylist *fit_rules_backup;
};

struct liblog_category *liblog_category_new(const char *name, struct logc_arraylist *rules);

void liblog_category_del(struct liblog_category *a_category);
void liblog_category_profile(struct liblog_category *a_category, int flag);

void liblog_category_commit_rules(struct liblog_category *a_category);
void liblog_category_rollback_rules(struct liblog_category *a_category);
int liblog_category_output(struct liblog_category *a_category, struct liblog_thread *a_thread);
int liblog_category_update_rules(struct liblog_category *a_category, struct logc_arraylist *new_rules);

//#define liblog_category_needless_level(a_category, lv)      (a_category && !((a_category->level_bitmap[lv / 8] >> (7 - lv % 8)) & 0x01))
#define liblog_category_needless_level(a_category, lv)      a_category && (liblog_env_conf->level > lv || !((a_category->level_bitmap[lv/8] >> (7 - lv % 8)) & 0x01))

#endif
