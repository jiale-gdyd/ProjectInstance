#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "rule.h"
#include "fmacros.h"
#include "category.h"
#include "logc_defs.h"

void liblog_category_profile(struct liblog_category *a_category, int flag)
{
    int i;
    struct liblog_rule *a_rule;

    logc_assert(a_category,);
    logc_profile(flag, "--category[%p][%s][%p]--", a_category, a_category->name, a_category->fit_rules);

    if (a_category->fit_rules) {
        logc_arraylist_foreach(a_category->fit_rules, i, a_rule) {
            liblog_rule_profile(a_rule, flag);
        }
    }
}

void liblog_category_del(struct liblog_category *a_category)
{
    logc_assert(a_category,);

    if (a_category->fit_rules) {
        logc_arraylist_del(a_category->fit_rules);
    }

    logc_debug("liblog_category_del[%p]", a_category);
    free(a_category);
}

static void liblog_cateogry_overlap_bitmap(struct liblog_category *a_category, struct liblog_rule *a_rule)
{
    size_t i;
    for (i = 0; i < sizeof(a_rule->level_bitmap); i++) {
        a_category->level_bitmap[i] |= a_rule->level_bitmap[i];
    }
}

static int liblog_category_obtain_rules(struct liblog_category *a_category, struct logc_arraylist *rules)
{
    int count = 0;
    int i, fit = 0;
    struct liblog_rule *a_rule;
    struct liblog_rule *wastebin_rule = NULL;

    if (a_category->fit_rules) {
        logc_arraylist_del(a_category->fit_rules);
    }

    memset(a_category->level_bitmap, 0x00, sizeof(a_category->level_bitmap));

    a_category->fit_rules = logc_arraylist_new(NULL);
    if (!(a_category->fit_rules)) {
        logc_error("logc_arraylist_new fail");
        return -1;
    }

    logc_arraylist_foreach(rules, i, a_rule) {
        fit = liblog_rule_match_category((struct liblog_rule *)a_rule, a_category->name);
        if (fit) {
            if (logc_arraylist_add(a_category->fit_rules, (struct liblog_rule *)a_rule)) {
                logc_error("logc_arrylist_add fail");
                goto err;
            }

            liblog_cateogry_overlap_bitmap(a_category, (struct liblog_rule *)a_rule);
            count++;
        }

        if (liblog_rule_is_wastebin((struct liblog_rule *)a_rule)) {
            wastebin_rule = (struct liblog_rule *)a_rule;
        }
    }

    if (count == 0) {
        if (wastebin_rule) {
            logc_debug("category[%s], no match rules, use wastebin_rule", a_category->name);
            if (logc_arraylist_add(a_category->fit_rules, wastebin_rule)) {
                logc_error("logc_arrylist_add fail");
                goto err;
            }

            liblog_cateogry_overlap_bitmap(a_category, wastebin_rule);
            count++;
        } else {
            logc_debug("category[%s], no match rules & no wastebin_rule", a_category->name);
        }
    }

    return 0;

err:
    logc_arraylist_del(a_category->fit_rules);
    a_category->fit_rules = NULL;

    return -1;
}

struct liblog_category *liblog_category_new(const char *name, struct logc_arraylist *rules)
{
    size_t len;
    struct liblog_category *a_category;

    logc_assert(name, NULL);
    logc_assert(rules, NULL);

    len = strlen(name);
    if (len > (sizeof(a_category->name) - 1)) {
        logc_error("name[%s] too long", name);
        return NULL;
    }

    a_category = (struct liblog_category *)calloc(1, sizeof(struct liblog_category));
    if (!a_category) {
        logc_error("calloc fail, errno[%d]", errno);
        return NULL;
    }

    strcpy(a_category->name, name);
    a_category->name_len = len;
    if (liblog_category_obtain_rules(a_category, rules)) {
        logc_error("liblog_category_fit_rules fail");
        goto err;
    }

    liblog_category_profile(a_category, LOGC_DEBUG);
    return a_category;

err:
    liblog_category_del(a_category);
    return NULL;
}

int liblog_category_update_rules(struct liblog_category *a_category, struct logc_arraylist *new_rules)
{
    logc_assert(a_category, -1);
    logc_assert(new_rules, -1);

    if (a_category->fit_rules_backup) {
        logc_arraylist_del(a_category->fit_rules_backup);
    }

    a_category->fit_rules_backup = a_category->fit_rules;
    a_category->fit_rules = NULL;

    memcpy(a_category->level_bitmap_backup, a_category->level_bitmap, sizeof(a_category->level_bitmap));

    if (liblog_category_obtain_rules(a_category, new_rules)) {
        logc_error("liblog_category_obtain_rules fail");
        a_category->fit_rules = NULL;

        return -1;
    }

    return 0;
}

void liblog_category_commit_rules(struct liblog_category *a_category)
{
    logc_assert(a_category, );

    if (!a_category->fit_rules_backup) {
        logc_warn("a_category->fit_rules_backup is NULL, never update before");
        return;
    }

    logc_arraylist_del(a_category->fit_rules_backup);

    a_category->fit_rules_backup = NULL;
    memset(a_category->level_bitmap_backup, 0x00, sizeof(a_category->level_bitmap_backup));
}

void liblog_category_rollback_rules(struct liblog_category *a_category)
{
    logc_assert(a_category, );

    if (!a_category->fit_rules_backup) {
        logc_warn("a_category->fit_rules_backup in NULL, never update before");
        return;
    }

    if (a_category->fit_rules) {
        logc_arraylist_del(a_category->fit_rules);
        a_category->fit_rules = a_category->fit_rules_backup;
        a_category->fit_rules_backup = NULL;
    } else {
        a_category->fit_rules = a_category->fit_rules_backup;
        a_category->fit_rules_backup = NULL;
    }

    memcpy(a_category->level_bitmap, a_category->level_bitmap_backup, sizeof(a_category->level_bitmap));
    memset(a_category->level_bitmap_backup, 0x00, sizeof(a_category->level_bitmap_backup));
}

int liblog_category_output(struct liblog_category *a_category, struct liblog_thread *a_thread)
{
    int i, rc = 0;
    struct liblog_rule *a_rule;

    logc_arraylist_foreach(a_category->fit_rules, i, a_rule) {
        rc = liblog_rule_output(a_rule, a_thread);
    }

    return rc;
}
