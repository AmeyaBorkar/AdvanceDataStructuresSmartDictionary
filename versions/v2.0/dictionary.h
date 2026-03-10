/* dictionary.h - Core WordRecord data model */
#ifndef DICTIONARY_H
#define DICTIONARY_H

#include "config.h"

typedef struct WordRecord {
    char word[MAX_WORD_LEN];
    char meaning[MAX_MEANING_LEN];
    char part_of_speech[MAX_POS_LEN];
    int  frequency_score;
    int  user_select_count;
} WordRecord;

void word_record_init(WordRecord *rec);
void word_record_print(const WordRecord *rec);
int word_record_compare(const WordRecord *a, const WordRecord *b);

#endif /* DICTIONARY_H */
