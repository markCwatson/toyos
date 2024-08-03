#include "ps.h"
#include "toyos.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"

int main(int argc, char** argv) {
    // get process list (must be freed... memory is allocated)
    struct process_info* processes = (struct process_info*)toyos_get_processes();

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
