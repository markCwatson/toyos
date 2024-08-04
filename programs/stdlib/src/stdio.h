#ifndef _TOYOS_STDIO_
#define _TOYOS_STDIO_

/**
 * Writes a character to the screen.
 * 
 * @param c The character to write.
 * @return int 0 on success, negative on failure.
 */
int putchar(int c);

/**
 * Writes a formatted string to the screen.
 * 
 * @param fmt The format string.
 * @return int 0 on success, negative on failure.
 */
int printf(const char *fmt, ...);

#endif
