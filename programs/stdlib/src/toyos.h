#ifndef _TOYOS_H_
#define _TOYOS_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define TOYOS_MAX_PROCESSES 12

struct process_info {
    int id;
    char filename[64];
};

struct command_argument {
    char argument[512];
    struct command_argument *next;
};

struct process_arguments {
    int argc;
    char **argv;
};

void print(const char *filename);
int toyos_getkey(void);
void *toyos_malloc(size_t size);
void toyos_putchar(char c);
void toyos_free(void *ptr);
int toyos_getkeyblock(void);
void toyos_terminal_readline(char *out, int max, bool output_while_typing);
void toyos_process_load_start(const char *filename);
void toyos_exit(void);
struct command_argument *toyos_parse_command(const char *command, int max);
void toyos_process_get_arguments(struct process_arguments *arguments);
int toyos_system(struct command_argument *arguments);
int toyos_system_run(const char *command);
void toyos_clear_terminal(void);
int toyos_fork(void);
void *toyos_get_processes(void);
void toyos_wait(void);
void toyos_done(void);
void toyos_kill(int pid);

#endif
