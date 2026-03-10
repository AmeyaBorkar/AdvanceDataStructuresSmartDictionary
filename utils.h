/* utils.h - Portable string utilities and console I/O helpers */
#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>   /* size_t */
#include "config.h"

/* ── Case normalisation ──────────────────────────────────────── */

/* Convert src to lowercase (ASCII-only), write result into dst[dst_size]. Returns dst. */
char *str_tolower(char *dst, const char *src, size_t dst_size);

/* Convert src to uppercase (ASCII-only), write result into dst[dst_size]. Returns dst. */
char *str_toupper(char *dst, const char *src, size_t dst_size);

/* ── Whitespace trimming (in-place) ─────────────────────────── */

/* Remove leading and trailing whitespace (space, tab, \r, \n) from str. Returns str. */
char *str_trim(char *str);

/* Remove only leading whitespace in-place. Returns str. */
char *str_trim_left(char *str);

/* Remove only trailing whitespace in-place. Returns str. */
char *str_trim_right(char *str);

/* ── Safe string copy ────────────────────────────────────────── */

/*
 * Copy at most (dst_size - 1) chars from src to dst, always NUL-terminating.
 * Behaves like BSD strlcpy. Returns number of chars written (excl. NUL).
 * If return value == dst_size-1, truncation occurred.
 */
size_t str_safe_copy(char *dst, const char *src, size_t dst_size);

/* ── String predicates ───────────────────────────────────────── */

/* Return 1 if str begins with prefix (case-sensitive), 0 otherwise. */
int str_starts_with(const char *str, const char *prefix);

/* Return 1 if str begins with prefix (case-insensitive), 0 otherwise. */
int str_starts_with_ci(const char *str, const char *prefix);

/* Return 1 if str is NULL or empty string, 0 otherwise. */
int str_is_empty(const char *str);

/* ── Console I/O helpers ─────────────────────────────────────── */

/* Consume and discard all chars up to and including the next newline in stdin. */
void input_flush_stdin(void);

/*
 * Read one line from stdin into buf (max buf_size chars), trim whitespace.
 * Returns buf on success, NULL on EOF or error.
 */
char *input_read_line(char *buf, size_t buf_size);

/* ── Display helpers ─────────────────────────────────────────── */

/* Print a horizontal rule of 'width' repeated 'ch' characters, then newline. */
void print_separator(char ch, int width);

/* Print APP_NAME and APP_VERSION as a formatted banner. */
void print_header(void);

#endif /* UTILS_H */
