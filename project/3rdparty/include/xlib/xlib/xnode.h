#ifndef __X_NODE_H__
#define __X_NODE_H__

#include "xmem.h"

X_BEGIN_DECLS

typedef struct _XNode XNode;

typedef enum {
    X_TRAVERSE_LEAVES     = 1 << 0,
    X_TRAVERSE_NON_LEAVES = 1 << 1,
    X_TRAVERSE_ALL        = X_TRAVERSE_LEAVES | X_TRAVERSE_NON_LEAVES,
    X_TRAVERSE_MASK       = 0x03,
    X_TRAVERSE_LEAFS      = X_TRAVERSE_LEAVES,
    X_TRAVERSE_NON_LEAFS  = X_TRAVERSE_NON_LEAVES
} XTraverseFlags;

typedef enum {
    X_IN_ORDER,
    X_PRE_ORDER,
    X_POST_ORDER,
    X_LEVEL_ORDER
} XTraverseType;

typedef void (*XNodeForeachFunc)(XNode *node, xpointer data);
typedef xboolean (*XNodeTraverseFunc)(XNode *node, xpointer data);

struct _XNode {
    xpointer data;
    XNode    *next;
    XNode    *prev;
    XNode    *parent;
    XNode    *children;
};

#define X_NODE_IS_ROOT(node)                (((XNode *)(node))->parent == NULL && ((XNode *)(node))->prev == NULL && ((XNode *)(node))->next == NULL)
#define X_NODE_IS_LEAF(node)                (((XNode *)(node))->children == NULL)

XLIB_AVAILABLE_IN_ALL
XNode *x_node_new(xpointer data);

XLIB_AVAILABLE_IN_ALL
void x_node_destroy(XNode *root);

XLIB_AVAILABLE_IN_ALL
void x_node_unlink(XNode *node);

XLIB_AVAILABLE_IN_ALL
XNode *x_node_copy_deep(XNode *node, XCopyFunc copy_func, xpointer data);

XLIB_AVAILABLE_IN_ALL
XNode *x_node_copy(XNode *node);

XLIB_AVAILABLE_IN_ALL
XNode *x_node_insert(XNode *parent, xint position, XNode *node);

XLIB_AVAILABLE_IN_ALL
XNode *x_node_insert_before(XNode *parent, XNode *sibling, XNode *node);

XLIB_AVAILABLE_IN_ALL
XNode *x_node_insert_after(XNode *parent, XNode *sibling, XNode *node);

XLIB_AVAILABLE_IN_ALL
XNode *x_node_prepend(XNode *parent, XNode *node);

XLIB_AVAILABLE_IN_ALL
xuint x_node_n_nodes(XNode *root, XTraverseFlags flags);

XLIB_AVAILABLE_IN_ALL
XNode *x_node_get_root(XNode *node);

XLIB_AVAILABLE_IN_ALL
xboolean x_node_is_ancestor(XNode *node, XNode *descendant);

XLIB_AVAILABLE_IN_ALL
xuint x_node_depth(XNode *node);

XLIB_AVAILABLE_IN_ALL
XNode *x_node_find(XNode *root, XTraverseType order, XTraverseFlags flags, xpointer data);

#define x_node_append(parent, node)                         x_node_insert_before((parent), NULL, (node))
#define x_node_insert_data(parent, position, data)          x_node_insert((parent), (position), x_node_new(data))
#define x_node_insert_data_after(parent, sibling, data)     x_node_insert_after((parent), (sibling), x_node_new(data))
#define x_node_insert_data_before(parent, sibling, data)    x_node_insert_before((parent), (sibling), z_node_new(data))
#define x_node_prepend_data(parent, data)                   x_node_prepend ((parent), x_node_new(data))
#define x_node_append_data(parent, data)                    x_node_insert_before((parent), NULL, x_node_new(data))

XLIB_AVAILABLE_IN_ALL
void x_node_traverse(XNode *root, XTraverseType order, XTraverseFlags flags, xint max_depth, XNodeTraverseFunc func, xpointer data);

XLIB_AVAILABLE_IN_ALL
xuint x_node_max_height(XNode *root);

XLIB_AVAILABLE_IN_ALL
void x_node_children_foreach(XNode *node, XTraverseFlags flags, XNodeForeachFunc func, xpointer data);

XLIB_AVAILABLE_IN_ALL
void x_node_reverse_children(XNode *node);

XLIB_AVAILABLE_IN_ALL
xuint x_node_n_children(XNode *node);

XLIB_AVAILABLE_IN_ALL
XNode *x_node_nth_child(XNode *node, xuint n);

XLIB_AVAILABLE_IN_ALL
XNode *x_node_last_child(XNode *node);

XLIB_AVAILABLE_IN_ALL
XNode *x_node_find_child(XNode *node, XTraverseFlags flags, xpointer data);

XLIB_AVAILABLE_IN_ALL
xint x_node_child_position(XNode *node, XNode *child);

XLIB_AVAILABLE_IN_ALL
xint x_node_child_index(XNode *node, xpointer data);

XLIB_AVAILABLE_IN_ALL
XNode *x_node_first_sibling(XNode *node);

XLIB_AVAILABLE_IN_ALL
XNode *x_node_last_sibling(XNode *node);

#define x_node_prev_sibling(node)           ((node) ? ((XNode *)(node))->prev : NULL)
#define x_node_next_sibling(node)           ((node) ? ((XNode *)(node))->next : NULL)
#define x_node_first_child(node)            ((node) ? ((XNode *)(node))->children : NULL)

X_END_DECLS

#endif
