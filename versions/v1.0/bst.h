/* bst.h - Binary Search Tree node and operation signatures */
#ifndef BST_H
#define BST_H

#include "dictionary.h"

typedef struct BSTNode {
    WordRecord       data;
    struct BSTNode  *left;
    struct BSTNode  *right;
} BSTNode;

BSTNode *bst_new_node(const WordRecord *rec);
void bst_insert(BSTNode **root, const WordRecord *rec);
BSTNode *bst_search(BSTNode *root, const char *word);
void bst_delete(BSTNode **root, const char *word);
void bst_inorder(BSTNode *root, void (*callback)(BSTNode *, void *), void *arg);
void bst_free(BSTNode **root);
int bst_height(BSTNode *root);
int bst_count(BSTNode *root);

#endif /* BST_H */
