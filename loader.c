/* loader.c - File loading and persistence */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "loader.h"
#include "avl.h"
#include "tbt.h"
#include "utils.h"

/* ── save_custom_words helper ────────────────────────────────── */

/* bst_inorder callback: write one node to the FILE* passed via arg.
   Extended format: word|pos|meaning|freq|picks — lets the loader
   restore frequency_score and user_select_count across sessions. */
static void write_word_cb(BSTNode *node, void *arg) {
    FILE *fp = (FILE *)arg;
    const WordRecord *r = &node->data;
    fprintf(fp, "%s|%s|%s|%d|%d\n",
            r->word,
            r->part_of_speech,
            r->meaning,
            r->frequency_score,
            r->user_select_count);
}

/* ── Public API ──────────────────────────────────────────────── */

int load_words(const char *path, BSTNode **bst_root,
               AVLNode **avl_root, TBTNode *tbt_header) {
    FILE      *fp;
    char       line[MAX_LINE_BUF];
    char      *pipe1;
    char      *pipe2;
    WordRecord rec;
    int        count = 0;

    /* avl_root and tbt_header receive the same records as the BST */

    fp = fopen(path, "r");
    if (!fp) return -1;

    while (fgets(line, (int)sizeof(line), fp)) {
        str_trim(line);

        /* Skip blank lines and comment lines */
        if (str_is_empty(line) || line[0] == '#') continue;

        /* Reject words that are too long before any processing */
        if (strlen(line) >= MAX_WORD_LEN && strchr(line, '|') == NULL) continue;

        word_record_init(&rec);

        pipe1 = strchr(line, '|');
        if (pipe1) {
            /* Rich format: word|pos[|meaning[|freq|picks]] */
            *pipe1 = '\0';
            str_trim(line);
            str_safe_copy(rec.word, line, sizeof(rec.word));

            pipe2 = strchr(pipe1 + 1, '|');
            if (pipe2) {
                /* At least word|pos|meaning */
                char *pipe3, *pipe4;
                *pipe2 = '\0';
                str_trim(pipe1 + 1);
                str_safe_copy(rec.part_of_speech, pipe1 + 1, sizeof(rec.part_of_speech));

                /* Check for extended format: word|pos|meaning|freq|picks */
                pipe3 = strchr(pipe2 + 1, '|');
                if (pipe3) {
                    /* meaning ends at pipe3 */
                    *pipe3 = '\0';
                    str_trim(pipe2 + 1);
                    str_safe_copy(rec.meaning, pipe2 + 1, sizeof(rec.meaning));

                    pipe4 = strchr(pipe3 + 1, '|');
                    if (pipe4) {
                        int f, p;
                        *pipe4 = '\0';
                        str_trim(pipe3 + 1);   /* freq field */
                        str_trim(pipe4 + 1);   /* picks field */
                        f = atoi(pipe3 + 1);
                        p = atoi(pipe4 + 1);
                        if (f > 0) rec.frequency_score   = f;
                        if (p > 0) rec.user_select_count = p;
                    }
                } else {
                    /* Plain 3-field: word|pos|meaning */
                    str_trim(pipe2 + 1);
                    str_safe_copy(rec.meaning, pipe2 + 1, sizeof(rec.meaning));
                }
            } else {
                /* word|pos (no meaning) */
                str_trim(pipe1 + 1);
                str_safe_copy(rec.part_of_speech, pipe1 + 1, sizeof(rec.part_of_speech));
            }
        } else {
            /* Simple format: word only */
            str_safe_copy(rec.word, line, sizeof(rec.word));
        }

        /* Skip if word field is empty after trimming */
        if (str_is_empty(rec.word)) continue;

        /* Skip oversized words (word field was truncated to MAX_WORD_LEN-1) */
        if (strlen(rec.word) >= MAX_WORD_LEN - 1 &&
            strlen(line) >= MAX_WORD_LEN - 1) continue;

        bst_insert(bst_root, &rec);          /* normalises to lowercase internally */
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

    /* avl_root and tbt_header frequency fields are updated alongside BST */

    fp = fopen(path, "r");
    if (!fp) return -1;

    while (fgets(line, (int)sizeof(line), fp)) {
        str_trim(line);

        /* Skip blank lines and comment lines */
        if (str_is_empty(line) || line[0] == '#') continue;

        comma = strchr(line, ',');
        if (!comma) continue;   /* malformed line — skip */

        *comma = '\0';
        str_trim(line);        /* word */
        str_trim(comma + 1);   /* score string */

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

int save_custom_words(const char *path, BSTNode *bst_root) {
    FILE *fp;

    fp = fopen(path, "w");
    if (!fp) return -1;

    bst_inorder(bst_root, write_word_cb, fp);

    fclose(fp);
    return 0;
}
