#include <xlib/xlib/config.h>
#include <xlib/xlib/xmem.h>
#include <xlib/xlib/xslice.h>
#include <xlib/xlib/xsequence.h>
#include <xlib/xlib/xtestutils.h>

typedef struct _XSequenceNode XSequenceNode;

struct _XSequence {
    XSequenceNode  *end_node;
    XDestroyNotify data_destroy_notify;
    xboolean       access_prohibited;
    XSequence      *real_sequence;
};

struct _XSequenceNode {
    xint          n_nodes;
    xuint32       priority;
    XSequenceNode *parent;
    XSequenceNode *left;
    XSequenceNode *right;
    xpointer      data;
};

static XSequenceNode *node_new(xpointer data);
static XSequenceNode *node_get_first(XSequenceNode *node);
static XSequenceNode *node_get_last(XSequenceNode *node);
static XSequenceNode *node_get_prev(XSequenceNode *node);
static XSequenceNode *node_get_next(XSequenceNode *node);
static xint node_get_pos(XSequenceNode *node);
static XSequenceNode *node_get_by_pos(XSequenceNode *node, xint pos);
static XSequenceNode *node_find(XSequenceNode *haystack, XSequenceNode *needle, XSequenceNode *end, XSequenceIterCompareFunc cmp, xpointer user_data);
static XSequenceNode *node_find_closest(XSequenceNode *haystack, XSequenceNode *needle, XSequenceNode *end, XSequenceIterCompareFunc cmp, xpointer user_data);
static xint node_get_length(XSequenceNode *node);
static void node_free(XSequenceNode *node, XSequence *seq);
static void node_cut(XSequenceNode *split);
static void node_insert_before(XSequenceNode *node, XSequenceNode *newt);
static void node_unlink(XSequenceNode *node);
static void node_join(XSequenceNode *left, XSequenceNode *right);
static void node_insert_sorted(XSequenceNode *node, XSequenceNode *newt, XSequenceNode *end, XSequenceIterCompareFunc cmp_func, xpointer cmp_data);

static void check_seq_access(XSequence *seq)
{
    if (X_UNLIKELY(seq->access_prohibited)) {
        x_warning("Accessing a sequence while it is being sorted or searched is not allowed");
    }
}

static XSequence *get_sequence(XSequenceNode *node)
{
    return (XSequence *)node_get_last(node)->data;
}

static xboolean seq_is_end(XSequence *seq, XSequenceIter *iter)
{
    return seq->end_node == iter;
}

static xboolean is_end(XSequenceIter *iter)
{
    XSequenceIter *parent = iter->parent;

    if (iter->right) {
        return FALSE;
    }

    if (!parent) {
        return TRUE;
    }

    while (parent->right == iter) {
        iter = parent;
        parent = iter->parent;

        if (!parent) {
            return TRUE;
        }
    }

    return FALSE;
}

typedef struct {
    XCompareDataFunc cmp_func;
    xpointer         cmp_data;
    XSequenceNode    *end_node;
} SortInfo;

static xint iter_compare(XSequenceIter *node1, XSequenceIter *node2, xpointer data)
{
    xint retval;
    const SortInfo *info = (const SortInfo *)data;

    if (node1 == info->end_node) {
        return 1;
    }

    if (node2 == info->end_node) {
        return -1;
    }

    retval = info->cmp_func(node1->data, node2->data, info->cmp_data);
    return retval;
}

XSequence *x_sequence_new(XDestroyNotify data_destroy)
{
    XSequence *seq = x_new(XSequence, 1);
    seq->data_destroy_notify = data_destroy;

    seq->end_node = node_new(seq);
    seq->access_prohibited = FALSE;
    seq->real_sequence = seq;

    return seq;
}

void x_sequence_free(XSequence *seq)
{
    x_return_if_fail(seq != NULL);

    check_seq_access(seq);
    node_free(seq->end_node, seq);
    x_free(seq);
}

void x_sequence_foreach_range(XSequenceIter *begin, XSequenceIter *end, XFunc func, xpointer user_data)
{
    XSequence *seq;
    XSequenceIter *iter;

    x_return_if_fail(func != NULL);
    x_return_if_fail(begin != NULL);
    x_return_if_fail(end != NULL);

    seq = get_sequence(begin);
    seq->access_prohibited = TRUE;

    iter = begin;
    while (iter != end) {
        XSequenceIter *next = node_get_next(iter);
        func(iter->data, user_data);
        iter = next;
    }

    seq->access_prohibited = FALSE;
}

void x_sequence_foreach(XSequence *seq, XFunc func, xpointer user_data)
{
    XSequenceIter *begin, *end;

    check_seq_access(seq);

    begin = x_sequence_get_begin_iter(seq);
    end = x_sequence_get_end_iter(seq);

    x_sequence_foreach_range(begin, end, func, user_data);
}

XSequenceIter *x_sequence_range_get_midpoint(XSequenceIter *begin, XSequenceIter *end)
{
    int begin_pos, end_pos, mid_pos;

    x_return_val_if_fail(begin != NULL, NULL);
    x_return_val_if_fail(end != NULL, NULL);
    x_return_val_if_fail(get_sequence(begin) == get_sequence(end), NULL);

    begin_pos = node_get_pos(begin);
    end_pos = node_get_pos(end);

    x_return_val_if_fail(end_pos >= begin_pos, NULL);
    mid_pos = begin_pos + (end_pos - begin_pos) / 2;

    return node_get_by_pos(begin, mid_pos);
}

xint x_sequence_iter_compare(XSequenceIter *a, XSequenceIter *b)
{
    xint a_pos, b_pos;
    XSequence *seq_a, *seq_b;

    x_return_val_if_fail(a != NULL, 0);
    x_return_val_if_fail(b != NULL, 0);

    seq_a = get_sequence(a);
    seq_b = get_sequence(b);
    x_return_val_if_fail(seq_a == seq_b, 0);

    check_seq_access(seq_a);
    check_seq_access(seq_b);

    a_pos = node_get_pos(a);
    b_pos = node_get_pos(b);

    if (a_pos == b_pos) {
        return 0;
    } else if (a_pos > b_pos) {
        return 1;
    } else {
        return -1;
    }
}

XSequenceIter *x_sequence_append(XSequence *seq, xpointer data)
{
    XSequenceNode *node;

    x_return_val_if_fail(seq != NULL, NULL);

    check_seq_access(seq);
    node = node_new(data);
    node_insert_before(seq->end_node, node);

    return node;
}

XSequenceIter *x_sequence_prepend(XSequence *seq, xpointer data)
{
    XSequenceNode *node, *first;

    x_return_val_if_fail(seq != NULL, NULL);

    check_seq_access(seq);
    node = node_new(data);
    first = node_get_first(seq->end_node);
    node_insert_before(first, node);

    return node;
}

XSequenceIter *x_sequence_insert_before(XSequenceIter *iter, xpointer data)
{
    XSequence *seq;
    XSequenceNode *node;

    x_return_val_if_fail(iter != NULL, NULL);

    seq = get_sequence(iter);
    check_seq_access(seq);
    node = node_new(data);
    node_insert_before(iter, node);

    return node;
}

void x_sequence_remove(XSequenceIter *iter)
{
    XSequence *seq;

    x_return_if_fail(iter != NULL);

    seq = get_sequence(iter);
    x_return_if_fail(!seq_is_end(seq, iter));

    check_seq_access(seq);
    node_unlink(iter);
    node_free(iter, seq);
}

void x_sequence_remove_range(XSequenceIter *begin, XSequenceIter *end)
{
    XSequence *seq_begin, *seq_end;

    seq_begin = get_sequence(begin);
    seq_end = get_sequence(end);
    x_return_if_fail(seq_begin == seq_end);

    x_sequence_move_range(NULL, begin, end);
}

void x_sequence_move_range(XSequenceIter *dest, XSequenceIter *begin, XSequenceIter *end)
{
    XSequenceNode *first;
    XSequence *src_seq, *end_seq, *dest_seq = NULL;

    x_return_if_fail(begin != NULL);
    x_return_if_fail(end != NULL);

    src_seq = get_sequence(begin);
    check_seq_access(src_seq);

    end_seq = get_sequence(end);
    check_seq_access(end_seq);

    if (dest) {
        dest_seq = get_sequence(dest);
        check_seq_access(dest_seq);
    }

    x_return_if_fail(src_seq == end_seq);

    if (dest == begin || dest == end) {
        return;
    }

    if (x_sequence_iter_compare(begin, end) >= 0) {
        return;
    }

    if (dest && dest_seq == src_seq && x_sequence_iter_compare(dest, begin) > 0 && x_sequence_iter_compare(dest, end) < 0) {
        return;
    }

    first = node_get_first(begin);

    node_cut(begin);
    node_cut(end);

    if (first != begin) {
        node_join(first, end);
    }

    if (dest) {
        first = node_get_first(dest);

        node_cut(dest);
        node_join(begin, dest);

        if (dest != first) {
            node_join(first, begin);
        }
    } else {
        node_free(begin, src_seq);
    }
}

void x_sequence_sort(XSequence *seq, XCompareDataFunc cmp_func, xpointer cmp_data)
{
    SortInfo info;

    info.cmp_func = cmp_func;
    info.cmp_data = cmp_data;
    info.end_node = seq->end_node;

    check_seq_access(seq);
    x_sequence_sort_iter(seq, iter_compare, &info);
}

XSequenceIter *x_sequence_insert_sorted(XSequence *seq, xpointer data, XCompareDataFunc cmp_func, xpointer cmp_data)
{
    SortInfo info;

    x_return_val_if_fail(seq != NULL, NULL);
    x_return_val_if_fail(cmp_func != NULL, NULL);

    info.cmp_func = cmp_func;
    info.cmp_data = cmp_data;
    info.end_node = seq->end_node;
    check_seq_access(seq);

    return x_sequence_insert_sorted_iter(seq, data, iter_compare, &info);
}

void x_sequence_sort_changed(XSequenceIter *iter, XCompareDataFunc cmp_func, xpointer cmp_data)
{
    SortInfo info;
    XSequence *seq;

    x_return_if_fail(iter != NULL);

    seq = get_sequence(iter);
    x_return_if_fail(!seq_is_end(seq, iter));

    info.cmp_func = cmp_func;
    info.cmp_data = cmp_data;
    info.end_node = seq->end_node;

    x_sequence_sort_changed_iter(iter, iter_compare, &info);
}

XSequenceIter *x_sequence_search(XSequence *seq, xpointer data, XCompareDataFunc cmp_func, xpointer cmp_data)
{
    SortInfo info;

    x_return_val_if_fail(seq != NULL, NULL);

    info.cmp_func = cmp_func;
    info.cmp_data = cmp_data;
    info.end_node = seq->end_node;
    check_seq_access(seq);

    return x_sequence_search_iter(seq, data, iter_compare, &info);
}

XSequenceIter *x_sequence_lookup(XSequence *seq, xpointer data, XCompareDataFunc cmp_func, xpointer cmp_data)
{
    SortInfo info;

    x_return_val_if_fail(seq != NULL, NULL);

    info.cmp_func = cmp_func;
    info.cmp_data = cmp_data;
    info.end_node = seq->end_node;
    check_seq_access(seq);

    return x_sequence_lookup_iter(seq, data, iter_compare, &info);
}

void x_sequence_sort_iter(XSequence *seq, XSequenceIterCompareFunc cmp_func, xpointer cmp_data)
{
    XSequence *tmp;
    XSequenceNode *begin, *end;

    x_return_if_fail(seq != NULL);
    x_return_if_fail(cmp_func != NULL);

    check_seq_access(seq);

    begin = x_sequence_get_begin_iter(seq);
    end = x_sequence_get_end_iter(seq);

    tmp = x_sequence_new(NULL);
    tmp->real_sequence = seq;

    x_sequence_move_range(x_sequence_get_begin_iter(tmp), begin, end);

    seq->access_prohibited = TRUE;
    tmp->access_prohibited = TRUE;

    while (!x_sequence_is_empty(tmp)) {
        XSequenceNode *node = x_sequence_get_begin_iter(tmp);
        node_insert_sorted(seq->end_node, node, seq->end_node, cmp_func, cmp_data);
    }

    tmp->access_prohibited = FALSE;
    seq->access_prohibited = FALSE;

    x_sequence_free(tmp);
}

void x_sequence_sort_changed_iter(XSequenceIter *iter, XSequenceIterCompareFunc iter_cmp, xpointer cmp_data)
{
    XSequence *seq, *tmp_seq;
    XSequenceIter *next, *prev;

    x_return_if_fail(iter != NULL);
    x_return_if_fail(iter_cmp != NULL);

    seq = get_sequence(iter);
    x_return_if_fail(!seq_is_end(seq, iter));

    check_seq_access(seq);

    next = node_get_next(iter);
    prev = node_get_prev(iter);

    if (prev != iter && iter_cmp(prev, iter, cmp_data) == 0) {
        return;
    }

    if (!is_end(next) && iter_cmp(next, iter, cmp_data) == 0) {
        return;
    }

    seq->access_prohibited = TRUE;

    tmp_seq = x_sequence_new(NULL);
    tmp_seq->real_sequence = seq;

    node_unlink(iter);
    node_insert_before(tmp_seq->end_node, iter);
    node_insert_sorted(seq->end_node, iter, seq->end_node, iter_cmp, cmp_data);

    x_sequence_free(tmp_seq);

    seq->access_prohibited = FALSE;
}

XSequenceIter *x_sequence_insert_sorted_iter(XSequence *seq, xpointer data, XSequenceIterCompareFunc iter_cmp, xpointer cmp_data)
{
    XSequence *tmp_seq;
    XSequenceNode *new_node;

    x_return_val_if_fail(seq != NULL, NULL);
    x_return_val_if_fail(iter_cmp != NULL, NULL);

    check_seq_access(seq);

    seq->access_prohibited = TRUE;
    tmp_seq = x_sequence_new(NULL);
    tmp_seq->real_sequence = seq;

    new_node = x_sequence_append(tmp_seq, data);

    node_insert_sorted(seq->end_node, new_node, seq->end_node, iter_cmp, cmp_data);
    x_sequence_free(tmp_seq);
    seq->access_prohibited = FALSE;

    return new_node;
}

XSequenceIter *x_sequence_search_iter(XSequence *seq, xpointer data, XSequenceIterCompareFunc iter_cmp, xpointer cmp_data)
{
    XSequence *tmp_seq;
    XSequenceNode *node;
    XSequenceNode *dummy;

    x_return_val_if_fail(seq != NULL, NULL);

    check_seq_access(seq);

    seq->access_prohibited = TRUE;
    tmp_seq = x_sequence_new(NULL);
    tmp_seq->real_sequence = seq;

    dummy = x_sequence_append(tmp_seq, data);
    node = node_find_closest(seq->end_node, dummy, seq->end_node, iter_cmp, cmp_data);

    x_sequence_free(tmp_seq);
    seq->access_prohibited = FALSE;

    return node;
}

XSequenceIter *x_sequence_lookup_iter(XSequence *seq, xpointer data, XSequenceIterCompareFunc iter_cmp, xpointer cmp_data)
{
    XSequence *tmp_seq;
    XSequenceNode *node;
    XSequenceNode *dummy;

    x_return_val_if_fail(seq != NULL, NULL);

    check_seq_access(seq);
    seq->access_prohibited = TRUE;
    tmp_seq = x_sequence_new(NULL);
    tmp_seq->real_sequence = seq;
    dummy = x_sequence_append(tmp_seq, data);
    node = node_find(seq->end_node, dummy, seq->end_node, iter_cmp, cmp_data);
    x_sequence_free(tmp_seq);
    seq->access_prohibited = FALSE;

    return node;
}

XSequence *x_sequence_iter_get_sequence(XSequenceIter *iter)
{
    XSequence *seq;

    x_return_val_if_fail(iter != NULL, NULL);

    seq = get_sequence(iter);
    return seq->real_sequence;
}

xpointer x_sequence_get(XSequenceIter *iter)
{
    x_return_val_if_fail(iter != NULL, NULL);
    x_return_val_if_fail(!is_end(iter), NULL);

    return iter->data;
}

void x_sequence_set(XSequenceIter *iter, xpointer data)
{
    XSequence *seq;

    x_return_if_fail(iter != NULL);

    seq = get_sequence(iter);
    x_return_if_fail(!seq_is_end(seq, iter));

    if (seq->data_destroy_notify) {
        seq->data_destroy_notify(iter->data);
    }

    iter->data = data;
}

xint x_sequence_get_length(XSequence *seq)
{
    return node_get_length(seq->end_node) - 1;
}

xboolean x_sequence_is_empty(XSequence *seq)
{
    return (seq->end_node->parent == NULL) && (seq->end_node->left == NULL);
}

XSequenceIter *x_sequence_get_end_iter(XSequence *seq)
{
    x_return_val_if_fail(seq != NULL, NULL);
    return seq->end_node;
}

XSequenceIter *x_sequence_get_begin_iter(XSequence *seq)
{
    x_return_val_if_fail(seq != NULL, NULL);
    return node_get_first(seq->end_node);
}

static int clamp_position(XSequence *seq, int pos)
{
    xint len = x_sequence_get_length(seq);
    if (pos > len || pos < 0) {
        pos = len;
    }

    return pos;
}

XSequenceIter *x_sequence_get_iter_at_pos(XSequence *seq, xint pos)
{
    x_return_val_if_fail(seq != NULL, NULL);
    pos = clamp_position(seq, pos);

    return node_get_by_pos(seq->end_node, pos);
}

void x_sequence_move(XSequenceIter *src, XSequenceIter *dest)
{
    x_return_if_fail(src != NULL);
    x_return_if_fail(dest != NULL);
    x_return_if_fail(!is_end(src));

    if (src == dest) {
        return;
    }

    node_unlink(src);
    node_insert_before(dest, src);
}

xboolean x_sequence_iter_is_end(XSequenceIter *iter)
{
    x_return_val_if_fail(iter != NULL, FALSE);
    return is_end(iter);
}

xboolean x_sequence_iter_is_begin(XSequenceIter *iter)
{
    x_return_val_if_fail(iter != NULL, FALSE);
    return (node_get_prev(iter) == iter);
}

xint x_sequence_iter_get_position(XSequenceIter *iter)
{
    x_return_val_if_fail(iter != NULL, -1);
    return node_get_pos(iter);
}

XSequenceIter *x_sequence_iter_next(XSequenceIter *iter)
{
    x_return_val_if_fail(iter != NULL, NULL);
    return node_get_next(iter);
}

XSequenceIter *x_sequence_iter_prev(XSequenceIter *iter)
{
    x_return_val_if_fail(iter != NULL, NULL);
    return node_get_prev(iter);
}

XSequenceIter *x_sequence_iter_move(XSequenceIter *iter, xint delta)
{
    xint len;
    xint new_pos;

    x_return_val_if_fail(iter != NULL, NULL);

    len = x_sequence_get_length(get_sequence(iter));
    new_pos = node_get_pos(iter) + delta;

    if (new_pos < 0) {
        new_pos = 0;
    } else if (new_pos > len) {
        new_pos = len;
    }

    return node_get_by_pos(iter, new_pos);
}

void x_sequence_swap(XSequenceIter *a, XSequenceIter *b)
{
    int a_pos, b_pos;
    XSequenceNode *leftmost, *rightmost, *rightmost_next;

    x_return_if_fail(!x_sequence_iter_is_end(a));
    x_return_if_fail(!x_sequence_iter_is_end(b));

    if (a == b) {
        return;
    }

    a_pos = x_sequence_iter_get_position(a);
    b_pos = x_sequence_iter_get_position(b);

    if (a_pos > b_pos) {
        leftmost = b;
        rightmost = a;
    } else {
        leftmost = a;
        rightmost = b;
    }

    rightmost_next = node_get_next(rightmost);

    x_sequence_move(rightmost, leftmost);
    x_sequence_move(leftmost, rightmost_next);
}

static xuint32 hash_uint32(xuint32 key)
{
    key = (key << 15) - key - 1;
    key = key ^ (key >> 12);
    key = key + (key << 2);
    key = key ^ (key >> 4);
    key = key + (key << 3) + (key << 11);
    key = key ^ (key >> 16);

    return key;
}

static inline xuint get_priority(XSequenceNode *node)
{
    return node->priority;
}

static xuint make_priority(xuint32 key)
{
    key = hash_uint32(key);
    return key? key : 1;
}

static XSequenceNode *find_root(XSequenceNode *node)
{
    while (node->parent) {
        node = node->parent;
    }

    return node;
}

static XSequenceNode *node_new(xpointer data)
{
    static xuint64 counter = 0;
    XSequenceNode *node = x_slice_new0(XSequenceNode);
    xuint32 hash_key = (xuint32)XPOINTER_TO_UINT(node);

    hash_key ^= (xuint32)counter;
    counter++;

    node->n_nodes = 1;
    node->priority = make_priority(hash_key);
    node->data = data;
    node->left = NULL;
    node->right = NULL;
    node->parent = NULL;

    return node;
}

static XSequenceNode *node_get_first(XSequenceNode *node)
{
    node = find_root(node);

    while (node->left) {
        node = node->left;
    }

    return node;
}

static XSequenceNode *node_get_last(XSequenceNode *node)
{
    node = find_root(node);

    while (node->right) {
        node = node->right;
    }

    return node;
}

#define NODE_LEFT_CHILD(n)          (((n)->parent) && ((n)->parent->left) == (n))
#define NODE_RIGHT_CHILD(n)         (((n)->parent) && ((n)->parent->right) == (n))

static XSequenceNode *node_get_next(XSequenceNode *node)
{
    XSequenceNode *n = node;

    if (n->right) {
        n = n->right;
        while (n->left)
            n = n->left;
    } else {
        while (NODE_RIGHT_CHILD(n)) {
            n = n->parent;
        }

        if (n->parent) {
            n = n->parent;
        } else {
            n = node;
        }
    }

    return n;
}

static XSequenceNode *node_get_prev(XSequenceNode *node)
{
    XSequenceNode *n = node;

    if (n->left) {
        n = n->left;
        while (n->right) {
            n = n->right;
        }
    } else {
        while (NODE_LEFT_CHILD(n)) {
            n = n->parent;
        }

        if (n->parent) {
            n = n->parent;
        } else {
            n = node;
        }
    }

    return n;
}

#define N_NODES(n)                  ((n)? (n)->n_nodes : 0)

static xint node_get_pos(XSequenceNode *node)
{
    int n_smaller = 0;

    if (node->left) {
        n_smaller = node->left->n_nodes;
    }

    while (node) {
        if (NODE_RIGHT_CHILD(node)) {
            n_smaller += N_NODES(node->parent->left) + 1;
        }

        node = node->parent;
    }

    return n_smaller;
}

static XSequenceNode *node_get_by_pos(XSequenceNode *node, xint pos)
{
    int i;

    node = find_root(node);

    while ((i = N_NODES(node->left)) != pos) {
        if (i < pos) {
            node = node->right;
            pos -= (i + 1);
        } else {
            node = node->left;
        }

        x_assert(node != NULL);
    }

    return node;
}

static XSequenceNode *node_find(XSequenceNode *haystack, XSequenceNode *needle, XSequenceNode *end, XSequenceIterCompareFunc iter_cmp, xpointer cmp_data)
{
    xint c;

    haystack = find_root(haystack);

    do {
        if (haystack == end) {
            c = 1;
        } else {
            c = iter_cmp (haystack, needle, cmp_data);
        }

        if (c == 0) {
            break;
        }

        if (c > 0) {
            haystack = haystack->left;
        } else {
            haystack = haystack->right;
        }
    } while (haystack != NULL);

    return haystack;
}

static XSequenceNode *node_find_closest(XSequenceNode *haystack, XSequenceNode *needle, XSequenceNode *end, XSequenceIterCompareFunc iter_cmp, xpointer cmp_data)
{
    xint c;
    XSequenceNode *best;

    haystack = find_root(haystack);

    do {
        best = haystack;

        if (haystack == end) {
            c = 1;
        } else {
            c = iter_cmp(haystack, needle, cmp_data);
        }

        if (c > 0) {
            haystack = haystack->left;
        } else {
            haystack = haystack->right;
        }
    } while (haystack != NULL);

    if (best != end && c <= 0) {
        best = node_get_next(best);
    }

    return best;
}

static xint node_get_length(XSequenceNode *node)
{
    node = find_root(node);
    return node->n_nodes;
}

static void real_node_free(XSequenceNode *node, XSequence *seq)
{
    if (node) {
        real_node_free(node->left, seq);
        real_node_free(node->right, seq);

        if (seq && seq->data_destroy_notify && node != seq->end_node) {
            seq->data_destroy_notify(node->data);
        }

        x_slice_free(XSequenceNode, node);
    }
}

static void node_free(XSequenceNode *node, XSequence *seq)
{
    node = find_root(node);
    real_node_free(node, seq);
}

static void node_update_fields(XSequenceNode *node)
{
    int n_nodes = 1;

    n_nodes += N_NODES(node->left);
    n_nodes += N_NODES(node->right);

    node->n_nodes = n_nodes;
}

static void node_rotate(XSequenceNode *node)
{
    XSequenceNode *tmp, *old;

    x_assert(node->parent);
    x_assert(node->parent != node);

    if (NODE_LEFT_CHILD(node)) {
        tmp = node->right;

        node->right = node->parent;
        node->parent = node->parent->parent;
        if (node->parent) {
            if (node->parent->left == node->right) {
                node->parent->left = node;
            } else {
                node->parent->right = node;
            }
        }

        x_assert(node->right);

        node->right->parent = node;
        node->right->left = tmp;

        if (node->right->left) {
            node->right->left->parent = node->right;
        }

        old = node->right;
    } else {
        tmp = node->left;

        node->left = node->parent;
        node->parent = node->parent->parent;
        if (node->parent) {
            if (node->parent->right == node->left) {
                node->parent->right = node;
            } else {
                node->parent->left = node;
            }
        }

        x_assert(node->left);

        node->left->parent = node;
        node->left->right = tmp;

        if (node->left->right) {
            node->left->right->parent = node->left;
        }

        old = node->left;
    }

    node_update_fields(old);
    node_update_fields(node);
}

static void node_update_fields_deep(XSequenceNode *node)
{
    if (node) {
        node_update_fields(node);
        node_update_fields_deep(node->parent);
    }
}

static void rotate_down(XSequenceNode *node, xuint priority)
{
    xuint left, right;

    left = node->left ? get_priority(node->left)  : 0;
    right = node->right ? get_priority(node->right) : 0;

    while (priority < left || priority < right) {
        if (left > right) {
            node_rotate(node->left);
        } else {
            node_rotate(node->right);
        }

        left = node->left ? get_priority(node->left)  : 0;
        right = node->right ? get_priority(node->right) : 0;
    }
}

static void node_cut(XSequenceNode *node)
{
    while (node->parent) {
        node_rotate(node);
    }

    if (node->left) {
        node->left->parent = NULL;
    }

    node->left = NULL;
    node_update_fields(node);

    rotate_down(node, get_priority(node));
}

static void node_join(XSequenceNode *left, XSequenceNode *right)
{
    XSequenceNode *fake = node_new(NULL);

    fake->left = find_root(left);
    fake->right = find_root(right);
    fake->left->parent = fake;
    fake->right->parent = fake;

    node_update_fields(fake);
    node_unlink(fake);
    node_free(fake, NULL);
}

static void node_insert_before(XSequenceNode *node, XSequenceNode *newt)
{
    newt->left = node->left;
    if (newt->left) {
        newt->left->parent = newt;
    }

    newt->parent = node;
    node->left = newt;

    node_update_fields_deep(newt);

    while (newt->parent && get_priority(newt) > get_priority(newt->parent)) {
        node_rotate(newt);
    }

    rotate_down(newt, get_priority(newt));
}

static void node_unlink(XSequenceNode *node)
{
    rotate_down(node, 0);

    if (NODE_RIGHT_CHILD(node)) {
        node->parent->right = NULL;
    } else if (NODE_LEFT_CHILD(node)) {
        node->parent->left = NULL;
    }

    if (node->parent) {
        node_update_fields_deep(node->parent);
    }

    node->parent = NULL;
}

static void node_insert_sorted(XSequenceNode *node, XSequenceNode *newt, XSequenceNode *end, XSequenceIterCompareFunc iter_cmp, xpointer cmp_data)
{
    XSequenceNode *closest;

    closest = node_find_closest(node, newt, end, iter_cmp, cmp_data);
    node_unlink(newt);
    node_insert_before(closest, newt);
}
