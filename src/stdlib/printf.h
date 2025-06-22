#ifndef _PRINTF_H_
#define _PRINTF_H_

#include <stdarg.h>
#include <stddef.h>

// Defines for terminal colors
#define COLOR_BLACK 0
#define COLOR_BLUE 1
#define COLOR_GREEN 2
#define COLOR_CYAN 3
#define COLOR_RED 4
#define COLOR_MAGENTA 5
#define COLOR_BROWN 6
#define COLOR_LIGHT_GREY 7
#define COLOR_DARK_GREY 8
#define COLOR_LIGHT_BLUE 9
#define COLOR_LIGHT_GREEN 10
#define COLOR_LIGHT_CYAN 11
#define COLOR_LIGHT_RED 12
#define COLOR_LIGHT_MAGENTA 13
#define COLOR_LIGHT_BROWN 14
#define COLOR_WHITE 15

/**
 * @brief ToyOS printf implementation.
 *
 * @param fmt A string that specifies the format of the output.
 * @return The number of characters that are written into the array, not counting the terminating null character.
 */
int printf(const char *fmt, ...);

/**
 * @brief ToyOS printf implementation with color support.
 *
 * This function is similar to `printf` but accepts additional arguments for foreground and background colors.
 *
 * @param fmt A string that specifies the format of the output.
 * @param fg The foreground color of the text.
 * @param bg The background color of the text.
 * @return The number of characters that are written into the array, not counting the terminating null character.
 */
int printf_colored(const char *fmt, unsigned char fg, unsigned char bg, ...);

#endif
