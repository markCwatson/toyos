#include "string.h"

/**
 * @brief Converts a character to lowercase.
 *
 * Converts the given character to lowercase if it is an uppercase letter.
 * If the character is not an uppercase letter, it is returned unchanged.
 *
 * @param s1 The character to convert.
 * @return The lowercase equivalent of the character, or the character itself if it is not uppercase.
 */
char tolower(char s1) {
    if (s1 >= 65 && s1 <= 90) {
        s1 += 32;
    }

    return s1;
}

/**
 * @brief Copies a string from source to destination.
 *
 * Copies the null-terminated string pointed to by src into the buffer pointed to by dest.
 * The destination buffer must be large enough to hold the copied string including the null terminator.
 *
 * @param dest Pointer to the destination buffer.
 * @param src Pointer to the source string.
 * @return A pointer to the destination buffer.
 */
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

/**
 * @brief Computes the length of a string.
 *
 * Returns the number of characters in the string pointed to by ptr, excluding the terminating null byte.
 *
 * @param ptr Pointer to the null-terminated string.
 * @return The length of the string.
 */
int strlen(const char* ptr) {
    int i = 0;
    
    while (*ptr != 0) {
        i++;
        ptr += 1;
    }

    return i;
}

/**
 * @brief Computes the length of a string up to a maximum number of characters.
 *
 * Calculates the number of characters in the string pointed to by ptr,
 * up to a maximum of max characters, excluding the terminating null byte.
 *
 * @param ptr Pointer to the null-terminated string.
 * @param max The maximum number of characters to count.
 * @return The length of the string, or max if the string is longer than max characters.
 */
int strnlen(const char* ptr, int max) {
    int i = 0;
    for (i = 0; i < max; i++) {
        if (ptr[i] == 0) {
            break;
        }
    }

    return i;
}

/**
 * @brief Computes the length of a string up to a specified terminator or maximum length.
 *
 * Calculates the number of characters in the string pointed to by str,
 * up to a maximum of max characters, or until the terminator character is encountered.
 *
 * @param str Pointer to the null-terminated string.
 * @param max The maximum number of characters to count.
 * @param terminator The character that terminates the counting.
 * @return The length of the string up to the terminator or max, whichever is smaller.
 */
int strnlen_terminator(const char* str, int max, char terminator) {
    for (int i = 0; i < max; i++) {
        if (str[i] == '\0' || str[i] == terminator) {
            return i;
        }
    }

    return max;
}

/**
 * @brief Converts a character to an integer.
 *
 * Converts a character representing a digit ('0'-'9') to its corresponding integer value.
 *
 * @param c The character to convert.
 * @return The integer value of the character, or -1 if the character is not a digit.
 */
int c_to_i(char c) {
    return c - 48;  // '0' character has ASCII value 48
}

/**
 * @brief Checks if a character is a digit.
 *
 * Determines whether the given character is a numeric digit ('0' to '9').
 *
 * @param c The character to check.
 * @return True if the character is a digit, false otherwise.
 */
bool is_digit(char c) {
    return c >= 48 && c <= 57;
}

/**
 * @brief Case-insensitive comparison of two strings up to a specified length.
 *
 * Compares up to n characters of the null-terminated strings s1 and s2,
 * ignoring the case of the characters. The comparison stops at the first
 * mismatching character or if a null byte is encountered.
 *
 * @param s1 Pointer to the first string.
 * @param s2 Pointer to the second string.
 * @param n The maximum number of characters to compare.
 * @return An integer less than, equal to, or greater than zero if s1 is found,
 *         respectively, to be less than, to match, or to be greater than s2.
 */
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

/**
 * @brief Compares two strings up to a specified length.
 *
 * Compares up to n characters of the null-terminated strings str1 and str2.
 * The comparison is case-sensitive and stops at the first mismatching character
 * or if a null byte is encountered.
 *
 * @param str1 Pointer to the first string.
 * @param str2 Pointer to the second string.
 * @param n The maximum number of characters to compare.
 * @return An integer less than, equal to, or greater than zero if str1 is found,
 *         respectively, to be less than, to match, or to be greater than str2.
 */
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
