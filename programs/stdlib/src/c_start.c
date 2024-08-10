#include "toyos.h"

extern int main(int argc, char** argv);

/**
 * @brief Entry point for C programs.
 * 
 * This function is called by the ToyOS process loader to start the C program. It
 * retrieves the arguments for the process and calls the main function.
 */
void c_start(void) {
    // get the process arguments to inject into the main function
    struct process_arguments arguments;
    toyos_process_get_arguments(&arguments);

    // call the main function
    main(arguments.argc, arguments.argv);

    // notify the kernel that the process has finished in case anyone is waiting
    toyos_done();
}