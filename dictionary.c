/* dictionary.c - WordRecord utility function implementations */
#include <stdio.h>
#include <string.h>
#include "dictionary.h"
#include "utils.h"

void word_record_init(WordRecord *rec) {
    if (!rec) return;
    /* Zero all fields: empty strings, zero integers */
    memset(rec, 0, sizeof(WordRecord));
    /* Set frequency to default (1) to distinguish "no data" from "unset") */
    rec->frequency_score   = FREQ_SCORE_DEFAULT;
    rec->user_select_count = 0;
}

int word_record_compare(const WordRecord *a, const WordRecord *b) {
    char la[MAX_WORD_LEN];
    char lb[MAX_WORD_LEN];
    if (!a || !b) return 0;
    /* Lowercase both words into stack buffers before comparing */
    str_tolower(la, a->word, sizeof(la));
    str_tolower(lb, b->word, sizeof(lb));
    return strcmp(la, lb);
}

void word_record_print(const WordRecord *rec) {
    if (!rec) return;
    printf("  Word          : %s\n",  rec->word);
    if (rec->meaning[0] != '\0')
        printf("  Meaning       : %s\n",  rec->meaning);
    if (rec->part_of_speech[0] != '\0')
        printf("  Part of speech: %s\n",  rec->part_of_speech);
    printf("  Freq score    : %d\n",  rec->frequency_score);
    printf("  User picks    : %d\n",  rec->user_select_count);
}
