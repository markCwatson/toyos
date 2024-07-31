#include "shell.h"
#include "stdio.h"
#include "stdlib.h"
#include "toyos.h"

/**
 * @brief Prints the "ToyOS" logo using ASCII art.
 *
 * This function displays the "ToyOS" name in a stylized ASCII art format.
 * The art uses simple characters to create a visually appealing representation of the OS name.
 */
static void print_toyos_logo(void) {
    const char* logo =
        "   _____              _  _     ___      ___   \n"
        "  |_   _|    ___     | || |   / _ \\    / __|  \n"
        "    | |     / _ \\     \\_, |  | (_) |   \\__ \\  \n"
        "   _|_|_    \\___/    _|__/    \\___/    |___/  \n"
        " _|\"\"\"\"\"| _|\"\"\"\"\"| _| \"\"\"\"| _|\"\"\"\"\"| _|\"\"\"\"\"| \n"
        " \"`-0-0-' \"`-0-0-' \"`-0-0-' \"`-0-0-' \"`-0-0-' version 0.0.0\n"
        "\n";

    print(logo);
}

int main(int argc, char** argv) {
    print_toyos_logo();

    while(1)  {
        print("$ ");
        char buf[1024];
        toyos_terminal_readline(buf, sizeof(buf), true);
        print("\n");
        toyos_process_load_start(buf);
        print("\n");
    }

    return 0;
}
