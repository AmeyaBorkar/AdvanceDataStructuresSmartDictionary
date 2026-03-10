/* loader.c - File loading */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "loader.h"
#include "utils.h"

int load_words(const char *path, BSTNode **bst_root,
               AVLNode **avl_root, TBTNode *tbt_header) {
    FILE      *fp;
    char       line[MAX_LINE_BUF];
    char      *pipe1;
    char      *pipe2;
    WordRecord rec;
    int        count = 0;

    fp = fopen(path, "r");
    if (!fp) return -1;

    while (fgets(line, (int)sizeof(line), fp)) {
        str_trim(line);

        if (str_is_empty(line) || line[0] == '#') continue;
        if (strlen(line) >= MAX_WORD_LEN && strchr(line, '|') == NULL) continue;

        word_record_init(&rec);

        pipe1 = strchr(line, '|');
        if (pipe1) {
            *pipe1 = '\0';
            str_trim(line);
            str_safe_copy(rec.word, line, sizeof(rec.word));

            pipe2 = strchr(pipe1 + 1, '|');
            if (pipe2) {
                *pipe2 = '\0';
                str_trim(pipe1 + 1);
                str_safe_copy(rec.part_of_speech, pipe1 + 1, sizeof(rec.part_of_speech));
                str_trim(pipe2 + 1);
                str_safe_copy(rec.meaning, pipe2 + 1, sizeof(rec.meaning));
            } else {
                str_trim(pipe1 + 1);
                str_safe_copy(rec.part_of_speech, pipe1 + 1, sizeof(rec.part_of_speech));
            }
        } else {
            str_safe_copy(rec.word, line, sizeof(rec.word));
        }

        if (str_is_empty(rec.word)) continue;

        bst_insert(bst_root, &rec);
        *avl_root = avl_insert(*avl_root, &rec);
        if (tbt_header) tbt_insert(tbt_header, &rec);
        count++;
    }

    fclose(fp);
    return count;
}

int load_frequencies(const char *path, BSTNode *bst_root,
                     AVLNode *avl_root, TBTNode *tbt_header) {
    FILE    *fp;
    char     line[MAX_LINE_BUF];
    char    *comma;
    BSTNode *node;
    int      score;
    int      updated = 0;

    fp = fopen(path, "r");
    if (!fp) return -1;

    while (fgets(line, (int)sizeof(line), fp)) {
        str_trim(line);

        if (str_is_empty(line) || line[0] == '#') continue;

        comma = strchr(line, ',');
        if (!comma) continue;

        *comma = '\0';
        str_trim(line);
        str_trim(comma + 1);

        if (str_is_empty(line) || str_is_empty(comma + 1)) continue;

        score = atoi(comma + 1);
        if (score <= 0)              score = FREQ_SCORE_DEFAULT;
        if (score > FREQ_SCORE_MAX)  score = FREQ_SCORE_MAX;

        node = bst_search(bst_root, line);
        if (node) {
            AVLNode *avl_node = avl_search(avl_root, line);
            TBTNode *tbt_node = tbt_header ? tbt_search(tbt_header, line) : NULL;
            node->data.frequency_score = score;
            if (avl_node) avl_node->data.frequency_score = score;
            if (tbt_node) tbt_node->data.frequency_score = score;
            updated++;
        }
    }

    fclose(fp);
    return updated;
}
