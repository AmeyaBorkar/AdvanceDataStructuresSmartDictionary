/* loader.h - File loading */
#ifndef LOADER_H
#define LOADER_H

#include "dictionary.h"
#include "bst.h"
#include "avl.h"
#include "tbt.h"

/* Load words from file into BST, AVL, and TBT. Returns count or -1 on error. */
int load_words(const char *path, BSTNode **bst_root,
               AVLNode **avl_root, TBTNode *tbt_header);

/* Read word,score pairs and update frequency_score on matching nodes.
   Returns count of updates or -1 on error. */
int load_frequencies(const char *path, BSTNode *bst_root,
                     AVLNode *avl_root, TBTNode *tbt_header);

#endif /* LOADER_H */
