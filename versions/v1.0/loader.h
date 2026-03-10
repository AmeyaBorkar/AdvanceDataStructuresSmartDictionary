/* loader.h - File loading */
#ifndef LOADER_H
#define LOADER_H

#include "dictionary.h"
#include "bst.h"

/* Load words from file into BST. Returns count or -1 on error. */
int load_words(const char *path, BSTNode **bst_root);

/* Read word,score pairs and update frequency_score on matching nodes.
   Returns count of updates or -1 on error. */
int load_frequencies(const char *path, BSTNode *bst_root);

#endif /* LOADER_H */
