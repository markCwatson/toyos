#ifndef _TOYOS_H_
#define _TOYOS_H_

#include <stddef.h>
#include <stdbool.h>

void print(const char* filename);
int toyos_getkey(void);
void* toyos_malloc(size_t size);
void* toyos_malloc(size_t size);
void toyos_putchar(char c);
void toyos_free(void* ptr);
int toyos_getkeyblock(void);
void toyos_terminal_readline(char* out, int max, bool output_while_typing);

#endif
