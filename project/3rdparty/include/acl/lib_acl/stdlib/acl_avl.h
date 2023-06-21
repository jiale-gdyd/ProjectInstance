#ifndef ACL_LIBACL_STDLIB_ACL_AVL_H
#define ACL_LIBACL_STDLIB_ACL_AVL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "acl_define.h"
#include <sys/types.h>
#include "avl_impl.h"

/*
 * Type used for the root of the AVL tree.
 */
typedef struct avl_tree avl_tree_t;

/*
 * The data nodes in the AVL tree must have a field of this type.
 */
typedef struct avl_node avl_node_t;

/*
 * An opaque type used to locate a position in the tree where a node
 * would be inserted.
 */
typedef uintptr_t avl_index_t;

/*
 * Direction constants used for avl_nearest().
 */
#define AVL_BEFORE  (0)
#define AVL_AFTER   (1)

/*
 * Initialize an AVL tree. Arguments are:
 *
 * tree   - the tree to be initialized
 * compar - function to compare two nodes, it must return exactly: -1, 0, or +1
 *          -1 for <, 0 for ==, and +1 for >
 * size   - the value of sizeof(struct my_type)
 * offset - the value of OFFSETOF(struct my_type, my_link)
 */
ACL_API void avl_create(avl_tree_t *tree, int (*compar) (const void *, const void *), size_t size, size_t offset);

/*
 * Find a node with a matching value in the tree. Returns the matching node
 * found. If not found, it returns NULL and then if "where" is not NULL it sets
 * "where" for use with avl_insert() or avl_nearest().
 *
 * node   - node that has the value being looked for
 * where  - position for use with avl_nearest() or avl_insert(), may be NULL
 */
ACL_API void *avl_find(avl_tree_t *tree, void *node, avl_index_t *where);

/*
 * Insert a node into the tree.
 *
 * node   - the node to insert
 * where  - position as returned from avl_find()
 */
ACL_API void avl_insert(avl_tree_t *tree, void *node, avl_index_t where);

/*
 * Insert "new_data" in "tree" in the given "direction" either after
 * or before the data "here".
 *
 * This might be usefull for avl clients caching recently accessed
 * data to avoid doing avl_find() again for insertion.
 *
 * new_data	- new data to insert
 * here		- existing node in "tree"
 * direction	- either AVL_AFTER or AVL_BEFORE the data "here".
 */
ACL_API void avl_insert_here(avl_tree_t *tree, void *new_data, void *here, int direction);

/*
 * Return the first or last valued node in the tree. Will return NULL
 * if the tree is empty.
 *
 */
ACL_API void *avl_first(avl_tree_t *tree);
ACL_API void *avl_last(avl_tree_t *tree);

/*
 * Return the next or previous valued node in the tree.
 * AVL_NEXT() will return NULL if at the last node.
 * AVL_PREV() will return NULL if at the first node.
 *
 * node   - the node from which the next or previous node is found
 */
#define AVL_NEXT(tree, node)    avl_walk(tree, node, AVL_AFTER)
#define AVL_PREV(tree, node)    avl_walk(tree, node, AVL_BEFORE)

/*
 * Find the node with the nearest value either greater or less than
 * the value from a previous avl_find(). Returns the node or NULL if
 * there isn't a matching one.
 *
 * where     - position as returned from avl_find()
 * direction - either AVL_BEFORE or AVL_AFTER
 *
 * EXAMPLE get the greatest node that is less than a given value:
 *
 *  avl_tree_t *tree;
 *  struct my_data look_for_value = {....};
 *  struct my_data *node;
 *  struct my_data *less;
 *  avl_index_t where;
 *
 *  node = avl_find(tree, &look_for_value, &where);
 *  if (node != NULL)
 *      less = AVL_PREV(tree, node);
 *  else
 *      less = avl_nearest(tree, where, AVL_BEFORE);
 */
ACL_API void *avl_nearest(avl_tree_t *tree, avl_index_t where, int direction);

/*
 * Add a single node to the tree.
 * The node must not be in the tree, and it must not
 * compare equal to any other node already in the tree.
 *
 * node   - the node to add
 */
ACL_API void avl_add(avl_tree_t *tree, void *node);


/*
 * Remove a single node from the tree.  The node must be in the tree.
 *
 * node   - the node to remove
 */
ACL_API void avl_remove(avl_tree_t *tree, void *node);

/*
 * Reinsert a node only if its order has changed relative to its nearest
 * neighbors. To optimize performance avl_update_lt() checks only the previous
 * node and avl_update_gt() checks only the next node. Use avl_update_lt() and
 * avl_update_gt() only if you know the direction in which the order of the
 * node may change.
 */
ACL_API acl_boolean_t avl_update(avl_tree_t *, void *);
ACL_API acl_boolean_t avl_update_lt(avl_tree_t *, void *);
ACL_API acl_boolean_t avl_update_gt(avl_tree_t *, void *);

/*
 * Return the number of nodes in the tree
 */
ACL_API ulong_t avl_numnodes(avl_tree_t *tree);

/*
 * Return B_TRUE if there are zero nodes in the tree, B_FALSE otherwise.
 */
ACL_API acl_boolean_t avl_is_empty(avl_tree_t *tree);

/*
 * Used to destroy any remaining nodes in a tree. The cookie argument should
 * be initialized to NULL before the first call. Returns a node that has been
 * removed from the tree and may be free()'d. Returns NULL when the tree is
 * empty.
 *
 * Once you call avl_destroy_nodes(), you can only continuing calling it and
 * finally avl_destroy(). No other AVL routines will be valid.
 *
 * cookie - a "void *" used to save state between calls to avl_destroy_nodes()
 *
 * EXAMPLE:
 *  avl_tree_t *tree;
 *  struct my_data *node;
 *  void *cookie;
 *
 *  cookie = NULL;
 *  while ((node = avl_destroy_nodes(tree, &cookie)) != NULL)
 *      free(node);
 *  avl_destroy(tree);
 */
ACL_API void *avl_destroy_nodes(avl_tree_t *tree, void **cookie);

/*
 * Final destroy of an AVL tree. Arguments are:
 *
 * tree   - the empty tree to destroy
 */
ACL_API void avl_destroy(avl_tree_t *tree);

#ifdef __cplusplus
}
#endif

#endif
