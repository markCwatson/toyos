#ifndef _IDT_H_
#define _IDT_H_

#include <stdint.h>

// Forward declaration of the interrupt frame structure
struct interrupt_frame;

// Function pointer type for interrupt service routines (ISRs)
typedef void* (*sys_cmd)(struct interrupt_frame* frame);

// Function pointer type for interrupt handlers
typedef void (*interrupt_cb_fn)(void);

/**
 * @brief Structure representing an entry in the interrupt descriptor table (IDT)
 * 
 * The IDT is an array of these structures, each representing a single interrupt or exception
 * that the CPU can raise. Each entry contains the address of the interrupt handler function
 * that should be called when the interrupt occurs, as well as some other information.
 */
struct idt_desc {
    uint16_t offset_1;      // Offset bits 0 - 15
    uint16_t selector;      // Selector thats in our GDT
    uint8_t zero;           // Does nothing, unused set to zero
    uint8_t type_attr;      // Descriptor type and attributes
    uint16_t offset_2;      // Offset bits 16-31
} __attribute__((packed));

/**
 * @brief Structure representing the interrupt descriptor table register (IDTR)
 * 
 * The IDTR is a special register that holds the base address and size of the interrupt
 * descriptor table. When an interrupt occurs, the CPU uses this information to find the
 * correct interrupt handler in the IDT.
 */
struct idtr_desc {
    uint16_t limit;         // Size of descriptor table -1
    uint32_t base;          // Base address of the start of the interrupt descriptor table
} __attribute__((packed));

/**
 * @brief Structure representing the state of the CPU when an interrupt occurs
 * 
 * When an interrupt occurs, the CPU pushes the state of the current process onto the stack
 * before calling the interrupt handler. This structure represents the state that is pushed
 * onto the stack, which includes the values of the general-purpose registers, the instruction
 * pointer, the code segment, the stack segment, and the flags register.
 */
struct interrupt_frame {
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t reserved;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t ip;
    uint32_t cs;
    uint32_t flags;
    uint32_t esp;
    uint32_t ss;
} __attribute__((packed));

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
void register_sys_command(int cmd, sys_cmd handler);

/**
 * @brief Initializes the interrupt descriptor table (IDT) with default handlers
 */
void idt_init(void);

/**
 * @brief Enables interrupts on the CPU
 */
void enable_interrupt(void);

/**
 * @brief Disables interrupts on the CPU
 */
void disable_interrupt(void);

#endif
