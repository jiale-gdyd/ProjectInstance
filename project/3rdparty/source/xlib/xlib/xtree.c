#include <xlib/xlib/config.h>
#include <xlib/xlib/xtree.h>
#include <xlib/xlib/xslice.h>
#include <xlib/xlib/xatomic.h>
#include <xlib/xlib/xmessages.h>
#include <xlib/xlib/xtestutils.h>

#define MAX_GTREE_HEIGHT        40

X_STATIC_ASSERT((X_XUINT64_CONSTANT(1) << (MAX_GTREE_HEIGHT - 2)) >= X_MAXUINT);

struct _XTree {
    XTreeNode        *root;
    XCompareDataFunc key_compare;
    XDestroyNotify   key_destroy_func;
    XDestroyNotify   value_destroy_func;
    xpointer         key_compare_data;
    xuint            nnodes;
    xint             ref_count;
};

struct _XTreeNode {
    xpointer  key;
    xpointer  value;
    XTreeNode *left;
    XTreeNode *right;
    xint8     balance;
    xuint8    left_child;
    xuint8    right_child;
};

static XTreeNode *x_tree_node_new(xpointer key, xpointer value);

static xboolean x_tree_remove_internal(XTree *tree, xconstpointer key, xboolean steal);
static XTreeNode *x_tree_insert_internal(XTree *tree, xpointer key, xpointer value, xboolean replace, xboolean null_ret_ok);

static XTreeNode *x_tree_node_balance(XTreeNode *node);
static XTreeNode *x_tree_find_node(XTree *tree, xconstpointer key);

static xint x_tree_node_pre_order(XTreeNode *node, XTraverseFunc traverse_func, xpointer data);
static xint x_tree_node_in_order(XTreeNode *node, XTraverseFunc traverse_func, xpointer data);
static xint x_tree_node_post_order(XTreeNode *node, XTraverseFunc traverse_func, xpointer data);
static XTreeNode *x_tree_node_search(XTreeNode *node, XCompareFunc search_func, xconstpointer data);

static XTreeNode *x_tree_node_rotate_left(XTreeNode *node);
static XTreeNode *x_tree_node_rotate_right(XTreeNode *node);

static XTreeNode *x_tree_node_new(xpointer key, xpointer value)
{
    XTreeNode *node = x_slice_new(XTreeNode);

    node->balance = 0;
    node->left = NULL;
    node->right = NULL;
    node->left_child = FALSE;
    node->right_child = FALSE;
    node->key = key;
    node->value = value;

    return node;
}

XTree *x_tree_new(XCompareFunc key_compare_func)
{
    x_return_val_if_fail(key_compare_func != NULL, NULL);
    return x_tree_new_full((XCompareDataFunc)key_compare_func, NULL, NULL, NULL);
}

XTree *x_tree_new_with_data(XCompareDataFunc key_compare_func, xpointer key_compare_data)
{
    x_return_val_if_fail(key_compare_func != NULL, NULL);
    return x_tree_new_full(key_compare_func, key_compare_data,  NULL, NULL);
}

XTree *x_tree_new_full(XCompareDataFunc key_compare_func, xpointer key_compare_data, XDestroyNotify key_destroy_func, XDestroyNotify value_destroy_func)
{
    XTree *tree;

    x_return_val_if_fail(key_compare_func != NULL, NULL);

    tree = x_slice_new(XTree);
    tree->root = NULL;
    tree->key_compare = key_compare_func;
    tree->key_destroy_func = key_destroy_func;
    tree->value_destroy_func = value_destroy_func;
    tree->key_compare_data  = key_compare_data;
    tree->nnodes = 0;
    tree->ref_count = 1;

    return tree;
}

XTreeNode *x_tree_node_first(XTree *tree)
{
    XTreeNode *tmp;

    x_return_val_if_fail(tree != NULL, NULL);

    if (!tree->root) {
        return NULL;
    }
    tmp = tree->root;

    while (tmp->left_child) {
        tmp = tmp->left;
    }

    return tmp;
}

XTreeNode *x_tree_node_last(XTree *tree)
{
    XTreeNode *tmp;

    x_return_val_if_fail(tree != NULL, NULL);

    if (!tree->root) {
        return NULL;
    }
    tmp = tree->root;

    while (tmp->right_child) {
        tmp = tmp->right;
    }

    return tmp;
}

XTreeNode *x_tree_node_previous(XTreeNode *node)
{
    XTreeNode *tmp;

    x_return_val_if_fail(node != NULL, NULL);

    tmp = node->left;
    if (node->left_child) {
        while (tmp->right_child) {
            tmp = tmp->right;
        }
    }

    return tmp;
}

XTreeNode *x_tree_node_next(XTreeNode *node)
{
    XTreeNode *tmp;

    x_return_val_if_fail(node != NULL, NULL);

    tmp = node->right;
    if (node->right_child) {
        while (tmp->left_child) {
            tmp = tmp->left;
        }
    }

    return tmp;
}

void x_tree_remove_all(XTree *tree)
{
    XTreeNode *node;
    XTreeNode *next;

    x_return_if_fail(tree != NULL);

    node = x_tree_node_first(tree);
    while (node) {
        next = x_tree_node_next(node);

        if (tree->key_destroy_func) {
            tree->key_destroy_func(node->key);
        }

        if (tree->value_destroy_func) {
            tree->value_destroy_func(node->value);
        }

        x_slice_free(XTreeNode, node);
        node = next;
    }

    tree->root = NULL;
    tree->nnodes = 0;
}

XTree *x_tree_ref(XTree *tree)
{
    x_return_val_if_fail(tree != NULL, NULL);

    x_atomic_int_inc(&tree->ref_count);
    return tree;
}

void x_tree_unref(XTree *tree)
{
    x_return_if_fail(tree != NULL);

    if (x_atomic_int_dec_and_test(&tree->ref_count)) {
        x_tree_remove_all(tree);
        x_slice_free(XTree, tree);
    }
}

void x_tree_destroy(XTree *tree)
{
    x_return_if_fail(tree != NULL);

    x_tree_remove_all(tree);
    x_tree_unref(tree);
}

static XTreeNode *x_tree_insert_replace_node_internal(XTree *tree, xpointer key, xpointer value, xboolean replace, xboolean null_ret_ok)
{
    XTreeNode *node;

    x_return_val_if_fail(tree != NULL, NULL);

    node = x_tree_insert_internal(tree, key, value, replace, null_ret_ok);

#ifdef X_TREE_DEBUG
    x_tree_node_check(tree->root);
#endif

    return node;
}

XTreeNode *x_tree_insert_node(XTree *tree, xpointer key, xpointer value)
{
    return x_tree_insert_replace_node_internal(tree, key, value, FALSE, TRUE);
}

void x_tree_insert(XTree *tree, xpointer key, xpointer value)
{
    x_tree_insert_replace_node_internal(tree, key, value, FALSE, FALSE);
}

XTreeNode *x_tree_replace_node(XTree *tree, xpointer key, xpointer value)
{
    return x_tree_insert_replace_node_internal(tree, key, value, TRUE, TRUE);
}

void x_tree_replace(XTree *tree, xpointer key, xpointer value)
{
    x_tree_insert_replace_node_internal(tree, key, value, TRUE, FALSE);
}

static xboolean x_tree_nnodes_inc_checked(XTree *tree, xboolean overflow_fatal)
{
    if (X_UNLIKELY(tree->nnodes == X_MAXUINT)) {
        if (overflow_fatal) {
            x_error("Incrementing GTree nnodes counter would overflow");
        }

        return FALSE;
    }

    tree->nnodes++;
    return TRUE;
}

static XTreeNode *x_tree_insert_internal(XTree *tree, xpointer key, xpointer value, xboolean replace, xboolean null_ret_ok)
{
    int idx;
    XTreeNode *node, *retnode;
    XTreeNode *path[MAX_GTREE_HEIGHT];

    x_return_val_if_fail(tree != NULL, NULL);

    if (!tree->root) {
        tree->root = x_tree_node_new(key, value);
#ifdef X_TREE_DEBUG
        x_assert(tree->nnodes == 0);
#endif
        tree->nnodes++;
        return tree->root;
    }

    idx = 0;
    path[idx++] = NULL;
    node = tree->root;

    while (1) {
        int cmp = tree->key_compare(key, node->key, tree->key_compare_data);
        if (cmp == 0) {
            if (tree->value_destroy_func) {
                tree->value_destroy_func (node->value);
            }

            node->value = value;
            if (replace) {
                if (tree->key_destroy_func) {
                    tree->key_destroy_func(node->key);
                }

                node->key = key;
            } else {
                if (tree->key_destroy_func) {
                    tree->key_destroy_func(key);
                }
            }

            return node;
        } else if (cmp < 0) {
            if (node->left_child) {
                path[idx++] = node;
                node = node->left;
            } else {
                XTreeNode *child;

                if (!x_tree_nnodes_inc_checked(tree, !null_ret_ok)) {
                    return NULL;
                }

                child = x_tree_node_new(key, value);
                child->left = node->left;
                child->right = node;
                node->left = child;
                node->left_child = TRUE;
                node->balance -= 1;
                retnode = child;
                break;
            }
        } else {
            if (node->right_child) {
                path[idx++] = node;
                node = node->right;
            } else {
                XTreeNode *child;

                if (!x_tree_nnodes_inc_checked(tree, !null_ret_ok)) {
                    return NULL;
                }

                child = x_tree_node_new(key, value);

                child->right = node->right;
                child->left = node;
                node->right = child;
                node->right_child = TRUE;
                node->balance += 1;

                retnode = child;
                break;
            }
        }
    }

    while (1) {
        XTreeNode *bparent = path[--idx];
        xboolean left_node = (bparent && node == bparent->left);

        x_assert(!bparent || bparent->left == node || bparent->right == node);

        if (node->balance < -1 || node->balance > 1) {
            node = x_tree_node_balance(node);
            if (bparent == NULL) {
                tree->root = node;
            } else if (left_node) {
                bparent->left = node;
            } else {
                bparent->right = node;
            }
        }

        if (node->balance == 0 || bparent == NULL) {
            break;
        }

        if (left_node) {
            bparent->balance -= 1;
        } else {
            bparent->balance += 1;
        }

        node = bparent;
    }

    return retnode;
}

xboolean x_tree_remove(XTree *tree, xconstpointer key)
{
    xboolean removed;

    x_return_val_if_fail(tree != NULL, FALSE);

    removed = x_tree_remove_internal(tree, key, FALSE);
    return removed;
}

xboolean x_tree_steal(XTree *tree, xconstpointer key)
{
    xboolean removed;

    x_return_val_if_fail(tree != NULL, FALSE);

    removed = x_tree_remove_internal(tree, key, TRUE);
    return removed;
}

static xboolean x_tree_remove_internal(XTree *tree, xconstpointer  key, xboolean steal)
{
    int idx;
    xboolean left_node;
    XTreeNode *path[MAX_GTREE_HEIGHT];
    XTreeNode *node, *parent, *balance;

    x_return_val_if_fail(tree != NULL, FALSE);

    if (!tree->root) {
        return FALSE;
    }

    idx = 0;
    path[idx++] = NULL;
    node = tree->root;

    while (1) {
        int cmp = tree->key_compare(key, node->key, tree->key_compare_data);
        if (cmp == 0) {
            break;
        } else if (cmp < 0) {
            if (!node->left_child) {
                return FALSE;
            }

            path[idx++] = node;
            node = node->left;
        } else {
            if (!node->right_child) {
                return FALSE;
            }

            path[idx++] = node;
            node = node->right;
        }
    }

    balance = parent = path[--idx];
    x_assert(!parent || parent->left == node || parent->right == node);
    left_node = (parent && node == parent->left);

    if (!node->left_child) {
        if (!node->right_child) {
            if (!parent) {
                tree->root = NULL;
            } else if (left_node) {
                parent->left_child = FALSE;
                parent->left = node->left;
                parent->balance += 1;
            } else {
                parent->right_child = FALSE;
                parent->right = node->right;
                parent->balance -= 1;
            }
        } else {
            XTreeNode *tmp = x_tree_node_next(node);
            tmp->left = node->left;

            if (!parent) {
                tree->root = node->right;
            } else if (left_node) {
                parent->left = node->right;
                parent->balance += 1;
            } else {
                parent->right = node->right;
                parent->balance -= 1;
            }
        }
    } else {
        if (!node->right_child) {
            XTreeNode *tmp = x_tree_node_previous(node);
            tmp->right = node->right;

            if (parent == NULL) {
                tree->root = node->left;
            } else if (left_node) {
                parent->left = node->left;
                parent->balance += 1;
            } else {
                parent->right = node->left;
                parent->balance -= 1;
            }
        } else {
            XTreeNode *prev = node->left;
            XTreeNode *next = node->right;
            XTreeNode *nextp = node;
            int old_idx = idx + 1;
            idx++;

            while (next->left_child) {
                path[++idx] = nextp = next;
                next = next->left;
            }

            path[old_idx] = next;
            balance = path[idx];

            if (nextp != node) {
                if (next->right_child) {
                    nextp->left = next->right;
                } else {
                    nextp->left_child = FALSE;
                }
                nextp->balance += 1;

                next->right_child = TRUE;
                next->right = node->right;
            } else {
                node->balance -= 1;
            }

            while (prev->right_child) {
                prev = prev->right;
            }
            prev->right = next;

            next->left_child = TRUE;
            next->left = node->left;
            next->balance = node->balance;

            if (!parent) {
                tree->root = next;
            } else if (left_node) {
                parent->left = next;
            } else {
                parent->right = next;
            }
        }
    }

    if (balance) {
        while (1) {
            XTreeNode *bparent = path[--idx];
            x_assert(!bparent || bparent->left == balance || bparent->right == balance);
            left_node = (bparent && balance == bparent->left);

            if(balance->balance < -1 || balance->balance > 1) {
                balance = x_tree_node_balance(balance);
                if (!bparent) {
                    tree->root = balance;
                } else if (left_node) {
                    bparent->left = balance;
                } else {
                    bparent->right = balance;
                }
            }

            if (balance->balance != 0 || !bparent) {
                break;
            }

            if (left_node) {
                bparent->balance += 1;
            } else {
                bparent->balance -= 1;
            }

            balance = bparent;
        }
    }

    if (!steal) {
        if (tree->key_destroy_func) {
            tree->key_destroy_func(node->key);
        }

        if (tree->value_destroy_func) {
            tree->value_destroy_func(node->value);
        }
    }

    x_slice_free(XTreeNode, node);
    tree->nnodes--;

    return TRUE;
}

xpointer x_tree_node_key(XTreeNode *node)
{
    x_return_val_if_fail(node != NULL, NULL);
    return node->key;
}

xpointer x_tree_node_value(XTreeNode *node)
{
    x_return_val_if_fail(node != NULL, NULL);
    return node->value;
}

XTreeNode *x_tree_lookup_node(XTree *tree, xconstpointer key)
{
    x_return_val_if_fail(tree != NULL, NULL);
    return x_tree_find_node(tree, key);
}

xpointer x_tree_lookup(XTree *tree, xconstpointer key)
{
    XTreeNode *node;

    node = x_tree_lookup_node(tree, key);
    return node ? node->value : NULL;
}

xboolean x_tree_lookup_extended(XTree *tree, xconstpointer lookup_key, xpointer *orig_key, xpointer *value)
{
    XTreeNode *node;

    x_return_val_if_fail(tree != NULL, FALSE);

    node = x_tree_find_node(tree, lookup_key);
    if (node) {
        if (orig_key) {
            *orig_key = node->key;
        }

        if (value) {
            *value = node->value;
        }

        return TRUE;
    } else {
        return FALSE;
    }
}

void x_tree_foreach(XTree *tree, XTraverseFunc func, xpointer user_data)
{
    XTreeNode *node;

    x_return_if_fail(tree != NULL);

    if (!tree->root) {
        return;
    }

    node = x_tree_node_first(tree);
    while (node) {
        if ((*func)(node->key, node->value, user_data)) {
            break;
        }

        node = x_tree_node_next(node);
    }
}

void x_tree_foreach_node(XTree *tree, XTraverseNodeFunc func, xpointer user_data)
{
    XTreeNode *node;

    x_return_if_fail(tree != NULL);

    if (!tree->root) {
        return;
    }

    node = x_tree_node_first(tree);
    while (node) {
        if ((*func)(node, user_data)) {
            break;
        }

        node = x_tree_node_next(node);
    }
}

void x_tree_traverse(XTree *tree, XTraverseFunc traverse_func, XTraverseType traverse_type, xpointer user_data)
{
    x_return_if_fail(tree != NULL);

    if (!tree->root) {
        return;
    }

    switch (traverse_type) {
        case X_PRE_ORDER:
            x_tree_node_pre_order(tree->root, traverse_func, user_data);
        break;

        case X_IN_ORDER:
            x_tree_node_in_order(tree->root, traverse_func, user_data);
            break;

        case X_POST_ORDER:
            x_tree_node_post_order(tree->root, traverse_func, user_data);
            break;

        case X_LEVEL_ORDER:
            x_warning("x_tree_traverse(): traverse type X_LEVEL_ORDER isn't implemented.");
            break;
    }
}

XTreeNode *x_tree_search_node(XTree *tree, XCompareFunc search_func, xconstpointer user_data)
{
    x_return_val_if_fail(tree != NULL, NULL);

    if (!tree->root) {
        return NULL;
    }

    return x_tree_node_search(tree->root, search_func, user_data);
}

xpointer x_tree_search(XTree *tree, XCompareFunc search_func, xconstpointer user_data)
{
    XTreeNode *node;

    node = x_tree_search_node(tree, search_func, user_data);
    return node ? node->value : NULL;
}

XTreeNode *x_tree_lower_bound(XTree *tree, xconstpointer key)
{
    xint cmp;
    XTreeNode *node, *result;

    x_return_val_if_fail(tree != NULL, NULL);

    node = tree->root;
    if (!node) {
        return NULL;
    }

    result = NULL;
    while (1) {
        cmp = tree->key_compare(key, node->key, tree->key_compare_data);
        if (cmp <= 0) {
            result = node;
            if (!node->left_child) {
                return result;
            }

            node = node->left;
        } else {
            if (!node->right_child) {
                return result;
            }

            node = node->right;
        }
    }
}

XTreeNode *x_tree_upper_bound(XTree *tree, xconstpointer key)
{
    xint cmp;
    XTreeNode *node, *result;

    x_return_val_if_fail(tree != NULL, NULL);

    node = tree->root;
    if (!node) {
        return NULL;
    }

    result = NULL;
    while (1) {
        cmp = tree->key_compare(key, node->key, tree->key_compare_data);
        if (cmp < 0) {
            result = node;
            if (!node->left_child) {
                return result;
            }

            node = node->left;
        } else {
            if (!node->right_child) {
                return result;
            }

            node = node->right;
        }
    }
}

xint x_tree_height(XTree *tree)
{
    xint height;
    XTreeNode *node;

    x_return_val_if_fail(tree != NULL, 0);

    if (!tree->root) {
        return 0;
    }

    height = 0;
    node = tree->root;

    while (1) {
        height += 1 + MAX(node->balance, 0);
        if (!node->left_child) {
            return height;
        }

        node = node->left;
    }
}

xint x_tree_nnodes(XTree *tree)
{
    x_return_val_if_fail(tree != NULL, 0);
    return tree->nnodes;
}

static XTreeNode *x_tree_node_balance(XTreeNode *node)
{
    if (node->balance < -1) {
        if (node->left->balance > 0) {
            node->left = x_tree_node_rotate_left(node->left);
        }

        node = x_tree_node_rotate_right(node);
    } else if (node->balance > 1) {
        if (node->right->balance < 0) {
            node->right = x_tree_node_rotate_right(node->right);
        }

        node = x_tree_node_rotate_left(node);
    }

    return node;
}

static XTreeNode *x_tree_find_node(XTree *tree, xconstpointer key)
{
    xint cmp;
    XTreeNode *node;

    node = tree->root;
    if (!node) {
        return NULL;
    }

    while (1) {
        cmp = tree->key_compare (key, node->key, tree->key_compare_data);
        if (cmp == 0) {
            return node;
        } else if (cmp < 0) {
            if (!node->left_child) {
                return NULL;
            }

            node = node->left;
        } else {
            if (!node->right_child) {
                return NULL;
            }

            node = node->right;
        }
    }
}

static xint x_tree_node_pre_order(XTreeNode *node, XTraverseFunc traverse_func, xpointer data)
{
    if ((*traverse_func)(node->key, node->value, data)) {
        return TRUE;
    }

    if (node->left_child) {
        if (x_tree_node_pre_order(node->left, traverse_func, data)) {
            return TRUE;
        }
    }

    if (node->right_child) {
        if (x_tree_node_pre_order(node->right, traverse_func, data)) {
            return TRUE;
        }
    }

    return FALSE;
}

static xint x_tree_node_in_order(XTreeNode *node, XTraverseFunc traverse_func, xpointer data)
{
    if (node->left_child) {
        if (x_tree_node_in_order(node->left, traverse_func, data)) {
            return TRUE;
        }
    }

    if ((*traverse_func)(node->key, node->value, data)) {
        return TRUE;
    }

    if (node->right_child) {
        if (x_tree_node_in_order(node->right, traverse_func, data)) {
            return TRUE;
        }
    }

    return FALSE;
}

static xint x_tree_node_post_order(XTreeNode *node, XTraverseFunc traverse_func, xpointer data)
{
    if (node->left_child) {
        if (x_tree_node_post_order(node->left, traverse_func, data)) {
            return TRUE;
        }
    }

    if (node->right_child) {
        if (x_tree_node_post_order(node->right, traverse_func, data)) {
            return TRUE;
        }
    }

    if ((*traverse_func)(node->key, node->value, data)) {
        return TRUE;
    }

    return FALSE;
}

static XTreeNode *x_tree_node_search(XTreeNode *node, XCompareFunc search_func, xconstpointer data)
{
    xint dir;

    if (!node) {
        return NULL;
    }

    while (1) {
        dir = (*search_func)(node->key, data);
        if (dir == 0) {
            return node;
        } else if (dir < 0)  {
            if (!node->left_child) {
                return NULL;
            }

            node = node->left;
        } else {
            if (!node->right_child) {
                return NULL;
            }

            node = node->right;
        }
    }
}

static XTreeNode *x_tree_node_rotate_left(XTreeNode *node)
{
    xint a_bal;
    xint b_bal;
    XTreeNode *right;

    right = node->right;
    if (right->left_child) {
        node->right = right->left;
    } else {
        node->right_child = FALSE;
        right->left_child = TRUE;
    }
    right->left = node;

    a_bal = node->balance;
    b_bal = right->balance;

    if (b_bal <= 0) {
        if (a_bal >= 1) {
            right->balance = b_bal - 1;
        } else {
            right->balance = a_bal + b_bal - 2;
        }

        node->balance = a_bal - 1;
    } else {
        if (a_bal <= b_bal) {
            right->balance = a_bal - 2;
        } else {
            right->balance = b_bal - 1;
        }

        node->balance = a_bal - b_bal - 1;
    }

    return right;
}

static XTreeNode *x_tree_node_rotate_right(XTreeNode *node)
{
    xint a_bal;
    xint b_bal;
    XTreeNode *left;

    left = node->left;

    if (left->right_child) {
        node->left = left->right;
    } else {
        node->left_child = FALSE;
        left->right_child = TRUE;
    }
    left->right = node;

    a_bal = node->balance;
    b_bal = left->balance;

    if (b_bal <= 0) {
        if (b_bal > a_bal) {
            left->balance = b_bal + 1;
        } else {
            left->balance = a_bal + 2;
        }

        node->balance = a_bal - b_bal + 1;
    } else {
        if (a_bal <= -1) {
            left->balance = b_bal + 1;
        } else {
            left->balance = a_bal + b_bal + 2;
        }

        node->balance = a_bal + 1;
    }

    return left;
}
