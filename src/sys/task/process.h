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
void *sys_command6_process_load_start(struct interrupt_frame *frame);

/**
 * @brief System command handler for exiting the current process.
 *
 * This function is called when the system command SYSTEM_COMMAND7_PROCESS_EXIT is invoked.
 *
 * @param frame The interrupt frame.
 * @return The return value of the system command.
 */
void *sys_command7_process_exit(struct interrupt_frame *frame);

/**
 * @brief System command handler for getting the program arguments.
 *
 * This function is called when the system command SYSTEM_COMMAND8_GET_PROGRAM_ARGUMENTS is invoked.
 *
 * @param frame The interrupt frame.
 * @return The return value of the system command.
 */
void *sys_command8_get_program_arguments(struct interrupt_frame *frame);
;

/**
 * @brief System command handler for invoking a system command.
 *
 * This function is called when the system command SYSTEM_COMMAND9_INVOKE_SYSTEM_COMMAND is invoked.
 *
 * @param frame The interrupt frame.
 * @return The return value of the system command.
 */
void *sys_command9_invoke_system_command(struct interrupt_frame *frame);

/**
 * @brief System command handler for fetching the process list.
 *
 * This function is called when the system command SYSTEM_COMMAND11_GET_PROCESSES is invoked.
 * It returns a list of processes.
 *
 * @warning The memory for the list is allocated from the current process's memory space
 * and must be freed by the caller.
 *
 * @param frame The interrupt frame.
 * @return The return value of the system command.
 */
void *sys_command11_get_processes(struct interrupt_frame *frame);

/**
 * @brief System command handler for checkint hte status of the process lock.
 *
 * This function is called when the system command SYSTEM_COMMAND12_CHECK_LOCK is invoked.
 *
 * @param frame The interrupt frame.
 * @return 0 if the process is not locked, error code if it is.
 */
void *sys_command12_check_lock(struct interrupt_frame *frame);

/**
 * @brief System command handler for notifying that a process has finished.
 *
 * This function is called when the system command SYSTEM_COMMAND13_DONE is invoked.
 *
 * @param frame The interrupt frame.
 * @return The return value of the system command.
 */
void *sys_command13_done(struct interrupt_frame *frame);

/**
 * @brief System command handler for forking the current process.
 *
 * This function is called when the system command SYSTEM_COMMAND14_FORK is invoked.
 * It creates a copy of the current process and returns the child process id to
 * the caller. The child process receives 0 as the return value.
 */
void *sys_command14_fork(struct interrupt_frame *frame);

#endif
