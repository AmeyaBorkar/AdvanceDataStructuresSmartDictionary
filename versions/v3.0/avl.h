/* avl.h - AVL Tree (self-balancing BST) node and operation signatures */
#ifndef AVL_H
#define AVL_H

#include "dictionary.h"

/*
 * AVLNode - a node in the self-balancing AVL tree.
 *
 * Extends BST with a 'height' field. Balance factor = height(left) - height(right);
 * AVL invariant keeps this in {-1, 0, +1} via rotations on insert/delete.
 *
 * Storing height (not balance factor directly) makes rotation updates O(1):
 * after a rotation, recalculate height from children without extra traversal.
 *
 * IMPORTANT: avl_insert returns the new subtree root (unlike bst_insert which
 * uses a double pointer). Callers must capture the return value:
 *   g_avl_root = avl_insert(g_avl_root, &rec);
 */
typedef struct AVLNode {
    WordRecord       data;    /* embedded record payload      */
    struct AVLNode  *left;    /* left child                   */
    struct AVLNode  *right;   /* right child                  */
    int              height;  /* height of this node (leaf=1) */
} AVLNode;

/* Allocate and initialise a new AVL node with height = 1. Returns NULL on failure. */
AVLNode *avl_new_node(const WordRecord *rec);

/* Insert rec, rebalancing as needed. Returns new root of subtree. */
AVLNode *avl_insert(AVLNode *root, const WordRecord *rec);

/* Search for word. Returns pointer to matching node, or NULL. */
AVLNode *avl_search(AVLNode *root, const char *word);

/* Delete word, rebalancing as needed. Returns new root of subtree. */
AVLNode *avl_delete(AVLNode *root, const char *word);

/* In-order traversal: calls callback(node, arg) for each node. */
void avl_inorder(AVLNode *root, void (*callback)(AVLNode *, void *), void *arg);

/* Free all nodes recursively. Sets *root to NULL. */
void avl_free(AVLNode **root);

/* Return height of node (0 for NULL). */
int avl_height(AVLNode *node);

/* Return balance factor of node: height(left) - height(right). Safe for NULL (returns 0). */
int avl_balance_factor(AVLNode *node);

/* Return total number of nodes. */
int avl_count(AVLNode *root);

#endif /* AVL_H */
