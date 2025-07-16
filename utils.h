#ifndef UTF8UTILS_H
#define UTF8UTILS_H

#include <string.h>
#include <stdlib.h>
#include <uchar.h>

typedef char32_t codepoint;

int is_little_endian();
char32_t swap32(char32_t cp);

int u8_byte_count(unsigned char u8c);
ssize_t u8_to_cp(const unsigned char *u8_str, codepoint *cp_buf, size_t bufsiz);
codepoint u8char_to_cp(const unsigned char *u8, int bytes);
ssize_t cp_to_u8(const codepoint *codepoints, size_t size, unsigned char *u8_buf, size_t bufsiz);

int isvalid_u8(const unsigned char *u8_str, int allow_surrogate);
int isvalid_cp(codepoint cp);

void next_codepoint(unsigned char **u8);
void prev_codepoint(unsigned char **u8, const unsigned char *buf_start);
void curr_codepoint(unsigned char **u8, const unsigned char *buf_start);
size_t u8_strlen(const unsigned char *u8);
#endif //UTF8UTILS_H
