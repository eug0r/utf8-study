#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <uchar.h>
#include <stddef.h>

#include "utils.h"

int is_little_endian() {
    uint32_t i=0x01234567;
    // return 0 for big endian, 1 for little endian.
    return (*((unsigned char*)(&i))) == 0x67;
}
char32_t swap32(char32_t cp) {
    return  ((cp & 0x000000FF) << 24)|
            ((cp & 0x0000FF00) << 8) |
            ((cp & 0x00FF0000) >> 8) |
            ((cp & 0xFF000000) >> 24);
}

int isvalid_u8(const unsigned char *u8_str, int allow_surrogate) {
    const unsigned char *u8 = u8_str;
    while (*u8) {
        if ((*u8 & 0xC0) == 0x80 ||
            *u8 == 0xC0 ||
            *u8 == 0xC1 ||
            *u8 >= 0xF5)
            return 0; //invalid start

        int bytes = u8_byte_count(*u8);
        for (int j = 1; j < bytes; j++) {
            if ((u8[j] & 0xC0) != 0x80)
                return 0;
        } //invalid continuation
        if ((u8[0] == 0xE0 && u8[1] < 0xA0) ||
            (u8[0] == 0xF0 && u8[1] < 0x90)  ||
            (u8[0] == 0xF4 && u8[1] > 0x8F))
            return 0; //overlong 3-4 byte case or out-of-range
        if (!allow_surrogate && u8[0] == 0xED && u8[1] >= 0xA0)
            return 0; //surrogate
        u8 += bytes;
    }
    return 1;
}

int isvalid_cp(codepoint cp) {
    return cp <= 0x10FFFF && (cp < 0xD800 || cp > 0xDFFF);
}

void next_codepoint(unsigned char **u8) {
    //doesn't assume a start-byte is passed in
    unsigned char *pos = *u8;
    if (!*pos)
        return;
    while ((*++pos & 0xC0) == 0x80) {} //stops at start-byte or NULL-byte.
    *u8 = pos;
}
void prev_codepoint(unsigned char **u8, const unsigned char *buf_start) {
    unsigned char *pos = *u8;
    if (pos <= buf_start)
        return;
    while (pos > buf_start && (*pos-- & 0xC0) == 0x80){} //moves to
    //one pos behind the start of the input codepoint.
    while (pos > buf_start && (*pos & 0xC0) == 0x80) {
        pos--;
    } //moves to start of the prev codepoint
    *u8 = pos;
}
void curr_codepoint(unsigned char **u8, const unsigned char *buf_start) {
    unsigned char *pos = *u8;
    while (pos > buf_start && (*pos & 0xC0) == 0x80) {
        pos--;
    }
    *u8 = pos;
}


ssize_t u8_to_cp(const unsigned char *u8_str, codepoint *cp_buf, size_t bufsiz) {
    // u8_str must be a prevalidated, Null terminated, utf8 string.
    const unsigned char *u8 = u8_str;
    ssize_t i = 0;
    for (; *u8 && i < (bufsiz - 1); i++) {
        if ((*u8 & 0xC0) == 0x80)
            return -1; //invalid start byte, sanity-check to stop total garbage

        const int bytes = u8_byte_count(*u8);
        codepoint cp = u8char_to_cp(u8, bytes);
        cp_buf[i] = cp;
        u8 += bytes;
    }
    cp_buf[i] = 0x00; //null terminate the cp buffer
    return i; //return no of written codepoints
}

int u8_byte_count(unsigned char u8c) {
    int b0 = (u8c >> 7) & 0x01;
    int b1 = (u8c >> 6) & 0x01;
    int b2 = (u8c >> 5) & 0x01;
    int b3 = (u8c >> 4) & 0x01;
    return 1 + b0*b1 + b0*b1*b2 + b0*b1*b2*b3;
}
codepoint u8char_to_cp(const unsigned char *u8, int bytes) {
    // -1 for invalid, 0 for overlong, 1 for valid.
    codepoint cp;
    switch (bytes) {
        case 1: {
            cp = *u8 & 0x7F;
            break;
        }
        case 2: {
            cp = ((u8[0] & 0x1F) << 6)
            | (u8[1] & 0x3F);
            break;
        }
        case 3: {
            cp = ((u8[0] & 0x0F) << 12)
            | ((u8[1] & 0x3F) << 6)
            | (u8[2] & 0x3F);
            break;
        }
        case 4:{
            cp = ((u8[0] & 0x07) << 18)
            | ((u8[1] & 0x3F) << 12)
            | ((u8[2] & 0x3F) << 6)
            | (u8[3] & 0x3F);
            break;

        }
        default:
            cp = 0;//not possible
    }
    return cp;
}

ssize_t cp_to_u8(const codepoint *codepoints, size_t size, unsigned char *u8_buf, size_t bufsiz) {
    ssize_t i = 0, j = 0;
    for (; i < size && j < bufsiz - 1; i++) {
        codepoint cp = codepoints[i];
        if (!isvalid_cp(cp))
            return -1;
        if (is_little_endian())
            cp = swap32(cp);
        if (cp <= 0x007F) {
            u8_buf[j++] = cp & 0x007F;
        }
        else if (cp <= 0x07FF && j < (bufsiz - 2)) {
            u8_buf[j++] = 0xC0 | ((cp >> 6) & 0x1F);
            u8_buf[j++] = 0x80 | (cp & 0x3F);
        }
        else if (cp <= 0xFFFF && j < (bufsiz - 3)) {
            u8_buf[j++] = 0xE0 | ((cp >> 12) & 0x0F);
            u8_buf[j++] = 0x80 | ((cp >> 6) & 0x3F);
            u8_buf[j++] = 0x80 | (cp & 0x3F);
        }
        else if (j < (bufsiz - 4)) { // <=10FFFF is already being checked
            u8_buf[j++] = 0xF0 | ((cp >> 18) & 0x07);
            u8_buf[j++] = 0x80 | ((cp >> 12) & 0x3F);
            u8_buf[j++] = 0x80 | ((cp >> 6) & 0x3F);
            u8_buf[j++] = 0x80 | (cp & 0x3F);
        }
        else
            break; //if all code-points are valid, once we reach
            // a code-point for which we don't have space
            //we stop writing.
    }
    u8_buf[j] = 0x00;
    return i; //returns number of code-points successfully written.
    //bytes successfully written to buffer can be strlen'ed.
}

size_t u8_strlen(const unsigned char *u8) {
    size_t strlen = 0;
    while (*u8++) {
        if ((*u8 & 0xC0) != 0x80)
            strlen++;
    }
    return strlen;
}