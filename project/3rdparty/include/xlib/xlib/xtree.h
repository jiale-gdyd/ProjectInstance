#ifndef __X_TREE_H__
#define __X_TREE_H__

#include "xnode.h"

X_BEGIN_DECLS

typedef struct _XTree XTree;
typedef struct _XTreeNode XTreeNode;

typedef xboolean (*XTraverseNodeFunc)(XTreeNode *node, xpointer data);
typedef xboolean (*XTraverseFunc)(xpointer key, xpointer value, xpointer data);

XLIB_AVAILABLE_IN_ALL
XTree *x_tree_new(XCompareFunc key_compare_func);

XLIB_AVAILABLE_IN_ALL
XTree *x_tree_new_with_data(XCompareDataFunc key_compare_func, xpointer key_compare_data);

XLIB_AVAILABLE_IN_ALL
XTree *x_tree_new_full(XCompareDataFunc key_compare_func, xpointer key_compare_data, XDestroyNotify key_destroy_func, XDestroyNotify value_destroy_func);

XLIB_AVAILABLE_IN_2_68
XTreeNode *x_tree_node_first(XTree *tree);

XLIB_AVAILABLE_IN_2_68
XTreeNode *x_tree_node_last(XTree *tree);

XLIB_AVAILABLE_IN_2_68
XTreeNode *x_tree_node_previous(XTreeNode *node);

XLIB_AVAILABLE_IN_2_68
XTreeNode *x_tree_node_next(XTreeNode *node);

XLIB_AVAILABLE_IN_ALL
XTree *x_tree_ref(XTree *tree);

XLIB_AVAILABLE_IN_ALL
void x_tree_unref(XTree *tree);

XLIB_AVAILABLE_IN_ALL
void x_tree_destroy(XTree *tree);

XLIB_AVAILABLE_IN_2_68
XTreeNode *x_tree_insert_node(XTree *tree, xpointer key, xpointer value);

XLIB_AVAILABLE_IN_ALL
void x_tree_insert(XTree *tree, xpointer key, xpointer value);

XLIB_AVAILABLE_IN_2_68
XTreeNode *x_tree_replace_node(XTree *tree, xpointer key, xpointer value);

XLIB_AVAILABLE_IN_ALL
void x_tree_replace(XTree *tree, xpointer key, xpointer value);

XLIB_AVAILABLE_IN_ALL
xboolean x_tree_remove(XTree *tree, xconstpointer key);

XLIB_AVAILABLE_IN_2_70
void x_tree_remove_all(XTree *tree);

XLIB_AVAILABLE_IN_ALL
xboolean x_tree_steal(XTree *tree, xconstpointer key);

XLIB_AVAILABLE_IN_2_68
xpointer x_tree_node_key(XTreeNode *node);

XLIB_AVAILABLE_IN_2_68
xpointer x_tree_node_value(XTreeNode *node);

XLIB_AVAILABLE_IN_2_68
XTreeNode *x_tree_lookup_node(XTree *tree, xconstpointer key);

XLIB_AVAILABLE_IN_ALL
xpointer x_tree_lookup(XTree *tree, xconstpointer key);

XLIB_AVAILABLE_IN_ALL
xboolean x_tree_lookup_extended(XTree *tree, xconstpointer lookup_key, xpointer *orig_key, xpointer *value);

XLIB_AVAILABLE_IN_ALL
void x_tree_foreach(XTree *tree, XTraverseFunc func, xpointer user_data);

XLIB_AVAILABLE_IN_2_68
void x_tree_foreach_node(XTree *tree, XTraverseNodeFunc func, xpointer user_data);

XLIB_DEPRECATED
void x_tree_traverse(XTree *tree, XTraverseFunc traverse_func, XTraverseType traverse_type, xpointer user_data);

XLIB_AVAILABLE_IN_2_68
XTreeNode *x_tree_search_node(XTree *tree, XCompareFunc search_func, xconstpointer user_data);

XLIB_AVAILABLE_IN_ALL
xpointer x_tree_search(XTree *tree, XCompareFunc search_func, xconstpointer user_data);

XLIB_AVAILABLE_IN_2_68
XTreeNode *x_tree_lower_bound(XTree *tree, xconstpointer key);

XLIB_AVAILABLE_IN_2_68
XTreeNode *x_tree_upper_bound(XTree *tree, xconstpointer key);

XLIB_AVAILABLE_IN_ALL
xint x_tree_height(XTree *tree);

XLIB_AVAILABLE_IN_ALL
xint x_tree_nnodes(XTree *tree);

X_END_DECLS

#endif
