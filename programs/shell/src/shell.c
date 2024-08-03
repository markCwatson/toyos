#include "shell.h"
#include "stdio.h"
#include "stdlib.h"
#include "toyos.h"
#include "string.h"

int main(int argc, char** argv) {
    while(1)  {
        print("$ ");
        char buf[1024];
        toyos_terminal_readline(buf, sizeof(buf), true);
        
        // check if clear was entered and clear the terminal
        if (strlen(buf) == 5 && strncmp(buf, "clear", 5) == 0) {
            toyos_clear_terminal();
            continue;
        }

        print("\n");
        toyos_system_run(buf);
        print("\n");
    }

    return 0;
}
