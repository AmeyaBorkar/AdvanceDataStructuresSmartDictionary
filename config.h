/* config.h - Global constants for Smart Dictionary & Autocomplete Engine */
#ifndef CONFIG_H
#define CONFIG_H

/* ── Buffer sizes ─────────────────────────────────────────── */
#define MAX_WORD_LEN      64      /* max chars in a word (incl. NUL)    */
#define MAX_MEANING_LEN   512     /* max chars in a definition           */
#define MAX_POS_LEN       32      /* max chars in part-of-speech tag     */
#define MAX_INPUT_BUF     128     /* console input buffer                */
#define MAX_LINE_BUF      1024    /* line buffer for file I/O            */

/* ── Capacity limits ──────────────────────────────────────── */
#define MAX_WORDS         100000  /* max dictionary entries in RAM       */
#define TOP_K_DEFAULT     10      /* default autocomplete results        */
#define TOP_K_MAX         50      /* ceiling for top-K config            */

/* ── Numeric defaults ─────────────────────────────────────── */
#define FREQ_SCORE_DEFAULT   1    /* assigned when corpus count = 0      */
#define FREQ_SCORE_MAX    100000  /* ceiling for normalisation           */

/* ── Data file paths (relative to executable location) ────── */
#define DATA_DIR             "data"
#define FILE_WORDS           "data/words.txt"
#define FILE_WORD_FREQ       "data/word_freq.txt"
#define FILE_CUSTOM_WORDS    "data/custom_words.txt"

/* ── Application metadata ─────────────────────────────────── */
#define APP_NAME    "Smart Dictionary & Autocomplete Engine"
#define APP_VERSION "1.0.0"

#endif /* CONFIG_H */
