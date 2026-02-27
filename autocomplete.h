/* autocomplete.h - Prefix-based autocomplete engine (Phase 6) */
#ifndef AUTOCOMPLETE_H
#define AUTOCOMPLETE_H

#include "dictionary.h"
#include "bst.h"
#include "avl.h"
#include "tbt.h"

/*
 * Find up to top_k words that start with prefix, ranked by composite score:
 *   composite_score = frequency_score + (10 * user_select_count)
 * Results are written into the caller-allocated results[top_k] array.
 * Returns the actual number of matches found (may be less than top_k).
 *
 * BST: recursive traversal with BST-pruning (O(log n + k) on average).
 * AVL: same recursive approach, O(log n + k) guaranteed.
 * TBT: iterative via inorder thread pointers — zero call stack, zero recursion.
 */
int autocomplete_bst(BSTNode *root,   const char *prefix,
                     WordRecord *results, int top_k);

int autocomplete_avl(AVLNode *root,   const char *prefix,
                     WordRecord *results, int top_k);

int autocomplete_tbt(TBTNode *header, const char *prefix,
                     WordRecord *results, int top_k);

/*
 * Increment user_select_count for word in all three trees simultaneously.
 * Call this when the user picks a suggestion from the autocomplete list.
 * This causes frequently selected words to rise in subsequent rankings.
 */
void autocomplete_record_selection(const char *word,
                                   BSTNode *bst_root,
                                   AVLNode *avl_root,
                                   TBTNode *tbt_header);

#endif /* AUTOCOMPLETE_H */
