#include "shell.h"
#include "stdio.h"
#include "stdlib.h"
#include "toyos.h"

int main(int argc, char** argv) {
    while(1)  {
        print("$ ");
        char buf[1024];
        toyos_terminal_readline(buf, sizeof(buf), true);
        print("\n");
        toyos_system_run(buf);
        print("\n");
    }

    return 0;
}
