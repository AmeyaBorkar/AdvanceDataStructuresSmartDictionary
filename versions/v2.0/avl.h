/* avl.h - AVL Tree (self-balancing BST) node and operation signatures */
#ifndef AVL_H
#define AVL_H

#include "dictionary.h"

typedef struct AVLNode {
    WordRecord       data;
    struct AVLNode  *left;
    struct AVLNode  *right;
    int              height;
} AVLNode;

AVLNode *avl_new_node(const WordRecord *rec);
AVLNode *avl_insert(AVLNode *root, const WordRecord *rec);
AVLNode *avl_search(AVLNode *root, const char *word);
AVLNode *avl_delete(AVLNode *root, const char *word);
void avl_inorder(AVLNode *root, void (*callback)(AVLNode *, void *), void *arg);
void avl_free(AVLNode **root);
int avl_height(AVLNode *node);
int avl_balance_factor(AVLNode *node);
int avl_count(AVLNode *root);

#endif /* AVL_H */
