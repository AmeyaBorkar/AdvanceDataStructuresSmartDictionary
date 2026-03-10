/* avl.c - AVL Tree implementation (Phase 4) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "avl.h"
#include "utils.h"

/* ── Static helpers ──────────────────────────────────────────── */

static int max_int(int a, int b) { return a > b ? a : b; }

static void update_height(AVLNode *n) {
    n->height = 1 + max_int(avl_height(n->left), avl_height(n->right));
}

static AVLNode *rotate_right(AVLNode *y) {
    AVLNode *x  = y->left;
    AVLNode *T2 = x->right;
    x->right = y;
    y->left  = T2;
    update_height(y);   /* y is now lower — update first */
    update_height(x);
    return x;           /* new subtree root */
}

static AVLNode *rotate_left(AVLNode *x) {
    AVLNode *y  = x->right;
    AVLNode *T2 = y->left;
    y->left  = x;
    x->right = T2;
    update_height(x);   /* x is now lower — update first */
    update_height(y);
    return y;           /* new subtree root */
}

static AVLNode *rebalance(AVLNode *node) {
    int bf = avl_balance_factor(node);

    /* LL — left-heavy and left child is also left-heavy or balanced */
    if (bf > 1 && avl_balance_factor(node->left) >= 0)
        return rotate_right(node);

    /* LR — left-heavy but left child is right-heavy */
    if (bf > 1 && avl_balance_factor(node->left) < 0) {
        node->left = rotate_left(node->left);
        return rotate_right(node);
    }

    /* RR — right-heavy and right child is also right-heavy or balanced */
    if (bf < -1 && avl_balance_factor(node->right) <= 0)
        return rotate_left(node);

    /* RL — right-heavy but right child is left-heavy */
    if (bf < -1 && avl_balance_factor(node->right) > 0) {
        node->right = rotate_right(node->right);
        return rotate_left(node);
    }

    return node;  /* already balanced */
}

static AVLNode *avl_min_node(AVLNode *n) {
    while (n->left) n = n->left;
    return n;
}

static AVLNode *avl_insert_impl(AVLNode *root, const WordRecord *rec) {
    if (!root) return avl_new_node(rec);

    int cmp = strcmp(rec->word, root->data.word);
    if      (cmp < 0) root->left  = avl_insert_impl(root->left,  rec);
    else if (cmp > 0) root->right = avl_insert_impl(root->right, rec);
    else              return root;  /* duplicate — skip */

    update_height(root);
    return rebalance(root);
}

static AVLNode *avl_delete_impl(AVLNode *root, const char *word) {
    if (!root) return NULL;

    int cmp = strcmp(word, root->data.word);
    if (cmp < 0) {
        root->left  = avl_delete_impl(root->left,  word);
    } else if (cmp > 0) {
        root->right = avl_delete_impl(root->right, word);
    } else {
        /* Node to delete found */
        if (!root->left || !root->right) {
            /* 0 or 1 child case */
            AVLNode *child = root->left ? root->left : root->right;
            free(root);
            return child;   /* NULL if leaf */
        }
        /* 2 children: replace with inorder successor, then delete successor */
        AVLNode *succ = avl_min_node(root->right);
        root->data    = succ->data;
        root->right   = avl_delete_impl(root->right, succ->data.word);
    }

    update_height(root);
    return rebalance(root);
}

static AVLNode *avl_search_impl(AVLNode *root, const char *word) {
    if (!root) return NULL;
    int cmp = strcmp(word, root->data.word);
    if (cmp < 0) return avl_search_impl(root->left,  word);
    if (cmp > 0) return avl_search_impl(root->right, word);
    return root;
}

/* ── Public API ──────────────────────────────────────────────── */

AVLNode *avl_new_node(const WordRecord *rec) {
    AVLNode *n = (AVLNode *)malloc(sizeof(AVLNode));
    if (!n) { perror("avl_new_node: malloc"); exit(EXIT_FAILURE); }
    n->data   = *rec;
    n->left   = NULL;
    n->right  = NULL;
    n->height = 1;
    return n;
}

AVLNode *avl_insert(AVLNode *root, const WordRecord *rec) {
    char buf[MAX_WORD_LEN];
    WordRecord r = *rec;
    str_tolower(buf, rec->word, sizeof(buf));
    str_safe_copy(r.word, buf, sizeof(r.word));
    return avl_insert_impl(root, &r);
}

AVLNode *avl_search(AVLNode *root, const char *word) {
    char buf[MAX_WORD_LEN];
    str_tolower(buf, word, sizeof(buf));
    return avl_search_impl(root, buf);
}

AVLNode *avl_delete(AVLNode *root, const char *word) {
    char buf[MAX_WORD_LEN];
    str_tolower(buf, word, sizeof(buf));
    return avl_delete_impl(root, buf);
}

void avl_inorder(AVLNode *root, void (*callback)(AVLNode *, void *), void *arg) {
    if (!root) return;
    avl_inorder(root->left, callback, arg);
    callback(root, arg);
    avl_inorder(root->right, callback, arg);
}

void avl_free(AVLNode **root) {
    if (!root || !*root) return;
    avl_free(&(*root)->left);
    avl_free(&(*root)->right);
    free(*root);
    *root = NULL;
}

int avl_height(AVLNode *node) {
    return node ? node->height : 0;
}

int avl_balance_factor(AVLNode *node) {
    return node ? avl_height(node->left) - avl_height(node->right) : 0;
}

int avl_count(AVLNode *root) {
    return root ? 1 + avl_count(root->left) + avl_count(root->right) : 0;
}
