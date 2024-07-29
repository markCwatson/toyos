#include "sys.h"
#include "idt/idt.h"
#include "task/task.h"
#include "io/io.h"

// For testing purposes
static void* sys_command0_test(struct interrupt_frame* frame) {
    int a = (int)task_get_stack_item(task_current(), 0);
    int b = (int)task_get_stack_item(task_current(), 1);
    return (void*)(a + b);
}

/**
 * @brief Registers the system commands that can be invoked using interrupt 0x80.
 */
void sys_register_commands(void) {
    register_sys_command(SYSTEM_COMMAND0_TEST, sys_command0_test);
    register_sys_command(SYSTEM_COMMAND1_PRINT, sys_command1_print);
    register_sys_command(SYSTEM_COMMAND2_GETKEY, sys_command2_getkey);
    register_sys_command(SYSTEM_COMMAND3_PUTCHAR, sys_command3_putchar);
}
