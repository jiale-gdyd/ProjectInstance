#include <errno.h>
#include <stdlib.h>
#include <pthread.h>

#include "logc_defs.h"
#include "logc_hashtable.h"

struct logc_hashtable {
    size_t                       nelem;

    struct logc_hashtable_entry  **tab;
    size_t                       tab_size;

    hashtable_hash_func          hash;
    hashtable_equal_func         equal;
    hashtable_del_func           key_del;
    hashtable_del_func           value_del;
};

struct logc_hashtable *logc_hashtable_new(size_t a_size, hashtable_hash_func hash, hashtable_equal_func equal, hashtable_del_func key_del, hashtable_del_func value_del)
{
    struct logc_hashtable *a_table;

    a_table = (struct logc_hashtable *)calloc(1, sizeof(*a_table));
    if (!a_table) {
        logc_error("calloc fail, errno[%d]", errno);
        return NULL;
    }

    a_table->tab = (struct logc_hashtable_entry **)calloc(a_size, sizeof(*(a_table->tab)));
    if (!a_table->tab) {
        logc_error("calloc fail, errno[%d]", errno);
        free(a_table);

        return NULL;
    }
    a_table->tab_size = a_size;

    a_table->nelem = 0;
    a_table->hash = hash;
    a_table->equal = equal;

    a_table->key_del = key_del;
    a_table->value_del = value_del;

    return a_table;
}

void logc_hashtable_del(struct logc_hashtable *a_table)
{
    size_t i;
    struct logc_hashtable_entry *p;
    struct logc_hashtable_entry *q;

    if (!a_table) {
        logc_error("a_table[%p] is NULL, just do nothing", a_table);
        return;
    }

    for (i = 0; i < a_table->tab_size; i++) {
        for (p = (a_table->tab)[i]; p; p = q) {
            q = p->next;
            if (a_table->key_del) {
                a_table->key_del(p->key);
            }

            if (a_table->value_del) {
                a_table->value_del(p->value);
            }
            free(p);
        }
    }

    if (a_table->tab) {
        free(a_table->tab);
    }
    free(a_table);
}

void logc_hashtable_clean(struct logc_hashtable *a_table)
{
    size_t i;
    struct logc_hashtable_entry *p;
    struct logc_hashtable_entry *q;

    for (i = 0; i < a_table->tab_size; i++) {
        for (p = (a_table->tab)[i]; p; p = q) {
            q = p->next;
            if (a_table->key_del) {
                a_table->key_del(p->key);
            }

            if (a_table->value_del) {
                a_table->value_del(p->value);
            }
            free(p);
        }

        (a_table->tab)[i] = NULL;
    }

    a_table->nelem = 0;
}

static int logc_hashtable_rehash(struct logc_hashtable *a_table)
{
    size_t i, j;
    size_t tab_size;
    struct logc_hashtable_entry *p;
    struct logc_hashtable_entry *q;
    struct logc_hashtable_entry **tab;

    tab_size = 2 * a_table->tab_size;
    tab = (struct logc_hashtable_entry **)calloc(tab_size, sizeof(*tab));
    if (!tab) {
        logc_error("calloc fail, errno[%d]", errno);
        return -1;
    }

    for (i = 0; i < a_table->tab_size; i++) {
        for (p = (a_table->tab)[i]; p; p = q) {
            q = p->next;

            p->next = NULL;
            p->prev = NULL;

            j = p->hash_key % tab_size;
            if (tab[j]) {
                tab[j]->prev = p;
                p->next = tab[j];
            }

            tab[j] = p;
        }
    }

    free(a_table->tab);
    a_table->tab = tab;
    a_table->tab_size = tab_size;

    return 0;
}

struct logc_hashtable_entry *logc_hashtable_get_entry(struct logc_hashtable *a_table, const void *a_key)
{
    unsigned int i;
    struct logc_hashtable_entry *p;

    i = a_table->hash(a_key) % a_table->tab_size;
    for (p = (a_table->tab)[i]; p; p = p->next) {
        if (a_table->equal(a_key, p->key)) {
            return p;
        }
    }

    return NULL;
}

void *logc_hashtable_get(struct logc_hashtable *a_table, const void *a_key)
{
    unsigned int i;
    struct logc_hashtable_entry *p;

    i = a_table->hash(a_key) % a_table->tab_size;
    for (p = (a_table->tab)[i]; p; p = p->next) {
        if (a_table->equal(a_key, p->key)) {
            return p->value;
        }
    }

    return NULL;
}

int logc_hashtable_put(struct logc_hashtable *a_table, void *a_key, void *a_value)
{
    int rc = 0;
    unsigned int i;
    struct logc_hashtable_entry *p = NULL;

    i = a_table->hash(a_key) % a_table->tab_size;
    for (p = (a_table->tab)[i]; p; p = p->next) {
        if (a_table->equal(a_key, p->key)) {
            break;
        }
    }

    if (p) {
        if (a_table->key_del) {
            a_table->key_del(p->key);
        }

        if (a_table->value_del) {
            a_table->value_del(p->value);
        }

        p->key = a_key;
        p->value = a_value;

        return 0;
    } else {
        if (a_table->nelem > (a_table->tab_size * 1.3)) {
            rc = logc_hashtable_rehash(a_table);
            if (rc) {
                logc_error("rehash fail");
                return -1;
            }
        }

        p = (struct logc_hashtable_entry *)calloc(1, sizeof(*p));
        if (!p) {
            logc_error("calloc fail, errno[%d]", errno);
            return -1;
        }

        p->hash_key = a_table->hash(a_key);
        p->key = a_key;
        p->value = a_value;
        p->next = NULL;
        p->prev = NULL;

        i = p->hash_key % a_table->tab_size;
        if ((a_table->tab)[i]) {
            (a_table->tab)[i]->prev = p;
            p->next = (a_table->tab)[i];
        }

        (a_table->tab)[i] = p;
        a_table->nelem++;
    }

    return 0;
}

void logc_hashtable_remove(struct logc_hashtable *a_table, const void *a_key)
{
    unsigned int i;
    struct logc_hashtable_entry *p;

    if (!a_table || !a_key) {
        logc_error("a_table[%p] or a_key[%p] is NULL, just do nothing", a_table, a_key);
        return;
    }

    i = a_table->hash(a_key) % a_table->tab_size;
    for (p = (a_table->tab)[i]; p; p = p->next) {
        if (a_table->equal(a_key, p->key)) {
            break;
        }
    }

    if (!p) {
        logc_error("p[%p] not found in hashtable", p);
        return;
    }

    if (a_table->key_del) {
        a_table->key_del(p->key);
    }

    if (a_table->value_del) {
        a_table->value_del(p->value);
    }

    if (p->next) {
        p->next->prev = p->prev;
    }

    if (p->prev) {
        p->prev->next = p->next;
    } else {
        unsigned int i;

        i = p->hash_key % a_table->tab_size;
        a_table->tab[i] = p->next;
    }

    free(p);
    a_table->nelem--;
}

struct logc_hashtable_entry *logc_hashtable_begin(struct logc_hashtable *a_table)
{
    size_t i;
    struct logc_hashtable_entry *p;

    for (i = 0; i < a_table->tab_size; i++) {
        for (p = (a_table->tab)[i]; p; p = p->next) {
            if (p) {
                return p;
            }
        }
    }

    return NULL;
}

struct logc_hashtable_entry *logc_hashtable_next(struct logc_hashtable *a_table, struct logc_hashtable_entry *a_entry)
{
    size_t i, j;

    if (a_entry->next) {
        return a_entry->next;
    }

    i = a_entry->hash_key % a_table->tab_size;
    for (j = i + 1; j < a_table->tab_size; j++) {
        if ((a_table->tab)[j]) {
            return (a_table->tab)[j];
        }
    }

    return NULL;
}

unsigned int logc_hashtable_str_hash(const void *str)
{
    unsigned int h = 5381;
    const char *p = (const char *)str;

    while (*p != '\0') {
        h = ((h << 5) + h) + (*p++);
    }

    return h;
}

int logc_hashtable_str_equal(const void *key1, const void *key2)
{
    return (LOG_STRCMP((const char *)key1, ==, (const char *)key2));
}
