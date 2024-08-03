#include "shell.h"
#include "stdio.h"
#include "stdlib.h"
#include "toyos.h"
#include "string.h"

int main(int argc, char** argv) {
    while(1)  {
        print("ToyOS $ ");

        char buf[1024];
        toyos_terminal_readline(buf, sizeof(buf), true);

        print("\n\n");
        toyos_system_run(buf);
    }

    return 0;
}
