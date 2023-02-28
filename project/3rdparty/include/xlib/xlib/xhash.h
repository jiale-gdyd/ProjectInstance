#ifndef __X_HASH_H__
#define __X_HASH_H__

#include "xtypes.h"
#include "xlist.h"
#include "xarray.h"

X_BEGIN_DECLS

typedef struct _XHashTable XHashTable;
typedef struct _XHashTableIter XHashTableIter;

typedef xboolean (*XHRFunc)(xpointer key, xpointer value, xpointer user_data);

struct _XHashTableIter {
    xpointer dummy1;
    xpointer dummy2;
    xpointer dummy3;
    int      dummy4;
    xboolean dummy5;
    xpointer dummy6;
};

XLIB_AVAILABLE_IN_ALL
XHashTable *x_hash_table_new(XHashFunc hash_func, XEqualFunc key_equal_func);

XLIB_AVAILABLE_IN_ALL
XHashTable *x_hash_table_new_full(XHashFunc hash_func, XEqualFunc key_equal_func, XDestroyNotify key_destroy_func, XDestroyNotify value_destroy_func);

XLIB_AVAILABLE_IN_2_72
XHashTable *x_hash_table_new_similar(XHashTable *other_hash_table);

XLIB_AVAILABLE_IN_ALL
void x_hash_table_destroy(XHashTable *hash_table);

XLIB_AVAILABLE_IN_ALL
xboolean x_hash_table_insert(XHashTable *hash_table, xpointer key, xpointer value);

XLIB_AVAILABLE_IN_ALL
xboolean x_hash_table_replace(XHashTable *hash_table, xpointer key, xpointer value);

XLIB_AVAILABLE_IN_ALL
xboolean x_hash_table_add(XHashTable *hash_table, xpointer key);

XLIB_AVAILABLE_IN_ALL
xboolean x_hash_table_remove(XHashTable *hash_table, xconstpointer key);

XLIB_AVAILABLE_IN_ALL
void x_hash_table_remove_all(XHashTable *hash_table);

XLIB_AVAILABLE_IN_ALL
xboolean x_hash_table_steal(XHashTable *hash_table, xconstpointer key);

XLIB_AVAILABLE_IN_2_58
xboolean x_hash_table_steal_extended(XHashTable *hash_table, xconstpointer lookup_key, xpointer *stolen_key, xpointer *stolen_value);

XLIB_AVAILABLE_IN_ALL
void x_hash_table_steal_all(XHashTable *hash_table);

XLIB_AVAILABLE_IN_2_76
XPtrArray *x_hash_table_steal_all_keys(XHashTable *hash_table);

XLIB_AVAILABLE_IN_2_76
XPtrArray *x_hash_table_steal_all_values(XHashTable *hash_table);

XLIB_AVAILABLE_IN_ALL
xpointer x_hash_table_lookup(XHashTable *hash_table, xconstpointer key);

XLIB_AVAILABLE_IN_ALL
xboolean x_hash_table_contains(XHashTable *hash_table, xconstpointer key);

XLIB_AVAILABLE_IN_ALL
xboolean x_hash_table_lookup_extended(XHashTable *hash_table, xconstpointer lookup_key, xpointer *orig_key, xpointer *value);

XLIB_AVAILABLE_IN_ALL
void x_hash_table_foreach(XHashTable *hash_table, XHFunc func, xpointer user_data);

XLIB_AVAILABLE_IN_ALL
xpointer x_hash_table_find(XHashTable *hash_table, XHRFunc predicate, xpointer user_data);

XLIB_AVAILABLE_IN_ALL
xuint x_hash_table_foreach_remove(XHashTable *hash_table, XHRFunc func, xpointer user_data);

XLIB_AVAILABLE_IN_ALL
xuint x_hash_table_foreach_steal(XHashTable *hash_table, XHRFunc func, xpointer user_data);

XLIB_AVAILABLE_IN_ALL
xuint x_hash_table_size(XHashTable *hash_table);

XLIB_AVAILABLE_IN_ALL
XList *x_hash_table_get_keys(XHashTable *hash_table);

XLIB_AVAILABLE_IN_ALL
XList *x_hash_table_get_values(XHashTable *hash_table);

XLIB_AVAILABLE_IN_2_40
xpointer *x_hash_table_get_keys_as_array(XHashTable *hash_table, xuint *length);

XLIB_AVAILABLE_IN_2_76
XPtrArray *x_hash_table_get_keys_as_ptr_array(XHashTable *hash_table);

XLIB_AVAILABLE_IN_2_76
XPtrArray *x_hash_table_get_values_as_ptr_array(XHashTable *hash_table);

XLIB_AVAILABLE_IN_ALL
void x_hash_table_iter_init(XHashTableIter *iter, XHashTable *hash_table);

XLIB_AVAILABLE_IN_ALL
xboolean x_hash_table_iter_next(XHashTableIter *iter, xpointer *key, xpointer *value);

XLIB_AVAILABLE_IN_ALL
XHashTable *x_hash_table_iter_get_hash_table(XHashTableIter *iter);

XLIB_AVAILABLE_IN_ALL
void x_hash_table_iter_remove(XHashTableIter *iter);

XLIB_AVAILABLE_IN_2_30
void x_hash_table_iter_replace(XHashTableIter *iter, xpointer value);

XLIB_AVAILABLE_IN_ALL
void x_hash_table_iter_steal(XHashTableIter *iter);

XLIB_AVAILABLE_IN_ALL
XHashTable *x_hash_table_ref(XHashTable *hash_table);

XLIB_AVAILABLE_IN_ALL
void x_hash_table_unref(XHashTable *hash_table);

#define x_hash_table_freeze(hash_table)         ((void)0)XLIB_DEPRECATED_MACRO_IN_2_26
#define x_hash_table_thaw(hash_table)           ((void)0)XLIB_DEPRECATED_MACRO_IN_2_26

XLIB_AVAILABLE_IN_ALL
xboolean x_str_equal(xconstpointer v1, xconstpointer v2);

#define x_str_equal(v1, v2)                     (strcmp((const char *)(v1), (const char *)(v2)) == 0)

XLIB_AVAILABLE_IN_ALL
xuint x_str_hash(xconstpointer v);

XLIB_AVAILABLE_IN_ALL
xboolean x_int_equal(xconstpointer v1, xconstpointer v2);

XLIB_AVAILABLE_IN_ALL
xuint x_int_hash(xconstpointer v);

XLIB_AVAILABLE_IN_ALL
xboolean x_int64_equal(xconstpointer v1, xconstpointer v2);

XLIB_AVAILABLE_IN_ALL
xuint x_int64_hash(xconstpointer v);

XLIB_AVAILABLE_IN_ALL
xboolean x_double_equal(xconstpointer v1, xconstpointer v2);

XLIB_AVAILABLE_IN_ALL
xuint x_double_hash(xconstpointer v);

XLIB_AVAILABLE_IN_ALL
xuint x_direct_hash(xconstpointer v) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
xboolean x_direct_equal(xconstpointer v1, xconstpointer v2) X_GNUC_CONST;

X_END_DECLS

#endif
