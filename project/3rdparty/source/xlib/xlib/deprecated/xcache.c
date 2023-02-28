#include <xlib/xlib/config.h>

#ifndef XLIB_DISABLE_DEPRECATION_WARNINGS
#define XLIB_DISABLE_DEPRECATION_WARNINGS
#endif

#include <xlib/xlib/xhash.h>
#include <xlib/xlib/xslice.h>
#include <xlib/xlib/xtestutils.h>
#include <xlib/xlib/deprecated/xcache.h>

typedef struct _XCacheNode XCacheNode;

struct _XCacheNode {
    xpointer value;
    xint     ref_count;
};

struct _XCache {
    XCacheNewFunc     value_new_func;
    XCacheDestroyFunc value_destroy_func;
    XCacheDupFunc     key_dup_func;
    XCacheDestroyFunc key_destroy_func;
    XHashTable        *key_table;
    XHashTable        *value_table;
};

static inline XCacheNode *x_cache_node_new(xpointer value)
{
    XCacheNode *node = x_slice_new(XCacheNode);
    node->value = value;
    node->ref_count = 1;

    return node;
}

static inline void x_cache_node_destroy(XCacheNode *node)
{
    x_slice_free(XCacheNode, node);
}

XCache *x_cache_new(XCacheNewFunc value_new_func, XCacheDestroyFunc value_destroy_func, XCacheDupFunc key_dup_func, XCacheDestroyFunc key_destroy_func, XHashFunc hash_key_func, XHashFunc hash_value_func, XEqualFunc key_equal_func)
{
    XCache *cache;

    x_return_val_if_fail(value_new_func != NULL, NULL);
    x_return_val_if_fail(value_destroy_func != NULL, NULL);
    x_return_val_if_fail(key_dup_func != NULL, NULL);
    x_return_val_if_fail(key_destroy_func != NULL, NULL);
    x_return_val_if_fail(hash_key_func != NULL, NULL);
    x_return_val_if_fail(hash_value_func != NULL, NULL);
    x_return_val_if_fail(key_equal_func != NULL, NULL);

    cache = x_slice_new(XCache);
    cache->value_new_func = value_new_func;
    cache->value_destroy_func = value_destroy_func;
    cache->key_dup_func = key_dup_func;
    cache->key_destroy_func = key_destroy_func;
    cache->key_table = x_hash_table_new(hash_key_func, key_equal_func);
    cache->value_table = x_hash_table_new(hash_value_func, NULL);

    return cache;
}

void x_cache_destroy(XCache *cache)
{
    x_return_if_fail(cache != NULL);

    x_hash_table_destroy(cache->key_table);
    x_hash_table_destroy(cache->value_table);
    x_slice_free(XCache, cache);
}

xpointer x_cache_insert(XCache *cache, xpointer key)
{
    xpointer value;
    XCacheNode *node;

    x_return_val_if_fail(cache != NULL, NULL);

    node = (XCacheNode *)x_hash_table_lookup(cache->key_table, key);
    if (node) {
        node->ref_count += 1;
        return node->value;
    }

    key = (*cache->key_dup_func)(key);
    value = (*cache->value_new_func)(key);
    node = x_cache_node_new(value);

    x_hash_table_insert(cache->key_table, key, node);
    x_hash_table_insert(cache->value_table, value, key);

    return node->value;
}

void x_cache_remove(XCache *cache, xconstpointer value)
{
    xpointer key;
    XCacheNode *node;

    x_return_if_fail(cache != NULL);

    key = x_hash_table_lookup(cache->value_table, value);
    node = (XCacheNode *)x_hash_table_lookup(cache->key_table, key);

    x_return_if_fail(node != NULL);

    node->ref_count -= 1;
    if (node->ref_count == 0) {
        x_hash_table_remove(cache->value_table, value);
        x_hash_table_remove(cache->key_table, key);

        (*cache->key_destroy_func)(key);
        (*cache->value_destroy_func)(node->value);
        x_cache_node_destroy(node);
    }
}

void x_cache_key_foreach(XCache *cache, XHFunc func, xpointer user_data)
{
    x_return_if_fail(cache != NULL);
    x_return_if_fail(func != NULL);

    x_hash_table_foreach(cache->value_table, func, user_data);
}

void x_cache_value_foreach(XCache *cache, XHFunc func, xpointer user_data)
{
    x_return_if_fail(cache != NULL);
    x_return_if_fail(func != NULL);

    x_hash_table_foreach(cache->key_table, func, user_data);
}
