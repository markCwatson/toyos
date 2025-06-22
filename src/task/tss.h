#ifndef _TASKSWITCHSEGMENT_H_
#define _TASKSWITCHSEGMENT_H_

#include <stdint.h>

/**
 * @brief Task State Segment (TSS) structure.
 *
 * This structure defines the Task State Segment (TSS) in the x86 architecture.
 * The TSS is used to hold information about a task, including its registers, stack pointers,
 * and segment selectors. The TSS is used during task switching to save the state of the
 * current task and restore the state of the next task.
 */
struct tss {
    uint32_t link;   /**< Unused link to the previous TSS (for hardware task switching) */
    uint32_t esp0;   /**< Stack pointer for ring 0 (used during privilege level changes) */
    uint32_t ss0;    /**< Stack segment for ring 0 */
    uint32_t esp1;   /**< Stack pointer for ring 1 (unused) */
    uint32_t esp2;   /**< Stack pointer for ring 2 (unused) */
    uint32_t ss2;    /**< Stack segment for ring 2 (unused) */
    uint32_t sr3;    /**< Reserved */
    uint32_t eip;    /**< Instruction pointer */
    uint32_t eflags; /**< Flags register */
    uint32_t eax;    /**< General-purpose register EAX */
    uint32_t ecx;    /**< General-purpose register ECX */
    uint32_t edx;    /**< General-purpose register EDX */
    uint32_t ebx;    /**< General-purpose register EBX */
    uint32_t esp;    /**< Stack pointer */
    uint32_t ebp;    /**< Base pointer */
    uint32_t esi;    /**< General-purpose register ESI */
    uint32_t edi;    /**< General-purpose register EDI */
    uint32_t es;     /**< Segment selector ES */
    uint32_t cs;     /**< Segment selector CS */
    uint32_t ss;     /**< Segment selector SS */
    uint32_t ds;     /**< Segment selector DS */
    uint32_t fs;     /**< Segment selector FS */
    uint32_t gs;     /**< Segment selector GS */
    uint32_t ldtr;   /**< Local Descriptor Table register */
    uint32_t iopb;   /**< I/O Map Base Address */
} __attribute__((packed));

/**
 * @brief Loads the TSS.
 *
 * This function loads the Task State Segment (TSS) using the `ltr` instruction,
 * which sets the TR register to point to the TSS descriptor in the GDT.
 *
 * @param tss_segment The segment selector for the TSS descriptor in the GDT.
 */
void tss_load(int tss_segment);

#endif
