#include "sys.h"
#include "idt/idt.h"

// \todo: remove
static void* sys_command0_sum(struct interrupt_frame* frame) {
    int a = frame->eax;
    int b = frame->ebx;
    return (void*)(a + b);
}

/**
 * @brief Registers the system commands that can be invoked using interrupt 0x80.
 */
void sys_register_commands(void) {
    register_sys_command(SYSTEM_COMMAND0_SUM, sys_command0_sum);
}
