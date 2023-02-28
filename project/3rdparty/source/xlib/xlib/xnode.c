#include <xlib/xlib/config.h>
#include <xlib/xlib/xnode.h>
#include <xlib/xlib/xslice.h>
#include <xlib/xlib/xtestutils.h>

#define x_node_alloc0()         x_slice_new0(XNode)
#define x_node_free(node)       x_slice_free(XNode, node)

XNode *x_node_new(xpointer data)
{
    XNode *node = x_node_alloc0();
    node->data = data;

    return node;
}

static void x_nodes_free(XNode *node)
{
    while (node) {
        XNode *next = node->next;
        if (node->children) {
            x_nodes_free(node->children);
        }

        x_node_free(node);
        node = next;
    }
}

void x_node_destroy(XNode *root)
{
    x_return_if_fail(root != NULL);

    if (!X_NODE_IS_ROOT(root)) {
        x_node_unlink(root);
    }

    x_nodes_free(root);
}

void x_node_unlink(XNode *node)
{
    x_return_if_fail (node != NULL);

    if (node->prev) {
        node->prev->next = node->next;
    } else if (node->parent) {
        node->parent->children = node->next;
    }

    node->parent = NULL;
    if (node->next) {
        node->next->prev = node->prev;
        node->next = NULL;
    }
    node->prev = NULL;
}

XNode *x_node_copy_deep(XNode *node, XCopyFunc copy_func, xpointer data)
{
    XNode *new_node = NULL;

    if (copy_func == NULL) {
        return x_node_copy(node);
    }

    if (node) {
        XNode *child, *new_child;

        new_node = x_node_new(copy_func (node->data, data));
        for (child = x_node_last_child(node); child; child = child->prev)  {
            new_child = x_node_copy_deep(child, copy_func, data);
            x_node_prepend(new_node, new_child);
        }
    }

    return new_node;
}

XNode *x_node_copy(XNode *node)
{
    XNode *new_node = NULL;

    if (node) {
        XNode *child;

        new_node = x_node_new(node->data);
        for (child = x_node_last_child(node); child; child = child->prev) {
            x_node_prepend(new_node, x_node_copy(child));
        }
    }

    return new_node;
}

XNode *x_node_insert(XNode *parent, xint position, XNode *node)
{
    x_return_val_if_fail(parent != NULL, node);
    x_return_val_if_fail(node != NULL, node);
    x_return_val_if_fail(X_NODE_IS_ROOT(node), node);

    if (position > 0) {
        return x_node_insert_before(parent, x_node_nth_child(parent, position), node);
    } else if (position == 0) {
        return x_node_prepend(parent, node);
    } else  {
        return x_node_append(parent, node);
    }
}

XNode *x_node_insert_before(XNode *parent, XNode *sibling, XNode *node)
{
    x_return_val_if_fail(parent != NULL, node);
    x_return_val_if_fail(node != NULL, node);
    x_return_val_if_fail(X_NODE_IS_ROOT(node), node);

    if (sibling) {
        x_return_val_if_fail(sibling->parent == parent, node);
    }
    node->parent = parent;

    if (sibling) {
        if (sibling->prev) {
            node->prev = sibling->prev;
            node->prev->next = node;
            node->next = sibling;
            sibling->prev = node;
        } else {
            node->parent->children = node;
            node->next = sibling;
            sibling->prev = node;
        }
    } else {
        if (parent->children) {
            sibling = parent->children;
            while (sibling->next) {
                sibling = sibling->next;
            }

            node->prev = sibling;
            sibling->next = node;
        } else {
            node->parent->children = node;
        }
    }

    return node;
}

XNode *x_node_insert_after(XNode *parent, XNode *sibling, XNode *node)
{
    x_return_val_if_fail(parent != NULL, node);
    x_return_val_if_fail(node != NULL, node);
    x_return_val_if_fail(X_NODE_IS_ROOT(node), node);

    if (sibling) {
        x_return_val_if_fail(sibling->parent == parent, node);
    }
    node->parent = parent;

    if (sibling) {
        if (sibling->next) {
            sibling->next->prev = node;
        }

        node->next = sibling->next;
        node->prev = sibling;
        sibling->next = node;
    } else {
        if (parent->children) {
            node->next = parent->children;
            parent->children->prev = node;
        }

        parent->children = node;
    }

    return node;
}

XNode *x_node_prepend(XNode *parent, XNode *node)
{
    x_return_val_if_fail(parent != NULL, node);
    return x_node_insert_before(parent, parent->children, node);
}

XNode *x_node_get_root(XNode *node)
{
    x_return_val_if_fail(node != NULL, NULL);

    while (node->parent) {
        node = node->parent;
    }

    return node;
}

xboolean x_node_is_ancestor(XNode *node, XNode *descendant)
{
    x_return_val_if_fail(node != NULL, FALSE);
    x_return_val_if_fail(descendant != NULL, FALSE);

    while (descendant) {
        if (descendant->parent == node) {
            return TRUE;
        }

        descendant = descendant->parent;
    }

    return FALSE;
}

xuint x_node_depth(XNode *node)
{
    xuint depth = 0;

    while (node) {
        depth++;
        node = node->parent;
    }

    return depth;
}

void x_node_reverse_children(XNode *node)
{
    XNode *last;
    XNode *child;

    x_return_if_fail(node != NULL);

    child = node->children;
    last = NULL;

    while (child) {
        last = child;
        child = last->next;
        last->next = last->prev;
        last->prev = child;
    }

    node->children = last;
}

xuint x_node_max_height(XNode *root)
{
    XNode *child;
    xuint max_height = 0;
    
    if (!root) {
        return 0;
    }

    child = root->children;
    while (child) {
        xuint tmp_height;

        tmp_height = x_node_max_height(child);
        if (tmp_height > max_height) {
            max_height = tmp_height;
        }
        child = child->next;
    }

    return max_height + 1;
}

static xboolean x_node_traverse_pre_order(XNode *node, XTraverseFlags flags, XNodeTraverseFunc func, xpointer data)
{
    if (node->children) {
        XNode *child;

        if ((flags & X_TRAVERSE_NON_LEAFS) && func(node, data)) {
            return TRUE;
        }

        child = node->children;
        while (child) {
            XNode *current;

            current = child;
            child = current->next;
            if (x_node_traverse_pre_order(current, flags, func, data)) {
                return TRUE;
            }
        }
    } else if ((flags & X_TRAVERSE_LEAFS) && func(node, data)) {
        return TRUE;
    }

    return FALSE;
}

static xboolean x_node_depth_traverse_pre_order(XNode *node, XTraverseFlags flags, xuint depth, XNodeTraverseFunc func, xpointer data)
{
    if (node->children) {
        XNode *child;

        if ((flags & X_TRAVERSE_NON_LEAFS) && func(node, data)) {
            return TRUE;
        }

        depth--;
        if (!depth) {
            return FALSE;
        }

        child = node->children;
        while (child) {
            XNode *current;

            current = child;
            child = current->next;
            if (x_node_depth_traverse_pre_order(current, flags, depth, func, data)) {
                return TRUE;
            }
        }
    } else if ((flags & X_TRAVERSE_LEAFS) && func(node, data)) {
        return TRUE;
    }

    return FALSE;
}

static xboolean x_node_traverse_post_order(XNode *node, XTraverseFlags flags, XNodeTraverseFunc func, xpointer data)
{
    if (node->children) {
        XNode *child;

        child = node->children;
        while (child) {
            XNode *current;

            current = child;
            child = current->next;
            if (x_node_traverse_post_order(current, flags, func, data)) {
                return TRUE;
            }
        }

        if ((flags & X_TRAVERSE_NON_LEAFS) && func(node, data)) {
            return TRUE;
        }
    } else if ((flags & X_TRAVERSE_LEAFS) && func(node, data)) {
        return TRUE;
    }

    return FALSE;
}

static xboolean x_node_depth_traverse_post_order(XNode *node, XTraverseFlags flags, xuint depth, XNodeTraverseFunc func, xpointer data)
{
    if (node->children) {
        depth--;
        if (depth) {
            XNode *child;
        
            child = node->children;
            while (child) {
                XNode *current;

                current = child;
                child = current->next;
                if (x_node_depth_traverse_post_order(current, flags, depth, func, data)) {
                    return TRUE;
                }
            }
        }

        if ((flags & X_TRAVERSE_NON_LEAFS) && func(node, data)) {
            return TRUE;
        }
    } else if ((flags & X_TRAVERSE_LEAFS) && func(node, data)) {
        return TRUE;
    }

    return FALSE;
}

static xboolean x_node_traverse_in_order(XNode *node, XTraverseFlags flags, XNodeTraverseFunc func, xpointer data)
{
    if (node->children) {
        XNode *child;
        XNode *current;

        child = node->children;
        current = child;
        child = current->next;

        if (x_node_traverse_in_order(current, flags, func, data)) {
            return TRUE;
        }

        if ((flags & X_TRAVERSE_NON_LEAFS) && func(node, data)) {
            return TRUE;
        }

        while (child) {
            current = child;
            child = current->next;
            if (x_node_traverse_in_order(current, flags, func, data)) {
                return TRUE;
            }
        }
    } else if ((flags & X_TRAVERSE_LEAFS) && func(node, data)) {
        return TRUE;
    }

    return FALSE;
}

static xboolean x_node_depth_traverse_in_order(XNode *node, XTraverseFlags flags, xuint depth, XNodeTraverseFunc func, xpointer data)
{
    if (node->children) {
        depth--;
        if (depth) {
            XNode *child;
            XNode *current;

            child = node->children;
            current = child;
            child = current->next;

            if (x_node_depth_traverse_in_order(current, flags, depth, func, data)) {
                return TRUE;
            }

            if ((flags & X_TRAVERSE_NON_LEAFS) && func(node, data)) {
                return TRUE;
            }

            while (child) {
                current = child;
                child = current->next;
                if (x_node_depth_traverse_in_order(current, flags, depth, func, data)) {
                    return TRUE;
                }
            }
        } else if ((flags & X_TRAVERSE_NON_LEAFS) && func(node, data)) {
            return TRUE;
        }
    }  else if ((flags & X_TRAVERSE_LEAFS) && func(node, data)) {
        return TRUE;
    }

    return FALSE;
}

static xboolean x_node_traverse_level(XNode *node, XTraverseFlags flags, xuint level, XNodeTraverseFunc func, xpointer data, xboolean *more_levels)
{
    if (level == 0)  {
        if (node->children) {
            *more_levels = TRUE;
            return (flags & X_TRAVERSE_NON_LEAFS) && func (node, data);
        } else {
            return (flags & X_TRAVERSE_LEAFS) && func (node, data);
        }
    } else  {
        node = node->children;

        while (node) {
            if (x_node_traverse_level(node, flags, level - 1, func, data, more_levels)) {
                return TRUE;
            }

            node = node->next;
        }
    }

    return FALSE;
}

static xboolean x_node_depth_traverse_level(XNode *node, XTraverseFlags flags, xint depth, XNodeTraverseFunc func, xpointer data)
{
    xuint level;
    xboolean more_levels;

    level = 0;
    while ((depth < 0) || (level != (xuint)depth)) {
        more_levels = FALSE;
        if (x_node_traverse_level(node, flags, level, func, data, &more_levels)) {
            return TRUE;
        }

        if (!more_levels) {
            break;
        }

        level++;
    }

    return FALSE;
}

void x_node_traverse(XNode *root, XTraverseType order, XTraverseFlags flags, xint depth, XNodeTraverseFunc func, xpointer data)
{
    x_return_if_fail(root != NULL);
    x_return_if_fail(func != NULL);
    x_return_if_fail(order <= X_LEVEL_ORDER);
    x_return_if_fail(flags <= X_TRAVERSE_MASK);
    x_return_if_fail((depth == -1) || (depth > 0));

    switch (order) {
        case X_PRE_ORDER:
            if (depth < 0) {
                x_node_traverse_pre_order(root, flags, func, data);
            } else {
                x_node_depth_traverse_pre_order(root, flags, depth, func, data);
            }
            break;

        case X_POST_ORDER:
            if (depth < 0) {
                x_node_traverse_post_order(root, flags, func, data);
            } else {
                x_node_depth_traverse_post_order(root, flags, depth, func, data);
            }
            break;

        case X_IN_ORDER:
            if (depth < 0) {
                x_node_traverse_in_order(root, flags, func, data);
            } else {
                x_node_depth_traverse_in_order(root, flags, depth, func, data);
            }
            break;

        case X_LEVEL_ORDER:
            x_node_depth_traverse_level(root, flags, depth, func, data);
            break;
    }
}

static xboolean x_node_find_func(XNode *node, xpointer data)
{
    xpointer *d = (xpointer *)data;

    if (*d != node->data) {
        return FALSE;
    }
    *(++d) = node;

    return TRUE;
}

XNode *x_node_find(XNode *root, XTraverseType order, XTraverseFlags flags, xpointer data)
{
    xpointer d[2];

    x_return_val_if_fail(root != NULL, NULL);
    x_return_val_if_fail(order <= X_LEVEL_ORDER, NULL);
    x_return_val_if_fail(flags <= X_TRAVERSE_MASK, NULL);

    d[0] = data;
    d[1] = NULL;
    x_node_traverse(root, order, flags, -1, x_node_find_func, d);

    return (XNode *)d[1];
}

static void x_node_count_func(XNode *node, XTraverseFlags flags, xuint *n)
{
    if (node->children) {
        XNode *child;

        if (flags & X_TRAVERSE_NON_LEAFS) {
            (*n)++;
        }

        child = node->children;
        while (child) {
            x_node_count_func(child, flags, n);
            child = child->next;
        }
    } else if (flags & X_TRAVERSE_LEAFS) {
        (*n)++;
    }
}

xuint x_node_n_nodes(XNode *root, XTraverseFlags flags)
{
    xuint n = 0;

    x_return_val_if_fail(root != NULL, 0);
    x_return_val_if_fail(flags <= X_TRAVERSE_MASK, 0);
    x_node_count_func(root, flags, &n);

    return n;
}

XNode *x_node_last_child(XNode *node)
{
    x_return_val_if_fail(node != NULL, NULL);

    node = node->children;
    if (node) {
        while (node->next) {
            node = node->next;
        }
    }

    return node;
}

XNode *x_node_nth_child(XNode *node, xuint n)
{
    x_return_val_if_fail(node != NULL, NULL);

    node = node->children;
    if (node) {
        while ((n-- > 0) && node) {
            node = node->next;
        }
    }

    return node;
}

xuint x_node_n_children(XNode *node)
{
    xuint n = 0;

    x_return_val_if_fail(node != NULL, 0);

    node = node->children;
    while (node) {
        n++;
        node = node->next;
    }

    return n;
}

XNode *x_node_find_child(XNode *node, XTraverseFlags flags, xpointer data)
{
    x_return_val_if_fail(node != NULL, NULL);
    x_return_val_if_fail(flags <= X_TRAVERSE_MASK, NULL);

    node = node->children;
    while (node) {
        if (node->data == data) {
            if (X_NODE_IS_LEAF(node)) {
                if (flags & X_TRAVERSE_LEAFS) {
                    return node;
                }
            } else {
                if (flags & X_TRAVERSE_NON_LEAFS) {
                    return node;
                }
            }
        }

        node = node->next;
    }

    return NULL;
}

xint x_node_child_position(XNode *node, XNode *child)
{
    xuint n = 0;

    x_return_val_if_fail(node != NULL, -1);
    x_return_val_if_fail(child != NULL, -1);
    x_return_val_if_fail(child->parent == node, -1);

    node = node->children;
    while (node) {
        if (node == child) {
            return n;
        }

        n++;
        node = node->next;
    }

    return -1;
}

xint x_node_child_index(XNode *node, xpointer data)
{
    xuint n = 0;

    x_return_val_if_fail(node != NULL, -1);

    node = node->children;
    while (node) {
        if (node->data == data) {
            return n;
        }

        n++;
        node = node->next;
    }

    return -1;
}

XNode *x_node_first_sibling(XNode *node)
{
    x_return_val_if_fail(node != NULL, NULL);

    if (node->parent) {
        return node->parent->children;
    }

    while (node->prev) {
        node = node->prev;
    }

    return node;
}

XNode *x_node_last_sibling(XNode *node)
{
    x_return_val_if_fail(node != NULL, NULL);

    while (node->next) {
        node = node->next;
    }

    return node;
}

void x_node_children_foreach(XNode *node, XTraverseFlags flags, XNodeForeachFunc func, xpointer data)
{
    x_return_if_fail(node != NULL);
    x_return_if_fail(flags <= X_TRAVERSE_MASK);
    x_return_if_fail(func != NULL);

    node = node->children;
    while (node) {
        XNode *current;

        current = node;
        node = current->next;
        if (X_NODE_IS_LEAF(current)) {
            if (flags & X_TRAVERSE_LEAFS) {
                func(current, data);
            }
        } else {
            if (flags & X_TRAVERSE_NON_LEAFS) {
                func(current, data);
            }
        }
    }
}
