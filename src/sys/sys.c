#include "sys.h"
#include "idt/idt.h"
#include "task/task.h"

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
}
