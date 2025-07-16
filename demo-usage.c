#include <stdio.h>
#include "utils.h"

int main(void) {
    codepoint cp = 0x1FFFFF;
    isvalid_cp(cp);
    printf("%x codepoint is %s\n", cp, isvalid_cp(cp) ? "valid" : "invalid");
    unsigned const char *str1 = "hello.";
    unsigned const char *str2 = "türkçe.";
    codepoint cp_buf[100];
    ssize_t written = u8_to_cp(str1, cp_buf, 100);
    if (written < 0) {
        printf("problem writing codepoints.\n");
    }
    printf("codepoints for %s are: ", (const char *)str1);
    for (int i = 0; i < written; i++) {
        printf("u+%04x ", cp_buf[i]);
    }
    written = u8_to_cp(str2, cp_buf, 100);
    if (written < 0) {
        printf("problem writing codepoints.\n");
    }
    printf("\n codepoints for %s are: ", (const char *)str2);
    for (int i = 0; i < written; i++) {
        printf("u+%04x ", cp_buf[i]);
    }
    printf("\ncharacter count of %s is %zu", (const char *)str2, u8_strlen(str2));
    //****
    unsigned char olong[4] = {0xE0, 0x80, 0xaf, 0x00};
    printf("\noverlong chars are %s\n", isvalid_u8(olong, 0)? "valid" : "invalid");
    printf("codepoints for %s are: ", (const char *)str2);
    written = u8_to_cp(olong, cp_buf, 100);
    if (written < 0) {
        printf("problem writing codepoints.\n");
    }
    printf("\n codepoints for %s are: ", (const char *)olong);
    for (int i = 0; i < written; i++) {
        printf("u+%04x ", cp_buf[i]);
    }
}