/* tbt.c - Threaded Binary Tree implementation (Phase 5) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tbt.h"
#include "dictionary.h"
#include "utils.h"

/* ── Static helpers ──────────────────────────────────────────── */

/*
 * Iterative free — walks inorder via thread pointers, freeing each data
 * node before advancing to its successor.  Safe on trees of any depth
 * (a recursive approach would stack-overflow at ~90 000 nodes on Windows).
 *
 * We capture the successor BEFORE freeing the current node so we never
 * dereference a freed pointer.  Left-thread pointers of later nodes may
 * dangle after their target is freed, but tbt_inorder_successor never
 * follows left threads, so those are never accessed.
 */
static void tbt_free_nodes(TBTNode *root, TBTNode *header) {
    TBTNode *cur, *next;
    if (!root) return;

    /* Walk to the leftmost (smallest) real node */
    cur = root;
    while (!cur->lthread) cur = cur->left;

    /* Visit every node in inorder sequence until we reach the header */
    while (cur != header) {
        next = tbt_inorder_successor(cur); /* get successor BEFORE free */
        free(cur);
        cur = next;
    }
}

/* ── Public API ──────────────────────────────────────────────── */

TBTNode *tbt_create_header(void) {
    TBTNode *h = (TBTNode *)malloc(sizeof(TBTNode));
    if (!h) { perror("tbt_create_header: malloc"); exit(EXIT_FAILURE); }
    word_record_init(&h->data);
    h->left    = h;   /* self-referential when empty */
    h->right   = h;   /* always points back to header (end sentinel) */
    h->lthread = 1;   /* treat left as thread when tree is empty */
    h->rthread = 1;   /* right is always a thread */
    return h;
}

TBTNode *tbt_new_node(const WordRecord *rec) {
    TBTNode *n = (TBTNode *)malloc(sizeof(TBTNode));
    if (!n) { perror("tbt_new_node: malloc"); exit(EXIT_FAILURE); }
    n->data    = *rec;
    n->left    = NULL;
    n->right   = NULL;
    n->lthread = 1;   /* threads will be wired on insertion */
    n->rthread = 1;
    return n;
}

void tbt_insert(TBTNode *header, const WordRecord *rec) {
    char buf[MAX_WORD_LEN];
    WordRecord r;
    TBTNode *parent, *cur, *n;
    int went_left, cmp;

    r = *rec;
    str_tolower(buf, rec->word, sizeof(buf));
    str_safe_copy(r.word, buf, sizeof(r.word));

    /* Navigate to insertion point (BST-style, respecting thread flags) */
    parent    = header;
    went_left = 1;                              /* first step goes left from header */
    cur       = header->lthread ? NULL : header->left;  /* tree root, or NULL if empty */

    while (cur) {
        cmp = strcmp(r.word, cur->data.word);
        if (cmp == 0) return;                   /* duplicate — silently skip */
        parent = cur;
        if (cmp < 0) {
            went_left = 1;
            cur = cur->lthread ? NULL : cur->left;
        } else {
            went_left = 0;
            cur = cur->rthread ? NULL : cur->right;
        }
    }

    n = tbt_new_node(&r);

    if (went_left) {
        /* Insert as left child of parent.
           New node's inorder predecessor = parent's current left-thread target.
           New node's inorder successor   = parent itself. */
        n->left    = parent->left;     /* inherit parent's left-thread target */
        n->lthread = parent->lthread;  /* thread or real — copied as-is */
        n->right   = parent;           /* right thread -> parent (successor) */
        n->rthread = 1;
        parent->left    = n;
        parent->lthread = 0;           /* parent's left is now a real link */
    } else {
        /* Insert as right child of parent.
           New node's inorder successor   = parent's current right-thread target.
           New node's inorder predecessor = parent itself. */
        n->right   = parent->right;    /* inherit parent's right-thread target */
        n->rthread = parent->rthread;  /* thread or real — copied as-is */
        n->left    = parent;           /* left thread -> parent (predecessor) */
        n->lthread = 1;
        parent->right   = n;
        parent->rthread = 0;           /* parent's right is now a real link */
    }
}

TBTNode *tbt_search(TBTNode *header, const char *word) {
    char buf[MAX_WORD_LEN];
    TBTNode *cur;
    int cmp;

    str_tolower(buf, word, sizeof(buf));

    cur = header->lthread ? NULL : header->left;
    while (cur) {
        cmp = strcmp(buf, cur->data.word);
        if (cmp == 0) return cur;
        if (cmp < 0) cur = cur->lthread ? NULL : cur->left;
        else         cur = cur->rthread ? NULL : cur->right;
    }
    return NULL;
}

TBTNode *tbt_inorder_successor(TBTNode *node) {
    /* If right is a thread, it already points directly to the inorder successor */
    if (node->rthread) return node->right;

    /* Otherwise go right once, then find the leftmost node in that subtree */
    node = node->right;
    while (!node->lthread) node = node->left;
    return node;
}

void tbt_inorder(TBTNode *header, void (*callback)(TBTNode *, void *), void *arg) {
    TBTNode *cur;
    if (!header || header->lthread) return;  /* NULL header or empty tree */

    /* Find the leftmost (smallest) real data node */
    cur = header->left;
    while (!cur->lthread) cur = cur->left;

    /* Visit each node until we loop back to the header sentinel */
    while (cur != header) {
        callback(cur, arg);
        cur = tbt_inorder_successor(cur);
    }
}

void tbt_delete(TBTNode *header, const char *word) {
    char       buf[MAX_WORD_LEN];
    WordRecord *arr;
    int         n, idx, i;
    TBTNode    *cur;

    str_tolower(buf, word, sizeof(buf));

    n = tbt_count(header);
    if (n == 0) return;

    /* Collect all records except the target into a temporary array */
    arr = (WordRecord *)malloc((size_t)n * sizeof(WordRecord));
    if (!arr) { perror("tbt_delete: malloc"); return; }

    idx = 0;
    if (!header->lthread) {
        cur = header->left;
        while (!cur->lthread) cur = cur->left;   /* walk to leftmost node */

        while (cur != header) {
            if (strcmp(cur->data.word, buf) != 0)
                arr[idx++] = cur->data;
            cur = tbt_inorder_successor(cur);
        }
    }

    /* Word not found — nothing changed */
    if (idx == n) { free(arr); return; }

    /* Free all data nodes iteratively */
    if (!header->lthread)
        tbt_free_nodes(header->left, header);

    /* Reset header to empty-tree state */
    header->left    = header;
    header->lthread = 1;
    /* header->right and header->rthread stay as end-sentinel (unchanged) */

    /* Re-insert all collected records */
    for (i = 0; i < idx; i++)
        tbt_insert(header, &arr[i]);

    free(arr);
}

void tbt_free(TBTNode **header) {
    if (!header || !*header) return;

    /* Free all data nodes iteratively */
    if (!(*header)->lthread)
        tbt_free_nodes((*header)->left, *header);

    /* Free the header sentinel */
    free(*header);
    *header = NULL;
}

int tbt_count(TBTNode *header) {
    int      count = 0;
    TBTNode *cur;

    if (!header || header->lthread) return 0;

    cur = header->left;
    while (!cur->lthread) cur = cur->left;  /* leftmost node */

    while (cur != header) {
        count++;
        cur = tbt_inorder_successor(cur);
    }
    return count;
}
