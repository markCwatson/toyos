#include "sys.h"
#include "idt/idt.h"
#include "task/task.h"

// System command handlers
#include "./io/io.h"
#include "./memory/heap.h"
#include "./task/process.h"

// For testing purposes
static void* sys_cmd0_test(struct interrupt_frame* frame) {
    int a = (int)task_get_stack_item(task_current(), 0);
    int b = (int)task_get_stack_item(task_current(), 1);
    return (void*)(a + b);
}

/**
 * Registers the system commands that can be invoked using interrupt 0x80.
 */
void sys_register_commands(void) {
    register_sys_cmd(SYSTEM_CMD0_TEST, sys_cmd0_test);
    register_sys_cmd(SYSTEM_CMD1_PRINT, sys_cmd1_print);
    register_sys_cmd(SYSTEM_CMD2_GETKEY, sys_cmd2_getkey);
    register_sys_cmd(SYSTEM_CMD3_PUTCHAR, sys_cmd3_putchar);
    register_sys_cmd(SYSTEM_CMD4_MALLOC, sys_cmd4_malloc);
    register_sys_cmd(SYSTEM_CMD5_FREE, sys_cmd5_free);
    register_sys_cmd(SYSTEM_CMD6_PROCESS_LOAD_START, sys_cmd6_process_load_start);
    register_sys_cmd(SYSTEM_CMD7_EXIT, sys_cmd7_process_exit);
    register_sys_cmd(SYSTEM_CMD8_GET_PROGRAM_ARGUMENTS, sys_cmd8_get_program_arguments);
    register_sys_cmd(SYSTEM_CMD9_INVOKE_SYSTEM_CMD, sys_cmd9_invoke_system_cmd);
    register_sys_cmd(SYSTEM_CMD10_CLEAR_TERMINAL, sys_cmd10_clear_terminal);
    register_sys_cmd(SYSTEM_CMD11_GET_PROCESSES, sys_cmd11_get_processes);
    register_sys_cmd(SYSTEM_CMD12_FORK, sys_cmd12_fork);
    register_sys_cmd(SYSTEM_CMD13_WAIT, sys_cmd13_wait);
    register_sys_cmd(SYSTEM_CMD14_KILL, sys_cmd14_kill);
    register_sys_cmd(SYSTEM_CMD15_EXEC, sys_cmd15_exec);
}
