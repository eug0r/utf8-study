#ifndef UTILS_H
#define UTILS_H

#include <string.h>
#include <stdlib.h>
#include <uchar.h>

typedef char32_t codepoint;

int is_little_endian();
char32_t swap32(char32_t cp);
typedef char32_t codepoint;
int u8_byte_count(unsigned char u8c);
int u8char_to_cp(codepoint *cp, const unsigned char *u8, int bytes);
ssize_t u8_to_cp(const unsigned char *u8_str, codepoint *cp_buf, size_t bufsiz);
ssize_t cp_to_u8(const codepoint *codepoints, size_t size, unsigned char *u8_buf, size_t bufsiz);
void print_cp();
int is_forbidden_byte(unsigned char u8c);
int isvalid_u8(char *u8);
int isvalid_cp(codepoint cp);
void next_codepoint();
void prev_codepoint();

#endif //UTILS_H
