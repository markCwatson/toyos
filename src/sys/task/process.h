#ifndef _SYS_PROCESS_H_
#define _SYS_PROCESS_H_

// Forward declaration of interrupt_frame struct
struct interrupt_frame;

/**
 * @brief System command handler for loading and starting a new process.
 * 
 * This function is called when the system command SYSTEM_COMMAND6_PROCESS_LOAD_START is invoked.
 * 
 * @param frame The interrupt frame.
 * @return The return value of the system command.
 */
void* sys_command6_process_load_start(struct interrupt_frame* frame);

/**
 * @brief System command handler for exiting the current process.
 * 
 * This function is called when the system command SYSTEM_COMMAND7_PROCESS_EXIT is invoked.
 * 
 * @param frame The interrupt frame.
 * @return The return value of the system command.
 */
void* sys_command7_process_exit(struct interrupt_frame* frame);

/**
 * @brief System command handler for getting the program arguments.
 * 
 * This function is called when the system command SYSTEM_COMMAND8_GET_PROGRAM_ARGUMENTS is invoked.
 * 
 * @param frame The interrupt frame.
 * @return The return value of the system command.
 */
void* sys_command8_get_program_arguments(struct interrupt_frame* frame);;

/**
 * @brief System command handler for invoking a system command.
 * 
 * This function is called when the system command SYSTEM_COMMAND9_INVOKE_SYSTEM_COMMAND is invoked.
 * 
 * @param frame The interrupt frame.
 * @return The return value of the system command.
 */
void* sys_command9_invoke_system_command(struct interrupt_frame* frame) ;

/**
 * @brief System command handler for printing the process list.
 * 
 * This function is called when the system command SYSTEM_COMMAND11_PRINT_PROCESS_LIST is invoked.
 * It prints the list of processes to the terminal.
 * 
 * @param frame The interrupt frame.
 * @return The return value of the system command.
 */
void* sys_command11_print_process_list(struct interrupt_frame* frame);

#endif
