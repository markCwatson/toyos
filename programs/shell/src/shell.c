#include "shell.h"
#include "stdio.h"
#include "stdlib.h"
#include "toyos.h"
#include "string.h"

int main(int argc, char** argv) {
    while(1)  {
        // \todo: prompt disabled for now because if a user enters a valid command, then by the time the
        // new process is loaded, a task switch occurs back to the shell and the shell will print
        // the prompt again beforethe new process runs. This is not the desired behavior.
        // print("ToyOS $ ");

        char buf[1024];
        toyos_terminal_readline(buf, sizeof(buf), true);

        print("\n\n");
        toyos_system_run(buf);
    }

    return 0;
}
