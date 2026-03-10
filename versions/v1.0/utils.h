/* utils.h - Portable string utilities and console I/O helpers */
#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include "config.h"

char *str_tolower(char *dst, const char *src, size_t dst_size);
char *str_toupper(char *dst, const char *src, size_t dst_size);
char *str_trim(char *str);
char *str_trim_left(char *str);
char *str_trim_right(char *str);
size_t str_safe_copy(char *dst, const char *src, size_t dst_size);
int str_starts_with(const char *str, const char *prefix);
int str_starts_with_ci(const char *str, const char *prefix);
int str_is_empty(const char *str);
void input_flush_stdin(void);
char *input_read_line(char *buf, size_t buf_size);
void print_separator(char ch, int width);
void print_header(void);

#endif /* UTILS_H */
