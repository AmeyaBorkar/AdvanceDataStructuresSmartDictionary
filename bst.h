/* bst.h - Binary Search Tree node and operation signatures */
#ifndef BST_H
#define BST_H

#include "dictionary.h"

/*
 * BSTNode - a node in the unbalanced Binary Search Tree.
 *
 * Baseline implementation: O(log n) average, O(n) worst case (sorted input).
 * WordRecord is embedded directly — one malloc per node, one free per node.
 */
typedef struct BSTNode {
    WordRecord       data;   /* embedded record payload                  */
    struct BSTNode  *left;   /* left subtree  (word < this node's word)  */
    struct BSTNode  *right;  /* right subtree (word > this node's word)  */
} BSTNode;

/* Allocate and initialise a new BST node. Returns NULL on malloc failure. */
BSTNode *bst_new_node(const WordRecord *rec);

/* Insert rec into the tree rooted at *root. Updates *root when tree grows. */
void bst_insert(BSTNode **root, const WordRecord *rec);

/* Search for word. Returns pointer to matching node, or NULL if not found. */
BSTNode *bst_search(BSTNode *root, const char *word);

/* Delete word from the tree. Updates *root if root changes. */
void bst_delete(BSTNode **root, const char *word);

/* In-order traversal: calls callback(node, arg) for each node. */
void bst_inorder(BSTNode *root, void (*callback)(BSTNode *, void *), void *arg);

/* Pre-order traversal: visits root before left/right subtrees.
   Used for saving — re-loading in preorder recreates the same tree
   structure, preventing the sorted-input → skewed-tree performance bug. */
void bst_preorder(BSTNode *root, void (*callback)(BSTNode *, void *), void *arg);

/* Free all nodes recursively. Sets *root to NULL. */
void bst_free(BSTNode **root);

/* Return height of the tree (0 for empty tree). */
int bst_height(BSTNode *root);

/* Return total number of nodes. */
int bst_count(BSTNode *root);

#endif /* BST_H */
