#include "idt.h"
#include "config.h"
#include "kernel.h"
#include "memory/memory.h"
#include "io/io.h"
#include "task/task.h"

struct idt_desc idt_descriptors[TOYOS_TOTAL_INTERRUPTS];
struct idtr_desc idtr_descriptor;

extern void int80h(void);
extern void int21h(void);
extern void no_interrupt(void);
extern void idt_load(struct idtr_desc* ptr);

static isr80h_cmd isr80h_commands[TOYOS_MAX_SYSCALLS];

/**
 * @brief Handles system call interrupt
 * 
 * This function is called when a system call interrupt occurs. It reads the system call
 * number from the interrupt frame and calls the appropriate system call handler function.
 * 
 * @param cmd The system call number.
 * @param frame The interrupt frame containing the system call number.
 * @return The result of the system call.
 */
static void* isr80h_handle_command(int cmd, struct interrupt_frame* frame) {
    if (cmd < 0 || cmd >= TOYOS_MAX_SYSCALLS) {
        alertk("Invalid system call number: %d\n", cmd);
        return NULL;
    }

    isr80h_cmd handler = isr80h_commands[cmd];
    if (!handler) {
        alertk("No handler for system call %d\n", cmd);
        return NULL;
    }

    return handler(frame);
}

/**
 * @brief Registers a system call handler function
 * 
 * This function registers a system call handler function for the given system call number.
 * When the system call interrupt occurs, the handler function will be called to handle the
 * system call.
 * 
 * @param cmd The system call number.
 * @param handler The system call handler function.
 * @return void
 */
void register_int80h_command(int cmd, isr80h_cmd handler) {
    if (cmd < 0 || cmd >= TOYOS_MAX_SYSCALLS) {
        panick("Invalid system call number: %d\n", cmd);
    }

    if (isr80h_commands[cmd]) {
        panick("System call %d already has a handler\n", cmd);
    }

    isr80h_commands[cmd] = handler;
}

/**
 * @brief Handles system call interrupt
 * 
 * This function is called when a system call interrupt occurs. It reads the system call
 * number from the interrupt frame and calls the appropriate system call handler function.
 * 
 * @param cmd The system call number.
 * @param frame The interrupt frame containing the system call number.
 * @return The result of the system call.
 * @see isr80h_handle_command
 */
void* isr80h_handler(int cmd, struct interrupt_frame* frame) {
    // Switch to the kernel page to access the kernel heap
    kernel_page();

    // Save the current task state
    task_current_save_state(frame);

    // Handle the system call
    void* res = isr80h_handle_command(cmd, frame);
    
    // Switch back to the task page to return to the task
    task_page();
    return res;
}


/**
 * @brief Handles keyboard interrupt
 */
void int21h_handler(void) {
    alertk("\nKeyboard pressed\n");

    // ack interrupt
    outb(0x20, 0x20);
}

/**
 * @brief Handles no interrupt
 */
void no_interrupt_handler(void) {
    // ack interrupt
    outb(0x20, 0x20);
}

/**
 * @brief Handles the divide by zero exception
 */
static void idt_zero(void) {
    panick("\nDivide by zero error!\n");
}

/**
 * @brief Sets an interrupt descriptor in the IDT
 * 
 * This function sets the interrupt descriptor at the specified index in the interrupt descriptor
 * table (IDT) to the given address. The address should be the entry point of the interrupt handler
 * function that should be called when the interrupt occurs.
 * 
 * @param interrupt_no The index of the interrupt descriptor to set.
 * @param address The address of the interrupt handler function.
 * @return void
 */
static void idt_set(int interrupt_no, void* address) {
    struct idt_desc* desc = &idt_descriptors[interrupt_no];
    desc->offset_1 = (uint32_t)address & 0x0000ffff;
    desc->selector = TOYOS_CODE_SELECTOR;
    desc->zero = 0x00;
    desc->type_attr = 0xee;
    desc->offset_2 = (uint32_t)address >> 16;
}

/**
 * @brief Initializes the interrupt descriptor table (IDT) with default handlers
 */
void idt_init(void) {
    memset(idt_descriptors, 0, sizeof(idt_descriptors));
    idtr_descriptor.limit = sizeof(idt_descriptors) - 1;
    idtr_descriptor.base = (uint32_t)idt_descriptors;

    // Set all interrupts to no_interrupt
    for (int i = 0; i < TOYOS_TOTAL_INTERRUPTS; i++) {
        idt_set(i, no_interrupt);
    }

    // int 0 (divide by zero)
    idt_set(0, idt_zero);
    // key board interrupt handler
    idt_set(0x21, int21h);
    // system call interrupt handler
    idt_set(0x80, int80h);

    // Load the interrupt descriptor table
    idt_load(&idtr_descriptor);
}
