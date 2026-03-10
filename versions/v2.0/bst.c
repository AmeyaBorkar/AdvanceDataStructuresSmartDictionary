/* bst.c - Binary Search Tree implementation */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bst.h"
#include "utils.h"

static BSTNode *bst_min_node(BSTNode *node) {
    while (node && node->left)
        node = node->left;
    return node;
}

static void bst_delete_impl(BSTNode **root, const char *lw) {
    int cmp;
    BSTNode *tmp;
    BSTNode *succ;

    if (!*root) return;

    cmp = strcmp(lw, (*root)->data.word);
    if      (cmp < 0) { bst_delete_impl(&(*root)->left,  lw); return; }
    else if (cmp > 0) { bst_delete_impl(&(*root)->right, lw); return; }

    if (!(*root)->left && !(*root)->right) {
        free(*root);
        *root = NULL;
    } else if (!(*root)->left) {
        tmp = *root;
        *root = (*root)->right;
        free(tmp);
    } else if (!(*root)->right) {
        tmp = *root;
        *root = (*root)->left;
        free(tmp);
    } else {
        succ = bst_min_node((*root)->right);
        (*root)->data = succ->data;
        bst_delete_impl(&(*root)->right, succ->data.word);
    }
}

BSTNode *bst_new_node(const WordRecord *rec) {
    BSTNode *node;
    if (!rec) return NULL;
    node = (BSTNode *)malloc(sizeof(BSTNode));
    if (!node) {
        fprintf(stderr, "bst_new_node: malloc failed\n");
        return NULL;
    }
    node->data  = *rec;
    node->left  = NULL;
    node->right = NULL;
    return node;
}

void bst_insert(BSTNode **root, const WordRecord *rec) {
    BSTNode **cur;
    WordRecord norm;
    int cmp;

    if (!root || !rec) return;
    norm = *rec;
    str_tolower(norm.word, rec->word, sizeof(norm.word));

    cur = root;
    while (*cur) {
        cmp = strcmp(norm.word, (*cur)->data.word);
        if      (cmp < 0) cur = &(*cur)->left;
        else if (cmp > 0) cur = &(*cur)->right;
        else return;
    }
    *cur = bst_new_node(&norm);
}

BSTNode *bst_search(BSTNode *root, const char *word) {
    char lw[MAX_WORD_LEN];
    int  cmp;

    if (!word) return NULL;
    str_tolower(lw, word, sizeof(lw));

    while (root) {
        cmp = strcmp(lw, root->data.word);
        if      (cmp == 0) return root;
        else if (cmp  < 0) root = root->left;
        else               root = root->right;
    }
    return NULL;
}

void bst_delete(BSTNode **root, const char *word) {
    char lw[MAX_WORD_LEN];
    if (!root || !word) return;
    str_tolower(lw, word, sizeof(lw));
    bst_delete_impl(root, lw);
}

void bst_inorder(BSTNode *root, void (*callback)(BSTNode *, void *), void *arg) {
    if (!root || !callback) return;
    bst_inorder(root->left, callback, arg);
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
    int lh, rh;
    if (!root) return 0;
    lh = bst_height(root->left);
    rh = bst_height(root->right);
    return 1 + (lh > rh ? lh : rh);
}

int bst_count(BSTNode *root) {
    if (!root) return 0;
    return 1 + bst_count(root->left) + bst_count(root->right);
}
