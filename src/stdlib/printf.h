#ifndef _PRINTF_H_
#define _PRINTF_H_

// Credit to Marco Paland (info@paland.com, https://github.com/mpaland/printf/tree/master)

#include <stdarg.h>
#include <stddef.h>

// Defines for terminal colors
#define COLOR_BLACK         0
#define COLOR_BLUE          1
#define COLOR_GREEN         2
#define COLOR_CYAN          3
#define COLOR_RED           4
#define COLOR_MAGENTA       5
#define COLOR_BROWN         6
#define COLOR_LIGHT_GREY    7
#define COLOR_DARK_GREY     8
#define COLOR_LIGHT_BLUE    9
#define COLOR_LIGHT_GREEN   10
#define COLOR_LIGHT_CYAN    11
#define COLOR_LIGHT_RED     12
#define COLOR_LIGHT_MAGENTA 13
#define COLOR_LIGHT_BROWN   14
#define COLOR_WHITE         15

/**
 * @brief Tiny printf implementation.
 * 
 * You have to implement `_putchar` if you use `printf()`.
 * To avoid conflicts with the regular `printf()` API, it is overridden by macro defines,
 * and internal underscore-appended functions like `printf_()` are used.
 * 
 * @param format A string that specifies the format of the output.
 * @return The number of characters that are written into the array, not counting the terminating null character.
 */
int printf(const char* format, ...);

/**
 * @brief ToyOS printf implementation with color support.
 * 
 * This function is similar to `printf` but accepts additional arguments for foreground and background colors.
 * 
 * @param format A string that specifies the format of the output.
 * @param fg The foreground color of the text.
 * @param bg The background color of the text.
 * @return The number of characters that are written into the array, not counting the terminating null character.
 */
int printf_colored(const char* format, unsigned char fg, unsigned char bg, ...);

/**
 * @brief Tiny sprintf implementation.
 * 
 * Due to security reasons (buffer overflow) YOU SHOULD CONSIDER USING (V)SNPRINTF INSTEAD!
 * 
 * @param buffer A pointer to the buffer where to store the formatted string. MUST be big enough to store the output!
 * @param format A string that specifies the format of the output.
 * @return The number of characters that are WRITTEN into the buffer, not counting the terminating null character.
 */
int sprintf(char* buffer, const char* format, ...);

/**
 * @brief Tiny snprintf/vsnprintf implementation.
 * 
 * These functions are safer alternatives to sprintf as they allow specifying the maximum number of characters
 * to write, thus preventing buffer overflows.
 * 
 * @param buffer A pointer to the buffer where to store the formatted string.
 * @param count The maximum number of characters to store in the buffer, including a terminating null character.
 * @param format A string that specifies the format of the output.
 * @param va A value identifying a variable arguments list.
 * @return The number of characters that COULD have been written into the buffer, not counting the terminating
 *         null character. A value equal to or larger than `count` indicates truncation. Only when the returned value
 *         is non-negative and less than `count`, the string has been completely written.
 */
int snprintf(char* buffer, size_t count, const char* format, ...);
int vsnprintf(char* buffer, size_t count, const char* format, va_list va);

/**
 * @brief Tiny vprintf implementation.
 * 
 * This function is similar to `printf` but accepts a `va_list` of arguments.
 * 
 * @param format A string that specifies the format of the output.
 * @param va A value identifying a variable arguments list.
 * @return The number of characters that are WRITTEN into the buffer, not counting the terminating null character.
 */
int vprintf(const char* format, va_list va);

/**
 * @brief printf with output function.
 * 
 * You may use this as a dynamic alternative to `printf()` with its fixed `_putchar()` output.
 * 
 * @param out An output function which takes one character and an argument pointer.
 * @param arg An argument pointer for user data passed to the output function.
 * @param format A string that specifies the format of the output.
 * @return The number of characters that are sent to the output function, not counting the terminating null character.
 */
int fctprintf(void (*out)(char character, void* arg), void* arg, const char* format, ...);

#endif
