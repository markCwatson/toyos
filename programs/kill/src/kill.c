#include "kill.h"
#include "stdio.h"
#include "string.h"
#include "toyos.h"

int main(int argc, char **argv) {
    int ret = 0;

    if (argc == 1) {
        printf("[Err-1] Usage: kill < process id >\n\n");
        ret = -1;
        goto done;
    }

    toyos_kill(ctoi(*argv[1]));

done:
    return ret;
}
