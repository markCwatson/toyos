#include "int80h.h"
#include "idt/idt.h"

// \todo: remove
static void* int80h_command0_sum(struct interrupt_frame* frame) {
    int a = frame->eax;
    int b = frame->ebx;
    return (void*)(a + b);
}

/**
 * @brief Registers the system commands that can be invoked using interrupt 0x80.
 */
void int80h_register_commands(void) {
    register_int80h_command(SYSTEM_COMMAND0_SUM, int80h_command0_sum);
}
