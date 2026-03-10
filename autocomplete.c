/* autocomplete.c - Prefix-based autocomplete engine (Phase 6) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "autocomplete.h"
#include "utils.h"

/* Maximum words we ever collect before trimming to top_k.
   Sized conservatively above any realistic dictionary size. */
#define MAX_CANDIDATES 512

/* ── Static helpers ──────────────────────────────────────────── */

static int composite_score(const WordRecord *r) {
    return r->frequency_score + 10 * r->user_select_count;
}

/* qsort comparator: higher composite score sorts first */
static int cmp_score_desc(const void *a, const void *b) {
    return composite_score((const WordRecord *)b)
         - composite_score((const WordRecord *)a);
}

/* Recursive BST prefix collector with BST-pruning.
 * strncmp(root->word, prefix, plen):
 *   > 0 → current word is alphabetically past prefix → only left subtree can match
 *   < 0 → current word is before prefix range      → only right subtree can match
 *   = 0 → current word starts with prefix          → collect + recurse both sides */
static void bst_collect(BSTNode *root, const char *prefix, size_t plen,
                         WordRecord *buf, int *count) {
    int cmp;
    if (!root || *count >= MAX_CANDIDATES) return;
    cmp = strncmp(root->data.word, prefix, plen);
    if (cmp > 0) {
        bst_collect(root->left,  prefix, plen, buf, count);
    } else if (cmp < 0) {
        bst_collect(root->right, prefix, plen, buf, count);
    } else {
        bst_collect(root->left,  prefix, plen, buf, count);
        if (*count < MAX_CANDIDATES)
            buf[(*count)++] = root->data;
        bst_collect(root->right, prefix, plen, buf, count);
    }
}

/* Identical algorithm for AVL nodes */
static void avl_collect(AVLNode *root, const char *prefix, size_t plen,
                         WordRecord *buf, int *count) {
    int cmp;
    if (!root || *count >= MAX_CANDIDATES) return;
    cmp = strncmp(root->data.word, prefix, plen);
    if (cmp > 0) {
        avl_collect(root->left,  prefix, plen, buf, count);
    } else if (cmp < 0) {
        avl_collect(root->right, prefix, plen, buf, count);
    } else {
        avl_collect(root->left,  prefix, plen, buf, count);
        if (*count < MAX_CANDIDATES)
            buf[(*count)++] = root->data;
        avl_collect(root->right, prefix, plen, buf, count);
    }
}

/* ── Public API ──────────────────────────────────────────────── */

int autocomplete_bst(BSTNode *root, const char *prefix,
                     WordRecord *results, int top_k) {
    char       buf[MAX_WORD_LEN];
    WordRecord candidates[MAX_CANDIDATES];
    int        count = 0, ret, i;

    str_tolower(buf, prefix, sizeof(buf));
    bst_collect(root, buf, strlen(buf), candidates, &count);
    qsort(candidates, (size_t)count, sizeof(WordRecord), cmp_score_desc);
    ret = count < top_k ? count : top_k;
    for (i = 0; i < ret; i++) results[i] = candidates[i];
    return ret;
}

int autocomplete_avl(AVLNode *root, const char *prefix,
                     WordRecord *results, int top_k) {
    char       buf[MAX_WORD_LEN];
    WordRecord candidates[MAX_CANDIDATES];
    int        count = 0, ret, i;

    str_tolower(buf, prefix, sizeof(buf));
    avl_collect(root, buf, strlen(buf), candidates, &count);
    qsort(candidates, (size_t)count, sizeof(WordRecord), cmp_score_desc);
    ret = count < top_k ? count : top_k;
    for (i = 0; i < ret; i++) results[i] = candidates[i];
    return ret;
}

int autocomplete_tbt(TBTNode *header, const char *prefix,
                     WordRecord *results, int top_k) {
    char       buf[MAX_WORD_LEN];
    size_t     plen;
    WordRecord candidates[MAX_CANDIDATES];
    int        count = 0, ret, i, cmp;
    TBTNode   *cur, *start;

    str_tolower(buf, prefix, sizeof(buf));
    plen = strlen(buf);

    if (plen == 0 || !header || header->lthread) return 0;

    /* Find lower bound: leftmost node whose word >= prefix (first plen chars).
     * Navigate BST-style: on cmp >= 0, save as candidate and go left
     *                     (earlier nodes in the sorted order may also match);
     *                     on cmp < 0, go right (need lexicographically larger). */
    start = NULL;
    cur   = header->lthread ? NULL : header->left;
    while (cur) {
        cmp = strncmp(cur->data.word, buf, plen);
        if (cmp >= 0) {
            start = cur;
            cur   = cur->lthread ? NULL : cur->left;
        } else {
            cur = cur->rthread ? NULL : cur->right;
        }
    }

    if (!start) return 0;   /* every word sorts before prefix */

    /* Walk forward from the lower-bound using inorder thread successor.
     * Collect matching nodes; break as soon as we pass the prefix range. */
    cur = start;
    while (cur != header) {
        cmp = strncmp(cur->data.word, buf, plen);
        if (cmp > 0) break;    /* past the prefix range — subsequent words are larger */
        if (cmp == 0 && count < MAX_CANDIDATES)
            candidates[count++] = cur->data;
        cur = tbt_inorder_successor(cur);
    }

    qsort(candidates, (size_t)count, sizeof(WordRecord), cmp_score_desc);
    ret = count < top_k ? count : top_k;
    for (i = 0; i < ret; i++) results[i] = candidates[i];
    return ret;
}

void autocomplete_record_selection(const char *word,
                                   BSTNode *bst_root,
                                   AVLNode *avl_root,
                                   TBTNode *tbt_header) {
    BSTNode *bn = bst_search(bst_root,   word);
    AVLNode *an = avl_search(avl_root,   word);
    TBTNode *tn = tbt_search(tbt_header, word);
    if (bn) bn->data.user_select_count++;
    if (an) an->data.user_select_count++;
    if (tn) tn->data.user_select_count++;
}
