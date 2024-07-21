#include "string.h"

char tolower(char s1) {
    if (s1 >= 65 && s1 <= 90) {
        s1 += 32;
    }

    return s1;
}

char* strcpy(char* dest, const char* src) {
    char* res = dest;

    while (*src != 0) {
        *dest = *src;
        src += 1;
        dest += 1;
    }

    // null terminator
    *dest = 0x00;

    return res;
}

int strlen(const char* ptr) {
    int i = 0;
    
    while (*ptr != 0) {
        i++;
        ptr += 1;
    }

    return i;
}

int strnlen(const char* ptr, int max) {
    int i = 0;
    for (i = 0; i < max; i++) {
        if (ptr[i] == 0) {
            break;
        }
    }

    return i;
}

int strnlen_terminator(const char* str, int max, char terminator) {
    for (int i = 0; i < max; i++) {
        if (str[i] == '\0' || str[i] == terminator) {
            return i;
        }
    }

    return max;
}

int c_to_i(char c) {
    return c - 48;
}

bool is_digit(char c) {
    return c >= 48 && c <= 57;
}

int istrncmp(const char* s1, const char* s2, int n) {
    unsigned char u1, u2;

    while (n-- > 0) {
        u1 = (unsigned char)*s1++;
        u2 = (unsigned char)*s2++;

        if (u1 != u2 && tolower(u1) != tolower(u2)) {
            return -1;
        }

        if (u1 == '\0') {   
            return 0;
        }
    }

    return 0;
}

int strncmp(const char* str1, const char* str2, int n) {
    if (!str1 || !str2 || n == 0) {
        return -1;
    }

    for (int i = 0; i < n; i++) {
        if (str1[i] != str2[i]) {
            return -2;
        }

        if (str1[i] == '\0') {
            return 0;
        }
    }

    return 0;
}
