#ifndef _TOYOS_H_
#define _TOYOS_H_

#include <stddef.h>
#include <stdbool.h>

struct command_argument {
    char argument[512];
    struct command_argument* next;
};

struct process_arguments {
    int argc;
    char** argv;
};

void print(const char* filename);
int toyos_getkey(void);
void* toyos_malloc(size_t size);
void* toyos_malloc(size_t size);
void toyos_putchar(char c);
void toyos_free(void* ptr);
int toyos_getkeyblock(void);
void toyos_terminal_readline(char* out, int max, bool output_while_typing);
void toyos_process_load_start(const char* filename);
void toyos_exit(void);
struct command_argument* toyos_parse_command(const char* command, int max);
void toyos_process_get_arguments(struct process_arguments* arguments);;
int toyos_system(struct command_argument* arguments);
int toyos_system_run(const char* command);
void toyos_clear_terminal(void);
void toyos_display_process_list(void);

#endif
