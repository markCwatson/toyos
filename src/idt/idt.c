#include "idt.h"
#include "config.h"
#include "kernel.h"
#include "memory/memory.h"
#include "io/io.h"

struct idt_desc idt_descriptors[TOYOS_TOTAL_INTERRUPTS];
struct idtr_desc idtr_descriptor;

extern void int21h(void);
extern void no_int(void);
extern void idt_load(struct idtr_desc* ptr);

void no_int_handler()
{
    // ack interrupt
    outb(0x20, 0x20);
}

void int21h_handler()
{
    printk("\nKeyboard pressed\n");

    // ack interrupt
    outb(0x20, 0x20);
}

void idt_zero()
{
    printk("\nDivide by zero error!\n");
}

void idt_set(int interrupt_no, void* address)
{
    struct idt_desc* desc = &idt_descriptors[interrupt_no];
    desc->offset_1 = (uint32_t)address & 0x0000ffff;
    desc->selector = KERNEL_CODE_SELECTOR;
    desc->zero = 0x00;
    desc->type_attr = 0xee;
    desc->offset_2 = (uint32_t)address >> 16;
}

void idt_init()
{
    memset(idt_descriptors, 0, sizeof(idt_descriptors));
    idtr_descriptor.limit = sizeof(idt_descriptors) - 1;
    idtr_descriptor.base = (uint32_t)idt_descriptors;

    for (int i = 0; i < TOYOS_TOTAL_INTERRUPTS; i++)
    {
        idt_set(i, no_int);
    }

    // int 0 (divide by zero)
    idt_set(0, idt_zero);
    // key board interrupt handler
    idt_set(0x21, int21h);

    // Load the interrupt descriptor table
    idt_load(&idtr_descriptor);
}
