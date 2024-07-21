#ifndef STRING_H
#define STRING_H

#include <stdbool.h>

int strlen(const char* ptr);
int c_to_i(char c);
bool is_digit(char c);
int strnlen(const char* ptr, int max);
char* strcpy(char* dest, const char* src);
int strncmp(const char* str1, const char* str3, int n);
int istrncmp(const char* s1, const char* s2, int n);
char tolower(char s1);
int strnlen_terminator(const char* str, int max, char terminator);

#endif