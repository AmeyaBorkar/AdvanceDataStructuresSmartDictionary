/* autocomplete.h - Prefix-based autocomplete engine (Phase 6) */
#ifndef AUTOCOMPLETE_H
#define AUTOCOMPLETE_H

#include "dictionary.h"
#include "bst.h"
#include "avl.h"
#include "tbt.h"

int autocomplete_bst(BSTNode *root,   const char *prefix,
                     WordRecord *results, int top_k);

int autocomplete_avl(AVLNode *root,   const char *prefix,
                     WordRecord *results, int top_k);

int autocomplete_tbt(TBTNode *header, const char *prefix,
                     WordRecord *results, int top_k);

void autocomplete_record_selection(const char *word,
                                   BSTNode *bst_root,
                                   AVLNode *avl_root,
                                   TBTNode *tbt_header);

#endif /* AUTOCOMPLETE_H */
