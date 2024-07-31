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

#endif
