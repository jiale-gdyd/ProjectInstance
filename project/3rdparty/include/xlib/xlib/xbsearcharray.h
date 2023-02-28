#ifndef __X_BSEARCH_ARRAY_H__
#define __X_BSEARCH_ARRAY_H__

#include "../xlib.h"
#include <string.h>

X_BEGIN_DECLS

#define X_BSEARCH_ARRAY_CMP(v1, v2)             ((v1) > (v2) ? +1 : (v1) == (v2) ? 0 : -1)

typedef xint (*XBSearchCompareFunc)(xconstpointer bsearch_node1, xconstpointer bsearch_node2);

typedef enum {
    X_BSEARCH_ARRAY_ALIGN_POWER2 = 1 << 0,
    X_BSEARCH_ARRAY_AUTO_SHRINK  = 1 << 1
} XBSearchArrayFlags;

typedef struct {
    xuint               sizeof_node;
    XBSearchCompareFunc cmp_nodes;
    xuint               flags;
} XBSearchConfig;

typedef union {
    xuint    n_nodes;
    xpointer alignment_dummy1;
    xlong    alignment_dummy2;
    xdouble  alignment_dummy3;
} XBSearchArray;

static inline XBSearchArray *x_bsearch_array_create(const XBSearchConfig *bconfig);
static inline xpointer x_bsearch_array_get_nth(XBSearchArray *barray, const XBSearchConfig *bconfig, xuint nth);

static inline xuint x_bsearch_array_get_index(XBSearchArray *barray, const XBSearchConfig *bconfig, xconstpointer node_in_array);

static inline XBSearchArray *x_bsearch_array_grow(XBSearchArray *barray, const XBSearchConfig *bconfig, xuint index);
static inline XBSearchArray *x_bsearch_array_remove(XBSearchArray *barray, const XBSearchConfig *bconfig, xuint index_);
static inline XBSearchArray *x_bsearch_array_insert(XBSearchArray *barray, const XBSearchConfig *bconfig, xconstpointer key_node);

static inline XBSearchArray *x_bsearch_array_replace(XBSearchArray *barray, const XBSearchConfig *bconfig, xconstpointer key_node);

static inline void x_bsearch_array_free(XBSearchArray *barray, const XBSearchConfig *bconfig);

#define x_bsearch_array_get_n_nodes(barray)                     (((XBSearchArray *)(barray))->n_nodes)
#define x_bsearch_array_lookup(barray, bconfig, key_node)       x_bsearch_array_lookup_fuzzy((barray), (bconfig), (key_node), 0)

#define x_bsearch_array_lookup_sibling(barray, bconfig, key_node)       \
    x_bsearch_array_lookup_fuzzy((barray), (bconfig), (key_node), 1)

#define x_bsearch_array_lookup_insertion(barray, bconfig, key_node)     \
    x_bsearch_array_lookup_fuzzy((barray), (bconfig), (key_node), 2)

#define X_BSEARCH_UPPER_POWER2(n)                               ((n) ? 1 << x_bit_storage((n) - 1) : 0)
#define X_BSEARCH_ARRAY_NODES(barray)                           (((xuint8 *)(barray)) + sizeof(XBSearchArray))

static inline XBSearchArray *x_bsearch_array_create(const XBSearchConfig *bconfig)
{
    xuint size;
    XBSearchArray *barray;

    x_return_val_if_fail(bconfig != NULL, NULL);

    size = sizeof (XBSearchArray) + bconfig->sizeof_node;
    if (bconfig->flags & X_BSEARCH_ARRAY_ALIGN_POWER2) {
        size = X_BSEARCH_UPPER_POWER2(size);
    }

    barray = (XBSearchArray *)x_malloc(size);
    memset(barray, 0, sizeof (XBSearchArray));

    return barray;
}

static inline xpointer x_bsearch_array_lookup_fuzzy(XBSearchArray *barray, const XBSearchConfig *bconfig, xconstpointer key_node, const xuint sibling_or_after);

static inline xpointer x_bsearch_array_lookup_fuzzy(XBSearchArray *barray, const XBSearchConfig *bconfig, xconstpointer key_node, const xuint sibling_or_after)
{
    xint cmp = 0;
    xuint sizeof_node = bconfig->sizeof_node;
    xuint n_nodes = barray->n_nodes, offs = 0;
    XBSearchCompareFunc cmp_nodes = bconfig->cmp_nodes;
    xuint8 *check = NULL, *nodes = X_BSEARCH_ARRAY_NODES(barray);

    while (offs < n_nodes) {
        xuint i = (offs + n_nodes) >> 1;

        check = nodes + i * sizeof_node;
        cmp = cmp_nodes(key_node, check);
        if (cmp == 0) {
            return sibling_or_after > 1 ? NULL : check;
        } else if (cmp < 0) {
            n_nodes = i;
        } else  {
            offs = i + 1;
        }
    }

    return X_LIKELY(!sibling_or_after) ? NULL : (sibling_or_after > 1 && cmp > 0) ? check + sizeof_node : check;
}

static inline xpointer x_bsearch_array_get_nth(XBSearchArray *barray, const XBSearchConfig *bconfig, xuint nth)
{
  return (X_LIKELY(nth < barray->n_nodes) ? X_BSEARCH_ARRAY_NODES(barray) + nth * bconfig->sizeof_node : NULL);
}

static inline xuint x_bsearch_array_get_index(XBSearchArray *barray, const XBSearchConfig *bconfig, xconstpointer node_in_array)
{
    xuint distance = ((xuint8 *)node_in_array) - X_BSEARCH_ARRAY_NODES(barray);

    x_return_val_if_fail(node_in_array != NULL, barray->n_nodes);
    distance /= bconfig->sizeof_node;

    return MIN(distance, barray->n_nodes + 1);
}

static inline XBSearchArray *x_bsearch_array_grow(XBSearchArray *barray, const XBSearchConfig *bconfig, xuint index_)
{
    xuint8 *node;
    xuint old_size = barray->n_nodes * bconfig->sizeof_node;
    xuint new_size = old_size + bconfig->sizeof_node;

    x_return_val_if_fail(index_ <= barray->n_nodes, NULL);

    if (X_UNLIKELY(bconfig->flags & X_BSEARCH_ARRAY_ALIGN_POWER2)) {
        new_size = X_BSEARCH_UPPER_POWER2(sizeof(XBSearchArray) + new_size);
        old_size = X_BSEARCH_UPPER_POWER2(sizeof(XBSearchArray) + old_size);
        if (old_size != new_size) {
            barray = (XBSearchArray *)x_realloc(barray, new_size);
        }
    } else {
        barray = (XBSearchArray *)x_realloc(barray, sizeof(XBSearchArray) + new_size);
    }

    node = X_BSEARCH_ARRAY_NODES(barray) + index_ * bconfig->sizeof_node;
    memmove(node + bconfig->sizeof_node, node, (barray->n_nodes - index_) * bconfig->sizeof_node);
    barray->n_nodes += 1;

    return barray;
}

static inline XBSearchArray *x_bsearch_array_insert(XBSearchArray *barray, const XBSearchConfig *bconfig, xconstpointer key_node)
{
    xuint8 *node;

    if (X_UNLIKELY(!barray->n_nodes)) {
        barray = x_bsearch_array_grow(barray, bconfig, 0);
        node = X_BSEARCH_ARRAY_NODES(barray);
    } else {
        node = (xuint8 *)x_bsearch_array_lookup_insertion(barray, bconfig, key_node);
        if (X_LIKELY(node)) {
            xuint index_ = x_bsearch_array_get_index(barray, bconfig, node);

            barray = x_bsearch_array_grow(barray, bconfig, index_);
            node = X_BSEARCH_ARRAY_NODES(barray) + index_ * bconfig->sizeof_node;
        } else {
            return barray;
        }
    }

    memcpy(node, key_node, bconfig->sizeof_node);
    return barray;
}

static inline XBSearchArray *x_bsearch_array_replace(XBSearchArray *barray, const XBSearchConfig *bconfig, xconstpointer key_node)
{
    xuint8 *node = (xuint8 *)x_bsearch_array_lookup(barray, bconfig, key_node);

    if (X_LIKELY(node)) {
        memcpy(node, key_node, bconfig->sizeof_node);
    } else {
        barray = x_bsearch_array_insert(barray, bconfig, key_node);
    }

    return barray;
}

static inline XBSearchArray *x_bsearch_array_remove(XBSearchArray *barray, const XBSearchConfig *bconfig, xuint index_)
{
    xuint8 *node;

    x_return_val_if_fail(index_ < barray->n_nodes, NULL);

    barray->n_nodes -= 1;
    node = X_BSEARCH_ARRAY_NODES(barray) + index_ * bconfig->sizeof_node;
    memmove(node, node + bconfig->sizeof_node, (barray->n_nodes - index_) * bconfig->sizeof_node);

    if (X_UNLIKELY(bconfig->flags & X_BSEARCH_ARRAY_AUTO_SHRINK)) {
        xuint new_size = barray->n_nodes * bconfig->sizeof_node;
        xuint old_size = new_size + bconfig->sizeof_node;

        if (X_UNLIKELY(bconfig->flags & X_BSEARCH_ARRAY_ALIGN_POWER2)) {
            new_size = X_BSEARCH_UPPER_POWER2(sizeof(XBSearchArray) + new_size);
            old_size = X_BSEARCH_UPPER_POWER2(sizeof(XBSearchArray) + old_size);
            if (old_size != new_size) {
                barray = (XBSearchArray *)x_realloc(barray, new_size);
            }
        } else {
            barray = (XBSearchArray *)x_realloc(barray, sizeof (XBSearchArray) + new_size);
        }
    }

    return barray;
}

static inline void x_bsearch_array_free(XBSearchArray *barray, const XBSearchConfig *bconfig)
{
    x_return_if_fail(barray != NULL);
    x_free(barray);
}

X_END_DECLS

#endif
