#include <string.h>

#include <xlib/xlib/config.h>

#include <xlib/xlib/xhash.h>
#include <xlib/xlib/xslice.h>
#include <xlib/xlib/xmacros.h>
#include <xlib/xlib/xatomic.h>
#include <xlib/xlib/xstrfuncs.h>
#include <xlib/xlib/xrefcount.h>
#include <xlib/xlib/xtestutils.h>
#include <xlib/xlib/xlib-private.h>

#define HASH_TABLE_MIN_SHIFT                    3

#define UNUSED_HASH_VALUE                       0
#define TOMBSTONE_HASH_VALUE                    1
#define HASH_IS_UNUSED(h_)                      ((h_) == UNUSED_HASH_VALUE)
#define HASH_IS_TOMBSTONE(h_)                   ((h_) == TOMBSTONE_HASH_VALUE)
#define HASH_IS_REAL(h_)                        ((h_) >= 2)

#define BIG_ENTRY_SIZE                          (SIZEOF_VOID_P)
#define SMALL_ENTRY_SIZE                        (SIZEOF_INT)

#if SMALL_ENTRY_SIZE < BIG_ENTRY_SIZE && BIG_ENTRY_SIZE <= 8
#define USE_SMALL_ARRAYS
#endif

struct _XHashTable {
    xsize           size;
    xint            mod;
    xuint           mask;
    xuint           nnodes;
    xuint           noccupied;

    xuint           have_big_keys : 1;
    xuint           have_big_values : 1;

    xpointer        keys;
    xuint           *hashes;
    xpointer        values;

    XHashFunc       hash_func;
    XEqualFunc      key_equal_func;
    xatomicrefcount ref_count;
    xintptr         version;
    XDestroyNotify  key_destroy_func;
    XDestroyNotify  value_destroy_func;
};

typedef struct {
    XHashTable *hash_table;
    xpointer   dummy1;
    xpointer   dummy2;
    xint       position;
    xboolean   dummy3;
    xint       version;
} RealIter;

typedef struct {
    char     a;
    RealIter b;
} RealIterAlign;

typedef struct {
    char           a;
    XHashTableIter b;
} XHashTableIterAlign;

X_STATIC_ASSERT(sizeof(XHashTableIter) == sizeof(RealIter));
//X_STATIC_ASSERT(X_ALIGNOF(XHashTableIter) >= X_ALIGNOF(RealIter));
X_STATIC_ASSERT(X_STRUCT_OFFSET(XHashTableIterAlign, b) >= X_STRUCT_OFFSET(RealIterAlign, b));

static const xint prime_mod [] = {
    1,          /* For 1 << 0 */
    2,
    3,
    7,
    13,
    31,
    61,
    127,
    251,
    509,
    1021,
    2039,
    4093,
    8191,
    16381,
    32749,
    65521,      /* For 1 << 16 */
    131071,
    262139,
    524287,
    1048573,
    2097143,
    4194301,
    8388593,
    16777213,
    33554393,
    67108859,
    134217689,
    268435399,
    536870909,
    1073741789,
    2147483647  /* For 1 << 31 */
};

static void x_hash_table_set_shift(XHashTable *hash_table, xint shift)
{
    hash_table->size = 1 << shift;
    hash_table->mod = prime_mod[shift];

    x_assert((hash_table->size & (hash_table->size - 1)) == 0);
    hash_table->mask = hash_table->size - 1;
}

static xint x_hash_table_find_closest_shift(xint n)
{
    xint i;

    for (i = 0; n; i++) {
        n >>= 1;
    }

    return i;
}

static void x_hash_table_set_shift_from_size(XHashTable *hash_table, xint size)
{
    xint shift;

    shift = x_hash_table_find_closest_shift(size);
    shift = MAX(shift, HASH_TABLE_MIN_SHIFT);

    x_hash_table_set_shift(hash_table, shift);
}

static inline xpointer x_hash_table_realloc_key_or_value_array(xpointer a, xuint size, X_GNUC_UNUSED xboolean is_big)
{
#ifdef USE_SMALL_ARRAYS
    return x_realloc(a, size * (is_big ? BIG_ENTRY_SIZE : SMALL_ENTRY_SIZE));
#else
    return x_renew(xpointer, a, size);
#endif
}

static inline xpointer x_hash_table_fetch_key_or_value(xpointer a, xuint index, xboolean is_big)
{
#ifndef USE_SMALL_ARRAYS
    is_big = TRUE;
#endif
    return is_big ? *(((xpointer *)a) + index) : XUINT_TO_POINTER(*(((xuint *)a) + index));
}

static inline void x_hash_table_assign_key_or_value(xpointer a, xuint index, xboolean is_big, xpointer v)
{
#ifndef USE_SMALL_ARRAYS
    is_big = TRUE;
#endif
    if (is_big) {
        *(((xpointer *)a) + index) = v;
    } else {
        *(((xuint *)a) + index) = XPOINTER_TO_UINT(v);
    }
}

static inline xpointer x_hash_table_evict_key_or_value(xpointer a, xuint index, xboolean is_big, xpointer v)
{
#ifndef USE_SMALL_ARRAYS
    is_big = TRUE;
#endif

    if (is_big) {
        xpointer r = *(((xpointer *)a) + index);
        *(((xpointer *)a) + index) = v;
        return r;
    } else {
        xpointer r = XUINT_TO_POINTER(*(((xuint *)a) + index));
        *(((xuint *) a) + index) = XPOINTER_TO_UINT(v);
        return r;
    }
}

static inline xuint x_hash_table_hash_to_index(XHashTable *hash_table, xuint hash)
{
    return (hash * 11) % hash_table->mod;
}

static inline xuint x_hash_table_lookup_node(XHashTable *hash_table, xconstpointer key, xuint *hash_return)
{
    xuint step = 0;
    xuint node_hash;
    xuint node_index;
    xuint hash_value;
    xuint first_tombstone = 0;
    xboolean have_tombstone = FALSE;

    hash_value = hash_table->hash_func(key);
    if (X_UNLIKELY(!HASH_IS_REAL(hash_value))) {
        hash_value = 2;
    }

    *hash_return = hash_value;

    node_index = x_hash_table_hash_to_index(hash_table, hash_value);
    node_hash = hash_table->hashes[node_index];

    while (!HASH_IS_UNUSED(node_hash)) {
        if (node_hash == hash_value) {
            xpointer node_key = x_hash_table_fetch_key_or_value(hash_table->keys, node_index, hash_table->have_big_keys);

            if (hash_table->key_equal_func) {
                if (hash_table->key_equal_func(node_key, key)) {
                    return node_index;
                }
            } else if (node_key == key) {
                return node_index;
            }
        } else if (HASH_IS_TOMBSTONE(node_hash) && !have_tombstone) {
            first_tombstone = node_index;
            have_tombstone = TRUE;
        }

        step++;
        node_index += step;
        node_index &= hash_table->mask;
        node_hash = hash_table->hashes[node_index];
    }

    if (have_tombstone) {
        return first_tombstone;
    }

    return node_index;
}

static void x_hash_table_remove_node(XHashTable *hash_table, xint i, xboolean notify)
{
    xpointer key;
    xpointer value;

    key = x_hash_table_fetch_key_or_value(hash_table->keys, i, hash_table->have_big_keys);
    value = x_hash_table_fetch_key_or_value(hash_table->values, i, hash_table->have_big_values);

    hash_table->hashes[i] = TOMBSTONE_HASH_VALUE;

    x_hash_table_assign_key_or_value(hash_table->keys, i, hash_table->have_big_keys, NULL);
    x_hash_table_assign_key_or_value(hash_table->values, i, hash_table->have_big_values, NULL);

    x_assert(hash_table->nnodes > 0);
    hash_table->nnodes--;

    if (notify && hash_table->key_destroy_func) {
        hash_table->key_destroy_func(key);
    }

    if (notify && hash_table->value_destroy_func) {
        hash_table->value_destroy_func(value);
    }
}

static void x_hash_table_setup_storage(XHashTable *hash_table)
{
    xboolean small = FALSE;

#ifdef USE_SMALL_ARRAYS
    small = TRUE;
#endif

    x_hash_table_set_shift (hash_table, HASH_TABLE_MIN_SHIFT);

    hash_table->have_big_keys = !small;
    hash_table->have_big_values = !small;

    hash_table->keys = x_hash_table_realloc_key_or_value_array(NULL, hash_table->size, hash_table->have_big_keys);
    hash_table->values = hash_table->keys;
    hash_table->hashes = x_new0(xuint, hash_table->size);
}

static void x_hash_table_remove_all_nodes(XHashTable *hash_table, xboolean notify, xboolean destruction)
{
    int i;
    xpointer key;
    xint old_size;
    xpointer value;
    xuint *old_hashes;
    xpointer *old_keys;
    xpointer *old_values;
    xboolean old_have_big_keys;
    xboolean old_have_big_values;

    if (hash_table->nnodes == 0) {
        return;
    }

    hash_table->nnodes = 0;
    hash_table->noccupied = 0;

    if (!notify || (hash_table->key_destroy_func == NULL && hash_table->value_destroy_func == NULL)) {
        if (!destruction) {
            memset(hash_table->hashes, 0, hash_table->size * sizeof (xuint));

#ifdef USE_SMALL_ARRAYS
            memset(hash_table->keys, 0, hash_table->size * (hash_table->have_big_keys ? BIG_ENTRY_SIZE : SMALL_ENTRY_SIZE));
            memset(hash_table->values, 0, hash_table->size * (hash_table->have_big_values ? BIG_ENTRY_SIZE : SMALL_ENTRY_SIZE));
#else
            memset(hash_table->keys, 0, hash_table->size * sizeof (xpointer));
            memset(hash_table->values, 0, hash_table->size * sizeof (xpointer));
#endif
        }

        return;
    }

    old_size = hash_table->size;
    old_have_big_keys = hash_table->have_big_keys;
    old_have_big_values = hash_table->have_big_values;
    old_keys = (xpointer *)x_steal_pointer(&hash_table->keys);
    old_values = (xpointer *)x_steal_pointer(&hash_table->values);
    old_hashes = (xuint *)x_steal_pointer(&hash_table->hashes);

    if (!destruction) {
        x_hash_table_setup_storage(hash_table);
    } else {
        hash_table->size = hash_table->mod = hash_table->mask = 0;
    }

    for (i = 0; i < old_size; i++) {
        if (HASH_IS_REAL(old_hashes[i])) {
            key = x_hash_table_fetch_key_or_value(old_keys, i, old_have_big_keys);
            value = x_hash_table_fetch_key_or_value(old_values, i, old_have_big_values);

            old_hashes[i] = UNUSED_HASH_VALUE;

            x_hash_table_assign_key_or_value(old_keys, i, old_have_big_keys, NULL);
            x_hash_table_assign_key_or_value(old_values, i, old_have_big_values, NULL);

            if (hash_table->key_destroy_func != NULL) {
                hash_table->key_destroy_func(key);
            }

            if (hash_table->value_destroy_func != NULL) {
                hash_table->value_destroy_func(value);
            }
        }
    }

    if (old_keys != old_values) {
        x_free(old_values);
    }

    x_free(old_keys);
    x_free(old_hashes);
}

static void realloc_arrays(XHashTable *hash_table, xboolean is_a_set)
{
    hash_table->hashes = x_renew(xuint, hash_table->hashes, hash_table->size);
    hash_table->keys = x_hash_table_realloc_key_or_value_array(hash_table->keys, hash_table->size, hash_table->have_big_keys);

    if (is_a_set) {
        hash_table->values = hash_table->keys;
    } else {
        hash_table->values = x_hash_table_realloc_key_or_value_array(hash_table->values, hash_table->size, hash_table->have_big_values);
    }
}

static inline xboolean get_status_bit(const xuint32 *bitmap, xuint index)
{
    return (bitmap[index / 32] >> (index % 32)) & 1;
}

static inline void set_status_bit(xuint32 *bitmap, xuint index)
{
    bitmap[index / 32] |= 1U << (index % 32);
}

#define DEFINE_RESIZE_FUNC(fname)                                                               \
static void fname(XHashTable *hash_table, xuint old_size, xuint32 *reallocated_buckets_bitmap)  \
{                                                                                               \
    xuint i;                                                                                    \
                                                                                                \
    for (i = 0; i < old_size; i++) {                                                            \
        xuint node_hash = hash_table->hashes[i];                                                \
        xpointer key, value X_GNUC_UNUSED;                                                      \
                                                                                                \
        if (!HASH_IS_REAL(node_hash)) {                                                         \
            hash_table->hashes[i] = UNUSED_HASH_VALUE;                                          \
            continue;                                                                           \
        }                                                                                       \
                                                                                                \
        if (get_status_bit(reallocated_buckets_bitmap, i)) {                                    \
            continue;                                                                           \
        }                                                                                       \
                                                                                                \
        hash_table->hashes[i] = UNUSED_HASH_VALUE;                                              \
        EVICT_KEYVAL(hash_table, i, NULL, NULL, key, value);                                    \
                                                                                                \
        for (;;) {                                                                              \
            xuint hash_val;                                                                     \
            xuint step = 0;                                                                     \
            xuint replaced_hash;                                                                \
                                                                                                \
            hash_val = x_hash_table_hash_to_index(hash_table, node_hash);                       \
                                                                                                \
            while (get_status_bit(reallocated_buckets_bitmap, hash_val)) {                      \
                step++;                                                                         \
                hash_val += step;                                                               \
                hash_val &= hash_table->mask;                                                   \
            }                                                                                   \
                                                                                                \
            set_status_bit(reallocated_buckets_bitmap, hash_val);                               \
                                                                                                \
            replaced_hash = hash_table->hashes[hash_val];                                       \
            hash_table->hashes[hash_val] = node_hash;                                           \
            if (!HASH_IS_REAL(replaced_hash)) {                                                 \
                ASSIGN_KEYVAL(hash_table, hash_val, key, value);                                \
                break;                                                                          \
            }                                                                                   \
                                                                                                \
            node_hash = replaced_hash;                                                          \
            EVICT_KEYVAL(hash_table, hash_val, key, value, key, value);                         \
        }                                                                                       \
    }                                                                                           \
}

#define ASSIGN_KEYVAL(ht, index, key, value)                                                        \
    X_STMT_START {                                                                                  \
        x_hash_table_assign_key_or_value((ht)->keys, (index), (ht)->have_big_keys, (key));          \
        x_hash_table_assign_key_or_value((ht)->values, (index), (ht)->have_big_values, (value));    \
    } X_STMT_END

#define EVICT_KEYVAL(ht, index, key, value, outkey, outvalue)                                               \
    X_STMT_START {                                                                                          \
        (outkey) = x_hash_table_evict_key_or_value((ht)->keys, (index), (ht)->have_big_keys, (key));        \
        (outvalue) = x_hash_table_evict_key_or_value((ht)->values, (index), (ht)->have_big_values, (value));\
    } X_STMT_END

DEFINE_RESIZE_FUNC(resize_map)

#undef ASSIGN_KEYVAL
#undef EVICT_KEYVAL

#define ASSIGN_KEYVAL(ht, index, key, value)                                                    \
    X_STMT_START {                                                                              \
        x_hash_table_assign_key_or_value((ht)->keys, (index), (ht)->have_big_keys, (key));      \
    } X_STMT_END

#define EVICT_KEYVAL(ht, index, key, value, outkey, outvalue)                                       \
    X_STMT_START {                                                                                  \
        (outkey) = x_hash_table_evict_key_or_value((ht)->keys, (index), (ht)->have_big_keys, (key));\
    } X_STMT_END

DEFINE_RESIZE_FUNC(resize_set)

#undef ASSIGN_KEYVAL
#undef EVICT_KEYVAL

static void x_hash_table_resize(XHashTable *hash_table)
{
    xsize old_size;
    xboolean is_a_set;
    xuint32 *reallocated_buckets_bitmap;

    old_size = hash_table->size;
    is_a_set = hash_table->keys == hash_table->values;

    x_hash_table_set_shift_from_size(hash_table, hash_table->nnodes * 1.333);

    if (hash_table->size > old_size) {
        realloc_arrays(hash_table, is_a_set);
        memset(&hash_table->hashes[old_size], 0, (hash_table->size - old_size) * sizeof (xuint));

        reallocated_buckets_bitmap = x_new0(xuint32, (hash_table->size + 31) / 32);
    } else {
        reallocated_buckets_bitmap = x_new0(xuint32, (old_size + 31) / 32);
    }

    if (is_a_set) {
        resize_set(hash_table, old_size, reallocated_buckets_bitmap);
    } else {
        resize_map(hash_table, old_size, reallocated_buckets_bitmap);
    }

    x_free(reallocated_buckets_bitmap);

    if (hash_table->size < old_size) {
        realloc_arrays(hash_table, is_a_set);
    }

    hash_table->noccupied = hash_table->nnodes;
}

static inline void x_hash_table_maybe_resize(XHashTable *hash_table)
{
    xsize size = hash_table->size;
    xsize noccupied = hash_table->noccupied;

    if ((size > hash_table->nnodes * 4 && size > 1 << HASH_TABLE_MIN_SHIFT) || (size <= noccupied + (noccupied / 16))) {
        x_hash_table_resize(hash_table);
    }
}

#ifdef USE_SMALL_ARRAYS
static inline xboolean entry_is_big(xpointer v)
{
    return (((xuintptr)v) >> ((BIG_ENTRY_SIZE - SMALL_ENTRY_SIZE) * 8)) != 0;
}

static inline xboolean x_hash_table_maybe_make_big_keys_or_values(xpointer *a_p, xpointer v, xint ht_size)
{
    if (entry_is_big(v)) {
        xint i;
        xpointer *a_new;
        xuint *a = (xuint *)*a_p;

        a_new = x_new(xpointer, ht_size);

        for (i = 0; i < ht_size; i++) {
            a_new[i] = XUINT_TO_POINTER(a[i]);
        }

        x_free(a);
        *a_p = a_new;
        return TRUE;
    }

    return FALSE;
}
#endif

static inline void x_hash_table_ensure_keyval_fits(XHashTable *hash_table, xpointer key, xpointer value)
{
    xboolean is_a_set = (hash_table->keys == hash_table->values);

#ifdef USE_SMALL_ARRAYS
    if (is_a_set) {
        if (hash_table->have_big_keys) {
            if (key != value) {
                hash_table->values = x_memdup2(hash_table->keys, sizeof(xpointer) * hash_table->size);
            }

            return;
        } else {
            if (key != value) {
                hash_table->values = x_memdup2(hash_table->keys, sizeof(xuint) * hash_table->size);
                is_a_set = FALSE;
            }
        }
    }

    if (!hash_table->have_big_keys) {
        hash_table->have_big_keys = x_hash_table_maybe_make_big_keys_or_values(&hash_table->keys, key, hash_table->size);
        if (is_a_set) {
            hash_table->values = hash_table->keys;
            hash_table->have_big_values = hash_table->have_big_keys;
        }
    }

    if (!is_a_set && !hash_table->have_big_values) {
        hash_table->have_big_values = x_hash_table_maybe_make_big_keys_or_values(&hash_table->values, value, hash_table->size);
    }
#else
    if (is_a_set && key != value) {
        hash_table->values = x_memdup2(hash_table->keys, sizeof(xpointer) * hash_table->size);
    }
#endif
}

XHashTable *x_hash_table_new(XHashFunc hash_func, XEqualFunc key_equal_func)
{
    return x_hash_table_new_full(hash_func, key_equal_func, NULL, NULL);
}

XHashTable *x_hash_table_new_full(XHashFunc hash_func, XEqualFunc key_equal_func, XDestroyNotify key_destroy_func, XDestroyNotify value_destroy_func)
{
    XHashTable *hash_table;

    hash_table = x_slice_new(XHashTable);
    x_atomic_ref_count_init(&hash_table->ref_count);
    hash_table->nnodes = 0;
    hash_table->noccupied = 0;
    hash_table->hash_func = hash_func ? hash_func : x_direct_hash;
    hash_table->key_equal_func = key_equal_func;
    hash_table->version = 0;
    hash_table->key_destroy_func = key_destroy_func;
    hash_table->value_destroy_func = value_destroy_func;

    x_hash_table_setup_storage(hash_table);

    return hash_table;
}

XHashTable *x_hash_table_new_similar(XHashTable *other_hash_table)
{
    x_return_val_if_fail(other_hash_table, NULL);
    return x_hash_table_new_full(other_hash_table->hash_func, other_hash_table->key_equal_func, other_hash_table->key_destroy_func, other_hash_table->value_destroy_func);
}

void x_hash_table_iter_init(XHashTableIter *iter, XHashTable *hash_table)
{
    RealIter *ri = (RealIter *)iter;

    x_return_if_fail(iter != NULL);
    x_return_if_fail(hash_table != NULL);

    ri->hash_table = hash_table;
    ri->position = -1;
    ri->version = hash_table->version;
}

xboolean x_hash_table_iter_next(XHashTableIter *iter, xpointer *key, xpointer *value)
{
    xint position;
    RealIter *ri = (RealIter *)iter;

    x_return_val_if_fail(iter != NULL, FALSE);
    x_return_val_if_fail(ri->version == ri->hash_table->version, FALSE);
    x_return_val_if_fail(ri->position < (xssize)ri->hash_table->size, FALSE);

    position = ri->position;

    do{
        position++;
        if (position >= (xssize)ri->hash_table->size) {
            ri->position = position;
            return FALSE;
        }
    } while (!HASH_IS_REAL(ri->hash_table->hashes[position]));

    if (key != NULL) {
        *key = x_hash_table_fetch_key_or_value(ri->hash_table->keys, position, ri->hash_table->have_big_keys);
    }

    if (value != NULL) {
        *value = x_hash_table_fetch_key_or_value(ri->hash_table->values, position, ri->hash_table->have_big_values);
    }

    ri->position = position;
    return TRUE;
}

XHashTable *x_hash_table_iter_get_hash_table(XHashTableIter *iter)
{
    x_return_val_if_fail(iter != NULL, NULL);
    return ((RealIter *)iter)->hash_table;
}

static void iter_remove_or_steal(RealIter *ri, xboolean notify)
{
    x_return_if_fail(ri != NULL);
    x_return_if_fail(ri->version == ri->hash_table->version);
    x_return_if_fail(ri->position >= 0);
    x_return_if_fail((xsize)ri->position < ri->hash_table->size);

    x_hash_table_remove_node(ri->hash_table, ri->position, notify);

    ri->version++;
    ri->hash_table->version++;
}

void x_hash_table_iter_remove(XHashTableIter *iter)
{
    iter_remove_or_steal((RealIter *)iter, TRUE);
}

static xboolean x_hash_table_insert_node(XHashTable *hash_table, xuint node_index, xuint key_hash, xpointer new_key, xpointer new_value, xboolean keep_new_key, xboolean reusing_key)
{
    xuint old_hash;
    xboolean already_exists;
    xpointer key_to_free = NULL;
    xpointer key_to_keep = NULL;
    xpointer value_to_free = NULL;

    old_hash = hash_table->hashes[node_index];
    already_exists = HASH_IS_REAL(old_hash);

    if (already_exists) {
        value_to_free = x_hash_table_fetch_key_or_value(hash_table->values, node_index, hash_table->have_big_values);

        if (keep_new_key) {
            key_to_free = x_hash_table_fetch_key_or_value(hash_table->keys, node_index, hash_table->have_big_keys);
            key_to_keep = new_key;
        } else {
            key_to_free = new_key;
            key_to_keep = x_hash_table_fetch_key_or_value(hash_table->keys, node_index, hash_table->have_big_keys);
        }
    } else {
        hash_table->hashes[node_index] = key_hash;
        key_to_keep = new_key;
    }

    x_hash_table_ensure_keyval_fits(hash_table, key_to_keep, new_value);
    x_hash_table_assign_key_or_value(hash_table->keys, node_index, hash_table->have_big_keys, key_to_keep);

    x_hash_table_assign_key_or_value(hash_table->values, node_index, hash_table->have_big_values, new_value);

    if (!already_exists) {
        hash_table->nnodes++;

        if (HASH_IS_UNUSED(old_hash)) {
            hash_table->noccupied++;
            x_hash_table_maybe_resize(hash_table);
        }

        hash_table->version++;
    }

    if (already_exists) {
        if (hash_table->key_destroy_func && !reusing_key) {
            (* hash_table->key_destroy_func)(key_to_free);
        }

        if (hash_table->value_destroy_func) {
            (* hash_table->value_destroy_func)(value_to_free);
        }
    }

    return !already_exists;
}

void x_hash_table_iter_replace(XHashTableIter *iter, xpointer value)
{
    RealIter *ri;
    xpointer key;
    xuint node_hash;

    ri = (RealIter *)iter;

    x_return_if_fail(ri != NULL);
    x_return_if_fail(ri->version == ri->hash_table->version);
    x_return_if_fail(ri->position >= 0);
    x_return_if_fail((xsize) ri->position < ri->hash_table->size);

    node_hash = ri->hash_table->hashes[ri->position];

    key = x_hash_table_fetch_key_or_value(ri->hash_table->keys, ri->position, ri->hash_table->have_big_keys);
    x_hash_table_insert_node(ri->hash_table, ri->position, node_hash, key, value, TRUE, TRUE);

    ri->version++;
    ri->hash_table->version++;
}

void x_hash_table_iter_steal(XHashTableIter *iter)
{
    iter_remove_or_steal((RealIter *)iter, FALSE);
}

XHashTable *x_hash_table_ref(XHashTable *hash_table)
{
    x_return_val_if_fail(hash_table != NULL, NULL);
    x_atomic_ref_count_inc(&hash_table->ref_count);

    return hash_table;
}

void x_hash_table_unref(XHashTable *hash_table)
{
    x_return_if_fail(hash_table != NULL);

    if (x_atomic_ref_count_dec(&hash_table->ref_count)) {
        x_hash_table_remove_all_nodes(hash_table, TRUE, TRUE);
        if (hash_table->keys != hash_table->values) {
            x_free(hash_table->values);
        }
    
        x_free(hash_table->keys);
        x_free(hash_table->hashes);
        x_slice_free(XHashTable, hash_table);
    }
}

void x_hash_table_destroy (XHashTable *hash_table)
{
    x_return_if_fail(hash_table != NULL);

    x_hash_table_remove_all(hash_table);
    x_hash_table_unref(hash_table);
}

xpointer x_hash_table_lookup(XHashTable *hash_table, xconstpointer key)
{
    xuint node_hash;
    xuint node_index;

    x_return_val_if_fail(hash_table != NULL, NULL);

    node_index = x_hash_table_lookup_node(hash_table, key, &node_hash);
    return HASH_IS_REAL(hash_table->hashes[node_index]) ? x_hash_table_fetch_key_or_value(hash_table->values, node_index, hash_table->have_big_values) : NULL;
}

xboolean x_hash_table_lookup_extended(XHashTable *hash_table, xconstpointer lookup_key, xpointer *orig_key, xpointer *value)
{
    xuint node_hash;
    xuint node_index;

    x_return_val_if_fail(hash_table != NULL, FALSE);

    node_index = x_hash_table_lookup_node(hash_table, lookup_key, &node_hash);

    if (!HASH_IS_REAL(hash_table->hashes[node_index])) {
        if (orig_key != NULL) {
            *orig_key = NULL;
        }

        if (value != NULL) {
            *value = NULL;
        }

        return FALSE;
    }

    if (orig_key) {
        *orig_key = x_hash_table_fetch_key_or_value(hash_table->keys, node_index, hash_table->have_big_keys);
    }

    if (value) {
        *value = x_hash_table_fetch_key_or_value(hash_table->values, node_index, hash_table->have_big_values);
    }

    return TRUE;
}

static xboolean x_hash_table_insert_internal(XHashTable *hash_table, xpointer key, xpointer value, xboolean keep_new_key)
{
    xuint key_hash;
    xuint node_index;

    x_return_val_if_fail(hash_table != NULL, FALSE);

    node_index = x_hash_table_lookup_node(hash_table, key, &key_hash);
    return x_hash_table_insert_node(hash_table, node_index, key_hash, key, value, keep_new_key, FALSE);
}

xboolean x_hash_table_insert(XHashTable *hash_table, xpointer key, xpointer value)
{
    return x_hash_table_insert_internal(hash_table, key, value, FALSE);
}

xboolean x_hash_table_replace(XHashTable *hash_table, xpointer key, xpointer value)
{
    return x_hash_table_insert_internal(hash_table, key, value, TRUE);
}

xboolean x_hash_table_add(XHashTable *hash_table, xpointer key)
{
    return x_hash_table_insert_internal(hash_table, key, key, TRUE);
}

xboolean x_hash_table_contains(XHashTable *hash_table, xconstpointer key)
{
    xuint node_hash;
    xuint node_index;

    x_return_val_if_fail(hash_table != NULL, FALSE);

    node_index = x_hash_table_lookup_node(hash_table, key, &node_hash);
    return HASH_IS_REAL(hash_table->hashes[node_index]);
}

static xboolean x_hash_table_remove_internal(XHashTable *hash_table, xconstpointer key, xboolean notify)
{
    xuint node_hash;
    xuint node_index;

    x_return_val_if_fail(hash_table != NULL, FALSE);

    node_index = x_hash_table_lookup_node(hash_table, key, &node_hash);

    if (!HASH_IS_REAL(hash_table->hashes[node_index])) {
        return FALSE;
    }

    x_hash_table_remove_node(hash_table, node_index, notify);
    x_hash_table_maybe_resize(hash_table);
    hash_table->version++;

    return TRUE;
}

xboolean x_hash_table_remove(XHashTable *hash_table, xconstpointer key)
{
    return x_hash_table_remove_internal(hash_table, key, TRUE);
}

xboolean x_hash_table_steal(XHashTable *hash_table, xconstpointer key)
{
    return x_hash_table_remove_internal(hash_table, key, FALSE);
}

xboolean x_hash_table_steal_extended(XHashTable *hash_table, xconstpointer lookup_key, xpointer *stolen_key, xpointer *stolen_value)
{
    xuint node_hash;
    xuint node_index;

    x_return_val_if_fail(hash_table != NULL, FALSE);

    node_index = x_hash_table_lookup_node(hash_table, lookup_key, &node_hash);

    if (!HASH_IS_REAL(hash_table->hashes[node_index])) {
        if (stolen_key != NULL) {
            *stolen_key = NULL;
        }

        if (stolen_value != NULL) {
            *stolen_value = NULL;
        }

        return FALSE;
    }

    if (stolen_key != NULL) {
        *stolen_key = x_hash_table_fetch_key_or_value(hash_table->keys, node_index, hash_table->have_big_keys);
        x_hash_table_assign_key_or_value(hash_table->keys, node_index, hash_table->have_big_keys, NULL);
    }

    if (stolen_value != NULL) {
        *stolen_value = x_hash_table_fetch_key_or_value(hash_table->values, node_index, hash_table->have_big_values);
        x_hash_table_assign_key_or_value(hash_table->values, node_index, hash_table->have_big_values, NULL);
    }

    x_hash_table_remove_node(hash_table, node_index, FALSE);
    x_hash_table_maybe_resize(hash_table);

    hash_table->version++;

    return TRUE;
}

void x_hash_table_remove_all(XHashTable *hash_table)
{
    x_return_if_fail(hash_table != NULL);

    if (hash_table->nnodes != 0) {
        hash_table->version++;
    }

    x_hash_table_remove_all_nodes(hash_table, TRUE, FALSE);
    x_hash_table_maybe_resize(hash_table);
}

void x_hash_table_steal_all(XHashTable *hash_table)
{
    x_return_if_fail(hash_table != NULL);

    if (hash_table->nnodes != 0) {
        hash_table->version++;
    }

    x_hash_table_remove_all_nodes(hash_table, FALSE, FALSE);
    x_hash_table_maybe_resize(hash_table);
}

XPtrArray *x_hash_table_steal_all_keys(XHashTable *hash_table)
{
    XPtrArray *array;
    XDestroyNotify key_destroy_func;

    x_return_val_if_fail(hash_table != NULL, NULL);

    array = x_hash_table_get_keys_as_ptr_array(hash_table);

    key_destroy_func = x_steal_pointer(&hash_table->key_destroy_func);
    x_ptr_array_set_free_func(array, key_destroy_func);

    x_hash_table_remove_all(hash_table);
    hash_table->key_destroy_func = x_steal_pointer(&key_destroy_func);

    return array;
}

XPtrArray *x_hash_table_steal_all_values(XHashTable *hash_table)
{
    XPtrArray *array;
    XDestroyNotify value_destroy_func;

    x_return_val_if_fail(hash_table != NULL, NULL);

    array = x_hash_table_get_values_as_ptr_array(hash_table);

    value_destroy_func = x_steal_pointer(&hash_table->value_destroy_func);
    x_ptr_array_set_free_func(array, value_destroy_func);

    x_hash_table_remove_all(hash_table);
    hash_table->value_destroy_func = x_steal_pointer(&value_destroy_func);

    return array;
}

static xuint x_hash_table_foreach_remove_or_steal(XHashTable *hash_table, XHRFunc func, xpointer user_data, xboolean notify)
{
    xsize i;
    xuint deleted = 0;
    xint version = hash_table->version;

    for (i = 0; i < hash_table->size; i++) {
        xuint node_hash = hash_table->hashes[i];
        xpointer node_key = x_hash_table_fetch_key_or_value(hash_table->keys, i, hash_table->have_big_keys);
        xpointer node_value = x_hash_table_fetch_key_or_value(hash_table->values, i, hash_table->have_big_values);

        if (HASH_IS_REAL(node_hash) && (* func)(node_key, node_value, user_data)) {
            x_hash_table_remove_node(hash_table, i, notify);
            deleted++;
        }

        x_return_val_if_fail(version == hash_table->version, 0);
    }

    x_hash_table_maybe_resize(hash_table);

    if (deleted > 0) {
        hash_table->version++;
    }

    return deleted;
}

xuint x_hash_table_foreach_remove(XHashTable *hash_table, XHRFunc func, xpointer user_data)
{
    x_return_val_if_fail(hash_table != NULL, 0);
    x_return_val_if_fail(func != NULL, 0);

    return x_hash_table_foreach_remove_or_steal(hash_table, func, user_data, TRUE);
}

xuint x_hash_table_foreach_steal(XHashTable *hash_table, XHRFunc func, xpointer user_data)
{
    x_return_val_if_fail(hash_table != NULL, 0);
    x_return_val_if_fail(func != NULL, 0);

    return x_hash_table_foreach_remove_or_steal(hash_table, func, user_data, FALSE);
}

void x_hash_table_foreach(XHashTable *hash_table, XHFunc func, xpointer user_data)
{
    xsize i;
    xint version;

    x_return_if_fail(hash_table != NULL);
    x_return_if_fail(func != NULL);

    version = hash_table->version;

    for (i = 0; i < hash_table->size; i++) {
        xuint node_hash = hash_table->hashes[i];
        xpointer node_key = x_hash_table_fetch_key_or_value(hash_table->keys, i, hash_table->have_big_keys);
        xpointer node_value = x_hash_table_fetch_key_or_value(hash_table->values, i, hash_table->have_big_values);

        if (HASH_IS_REAL(node_hash)) {
            (* func)(node_key, node_value, user_data);
        }

        x_return_if_fail(version == hash_table->version);
    }
}

xpointer x_hash_table_find(XHashTable *hash_table, XHRFunc predicate, xpointer user_data)
{
    xsize i;
    xint version;
    xboolean match;

    x_return_val_if_fail(hash_table != NULL, NULL);
    x_return_val_if_fail(predicate != NULL, NULL);

    version = hash_table->version;
    match = FALSE;

    for (i = 0; i < hash_table->size; i++) {
        xuint node_hash = hash_table->hashes[i];
        xpointer node_key = x_hash_table_fetch_key_or_value(hash_table->keys, i, hash_table->have_big_keys);
        xpointer node_value = x_hash_table_fetch_key_or_value(hash_table->values, i, hash_table->have_big_values);

        if (HASH_IS_REAL(node_hash)) {
            match = predicate(node_key, node_value, user_data);
        }

        x_return_val_if_fail(version == hash_table->version, NULL);
        if (match) {
            return node_value;
        }
    }

    return NULL;
}

xuint x_hash_table_size(XHashTable *hash_table)
{
    x_return_val_if_fail(hash_table != NULL, 0);
    return hash_table->nnodes;
}

XList *x_hash_table_get_keys(XHashTable *hash_table)
{
    xsize i;
    XList *retval;

    x_return_val_if_fail(hash_table != NULL, NULL);

    retval = NULL;
    for (i = 0; i < hash_table->size; i++) {
        if (HASH_IS_REAL(hash_table->hashes[i])) {
            retval = x_list_prepend(retval, x_hash_table_fetch_key_or_value(hash_table->keys, i, hash_table->have_big_keys));
        }
    }

    return retval;
}

xpointer *x_hash_table_get_keys_as_array(XHashTable *hash_table, xuint *length)
{
    xsize i, j = 0;
    xpointer *result;

    result = x_new(xpointer, hash_table->nnodes + 1);
    for (i = 0; i < hash_table->size; i++) {
        if (HASH_IS_REAL(hash_table->hashes[i])) {
            result[j++] = x_hash_table_fetch_key_or_value(hash_table->keys, i, hash_table->have_big_keys);
        }
    }

    x_assert(j == hash_table->nnodes);
    result[j] = NULL;

    if (length) {
        *length = j;
    }

    return result;
}

XPtrArray *x_hash_table_get_keys_as_ptr_array(XHashTable *hash_table)
{
    XPtrArray *array;

    x_return_val_if_fail(hash_table != NULL, NULL);

    array = x_ptr_array_sized_new(hash_table->size);
    for (xsize i = 0; i < hash_table->size; ++i) {
        if (HASH_IS_REAL(hash_table->hashes[i])) {
            x_ptr_array_add(array, x_hash_table_fetch_key_or_value(hash_table->keys, i, hash_table->have_big_keys));
        }
    }
    x_assert(array->len == hash_table->nnodes);

    return array;
}

XList *x_hash_table_get_values(XHashTable *hash_table)
{
    xsize i;
    XList *retval;

    x_return_val_if_fail(hash_table != NULL, NULL);

    retval = NULL;
    for (i = 0; i < hash_table->size; i++) {
        if (HASH_IS_REAL(hash_table->hashes[i])) {
            retval = x_list_prepend(retval, x_hash_table_fetch_key_or_value(hash_table->values, i, hash_table->have_big_values));
        }
    }

    return retval;
}

XPtrArray *x_hash_table_get_values_as_ptr_array(XHashTable *hash_table)
{
    XPtrArray *array;

    x_return_val_if_fail(hash_table != NULL, NULL);

    array = x_ptr_array_sized_new(hash_table->size);
    for (xsize i = 0; i < hash_table->size; ++i) {
        if (HASH_IS_REAL(hash_table->hashes[i])) {
            x_ptr_array_add(array, x_hash_table_fetch_key_or_value(hash_table->values, i, hash_table->have_big_values));
        }
    }
    x_assert(array->len == hash_table->nnodes);

    return array;
}

xboolean (x_str_equal)(xconstpointer v1, xconstpointer v2)
{
    const xchar *string1 = (const xchar *)v1;
    const xchar *string2 = (const xchar *)v2;

    return strcmp(string1, string2) == 0;
}

xuint x_str_hash(xconstpointer v)
{
    xuint32 h = 5381;
    const signed char *p;

    for (p = (const signed char *)v; *p != '\0'; p++) {
        h = (h << 5) + h + *p;
    }

    return h;
}

xuint x_direct_hash(xconstpointer v)
{
    return XPOINTER_TO_UINT(v);
}

xboolean x_direct_equal(xconstpointer v1, xconstpointer v2)
{
    return v1 == v2;
}

xboolean x_int_equal(xconstpointer v1, xconstpointer v2)
{
    return *((const xint *)v1) == *((const xint *)v2);
}

xuint x_int_hash(xconstpointer v)
{
    return *(const xint *)v;
}

xboolean x_uint_equal(xconstpointer v1, xconstpointer v2)
{
    return *((const xuint *) v1) == *((const xuint *) v2);
}

xuint x_uint_hash(xconstpointer v)
{
    return *(const xuint *)v;
}

xboolean x_int64_equal(xconstpointer v1, xconstpointer v2)
{
    return *((const xint64 *)v1) == *((const xint64 *)v2);
}

xuint x_int64_hash(xconstpointer v)
{
    const xuint64 *bits = (const xuint64 *)v;
    return (xuint)((*bits >> 32) ^ (*bits & 0xffffffffU));
}

xboolean x_double_equal(xconstpointer v1, xconstpointer v2)
{
    return *((const xdouble *)v1) == *((const xdouble *)v2);
}

xuint x_double_hash(xconstpointer v)
{
    const xuint64 *bits = (const xuint64 *)v;
    return (xuint)((*bits >> 32) ^ (*bits & 0xffffffffU));
}
