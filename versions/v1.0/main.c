/* main.c - Smart Dictionary v1.0: BST-only console menu */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "dictionary.h"
#include "utils.h"
#include "bst.h"
#include "loader.h"

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
static int      g_word_count  = 0;

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

    bst_free(&g_bst_root);

    for (i = 0; i < n; i++) {
        word_record_init(&rec);
        str_safe_copy(rec.word,           TEST_WORDS[i].word,    sizeof(rec.word));
        str_safe_copy(rec.meaning,        TEST_WORDS[i].meaning, sizeof(rec.meaning));
        str_safe_copy(rec.part_of_speech, TEST_WORDS[i].pos,     sizeof(rec.part_of_speech));
        rec.frequency_score = TEST_WORDS[i].freq;
        bst_insert(&g_bst_root, &rec);
    }

    g_word_count = bst_count(g_bst_root);
    printf("  Loaded %d test words into BST.\n", g_word_count);
    printf("  BST height : %d\n", bst_height(g_bst_root));
}

/* ── Display callback ────────────────────────────────────────── */
static void bst_print_row(BSTNode *node, void *arg) {
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

    print_header();

    /* ── Auto-load dictionary ── */
    {
        int n, m;
        n = load_words(FILE_WORDS, &g_bst_root);
        if (n > 0) {
            m = load_frequencies(FILE_WORD_FREQ, g_bst_root);
            g_word_count = bst_count(g_bst_root);
            printf("\n  Loaded %d words from %s", n, FILE_WORDS);
            if (m >= 0) printf("  (+%d freq updates)", m);
            printf("\n  BST height: %d\n", bst_height(g_bst_root));
        } else {
            printf("\n  No dictionary file found. Use option 6 to load words.\n");
        }
    }

    while (running) {
        printf("\n");
        print_separator('-', 60);
        printf(" MAIN MENU  [Active tree: BST]  [Words: %d]\n", g_word_count);
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

    bst_free(&g_bst_root);
    printf("Exiting Smart Dictionary. Goodbye.\n");
    return 0;
}

/* ── Menu handlers ───────────────────────────────────────────── */

static void menu_search_word(void) {
    char     word[MAX_WORD_LEN];
    BSTNode *result;

    printf("\n-- Search Word --\n");
    printf("Enter word to search: ");
    input_read_line(word, sizeof(word));
    if (str_is_empty(word)) { printf("  No input provided.\n"); return; }

    if (g_word_count == 0) {
        printf("  Dictionary is empty. Load words first (option 6).\n");
        return;
    }

    result = bst_search(g_bst_root, word);
    if (result) {
        printf("  Found (BST):\n");
        print_separator('-', 40);
        word_record_print(&result->data);
        print_separator('-', 40);
    } else {
        printf("  Word '%s' not found in BST.\n", word);
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
    g_word_count = bst_count(g_bst_root);

    if (g_word_count > prev_count) {
        printf("  Inserted '%s' into BST. Total words: %d\n",
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
    g_word_count = bst_count(g_bst_root);

    if (g_word_count < prev_count) {
        printf("  Deleted '%s' from BST. Total words: %d\n",
               word, g_word_count);
    } else {
        printf("  Word '%s' not found.\n", word);
    }
}

static void menu_autocomplete(void) {
    printf("\n-- Autocomplete --\n");
    printf("  [Not yet implemented — coming in v2.0]\n");
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
    bst_inorder(g_bst_root, bst_print_row, &counter);
    print_separator('-', 58);
    printf("  Total: %d words  |  BST height: %d\n",
           g_word_count, bst_height(g_bst_root));
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
        bst_free(&g_bst_root);
        g_word_count = 0;
    }

    n = load_words(FILE_WORDS, &g_bst_root);
    if (n < 0) {
        printf("  '%s' not found — loading 15 hardcoded test words instead.\n",
               FILE_WORDS);
        load_test_data();
        return;
    }
    printf("  Loaded %d words from %s\n", n, FILE_WORDS);

    m = load_frequencies(FILE_WORD_FREQ, g_bst_root);
    if (m >= 0)
        printf("  Updated %d frequency scores from %s\n", m, FILE_WORD_FREQ);

    g_word_count = bst_count(g_bst_root);
    printf("  BST height: %d\n", bst_height(g_bst_root));
}

static void menu_switch_tree(void) {
    printf("\n-- Switch Active Tree Structure --\n");
    printf("  [Not yet implemented — coming in v2.0]\n");
    printf("  Currently only BST is available.\n");
}

static void menu_benchmark(void) {
    printf("\n-- Benchmark Comparison --\n");
    printf("  [Not yet implemented — coming in v3.0]\n");
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
    printf("    File   Dictionary loader           words.txt + freq\n");
    print_separator('-', 60);
    printf("  Data:\n");
    printf("    Loads  %s  (canonical words + meanings)\n", FILE_WORDS);
    printf("    Freqs  %s\n", FILE_WORD_FREQ);
    print_separator('-', 60);
    printf("  Academic project: Advanced Data Structures\n");
    printf("  Comparative tree-based dictionary & autocomplete engine\n");
    print_separator('=', 60);
    printf("\n");
}
