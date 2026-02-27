/* bst.c - Binary Search Tree implementation */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bst.h"
#include "utils.h"

/* ── Private helpers ─────────────────────────────────────────── */

/* Return the leftmost (minimum-key) node in a subtree. */
static BSTNode *bst_min_node(BSTNode *node) {
    while (node && node->left)
        node = node->left;
    return node;
}

/* max of two ints — used by bst_height */
static int max_int(int a, int b) {
    return (a > b) ? a : b;
}

/*
 * Recursive insert helper. 'norm' is a WordRecord whose word field
 * has already been lowercased by the public bst_insert().
 * Duplicates (same lowercase word) are silently ignored.
 */
static void bst_insert_impl(BSTNode **root, const WordRecord *norm) {
    int cmp;
    if (!*root) {
        *root = bst_new_node(norm);
        return;
    }
    cmp = strcmp(norm->word, (*root)->data.word);
    if      (cmp < 0) bst_insert_impl(&(*root)->left,  norm);
    else if (cmp > 0) bst_insert_impl(&(*root)->right, norm);
    /* cmp == 0: duplicate — skip silently */
}

/*
 * Recursive search helper. 'lw' is the pre-lowercased search key.
 * Normalising once in the public function avoids redundant str_tolower
 * calls at every level of recursion.
 */
static BSTNode *bst_search_impl(BSTNode *root, const char *lw) {
    int cmp;
    if (!root) return NULL;
    cmp = strcmp(lw, root->data.word);
    if      (cmp == 0) return root;
    else if (cmp <  0) return bst_search_impl(root->left,  lw);
    else               return bst_search_impl(root->right, lw);
}

/*
 * Recursive delete helper. 'lw' is the pre-lowercased target word.
 *
 * Three cases:
 *   1. Leaf node          → free and set pointer to NULL
 *   2. One child          → replace node with its child
 *   3. Two children       → replace data with inorder successor's data,
 *                           then delete the successor from the right subtree
 *
 * Case 3 copies the full WordRecord struct (safe: no heap pointers inside).
 */
static void bst_delete_impl(BSTNode **root, const char *lw) {
    int cmp;
    BSTNode *tmp;
    BSTNode *succ;

    if (!*root) return;

    cmp = strcmp(lw, (*root)->data.word);
    if      (cmp < 0) { bst_delete_impl(&(*root)->left,  lw); return; }
    else if (cmp > 0) { bst_delete_impl(&(*root)->right, lw); return; }

    /* Found the node to delete */
    if (!(*root)->left && !(*root)->right) {
        /* Case 1: leaf */
        free(*root);
        *root = NULL;
    } else if (!(*root)->left) {
        /* Case 2a: only right child */
        tmp = *root;
        *root = (*root)->right;
        free(tmp);
    } else if (!(*root)->right) {
        /* Case 2b: only left child */
        tmp = *root;
        *root = (*root)->left;
        free(tmp);
    } else {
        /* Case 3: two children — use inorder successor */
        succ = bst_min_node((*root)->right);
        (*root)->data = succ->data;                       /* struct copy */
        bst_delete_impl(&(*root)->right, succ->data.word); /* delete successor */
    }
}

/* ── Public API ──────────────────────────────────────────────── */

BSTNode *bst_new_node(const WordRecord *rec) {
    BSTNode *node;
    if (!rec) return NULL;
    node = (BSTNode *)malloc(sizeof(BSTNode));
    if (!node) {
        fprintf(stderr, "bst_new_node: malloc failed\n");
        return NULL;
    }
    node->data  = *rec;   /* full struct copy — safe, no heap pointers */
    node->left  = NULL;
    node->right = NULL;
    return node;
}

void bst_insert(BSTNode **root, const WordRecord *rec) {
    WordRecord norm;
    if (!root || !rec) return;
    norm = *rec;                                          /* struct copy */
    str_tolower(norm.word, rec->word, sizeof(norm.word)); /* normalise key */
    bst_insert_impl(root, &norm);
}

BSTNode *bst_search(BSTNode *root, const char *word) {
    char lw[MAX_WORD_LEN];
    if (!word) return NULL;
    str_tolower(lw, word, sizeof(lw));
    return bst_search_impl(root, lw);
}

void bst_delete(BSTNode **root, const char *word) {
    char lw[MAX_WORD_LEN];
    if (!root || !word) return;
    str_tolower(lw, word, sizeof(lw));
    bst_delete_impl(root, lw);
}

void bst_inorder(BSTNode *root, void (*callback)(BSTNode *, void *), void *arg) {
    if (!root || !callback) return;
    bst_inorder(root->left,  callback, arg);
    callback(root, arg);
    bst_inorder(root->right, callback, arg);
}

void bst_free(BSTNode **root) {
    if (!root || !*root) return;
    bst_free(&(*root)->left);
    bst_free(&(*root)->right);
    free(*root);
    *root = NULL;
}

int bst_height(BSTNode *root) {
    if (!root) return 0;
    return 1 + max_int(bst_height(root->left), bst_height(root->right));
}

int bst_count(BSTNode *root) {
    if (!root) return 0;
    return 1 + bst_count(root->left) + bst_count(root->right);
}
