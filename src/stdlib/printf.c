
#include <stdbool.h>
#include <stdint.h>

#include "printf.h"
#include "stdlib/string.h"
#include "terminal/terminal.h"

/**
 * @brief Prints a string to the terminal using color attributes.
 *
 * This function writes each character of the given string to the terminal, using caller defined
 * color attribute. It is typically used for kernel-level logging and debugging.
 *
 * @param str The null-terminated string to print.
 * @param fg The foreground color of the text.
 * @param bg The background color of the text.
 */
static void print(const char *str, unsigned char fg, unsigned char bg) {
    size_t len = strlen(str);

    for (int i = 0; i < len; i++) {
        terminal_writechar(str[i], fg, bg);
    }
}

/**
 * @brief ToyOS printf implementation.
 *
 * @param fmt A string that specifies the format of the output.
 * @return The number of characters that are written into the array, not counting the terminating null character.
 */
int printf(const char *fmt, ...) {
    va_list ap;
    const char *p;
    char *sval;
    int ival;

    va_start(ap, fmt);
    for (p = fmt; *p; p++) {
        if (*p != '%') {
            terminal_writechar(*p, COLOR_WHITE, COLOR_BLUE);
            continue;
        }

        switch (*++p) {
        case 'i':
            ival = va_arg(ap, int);
            print(itoa(ival), COLOR_WHITE, COLOR_BLUE);
            break;

        case 's':
            sval = va_arg(ap, char *);
            print(sval, COLOR_WHITE, COLOR_BLUE);
            break;

        default:
            terminal_writechar(*p, COLOR_WHITE, COLOR_BLUE);
            break;
        }
    }

    va_end(ap);

    terminal_update_cursor();

    return 0;
}

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
int printf_colored(const char *fmt, unsigned char fg, unsigned char bg, ...) {
    va_list ap;
    const char *p;
    char *sval;
    int ival;

    va_start(ap, bg);
    for (p = fmt; *p; p++) {
        if (*p != '%') {
            terminal_writechar(*p, fg, bg);
            continue;
        }

        switch (*++p) {
        case 'i':
            ival = va_arg(ap, int);
            print(itoa(ival), fg, bg);
            break;

        case 's':
            sval = va_arg(ap, char *);
            print(sval, fg, bg);
            break;

        default:
            terminal_writechar(*p, fg, bg);
            break;
        }
    }

    va_end(ap);

    terminal_update_cursor();

    return 0;
}
