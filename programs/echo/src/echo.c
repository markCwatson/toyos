#include "echo.h"
#include "toyos.h"
#include "stdio.h"
#include "string.h"

int main(int argc, char** argv) {
    char quote = argv[1][0];
    if (quote != '\'' && quote != '"') {
        printf("[Err-1] Usage: echo 'message contained in quotes'\n");
        return -1;
    }

    int len = strlen(argv[argc - 1]);
    char last = argv[argc - 1][len - 1];
    if (last != quote) {
        printf("[Err-2] Usage: echo 'message contained in quotes'\n");
        return -1;
    }

    for (int i = 1; i < argc; i++) {
        printf("%s ", argv[i]);
    }

    printf("\n");

    return 0;
}
