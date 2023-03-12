#ifndef LOGC_HASHTABLE_H
#define LOGC_HASHTABLE_H

#include <stdlib.h>

struct logc_hashtable_entry {
    unsigned int                hash_key;
    void                        *key;
    void                        *value;
    struct logc_hashtable_entry *prev;
    struct logc_hashtable_entry *next;
};

struct logc_hashtable;

typedef void (*hashtable_del_func)(void *kv);
typedef unsigned int (*hashtable_hash_func)(const void *key);
typedef int (*hashtable_equal_func)(const void *key1, const void *key2);

struct logc_hashtable *logc_hashtable_new(size_t a_size, hashtable_hash_func hash, hashtable_equal_func equal, hashtable_del_func key_del, hashtable_del_func value_del);

void logc_hashtable_del(struct logc_hashtable *a_table);
void logc_hashtable_clean(struct logc_hashtable *a_table);

int logc_hashtable_put(struct logc_hashtable *a_table, void *a_key, void *a_value);
struct logc_hashtable_entry *logc_hashtable_get_entry(struct logc_hashtable *a_table, const void *a_key);

void *logc_hashtable_get(struct logc_hashtable *a_table, const void *a_key);
void logc_hashtable_remove(struct logc_hashtable *a_table, const void *a_key);

struct logc_hashtable_entry *logc_hashtable_begin(struct logc_hashtable *a_table);
struct logc_hashtable_entry *logc_hashtable_next(struct logc_hashtable *a_table, struct logc_hashtable_entry *a_entry);

#define logc_hashtable_foreach(a_table, a_entry)        for(a_entry = logc_hashtable_begin(a_table); a_entry; a_entry = logc_hashtable_next(a_table, a_entry))

unsigned int logc_hashtable_str_hash(const void *str);
int logc_hashtable_str_equal(const void *key1, const void *key2);

#endif
