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

/*
 * Recursive delete helper. 'lw' is the pre-lowercased target word.
 * Left recursive: delete is O(depth) in call-stack usage, but after
 * the preorder-save fix the tree height stays ~50, making this safe.
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
        (*root)->data = succ->data;                        /* struct copy */
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

/*
 * Iterative insert — avoids stack overflow on skewed/sorted input.
 * Walks the tree with a pointer-to-pointer cursor; no recursion needed.
 */
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
        else return; /* duplicate — skip silently */
    }
    *cur = bst_new_node(&norm);
}

/*
 * Iterative search — safe on any tree depth.
 */
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

/*
 * Morris inorder traversal — O(n) time, O(1) extra space, no recursion.
 * Temporarily threads right-null pointers; restores them before returning.
 * Safe on trees of any depth including fully right-skewed 90k-node trees.
 */
void bst_inorder(BSTNode *root, void (*callback)(BSTNode *, void *), void *arg) {
    BSTNode *cur = root, *pre;
    if (!callback) return;
    while (cur) {
        if (!cur->left) {
            callback(cur, arg);
            cur = cur->right;
        } else {
            /* Find inorder predecessor */
            pre = cur->left;
            while (pre->right && pre->right != cur)
                pre = pre->right;
            if (!pre->right) {
                pre->right = cur;   /* thread: remember current for later */
                cur = cur->left;
            } else {
                pre->right = NULL;  /* remove thread */
                callback(cur, arg);
                cur = cur->right;
            }
        }
    }
}

/*
 * Morris preorder traversal — visits root BEFORE descending left.
 * Used by save_custom_words so the saved file re-creates the same BST
 * structure on the next load (avoids the sorted-inorder → skewed-tree bug).
 */
void bst_preorder(BSTNode *root, void (*callback)(BSTNode *, void *), void *arg) {
    BSTNode *cur = root, *pre;
    if (!callback) return;
    while (cur) {
        if (!cur->left) {
            callback(cur, arg);
            cur = cur->right;
        } else {
            pre = cur->left;
            while (pre->right && pre->right != cur)
                pre = pre->right;
            if (!pre->right) {
                pre->right = cur;
                callback(cur, arg); /* visit BEFORE going left */
                cur = cur->left;
            } else {
                pre->right = NULL;
                cur = cur->right;
            }
        }
    }
}

/*
 * Iterative free — tree-to-vine (Day-Stout-Warren vine phase).
 * Converts every left child into a right child via right-rotation,
 * then frees the resulting right-linked list in one pass.
 * O(n) time, O(1) extra space, safe on trees of any depth.
 */
void bst_free(BSTNode **root) {
    BSTNode *cur = *root, *tmp;
    while (cur) {
        if (cur->left) {
            /* Right-rotate: pull left child up */
            tmp      = cur->left;
            cur->left = tmp->right;
            tmp->right = cur;
            cur = tmp;
        } else {
            /* No left child — free node, move right */
            tmp = cur->right;
            free(cur);
            cur = tmp;
        }
    }
    *root = NULL;
}

/*
 * Iterative height — DFS with a small fixed-size stack.
 * For right-skewed trees the DFS stack never exceeds 2 entries (no left
 * children, so only one right child is ever pending at a time).
 * For balanced trees with n ≤ 100 000 the height is ≤ 51, so a stack
 * of 128 frames is far more than enough.
 */
int bst_height(BSTNode *root) {
    struct { BSTNode *n; int d; } stk[128];
    int top = 0, max_d = 0, d;
    BSTNode *n;

    if (!root) return 0;
    stk[top].n = root; stk[top].d = 1; top++;
    while (top > 0) {
        n = stk[top - 1].n;
        d = stk[top - 1].d;
        top--;
        if (d > max_d) max_d = d;
        /* Push children — stack can hold at most O(height) entries */
        if (n->right) { stk[top].n = n->right; stk[top].d = d + 1; top++; }
        if (n->left)  { stk[top].n = n->left;  stk[top].d = d + 1; top++; }
    }
    return max_d;
}

/*
 * Morris inorder count — same traversal as bst_inorder but just increments
 * a counter instead of calling a callback. O(n) time, O(1) space.
 */
int bst_count(BSTNode *root) {
    int count = 0;
    BSTNode *cur = root, *pre;
    while (cur) {
        if (!cur->left) {
            count++;
            cur = cur->right;
        } else {
            pre = cur->left;
            while (pre->right && pre->right != cur)
                pre = pre->right;
            if (!pre->right) {
                pre->right = cur;
                cur = cur->left;
            } else {
                pre->right = NULL;
                count++;
                cur = cur->right;
            }
        }
    }
    return count;
}
