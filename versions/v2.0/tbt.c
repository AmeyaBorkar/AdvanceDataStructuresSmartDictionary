/* tbt.c - Threaded Binary Tree implementation (Phase 5) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tbt.h"
#include "dictionary.h"
#include "utils.h"

static void tbt_free_nodes(TBTNode *root, TBTNode *header) {
    TBTNode *cur, *next;
    if (!root) return;

    cur = root;
    while (!cur->lthread) cur = cur->left;

    while (cur != header) {
        next = tbt_inorder_successor(cur);
        free(cur);
        cur = next;
    }
}

TBTNode *tbt_create_header(void) {
    TBTNode *h = (TBTNode *)malloc(sizeof(TBTNode));
    if (!h) { perror("tbt_create_header: malloc"); exit(EXIT_FAILURE); }
    word_record_init(&h->data);
    h->left    = h;
    h->right   = h;
    h->lthread = 1;
    h->rthread = 1;
    return h;
}

TBTNode *tbt_new_node(const WordRecord *rec) {
    TBTNode *n = (TBTNode *)malloc(sizeof(TBTNode));
    if (!n) { perror("tbt_new_node: malloc"); exit(EXIT_FAILURE); }
    n->data    = *rec;
    n->left    = NULL;
    n->right   = NULL;
    n->lthread = 1;
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

    parent    = header;
    went_left = 1;
    cur       = header->lthread ? NULL : header->left;

    while (cur) {
        cmp = strcmp(r.word, cur->data.word);
        if (cmp == 0) return;
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
        n->left    = parent->left;
        n->lthread = parent->lthread;
        n->right   = parent;
        n->rthread = 1;
        parent->left    = n;
        parent->lthread = 0;
    } else {
        n->right   = parent->right;
        n->rthread = parent->rthread;
        n->left    = parent;
        n->lthread = 1;
        parent->right   = n;
        parent->rthread = 0;
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
    if (node->rthread) return node->right;
    node = node->right;
    while (!node->lthread) node = node->left;
    return node;
}

void tbt_inorder(TBTNode *header, void (*callback)(TBTNode *, void *), void *arg) {
    TBTNode *cur;
    if (!header || header->lthread) return;

    cur = header->left;
    while (!cur->lthread) cur = cur->left;

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

    arr = (WordRecord *)malloc((size_t)n * sizeof(WordRecord));
    if (!arr) { perror("tbt_delete: malloc"); return; }

    idx = 0;
    if (!header->lthread) {
        cur = header->left;
        while (!cur->lthread) cur = cur->left;

        while (cur != header) {
            if (strcmp(cur->data.word, buf) != 0)
                arr[idx++] = cur->data;
            cur = tbt_inorder_successor(cur);
        }
    }

    if (idx == n) { free(arr); return; }

    if (!header->lthread)
        tbt_free_nodes(header->left, header);

    header->left    = header;
    header->lthread = 1;

    for (i = 0; i < idx; i++)
        tbt_insert(header, &arr[i]);

    free(arr);
}

void tbt_free(TBTNode **header) {
    if (!header || !*header) return;
    if (!(*header)->lthread)
        tbt_free_nodes((*header)->left, *header);
    free(*header);
    *header = NULL;
}

int tbt_count(TBTNode *header) {
    int      count = 0;
    TBTNode *cur;

    if (!header || header->lthread) return 0;

    cur = header->left;
    while (!cur->lthread) cur = cur->left;

    while (cur != header) {
        count++;
        cur = tbt_inorder_successor(cur);
    }
    return count;
}
