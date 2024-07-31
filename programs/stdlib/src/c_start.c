#include "toyos.h"

extern int main(int argc, char** argv);

/**
 * @brief Entry point for C programs.
 * 
 * This function is called by the ToyOS process loader to start the C program. It
 * retrieves the arguments for the process and calls the main function.
 */
void c_start(void) {
    struct process_arguments arguments;
    toyos_process_get_arguments(&arguments);
    main(arguments.argc, arguments.argv);
}