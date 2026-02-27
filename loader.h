/* loader.h - File loading and persistence */
#ifndef LOADER_H
#define LOADER_H

#include "dictionary.h"
#include "bst.h"
#include "avl.h"
#include "tbt.h"

/*
 * Load words from a file into the BST (and AVL/TBT once implemented).
 *
 * Supported file formats (auto-detected per line):
 *   word|pos|meaning   -- rich format (pipe-delimited)
 *   word|pos           -- rich format without meaning
 *   word               -- simple format (one word per line)
 *   # comment          -- skipped
 *   (blank line)       -- skipped
 *
 * Returns the number of words successfully inserted, or -1 on file open error.
 * Duplicates in the file are silently skipped by bst_insert.
 */
int load_words(const char *path, BSTNode **bst_root,
               AVLNode **avl_root, TBTNode *tbt_header);

/*
 * Read comma-separated word,score pairs from path and update the
 * frequency_score field of matching BST nodes.
 *
 * File format (per line):
 *   word,score    -- integer score in range [1, FREQ_SCORE_MAX]
 *   # comment     -- skipped
 *   (blank line)  -- skipped
 *
 * Returns the number of nodes updated, or -1 on file open error.
 * Words not found in the tree are silently skipped.
 */
int load_frequencies(const char *path, BSTNode *bst_root,
                     AVLNode *avl_root, TBTNode *tbt_header);

/*
 * Write all words in the BST (in sorted order) to path in pipe format:
 *   word|pos|meaning
 *
 * This snapshot can be reloaded via load_words() in a future session.
 * Returns 0 on success, -1 on file open error.
 */
int save_custom_words(const char *path, BSTNode *bst_root);

#endif /* LOADER_H */
