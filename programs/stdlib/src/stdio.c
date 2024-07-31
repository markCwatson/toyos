#include "stdio.h"
#include "toyos.h"
#include "stdlib.h"
#include <stdarg.h>

/**
 * @brief Writes a character to the screen.
 * 
 * @param c The character to write.
 * @return int 0 on success, negative on failure.
 */
int putchar(int c) {
    toyos_putchar((char)c);
    return 0;
}

/**
 * @brief Writes a formatted string to the screen.
 * 
 * @param fmt The format string.
 * @return int 0 on success, negative on failure.
 */
int printf(const char *fmt, ...) {
    va_list ap;
    const char *p;
    char* sval;
    int ival;

    va_start(ap, fmt);
    for (p = fmt; *p; p++) {
        if (*p != '%') {
            putchar(*p);
            continue;
        }

        switch (*++p) {
            case 'i':
                ival = va_arg(ap, int);
                print(itoa(ival));
                break;

            case 's':
                sval = va_arg(ap, char *);
                print(sval);
                break;

            default:
                putchar(*p);
                break;
        }
    }

    va_end(ap);

    return 0;
}
