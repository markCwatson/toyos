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
 * @brief Retrieves the current running task.
 * 
 * @return Pointer to the current task.
 */
struct task* task_current(void);

/**
 * @brief Switches to a new task
 * 
 * @param task The task to switch to
 * @return int Returns 0 on success, negative value on failure
 */
int task_switch(struct task *task);

/**
 * @brief Saves the state of the current task
 * 
 * @param frame The interrupt frame containing the CPU state
 * @return void
 */
void task_current_save_state(struct interrupt_frame* frame);

/**
 * @brief Switches to the next task in the linked list
 * 
 * @details This function is called by the timer interrupt handler to switch to 
 * the next task in the linked list of tasks.
 * 
 * @return int Returns 0 on success, negative value on failure
 */
int task_page(void);

/**
 * @brief Runs the first ever task
 */
void task_run_first_ever_task(void);

/**
 * @brief Frees the resources associated with a task.
 * 
 * @param task The task to free.
 * @return 0 on success, or a negative error code on failure.
 */
int task_free(struct task* task);

/**
 * @brief Retrieves the value of a stack item for a given task
 * 
 * @details This function retrieves the value of a stack item for a given task. It switches to the
 * task's page directory, retrieves the value of the stack item, and then switches back to the kernel
 * page directory.
 * 
 * @param task The task to retrieve the stack item from
 * @param index The index of the stack item to retrieve
 * @return void* The value of the stack item
 */
void* task_get_stack_item(struct task* task, int index);

/**
 * @brief Copies a string from a task's memory to the kernel space
 * 
 * @details This function copies a string from a task's memory to the kernel space. It allocates
 * memory in the kernel space to store the string, copies the string from the task's memory to the
 * kernel space, and then copies the string to the physical address provided.
 * 
 * @param task The task to copy the string from
 * @param virtual The virtual address of the string in the task's memory
 * @param phys The physical address to copy the string to
 * @param max The maximum number of bytes to copy
 * @return int Returns 0 on success, negative value on failure
 */
int copy_string_from_task(struct task* task, void* virtual, void* phys, int max);

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
