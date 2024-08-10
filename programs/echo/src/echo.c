#include "echo.h"
#include "toyos.h"
#include "stdio.h"
#include "string.h"

int main(int argc, char** argv) {
    int ret = 0;

    if (argc == 1) {
        printf("[Err-1] Usage: echo < string >\n\n");
        ret = -1;
        goto done;
    }

    for (int i = 1; i < argc; i++) {
        printf("%s ", argv[i]);
    }

    printf("\n\n");

done:
    return ret;
}
