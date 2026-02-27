/* tbt.h - Threaded Binary Tree node and operation signatures */
#ifndef TBT_H
#define TBT_H

#include "dictionary.h"

/*
 * TBTNode - a node in the Right-Threaded Binary Tree (RTBT).
 *
 * In a standard BST, NULL left/right pointers are wasted. In a TBT, these
 * NULL slots are repurposed as "threads" pointing to the inorder predecessor
 * (left thread) or successor (right thread). This enables O(n) inorder
 * traversal WITHOUT a stack or recursion — the key advantage for autocomplete.
 *
 * Thread flag convention (Knuth Vol.1):
 *   lthread = 0  =>  left  points to a real left child
 *   lthread = 1  =>  left  points to the inorder predecessor (thread)
 *   rthread = 0  =>  right points to a real right child
 *   rthread = 1  =>  right points to the inorder successor (thread)
 *
 * int flags (not bitfields): avoids undefined behaviour from bitfield
 * sign/size ambiguity in C99 on MinGW.
 *
 * Header node pattern (Knuth):
 *   A special sentinel header node is used whose left pointer points to
 *   the tree root (lthread=0). Traversal starts by going to the leftmost
 *   real node. This eliminates all NULL checks during traversal and
 *   simplifies insertion into an empty tree.
 */
typedef struct TBTNode {
    WordRecord       data;     /* embedded record payload              */
    struct TBTNode  *left;     /* left child or inorder predecessor    */
    struct TBTNode  *right;    /* right child or inorder successor     */
    int              lthread;  /* 0 = real child, 1 = thread           */
    int              rthread;  /* 0 = real child, 1 = thread           */
} TBTNode;

/*
 * Allocate and return a header/sentinel node for a new TBT.
 * Initially: header->left = header (lthread=1), header->right = header (rthread=1).
 * Returns NULL on malloc failure.
 */
TBTNode *tbt_create_header(void);

/* Allocate a new TBT data node (both thread flags = 1, both pointers = NULL initially). */
TBTNode *tbt_new_node(const WordRecord *rec);

/* Insert rec into the TBT whose header is 'header'. Updates header->left if tree was empty. */
void tbt_insert(TBTNode *header, const WordRecord *rec);

/* Search for word. Returns pointer to matching node, or NULL. */
TBTNode *tbt_search(TBTNode *header, const char *word);

/* Delete word using simplified rebuild approach. */
void tbt_delete(TBTNode *header, const char *word);

/*
 * In-order traversal using threads (no stack, no recursion).
 * Calls callback(node, arg) for each real data node (skips header).
 */
void tbt_inorder(TBTNode *header, void (*callback)(TBTNode *, void *), void *arg);

/* Return the inorder successor of node. Used by traversal and autocomplete. */
TBTNode *tbt_inorder_successor(TBTNode *node);

/* Free all nodes including the header. Sets *header to NULL. */
void tbt_free(TBTNode **header);

/* Return count of real data nodes (excludes header). */
int tbt_count(TBTNode *header);

#endif /* TBT_H */
