#ifndef _SYS_IO_H_
#define _SYS_IO_H_

// Forward declaration of interrupt_frame
struct interrupt_frame;

/**
 * @brief Prints a string to the console.
 * 
 * This function is a system command that can be invoked using interrupt 0x80. It prints a string
 * to the console using the terminal_writechar function.
 * 
 * @param frame The interrupt frame containing the system call arguments.
 * @return void
 */
void* sys_command1_print(struct interrupt_frame* frame);

/**
 * @brief Gets a key from the keyboard buffer.
 * 
 * This function is a system command that can be invoked using interrupt 0x80. It gets a key from
 * the keyboard buffer and returns it to the calling task.
 * 
 * @param frame The interrupt frame containing the system call arguments.
 * @return void* The key code.
 */
void* sys_command2_getkey(struct interrupt_frame* frame);

/**
 * @brief Puts a character to the console.
 * 
 * This function is a system command that can be invoked using interrupt 0x80. It puts a character
 * to the console using the terminal_writechar function.
 * 
 * @param frame The interrupt frame containing the system call arguments.
 * @return void
 */
void* sys_command3_putchar(struct interrupt_frame* frame);

/**
 * @brief Clears the terminal.
 * 
 * This function is a system command that can be invoked using interrupt 0x80. It clears the
 * terminal using the terminal_clear function.
 * 
 * @param frame The interrupt frame containing the system call arguments.
 * @return void
 */
void* sys_command10_clear_terminal(struct interrupt_frame* frame);

#endif
