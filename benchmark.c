/* benchmark.c - Performance benchmarking and comparison (Phase 7) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "benchmark.h"
#include "dictionary.h"
#include "utils.h"

/* Number of search repetitions per trial — large enough to get measurable time */
#define BENCH_SEARCH_REPS  1000

/* Dataset sizes to benchmark */
static const int BENCH_SIZES[] = { 500, 2000, 5000 };
#define NUM_SIZES  3

/* ── Null traversal callbacks (traverse without printing) ────── */
static void null_bst(BSTNode *n, void *a) { (void)n; (void)a; }
static void null_avl(AVLNode *n, void *a) { (void)n; (void)a; }
static void null_tbt(TBTNode *n, void *a) { (void)n; (void)a; }

/* ── Helpers ─────────────────────────────────────────────────── */

/* Milliseconds elapsed since start */
static double ms_since(clock_t start) {
    return (double)(clock() - start) * 1000.0 / (double)CLOCKS_PER_SEC;
}

/*
 * Generate n unique WordRecords in pseudo-random insertion order.
 * Words are formatted "wd%05d" (wd00001..wd0N).
 * Fisher-Yates shuffle with srand(42) gives the same order every run.
 */
static void gen_words(WordRecord *arr, int n) {
    int i, j;
    WordRecord tmp;

    for (i = 0; i < n; i++) {
        word_record_init(&arr[i]);
        sprintf(arr[i].word, "wd%05d", i + 1);   /* wd00001 … wd05000 */
        str_safe_copy(arr[i].part_of_speech, "noun", sizeof(arr[i].part_of_speech));
        arr[i].frequency_score = 1 + (i % 100);
    }

    /* Fisher-Yates shuffle — reproducible via fixed seed */
    srand(42);
    for (i = n - 1; i > 0; i--) {
        j = rand() % (i + 1);
        tmp = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
    }
}

/*
 * Run one benchmark trial for a dataset of n words.
 * Prints a 4-row result block (insert, height, search, traverse).
 */
static void bench_one(int n) {
    WordRecord *words;
    BSTNode    *bst = NULL;
    AVLNode    *avl = NULL;
    TBTNode    *tbt = NULL;
    char        wbuf[MAX_WORD_LEN];
    int         i, r;
    clock_t     t;
    double      bst_ins, avl_ins, tbt_ins;
    double      bst_srch, avl_srch, tbt_srch;
    double      bst_trav, avl_trav, tbt_trav;
    int         bst_h, avl_h;

    words = (WordRecord *)malloc((size_t)n * sizeof(WordRecord));
    if (!words) {
        printf("  [benchmark] malloc failed for n=%d — skipping.\n", n);
        return;
    }
    gen_words(words, n);

    /* ── Bulk insertion ── */
    t = clock();
    for (i = 0; i < n; i++) bst_insert(&bst, &words[i]);
    bst_ins = ms_since(t);

    t = clock();
    for (i = 0; i < n; i++) avl = avl_insert(avl, &words[i]);
    avl_ins = ms_since(t);

    tbt = tbt_create_header();
    t = clock();
    for (i = 0; i < n; i++) tbt_insert(tbt, &words[i]);
    tbt_ins = ms_since(t);

    bst_h = bst_height(bst);
    avl_h = avl_height(avl);

    /* ── Repeated search (BENCH_SEARCH_REPS lookups) ── */
    srand(99);
    t = clock();
    for (r = 0; r < BENCH_SEARCH_REPS; r++) {
        sprintf(wbuf, "wd%05d", 1 + rand() % n);
        bst_search(bst, wbuf);
    }
    bst_srch = ms_since(t);

    srand(99);
    t = clock();
    for (r = 0; r < BENCH_SEARCH_REPS; r++) {
        sprintf(wbuf, "wd%05d", 1 + rand() % n);
        avl_search(avl, wbuf);
    }
    avl_srch = ms_since(t);

    srand(99);
    t = clock();
    for (r = 0; r < BENCH_SEARCH_REPS; r++) {
        sprintf(wbuf, "wd%05d", 1 + rand() % n);
        tbt_search(tbt, wbuf);
    }
    tbt_srch = ms_since(t);

    /* ── Full sorted traversal ── */
    t = clock();
    bst_inorder(bst, null_bst, NULL);
    bst_trav = ms_since(t);

    t = clock();
    avl_inorder(avl, null_avl, NULL);
    avl_trav = ms_since(t);

    t = clock();
    tbt_inorder(tbt, null_tbt, NULL);
    tbt_trav = ms_since(t);

    /* ── Print result rows ── */
    printf("  %-24s|  %7.3f  |  %7.3f  |  %7.3f\n",
           "  Bulk insert (ms)", bst_ins, avl_ins, tbt_ins);
    printf("  %-24s|  %7d  |  %7d  |  %7s\n",
           "  Tree height", bst_h, avl_h, "  n/a");
    printf("  %-24s|  %7.3f  |  %7.3f  |  %7.3f\n",
           "  Search x1000 (ms)", bst_srch, avl_srch, tbt_srch);
    printf("  %-24s|  %7.3f  |  %7.3f  |  %7.3f\n",
           "  Traverse full (ms)", bst_trav, avl_trav, tbt_trav);

    bst_free(&bst);
    avl_free(&avl);
    tbt_free(&tbt);
    free(words);
}

/* ── Public API ──────────────────────────────────────────────── */

void benchmark_run_all(void) {
    int i;
    const char *hdr =
        "  %-24s|  %-9s|  %-9s|  %-9s";
    const char *sep =
        "  ------------------------+----------+---------+---------";

    printf("\n");
    print_separator('=', 60);
    printf("  BENCHMARK: BST vs AVL vs TBT\n");
    printf("  Word order: pseudo-random (Fisher-Yates, seed=42)\n");
    printf("  Timing via clock() — values < 0.001 ms may appear as 0.000\n");
    print_separator('=', 60);

    for (i = 0; i < NUM_SIZES; i++) {
        int n = BENCH_SIZES[i];
        printf("\n");
        printf(hdr, "  Dataset: words", "  BST", "  AVL", "  TBT");
        printf("\n");
        printf("  %-24s|  %-9d|  %-9d|  %-9d\n",
               "  Size (words)", n, n, n);
        printf("%s\n", sep);
        bench_one(n);
        printf("%s\n", sep);
    }

    printf("\n");
    print_separator('=', 60);
    printf("  Notes:\n");
    printf("  BST  - Unbalanced; height depends on insertion order.\n");
    printf("         Worst case O(n) for sorted input.\n");
    printf("  AVL  - Self-balancing; height always O(log n).\n");
    printf("         Slightly higher insert cost due to rotations.\n");
    printf("  TBT  - Threaded BST; traverse needs no stack/recursion.\n");
    printf("         Insert slightly costlier (thread pointer setup).\n");
    print_separator('=', 60);
    printf("\n");
}
