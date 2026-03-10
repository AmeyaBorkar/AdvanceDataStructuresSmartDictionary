/* config.h - Global constants for Smart Dictionary & Autocomplete Engine */
#ifndef CONFIG_H
#define CONFIG_H

#define MAX_WORD_LEN      64
#define MAX_MEANING_LEN   512
#define MAX_POS_LEN       32
#define MAX_INPUT_BUF     128
#define MAX_LINE_BUF      1024

#define MAX_WORDS         100000
#define TOP_K_DEFAULT     10
#define TOP_K_MAX         50

#define FREQ_SCORE_DEFAULT   1
#define FREQ_SCORE_MAX    100000

#define DATA_DIR             "data"
#define FILE_WORDS           "data/words.txt"
#define FILE_WORD_FREQ       "data/word_freq.txt"

#define APP_NAME    "Smart Dictionary & Autocomplete Engine"
#define APP_VERSION "2.0.0"

#endif /* CONFIG_H */
