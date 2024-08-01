#include "io.h"
#include "task/task.h"
#include "kernel.h"
#include "keyboard/keyboard.h"
#include "terminal/terminal.h"

/**
 * @brief Prints a string to the console.
 * 
 * This function is a system command that can be invoked using interrupt 0x80. It prints a string
 * to the console using the terminal_writechar function.
 * 
 * @param frame The interrupt frame containing the system call arguments.
 * @return void
 */
void* sys_command1_print(struct interrupt_frame* frame) {
    if (!frame) {
        return NULL;
    }

    // Get the message buffer from the user space
    char buf[1024];
    void* user_space_msg_buffer = task_get_stack_item(task_current(), 0);
    copy_string_from_task(task_current(), user_space_msg_buffer, buf, sizeof(buf));

    // Print the message to the console
    printk_colored(buf, VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLUE);
    return NULL;
}

/**
 * @brief Gets a key from the keyboard buffer.
 * 
 * This function is a system command that can be invoked using interrupt 0x80. It gets a key from
 * the keyboard buffer and returns it to the calling task.
 * 
 * @param frame The interrupt frame containing the system call arguments.
 * @return void* The key code.
 */
void* sys_command2_getkey(struct interrupt_frame* frame) {
    if (!frame) {
        return NULL;
    }

    char c = keyboard_pop();
    return (void*)((int)c);
}

/**
 * @brief Puts a character to the console.
 * 
 * This function is a system command that can be invoked using interrupt 0x80. It puts a character
 * to the console using the terminal_writechar function.
 * 
 * @param frame The interrupt frame containing the system call arguments.
 * @return void
 */
void* sys_command3_putchar(struct interrupt_frame* frame) {
    if (!frame) {
        return NULL;
    }

    // Get the character from the user space and write it to the console
    char c = (char)(int)task_get_stack_item(task_current(), 0);
    terminal_update_cursor();
    terminal_writechar(c, VGA_COLOR_WHITE, VGA_COLOR_BLUE);

    return NULL;
}
