#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "toyos.h"

int ps(void) {
    struct process_info *processes = (struct process_info *)toyos_get_processes();

    // find process with longest filename for formatting purposes
    int max_pid_len = 0;
    for (int i = 0; i < TOYOS_MAX_PROCESSES; i++) {
        if (processes[i].id < 0) {
            continue;
        }

        int pid_len = strlen(itoa(processes[i].id));
        if (pid_len > max_pid_len) {
            max_pid_len = pid_len;
        }
    }

    // print process list
    char padding[max_pid_len];
    strncpy(padding, "     ", max_pid_len);

    printf(" PID  %sPATH\n", padding);
    printf(" ---  %s----\n", padding);

    for (int i = 0; i < TOYOS_MAX_PROCESSES; i++) {
        if (processes[i].id < 0) {
            continue;
        }

        // add any necessary padding to the filename
        char this_padding[max_pid_len - strlen(processes[i].filename)];
        strncpy(this_padding, "     ", max_pid_len);

        printf("  %i   %s%s\n", processes[i].id, processes[i].filename, this_padding);
    }

    print("\n");
    toyos_free(processes);

    return 0;
}

int main(int argc, char **argv) {
    int pid = toyos_fork();
    if (pid == 0) {
        printf("fork: i'm the child process\n\n");
        for (;;)
            ;
    } else {
        for (int i = 0; i < 5; i++)
            for (int i = 0; i < 10000000; i++)
                ;
        printf("fork: i'm the parent with child pid=%i\n\n", pid);
        printf("Running processes:\n");
        ps();
    }
    return 0;
}
