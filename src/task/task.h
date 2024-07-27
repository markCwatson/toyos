#ifndef _TASK_H_
#define _TASK_H_

#include "config.h"
#include "memory/paging/paging.h"

/**
 * @brief Represents the CPU registers for a task.
 */
struct registers {
    uint32_t edi;    /**< General-purpose register used for string operations and loop counters */
    uint32_t esi;    /**< General-purpose register used for string operations and data pointers */
    uint32_t ebp;    /**< Base pointer register, typically used to point to the base of the stack frame */
    uint32_t ebx;    /**< General-purpose register often used as a base pointer in memory operations */
    uint32_t edx;    /**< General-purpose register used in input/output operations and as a data register */
    uint32_t ecx;    /**< Counter register, often used for loops and as a count in string operations */
    uint32_t eax;    /**< Accumulator register, used in arithmetic operations and as a return value register */
    uint32_t ip;     /**< Instruction pointer, holds the address of the next instruction to be executed */
    uint32_t cs;     /**< Code segment register, holds the segment selector for the code segment */
    uint32_t flags;  /**< Flags register, contains status flags, control flags, and system flags */
    uint32_t esp;    /**< Stack pointer register, points to the top of the current stack */
    uint32_t ss;     /**< Stack segment register, holds the segment selector for the stack segment */
};

/**
 * Forward declaration of the process structure.
 */
struct process;

/**
 * Forward declaration of the interrupt frame structure.
 */
struct interrupt_frame;

/**
 * @brief Represents a task in the operating system.
 */
struct task {
    struct paging_4gb_chunk* page_directory;    /**< The page directory of the task */
    struct registers registers;                 /**< The saved registers for the task */
    struct process* process;                    /**< The process associated with this task */
    struct task* next;                          /**< Pointer to the next task in the linked list */
    struct task* prev;                          /**< Pointer to the previous task in the linked list */
};

/**
 * @brief Creates a new task for a given process.
 * 
 * @param process The process to associate with the new task.
 * @return Pointer to the created task, or NULL on failure.
 */
struct task* task_new(struct process* process);

/**
 * @brief Gets the next task in the linked list of tasks.
 * 
 * @return Pointer to the next task.
 */
struct task* task_get_next(void);

/**
 * @brief Frees the resources associated with a task.
 * 
 * @param task The task to free.
 * @return 0 on success, or a negative error code on failure.
 */
int task_free(struct task* task);

/**
 * @brief Handles the task return process, restoring registers.
 * 
 * @param regs Pointer to the registers to restore.
 */
void task_return(struct registers* regs);

/**
 * @brief Restores general-purpose registers from the given state.
 * 
 * @param regs Pointer to the registers to restore.
 */
void restore_general_purpose_registers(struct registers* regs);

/**
 * @brief Prepares user-level registers for a new task.
 */
void user_registers(void);

#endif
