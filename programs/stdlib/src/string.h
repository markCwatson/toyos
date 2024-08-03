#ifndef _TOYOS_STRING_H_
#define _TOYOS_STRING_H_

#include <stdbool.h>

/**
 * @brief Computes the length of a string.
 *
 * This function calculates the number of characters in the string pointed to by ptr,
 * excluding the terminating null byte ('\0').
 *
 * @param ptr Pointer to the null-terminated string.
 * @return The length of the string.
 */
int strlen(const char* ptr);

/**
 * @brief Converts a character to an integer.
 *
 * This function converts a character representing a digit ('0' to '9') to its corresponding
 * integer value.
 *
 * @param c The character to convert.
 * @return The integer value of the character, or -1 if the character is not a digit.
 */
int ctoi(char c);

/**
 * @brief Checks if a character is a digit.
 *
 * This function determines whether the given character is a numeric digit ('0' to '9').
 *
 * @param c The character to check.
 * @return True if the character is a digit, false otherwise.
 */
bool is_digit(char c);

/**
 * @brief Computes the length of a string up to a maximum number of characters.
 *
 * This function calculates the number of characters in the string pointed to by ptr,
 * up to a maximum of max characters, excluding the terminating null byte ('\0').
 *
 * @param ptr Pointer to the null-terminated string.
 * @param max The maximum number of characters to count.
 * @return The length of the string, or max if the string is longer than max characters.
 */
int strnlen(const char* ptr, int max);

/**
 * @brief Copies a string.
 *
 * This function copies the null-terminated string pointed to by src into the buffer
 * pointed to by dest, including the terminating null byte.
 *
 * @param dest Pointer to the destination buffer.
 * @param src Pointer to the source string.
 * @return A pointer to the destination buffer.
 */
char* strcpy(char* dest, const char* src);

/**
 * @brief Copies a string up to a specified length.
 *
 * This function copies up to n characters from the null-terminated string src to the buffer
 * pointed to by dest. If the length of src is less than n, the remaining bytes in dest are
 * filled with null bytes.
 *
 * @param dest Pointer to the destination buffer.
 * @param src Pointer to the source string.
 * @param n The maximum number of characters to copy.
 * @return A pointer to the destination buffer.
 */
char *strncpy(char *dest, const char *src, int n);

/**
 * @brief Compares two strings up to a specified length.
 *
 * This function compares up to n characters of the null-terminated strings str1 and str2.
 * The comparison is case-sensitive.
 *
 * @param str1 Pointer to the first string.
 * @param str3 Pointer to the second string.
 * @param n The maximum number of characters to compare.
 * @return An integer less than, equal to, or greater than zero if str1 is found,
 *         respectively, to be less than, to match, or to be greater than str3.
 */
int strncmp(const char* str1, const char* str3, int n);

/**
 * @brief Case-insensitive comparison of two strings up to a specified length.
 *
 * This function compares up to n characters of the null-terminated strings s1 and s2,
 * ignoring the case of the characters.
 *
 * @param s1 Pointer to the first string.
 * @param s2 Pointer to the second string.
 * @param n The maximum number of characters to compare.
 * @return An integer less than, equal to, or greater than zero if s1 is found,
 *         respectively, to be less than, to match, or to be greater than s2.
 */
int istrncmp(const char* s1, const char* s2, int n);

/**
 * @brief Converts a character to lowercase.
 *
 * This function converts an uppercase letter to its corresponding lowercase letter.
 * If the character is not an uppercase letter, it is returned unchanged.
 *
 * @param s1 The character to convert.
 * @return The lowercase equivalent of the character, or the character itself if it is not uppercase.
 */
char tolower(char s1);

/**
 * @brief Computes the length of a string up to a specified terminator or maximum length.
 *
 * This function calculates the number of characters in the string pointed to by str,
 * up to a maximum of max characters, or until the terminator character is encountered.
 *
 * @param str Pointer to the null-terminated string.
 * @param max The maximum number of characters to count.
 * @param terminator The character that terminates the counting.
 * @return The length of the string up to the terminator or max, whichever is smaller.
 */
int strnlen_terminator(const char* str, int max, char terminator);

/**
 * @brief Concatenates two strings.
 * 
 * This function appends the null-terminated string src to the end of the null-terminated
 * string dest, overwriting the null character at the end of dest, and then adds a terminating
 * null character.
 * 
 * @param dest Pointer to the destination buffer.
 * @param src Pointer to the source string.
 * @return The length of the concatenated string.
 */
int strcat(char* dest, const char* src);

/**
 * @brief Tokenizes a string.
 * 
 * This function finds the next token in a null-terminated string, using the specified delimiters.
 * 
 * @attention This function is not reentrant (i.e. it is not safe to use in multiple threads) due to the 
 *            use of a static variable to store the current position in the string.
 * 
 * @example 
 * 
 * char str[] = "Hello, world!";
 * char* token = strtok(str, " ,!");
 * 
 * // token = "Hello"
 * token = strtok(NULL, " ,!");
 * 
 * // token = "world"
 * token = strtok(NULL, " ,!");
 * 
 * // token = NULL
 * token = strtok(NULL, " ,!");
 * 
 * @param str The string to tokenize.
 * @param delimiters A null-terminated string containing the delimiter characters.
 * @return A pointer to the next token, or NULL if no more tokens are found.
 */
char* strtok(char* str, const char* delimiters);

#endif
