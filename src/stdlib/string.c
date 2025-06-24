#include "string.h"

char tolower(char s1) {
    if (s1 >= 65 && s1 <= 90) {
        s1 += 32;
    }

    return s1;
}

char *strcpy(char *dest, const char *src) {
    char *res = dest;

    while (*src != 0) {
        *dest = *src;
        src += 1;
        dest += 1;
    }

    // null terminator
    *dest = 0x00;

    return res;
}

int strcat(char *dest, const char *src) {
    int i = 0;
    int j = 0;

    while (dest[i] != 0) {
        i += 1;
    }

    while (src[j] != 0) {
        dest[i] = src[j];
        i += 1;
        j += 1;
    }

    dest[i] = 0x00;
    return i;
}

char *strncpy(char *dest, const char *src, int n) {
    int i = 0;

    while (i < n - 1) {
        if (src[i] == 0x00) {
            break;
        }

        dest[i] = src[i];
        i += 1;
    }

    // null terminator
    dest[i] = 0x00;
    return dest;
}

int strlen(const char *ptr) {
    int i = 0;

    while (*ptr != 0) {
        i++;
        ptr += 1;
    }

    return i;
}

int strnlen(const char *ptr, int max) {
    int i = 0;
    for (i = 0; i < max; i++) {
        if (ptr[i] == 0) {
            break;
        }
    }

    return i;
}

int strnlen_terminator(const char *str, int max, char terminator) {
    for (int i = 0; i < max; i++) {
        if (str[i] == '\0' || str[i] == terminator) {
            return i;
        }
    }

    return max;
}

char *itoa(int i) {
    static char text[12];
    int loc = 11;
    text[11] = 0;
    char neg = 1;

    if (i >= 0) {
        neg = 0;
        i = -i;
    }

    while (i) {
        text[--loc] = '0' - (i % 10);
        i /= 10;
    }

    if (loc == 11) {
        text[--loc] = '0';
    }

    if (neg) {
        text[--loc] = '-';
    }

    return &text[loc];
}

int ctoi(char c) {
    return c - 48;  // '0' character has ASCII value 48
}

char *itoa_hex(unsigned int i) {
    static char text[12];
    int loc = 11;
    text[11] = 0;

    if (i == 0) {
        text[--loc] = '0';
        return &text[loc];
    }

    while (i) {
        text[--loc] = "0123456789ABCDEF"[i % 16];
        i /= 16;
    }

    return &text[loc];
}

bool is_digit(char c) {
    return c >= 48 && c <= 57;
}

int istrncmp(const char *s1, const char *s2, int n) {
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

int strncmp(const char *str1, const char *str2, int n) {
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
