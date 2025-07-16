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
void next_codepoint(){}
void prev_codepoint(){}

ssize_t u8_to_cp(const unsigned char *u8_str, codepoint *cp_buf, size_t bufsiz) {
    // u8_str must be Null Terminated.
    const unsigned char *u8 = u8_str;
    ssize_t i = 0;
    for (; *u8 && i < (bufsiz - 1); i++) {
        codepoint cp = 0;
        // start byte:
        if ((*u8 & 0xC0) == 0x80 || is_forbidden_byte(*u8))
            return -1; //invalid or continuation at the start of byte
        int bytes = u8_byte_count(*u8);

        for (int j = 1; j < bytes; j++) {
            if ((u8[j] & 0xC0) != 0x80)
                return -1; //invalid continuation byte
        }
        if (u8char_to_cp(&cp, u8, bytes) != 1)
            return -1; //out of range or overlong
        cp_buf[i] = cp;
        u8 += bytes;
    }
    cp_buf[i] = 0x00; //null terminate the cp buffer
    return i;
}
int is_forbidden_byte(unsigned char u8c) {
    return (u8c == 0xC0) || (u8c == 0xC1) || (u8c >= 0xF5);
    // for the first byte, 0xC0 and 0xC1 are overlong
    // and >=0xF5 is out-of-range, this is just for
    //early-exit, both overlong and bounds are checked
    // in u8char_to_cp.
}

int u8_byte_count(unsigned char u8c) {
    int b0 = (u8c >> 7) & 0x01;
    int b1 = (u8c >> 6) & 0x01;
    int b2 = (u8c >> 5) & 0x01;
    int b3 = (u8c >> 4) & 0x01;
    return 1 + b0*b1 + b0*b1*b2 + b0*b1*b2*b3;
}
int u8char_to_cp(codepoint *cp, const unsigned char *u8, int bytes) {
    // -1 for invalid, 0 for overlong, 1 for valid.
    switch (bytes) {
        case 1: {
            *cp = *u8 & 0x7F;
            break;
        }
        case 2: {
            *cp = ((u8[0] & 0x1F) << 6)
            | (u8[1] & 0x3F);
            if (*cp < 0x80)
                return 0;
            break;
        }
        case 3: {
            *cp = ((u8[0] & 0x0F) << 12)
            | ((u8[1] & 0x3F) << 6)
            | (u8[2] & 0x3F);
            if (*cp < 0x0800)
                return 0;
            if (*cp >= 0xD800 && *cp <= 0xDFFF)
                return -1;
            break;
        }
        case 4:{
            *cp = ((u8[0] & 0x07) << 18)
            | ((u8[1] & 0x3F) << 12)
            | ((u8[2] & 0x3F) << 6)
            | (u8[3] & 0x3F);
            if (*cp < 0x010000)
                return 0;
            if (*cp > 0x10FFFF)
                return -1;
            break;

        }
        default:
            return -1;
    }
    return 1;
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



void print_cp() {
    is_little_endian();
}
/* substring search
 * byte-based and code point based
 * strcmp
 * strlen
 * Normalization NFC/NFD
 * Case Folding */


/*Formally, Unicode defines a codespace of values in the range 0 to 0x10FFFF inclusive,
 *and a Unicode codepoint is any integer falling within that range.
 *However, due to historical reasons, it became necessary to "carve out" a subset of the codespace,
 *excluding codepoints in the range 0xD7FF–0xE000. That subset of codepoints excluding that range
 *are known as Unicode scalar values. The codepoints in the range 0xD7FF-0xE000 are known as "surrogate" codepoints.
 *The surrogate codepoints will never be assigned a semantic meaning, and can only validly appear
 *in UTF-16 encoded text. */