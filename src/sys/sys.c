#include "sys.h"
#include "idt/idt.h"
#include "task/task.h"

// System command handlers
#include "./io/io.h"
#include "./memory/heap.h"
#include "./task/process.h"

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
    register_sys_command(SYSTEM_COMMAND4_MALLOC, sys_command4_malloc);
    register_sys_command(SYSTEM_COMMAND5_FREE, sys_command5_free);
    register_sys_command(SYSTEM_COMMAND6_PROCESS_LOAD_START, sys_command6_process_load_start);
    register_sys_command(SYSTEM_COMMAND7_EXIT, sys_command7_process_exit);
    register_sys_command(SYSTEM_COMMAND8_GET_PROGRAM_ARGUMENTS, sys_command8_get_program_arguments);
    register_sys_command(SYSTEM_COMMAND9_INVOKE_SYSTEM_COMMAND, sys_command9_invoke_system_command);
    register_sys_command(SYSTEM_COMMAND10_CLEAR_TERMINAL, sys_command10_clear_terminal);
    register_sys_command(SYSTEM_COMMAND11_GET_PROCESSES, sys_command11_get_processes);
}
