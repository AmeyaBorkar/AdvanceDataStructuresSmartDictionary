/* main.c - Smart Dictionary & Autocomplete Engine: console menu */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "dictionary.h"
#include "utils.h"
#include "bst.h"
#include "avl.h"
#include "tbt.h"
#include "loader.h"
#include "autocomplete.h"
#include "benchmark.h"

/* ── Forward declarations ────────────────────────────────────── */
static void menu_search_word(void);
static void menu_insert_word(void);
static void menu_delete_word(void);
static void menu_autocomplete(void);
static void menu_display_all(void);
static void menu_load_from_file(void);
static void menu_switch_tree(void);
static void menu_benchmark(void);
static void menu_about(void);

/* ── Global tree state ───────────────────────────────────────── */
static BSTNode *g_bst_root   = NULL;
static AVLNode *g_avl_root   = NULL;
static TBTNode *g_tbt_header = NULL;
static int      g_active_tree = 1;    /* 1=BST, 2=AVL, 3=TBT */
static int      g_word_count  = 0;

static const char *active_tree_name(void) {
    if (g_active_tree == 2) return "AVL";
    if (g_active_tree == 3) return "TBT";
    return "BST";
}

/* ── Test dataset ────────────────────────────────────────────── */
typedef struct { const char *word; const char *meaning; const char *pos; int freq; } TestEntry;

static const TestEntry TEST_WORDS[] = {
    {"mango",    "A tropical fruit with sweet orange flesh",        "noun", 85},
    {"apple",    "A round fruit, typically red or green",           "noun", 95},
    {"zebra",    "An African mammal with black and white stripes",  "noun", 40},
    {"cat",      "A small domesticated carnivorous mammal",         "noun", 90},
    {"dog",      "A domesticated carnivorous mammal",               "noun", 92},
    {"banana",   "A long curved yellow tropical fruit",             "noun", 88},
    {"orange",   "A citrus fruit with bright orange skin",          "noun", 83},
    {"grape",    "A small sweet berry growing in clusters",         "noun", 70},
    {"kite",     "A toy flown in the wind on a long string",        "noun", 55},
    {"igloo",    "A dome-shaped shelter made of ice blocks",        "noun", 35},
    {"lemon",    "A sour yellow citrus fruit",                      "noun", 75},
    {"notebook", "A book with blank pages for writing notes",       "noun", 60},
    {"jungle",   "A tropical forest with dense vegetation",         "noun", 50},
    {"fish",     "A cold-blooded aquatic vertebrate animal",        "noun", 80},
    {"elephant", "The largest land animal, with a long trunk",      "noun", 65}
};

static void load_test_data(void) {
    int i;
    int n = (int)(sizeof(TEST_WORDS) / sizeof(TEST_WORDS[0]));
    WordRecord rec;

    /* Free and reinitialise all three trees */
    bst_free(&g_bst_root);
    avl_free(&g_avl_root);
    tbt_free(&g_tbt_header);
    g_tbt_header = tbt_create_header();

    for (i = 0; i < n; i++) {
        word_record_init(&rec);
        str_safe_copy(rec.word,           TEST_WORDS[i].word,    sizeof(rec.word));
        str_safe_copy(rec.meaning,        TEST_WORDS[i].meaning, sizeof(rec.meaning));
        str_safe_copy(rec.part_of_speech, TEST_WORDS[i].pos,     sizeof(rec.part_of_speech));
        rec.frequency_score = TEST_WORDS[i].freq;
        bst_insert(&g_bst_root, &rec);
        g_avl_root = avl_insert(g_avl_root, &rec);
        tbt_insert(g_tbt_header, &rec);
    }

    g_word_count = bst_count(g_bst_root);
    printf("  Loaded %d test words into BST / AVL / TBT.\n", g_word_count);
    printf("  BST height : %d  |  AVL height : %d\n",
           bst_height(g_bst_root), avl_height(g_avl_root));
}

/* ── Display callbacks for each tree ────────────────────────── */
static void bst_print_row(BSTNode *node, void *arg) {
    int *counter = (int *)arg;
    (*counter)++;
    printf("  %3d. %-22s  %-13s  freq=%d\n",
           *counter,
           node->data.word,
           node->data.part_of_speech[0] ? node->data.part_of_speech : "-",
           node->data.frequency_score);
}

static void avl_print_row(AVLNode *node, void *arg) {
    int *counter = (int *)arg;
    (*counter)++;
    printf("  %3d. %-22s  %-13s  freq=%d\n",
           *counter,
           node->data.word,
           node->data.part_of_speech[0] ? node->data.part_of_speech : "-",
           node->data.frequency_score);
}

static void tbt_print_row(TBTNode *node, void *arg) {
    int *counter = (int *)arg;
    (*counter)++;
    printf("  %3d. %-22s  %-13s  freq=%d\n",
           *counter,
           node->data.word,
           node->data.part_of_speech[0] ? node->data.part_of_speech : "-",
           node->data.frequency_score);
}

/* ── Main entry point ────────────────────────────────────────── */
int main(void) {
    char input[MAX_INPUT_BUF];
    int  choice;
    int  running = 1;

    /* Initialise TBT header sentinel before any inserts */
    g_tbt_header = tbt_create_header();

    print_header();

    /* ── Auto-load previous session or fresh dictionary ── */
    {
        int n, m;
        /* Try custom_words.txt first (has freq + picks from last session) */
        n = load_words(FILE_CUSTOM_WORDS, &g_bst_root, &g_avl_root, g_tbt_header);
        if (n > 0) {
            /* Also refresh frequencies from canonical source */
            m = load_frequencies(FILE_WORD_FREQ, g_bst_root, g_avl_root, g_tbt_header);
            g_word_count = bst_count(g_bst_root);
            printf("\n  Session restored: %d words from %s", n, FILE_CUSTOM_WORDS);
            if (m >= 0) printf("  (+%d freq updates)", m);
            printf("\n  BST height: %d  |  AVL height: %d\n",
                   bst_height(g_bst_root), avl_height(g_avl_root));
        } else {
            /* First run — load from canonical words.txt */
            n = load_words(FILE_WORDS, &g_bst_root, &g_avl_root, g_tbt_header);
            if (n > 0) {
                m = load_frequencies(FILE_WORD_FREQ, g_bst_root, g_avl_root, g_tbt_header);
                g_word_count = bst_count(g_bst_root);
                printf("\n  Loaded %d words from %s", n, FILE_WORDS);
                if (m >= 0) printf("  (+%d freq updates)", m);
                printf("\n  BST height: %d  |  AVL height: %d\n",
                       bst_height(g_bst_root), avl_height(g_avl_root));
            } else {
                printf("\n  No dictionary file found. Use option 6 to load words.\n");
            }
        }
    }

    while (running) {
        printf("\n");
        print_separator('-', 60);
        printf(" MAIN MENU  [Active tree: %s]  [Words: %d]\n",
               active_tree_name(), g_word_count);
        print_separator('-', 60);
        printf("  1. Search for a word\n");
        printf("  2. Insert a new word\n");
        printf("  3. Delete a word\n");
        printf("  4. Autocomplete (prefix search)\n");
        printf("  5. Display all words (sorted)\n");
        printf("  6. Load dictionary (file / test data)\n");
        printf("  7. Switch active tree structure\n");
        printf("  8. Run benchmark comparison\n");
        printf("  9. About this application\n");
        printf("  0. Exit\n");
        print_separator('-', 60);
        printf("Enter choice: ");

        if (!input_read_line(input, sizeof(input))) break;
        choice = atoi(input);

        switch (choice) {
            case 1:  menu_search_word();    break;
            case 2:  menu_insert_word();    break;
            case 3:  menu_delete_word();    break;
            case 4:  menu_autocomplete();   break;
            case 5:  menu_display_all();    break;
            case 6:  menu_load_from_file(); break;
            case 7:  menu_switch_tree();    break;
            case 8:  menu_benchmark();      break;
            case 9:  menu_about();          break;
            case 0:  running = 0;           break;
            default:
                printf("  Invalid choice. Enter a number 0-9.\n");
        }
    }

    /* Save current dictionary state before exit */
    if (g_bst_root) {
        if (save_custom_words(FILE_CUSTOM_WORDS, g_bst_root) == 0)
            printf("\n  Dictionary saved to %s\n", FILE_CUSTOM_WORDS);
        else
            printf("\n  Warning: could not save to %s\n", FILE_CUSTOM_WORDS);
    }

    /* Free all three trees */
    avl_free(&g_avl_root);
    tbt_free(&g_tbt_header);
    bst_free(&g_bst_root);

    printf("Exiting Smart Dictionary. Goodbye.\n");
    return 0;
}

/* ── Menu handlers ───────────────────────────────────────────── */

static void menu_search_word(void) {
    char     word[MAX_WORD_LEN];
    BSTNode *bst_result;
    AVLNode *avl_result;
    TBTNode *tbt_result;

    printf("\n-- Search Word --\n");
    printf("Enter word to search: ");
    input_read_line(word, sizeof(word));
    if (str_is_empty(word)) { printf("  No input provided.\n"); return; }

    if (g_word_count == 0) {
        printf("  Dictionary is empty. Load words first (option 6).\n");
        return;
    }

    if (g_active_tree == 2) {
        avl_result = avl_search(g_avl_root, word);
        if (avl_result) {
            printf("  Found (AVL):\n");
            print_separator('-', 40);
            word_record_print(&avl_result->data);
            print_separator('-', 40);
        } else {
            printf("  Word '%s' not found in AVL.\n", word);
        }
    } else if (g_active_tree == 3) {
        tbt_result = tbt_search(g_tbt_header, word);
        if (tbt_result) {
            printf("  Found (TBT):\n");
            print_separator('-', 40);
            word_record_print(&tbt_result->data);
            print_separator('-', 40);
        } else {
            printf("  Word '%s' not found in TBT.\n", word);
        }
    } else {
        bst_result = bst_search(g_bst_root, word);
        if (bst_result) {
            printf("  Found (BST):\n");
            print_separator('-', 40);
            word_record_print(&bst_result->data);
            print_separator('-', 40);
        } else {
            printf("  Word '%s' not found in BST.\n", word);
        }
    }
}

static void menu_insert_word(void) {
    WordRecord rec;
    char       word[MAX_WORD_LEN];
    char       meaning[MAX_MEANING_LEN];
    char       pos[MAX_POS_LEN];
    int        prev_count;

    printf("\n-- Insert Word --\n");

    printf("Enter word (required): ");
    input_read_line(word, sizeof(word));
    if (str_is_empty(word)) { printf("  Word cannot be empty. Aborted.\n"); return; }

    printf("Enter meaning (Enter to skip): ");
    input_read_line(meaning, sizeof(meaning));

    printf("Enter part of speech (Enter to skip): ");
    input_read_line(pos, sizeof(pos));

    word_record_init(&rec);
    str_safe_copy(rec.word,           word,    sizeof(rec.word));
    str_safe_copy(rec.meaning,        meaning, sizeof(rec.meaning));
    str_safe_copy(rec.part_of_speech, pos,     sizeof(rec.part_of_speech));
    rec.frequency_score = FREQ_SCORE_DEFAULT;

    prev_count = g_word_count;
    bst_insert(&g_bst_root, &rec);
    g_avl_root = avl_insert(g_avl_root, &rec);
    tbt_insert(g_tbt_header, &rec);
    g_word_count = bst_count(g_bst_root);

    if (g_word_count > prev_count) {
        printf("  Inserted '%s' into BST / AVL / TBT. Total words: %d\n",
               word, g_word_count);
    } else {
        printf("  Word '%s' already exists (duplicate skipped).\n", word);
    }
}

static void menu_delete_word(void) {
    char word[MAX_WORD_LEN];
    int  prev_count;

    printf("\n-- Delete Word --\n");

    if (g_word_count == 0) {
        printf("  Dictionary is empty. Nothing to delete.\n");
        return;
    }

    printf("Enter word to delete: ");
    input_read_line(word, sizeof(word));
    if (str_is_empty(word)) { printf("  No input provided.\n"); return; }

    prev_count = g_word_count;
    bst_delete(&g_bst_root, word);
    g_avl_root = avl_delete(g_avl_root, word);
    tbt_delete(g_tbt_header, word);
    g_word_count = bst_count(g_bst_root);

    if (g_word_count < prev_count) {
        printf("  Deleted '%s' from BST / AVL / TBT. Total words: %d\n",
               word, g_word_count);
    } else {
        printf("  Word '%s' not found.\n", word);
    }
}

static void menu_autocomplete(void) {
    char       prefix[MAX_WORD_LEN];
    char       sel[MAX_INPUT_BUF];
    WordRecord results[TOP_K_DEFAULT];
    int        n, i, choice;

    printf("\n-- Autocomplete --\n");
    printf("Enter prefix: ");
    input_read_line(prefix, sizeof(prefix));
    if (str_is_empty(prefix)) { printf("  No prefix provided.\n"); return; }

    if (g_word_count == 0) {
        printf("  Dictionary is empty. Load words first (option 6).\n");
        return;
    }

    /* Dispatch to the active tree's autocomplete function */
    if (g_active_tree == 2)
        n = autocomplete_avl(g_avl_root,   prefix, results, TOP_K_DEFAULT);
    else if (g_active_tree == 3)
        n = autocomplete_tbt(g_tbt_header, prefix, results, TOP_K_DEFAULT);
    else
        n = autocomplete_bst(g_bst_root,   prefix, results, TOP_K_DEFAULT);

    if (n == 0) {
        printf("  No words found matching '%s'.\n", prefix);
        return;
    }

    /* Display numbered suggestions */
    printf("\n  Results for \"%s\" [%s]  (%d match%s):\n",
           prefix, active_tree_name(), n, n == 1 ? "" : "es");
    print_separator('-', 54);
    for (i = 0; i < n; i++) {
        printf("  %2d. %-22s  %-12s  score=%-5d  picks=%d\n",
               i + 1,
               results[i].word,
               results[i].part_of_speech[0] ? results[i].part_of_speech : "-",
               results[i].frequency_score + 10 * results[i].user_select_count,
               results[i].user_select_count);
    }
    print_separator('-', 54);

    /* Let the user select a suggestion to record their pick */
    printf("  Select word (1-%d, or 0 to skip): ", n);
    input_read_line(sel, sizeof(sel));
    choice = atoi(sel);

    if (choice >= 1 && choice <= n) {
        autocomplete_record_selection(results[choice - 1].word,
                                      g_bst_root, g_avl_root, g_tbt_header);
        printf("  Recorded: '%s'  (picks now %d)\n",
               results[choice - 1].word,
               results[choice - 1].user_select_count + 1);
    } else {
        printf("  Skipped.\n");
    }
}

static void menu_display_all(void) {
    int counter = 0;

    printf("\n-- Display All Words (Sorted In-Order) --\n");

    if (g_word_count == 0) {
        printf("  Dictionary is empty. Load words first (option 6).\n");
        return;
    }

    printf("  %-3s  %-22s  %-13s  %s\n", "No.", "Word", "Part of Speech", "Freq");
    print_separator('-', 58);

    if (g_active_tree == 2) {
        avl_inorder(g_avl_root, avl_print_row, &counter);
        print_separator('-', 58);
        printf("  Total: %d words  |  AVL height: %d\n",
               avl_count(g_avl_root), avl_height(g_avl_root));
    } else if (g_active_tree == 3) {
        tbt_inorder(g_tbt_header, tbt_print_row, &counter);
        print_separator('-', 58);
        printf("  Total: %d words  (TBT iterative, no stack)\n",
               tbt_count(g_tbt_header));
    } else {
        bst_inorder(g_bst_root, bst_print_row, &counter);
        print_separator('-', 58);
        printf("  Total: %d words  |  BST height: %d\n",
               g_word_count, bst_height(g_bst_root));
    }
}

static void menu_load_from_file(void) {
    char input[MAX_INPUT_BUF];
    int  n, m;

    printf("\n-- Load Dictionary --\n");

    if (g_word_count > 0) {
        printf("  Dictionary already has %d words. Reload? (y/n): ",
               g_word_count);
        input_read_line(input, sizeof(input));
        if (input[0] != 'y' && input[0] != 'Y') {
            printf("  Load cancelled.\n");
            return;
        }
        /* Free all trees before reloading */
        bst_free(&g_bst_root);
        avl_free(&g_avl_root);
        tbt_free(&g_tbt_header);
        g_tbt_header = tbt_create_header();
        g_word_count = 0;
    }

    /* Try real file first */
    n = load_words(FILE_WORDS, &g_bst_root, &g_avl_root, g_tbt_header);
    if (n < 0) {
        printf("  '%s' not found — loading 15 hardcoded test words instead.\n",
               FILE_WORDS);
        load_test_data();
        return;
    }
    printf("  Loaded %d words from %s\n", n, FILE_WORDS);

    /* Optionally enrich with frequency scores */
    m = load_frequencies(FILE_WORD_FREQ, g_bst_root, g_avl_root, g_tbt_header);
    if (m >= 0)
        printf("  Updated %d frequency scores from %s\n", m, FILE_WORD_FREQ);

    g_word_count = bst_count(g_bst_root);
    printf("  BST height  : %d  |  AVL height : %d\n",
           bst_height(g_bst_root), avl_height(g_avl_root));
    printf("  BST/AVL/TBT : %d / %d / %d words\n",
           bst_count(g_bst_root), avl_count(g_avl_root),
           tbt_count(g_tbt_header));
}

static void menu_switch_tree(void) {
    char input[MAX_INPUT_BUF];
    int  choice;
    printf("\n-- Switch Active Tree Structure --\n");
    printf("  1. BST  (Binary Search Tree)           - O(log n) avg\n");
    printf("  2. AVL  (Self-Balancing BST)            - O(log n) guaranteed\n");
    printf("  3. TBT  (Threaded Binary Tree)          - stack-free traversal\n");
    printf("Select tree (1-3): ");
    input_read_line(input, sizeof(input));
    choice = atoi(input);
    if (choice >= 1 && choice <= 3) {
        g_active_tree = choice;
        printf("  Active tree switched to: %s\n", active_tree_name());
    } else {
        printf("  Invalid selection. Enter 1, 2, or 3.\n");
    }
}

static void menu_benchmark(void) {
    printf("\n-- Benchmark Comparison --\n");
    printf("  Building fresh trees from synthetic data. Please wait...\n");
    benchmark_run_all();
}

static void menu_about(void) {
    printf("\n");
    print_separator('=', 60);
    printf("  %s\n",       APP_NAME);
    printf("  Version     : %s\n", APP_VERSION);
    printf("  Language    : C99  |  Compiler: gcc (MinGW-w64)\n");
    printf("  Platform    : Windows  |  Build flags: -Wall -Wextra\n");
    print_separator('-', 60);
    printf("  Features:\n");
    printf("    BST    Binary Search Tree         insert/search/delete\n");
    printf("    AVL    Self-Balancing BST          guaranteed O(log n)\n");
    printf("    TBT    Threaded Binary Tree        stack-free traversal\n");
    printf("    AC     Prefix autocomplete         BST-pruned + TBT iter\n");
    printf("    BENCH  Performance benchmark       timed on 500-5000 words\n");
    print_separator('-', 60);
    printf("  Persistence:\n");
    printf("    Loads  %s  (canonical words + meanings)\n", FILE_WORDS);
    printf("    Saves  %s  (freq + picks preserved)\n", FILE_CUSTOM_WORDS);
    printf("    Freqs  %s\n", FILE_WORD_FREQ);
    print_separator('-', 60);
    printf("  Academic project: Advanced Data Structures\n");
    printf("  Comparative tree-based dictionary & autocomplete engine\n");
    print_separator('=', 60);
    printf("\n");
}
