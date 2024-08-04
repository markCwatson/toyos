#ifndef _SYS_PROCESS_H_
#define _SYS_PROCESS_H_

// Forward declaration of interrupt_frame struct
struct interrupt_frame;

/**
 * System command handler for loading and starting a new process.
 * 
 * This function is called when the system command SYSTEM_CMD6_PROCESS_LOAD_START is invoked.
 * 
 * @param frame The interrupt frame.
 * @return The return value of the system command.
 */
void* sys_cmd6_process_load_start(struct interrupt_frame* frame);

/**
 * System command handler for exiting the current process.
 * 
 * This function is called when the system command SYSTEM_CMD7_PROCESS_EXIT is invoked.
 * 
 * @param frame The interrupt frame.
 * @return The return value of the system command.
 */
void* sys_cmd7_process_exit(struct interrupt_frame* frame);

/**
 * System command handler for getting the program arguments.
 * 
 * This function is called when the system command SYSTEM_CMD8_GET_PROGRAM_ARGUMENTS is invoked.
 * 
 * @param frame The interrupt frame.
 * @return The return value of the system command.
 */
void* sys_cmd8_get_program_arguments(struct interrupt_frame* frame);;

/**
 * System command handler for invoking a system command.
 * 
 * This function is called when the system command SYSTEM_CMD9_INVOKE_SYSTEM_CMD is invoked.
 * 
 * @param frame The interrupt frame.
 * @return The return value of the system command.
 */
void* sys_cmd9_invoke_system_cmd(struct interrupt_frame* frame) ;

/**
 * System command handler for fetching the process list.
 * 
 * This function is called when the system command SYSTEM_CMD11_GET_PROCESSES is invoked.
 * It returns a list of processes.
 * 
 * @warning The memory for the list is allocated from the current process's memory space
 * and must be freed by the caller.
 * 
 * @param frame The interrupt frame.
 * @return A list of processes information.
 */
void* sys_cmd11_get_processes(struct interrupt_frame* frame);

/**
 * System command handler for forking a new process.
 * 
 * This function is called when the system command SYSTEM_CMD12_FORK is invoked.
 * 
 * @param frame The interrupt frame.
 * @return The PID of the child process, or a negative error code on failure.
 */
void* sys_cmd12_fork(struct interrupt_frame* frame);

/**
 * System command handler for waiting for a process to exit.
 * 
 * This function is called when the system command SYSTEM_CMD13_WAIT is invoked.
 * 
 * @param frame The interrupt frame.
 * @return The return value of the system command.
 */
void* sys_cmd13_wait(struct interrupt_frame* frame);

/**
 * System command handler for killing a process.
 * 
 * This function is called when the system command SYSTEM_CMD14_KILL is invoked.
 * 
 * @param frame The interrupt frame.
 * @return The return value of the system command.
 */
void* sys_cmd14_kill(struct interrupt_frame* frame);

/**
 * System command handler for executing a new process.
 * 
 * This function is called when the system command SYSTEM_CMD15_EXEC is invoked.
 * 
 * @param frame The interrupt frame.
 * @return The return value of the system command.
 */
void* sys_cmd15_exec(struct interrupt_frame* frame);

#endif
