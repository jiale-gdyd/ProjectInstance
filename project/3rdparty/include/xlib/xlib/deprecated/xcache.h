#ifndef __X_CACHE_H__
#define __X_CACHE_H__

#include "../xlist.h"

X_BEGIN_DECLS

typedef struct _XCache XCache XLIB_DEPRECATED_TYPE_IN_2_26_FOR(XHashTable);

typedef xpointer (*XCacheNewFunc)(xpointer key) XLIB_DEPRECATED_TYPE_IN_2_26;
typedef xpointer (*XCacheDupFunc)(xpointer value) XLIB_DEPRECATED_TYPE_IN_2_26;
typedef void (*XCacheDestroyFunc)(xpointer value) XLIB_DEPRECATED_TYPE_IN_2_26;

X_GNUC_BEGIN_IGNORE_DEPRECATIONS

XLIB_DEPRECATED
XCache *x_cache_new(XCacheNewFunc value_new_func, XCacheDestroyFunc value_destroy_func, XCacheDupFunc key_dup_func, XCacheDestroyFunc key_destroy_func, XHashFunc hash_key_func, XHashFunc hash_value_func, XEqualFunc key_equal_func);

XLIB_DEPRECATED
void x_cache_destroy(XCache *cache);

XLIB_DEPRECATED
xpointer x_cache_insert(XCache *cache, xpointer key);

XLIB_DEPRECATED
void x_cache_remove(XCache *cache, xconstpointer value);

XLIB_DEPRECATED
void x_cache_key_foreach(XCache *cache, XHFunc func, xpointer user_data);

XLIB_DEPRECATED
void x_cache_value_foreach(XCache *cache, XHFunc func, xpointer user_data);

X_GNUC_END_IGNORE_DEPRECATIONS

X_END_DECLS

#endif
