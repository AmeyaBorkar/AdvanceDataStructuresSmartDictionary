/* utils.c - Portable string utilities and console I/O helpers */
#include <stdio.h>
#include <string.h>
#include "utils.h"
#include "config.h"

/* ── Case normalisation ──────────────────────────────────────── */

char *str_tolower(char *dst, const char *src, size_t dst_size) {
    size_t i = 0;
    if (!dst || !src || dst_size == 0) return dst;
    for (i = 0; i < dst_size - 1 && src[i] != '\0'; i++) {
        /* ASCII-only: avoids locale-dependent ctype tolower() */
        dst[i] = (src[i] >= 'A' && src[i] <= 'Z')
                     ? (char)(src[i] + 32)
                     : src[i];
    }
    dst[i] = '\0';
    return dst;
}

char *str_toupper(char *dst, const char *src, size_t dst_size) {
    size_t i = 0;
    if (!dst || !src || dst_size == 0) return dst;
    for (i = 0; i < dst_size - 1 && src[i] != '\0'; i++) {
        dst[i] = (src[i] >= 'a' && src[i] <= 'z')
                     ? (char)(src[i] - 32)
                     : src[i];
    }
    dst[i] = '\0';
    return dst;
}

/* ── Whitespace trimming ─────────────────────────────────────── */

char *str_trim_right(char *str) {
    int len;
    if (!str) return str;
    len = (int)strlen(str);
    while (len > 0 && (str[len - 1] == ' '  || str[len - 1] == '\t' ||
                       str[len - 1] == '\r' || str[len - 1] == '\n')) {
        str[--len] = '\0';
    }
    return str;
}

char *str_trim_left(char *str) {
    int offset = 0;
    if (!str) return str;
    while (str[offset] == ' '  || str[offset] == '\t' ||
           str[offset] == '\r' || str[offset] == '\n') {
        offset++;
    }
    if (offset > 0) {
        /* memmove handles overlapping regions correctly */
        memmove(str, str + offset, strlen(str + offset) + 1);
    }
    return str;
}

char *str_trim(char *str) {
    if (!str) return str;
    str_trim_right(str);
    str_trim_left(str);
    return str;
}

/* ── Safe string copy ────────────────────────────────────────── */

size_t str_safe_copy(char *dst, const char *src, size_t dst_size) {
    size_t i = 0;
    if (!dst || dst_size == 0) return 0;
    if (!src) { dst[0] = '\0'; return 0; }
    for (i = 0; i < dst_size - 1 && src[i] != '\0'; i++) {
        dst[i] = src[i];
    }
    dst[i] = '\0';
    return i;  /* number of chars written (excl. NUL); equals dst_size-1 if truncated */
}

/* ── String predicates ───────────────────────────────────────── */

int str_starts_with(const char *str, const char *prefix) {
    if (!str || !prefix) return 0;
    while (*prefix) {
        if (*str != *prefix) return 0;
        str++;
        prefix++;
    }
    return 1;
}

int str_starts_with_ci(const char *str, const char *prefix) {
    char s_buf[MAX_WORD_LEN];
    char p_buf[MAX_WORD_LEN];
    if (!str || !prefix) return 0;
    str_tolower(s_buf, str,    sizeof(s_buf));
    str_tolower(p_buf, prefix, sizeof(p_buf));
    return str_starts_with(s_buf, p_buf);
}

int str_is_empty(const char *str) {
    return (str == NULL || str[0] == '\0');
}

/* ── Console I/O helpers ─────────────────────────────────────── */

void input_flush_stdin(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
        ;
}

char *input_read_line(char *buf, size_t buf_size) {
    if (!fgets(buf, (int)buf_size, stdin)) return NULL;
    str_trim(buf);
    return buf;
}

/* ── Display helpers ─────────────────────────────────────────── */

void print_separator(char ch, int width) {
    int i;
    for (i = 0; i < width; i++) putchar(ch);
    putchar('\n');
}

void print_header(void) {
    print_separator('=', 60);
    printf("  %s  v%s\n", APP_NAME, APP_VERSION);
    print_separator('=', 60);
}
