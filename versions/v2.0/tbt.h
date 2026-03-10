/* tbt.h - Threaded Binary Tree node and operation signatures */
#ifndef TBT_H
#define TBT_H

#include "dictionary.h"

typedef struct TBTNode {
    WordRecord       data;
    struct TBTNode  *left;
    struct TBTNode  *right;
    int              lthread;
    int              rthread;
} TBTNode;

TBTNode *tbt_create_header(void);
TBTNode *tbt_new_node(const WordRecord *rec);
void tbt_insert(TBTNode *header, const WordRecord *rec);
TBTNode *tbt_search(TBTNode *header, const char *word);
void tbt_delete(TBTNode *header, const char *word);
void tbt_inorder(TBTNode *header, void (*callback)(TBTNode *, void *), void *arg);
TBTNode *tbt_inorder_successor(TBTNode *node);
void tbt_free(TBTNode **header);
int tbt_count(TBTNode *header);

#endif /* TBT_H */
