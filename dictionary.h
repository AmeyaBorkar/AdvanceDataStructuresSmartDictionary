/* dictionary.h - Core WordRecord data model */
#ifndef DICTIONARY_H
#define DICTIONARY_H

#include "config.h"

/*
 * WordRecord - the data payload for every dictionary entry.
 *
 * Fixed-size char arrays (not char*) keep each node self-contained:
 * one malloc per node, one free per node, no dangling pointers.
 *
 * Layout (~616 bytes per record):
 *   char word[64]            - 64 bytes  (primary key, lowercase-normalised)
 *   char meaning[512]        - 512 bytes (human-readable definition)
 *   char part_of_speech[32]  - 32 bytes  (e.g. "noun", "verb")
 *   int  frequency_score     - 4 bytes   (corpus frequency)
 *   int  user_select_count   - 4 bytes   (incremented on user pick)
 */
typedef struct WordRecord {
    char word[MAX_WORD_LEN];
    char meaning[MAX_MEANING_LEN];
    char part_of_speech[MAX_POS_LEN];
    int  frequency_score;
    int  user_select_count;
} WordRecord;

/* Initialise all fields to safe empty state (zeroed strings, freq = FREQ_SCORE_DEFAULT). */
void word_record_init(WordRecord *rec);

/* Print a single WordRecord to stdout in a formatted block. */
void word_record_print(const WordRecord *rec);

/*
 * Compare two WordRecord instances by word field (case-insensitive, lexicographic).
 * Returns negative / zero / positive like strcmp.
 * Used as the BST/AVL ordering predicate.
 */
int word_record_compare(const WordRecord *a, const WordRecord *b);

#endif /* DICTIONARY_H */
