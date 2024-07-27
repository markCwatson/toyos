#include "task.h"
#include "kernel.h"
#include "status.h"
#include "process.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"
#include "string/string.h"
#include "memory/paging/paging.h"
// #include "loader/formats/elfloader.h"
#include "idt/idt.h"

// The current task that is running
struct task* current_task = NULL; 

// Task linked list
struct task* task_tail = NULL;
struct task* task_head = NULL;

/**
 * @brief Initializes a task structure
 * 
 * @param task The task structure to initialize
 * @param process The process associated with the task
 * @return int Returns 0 on success, negative value on failure
 */
static int task_init(struct task* task, struct process *process);

/**
 * @brief Creates a new task for a given process
 * 
 * @param process The process to associate with the new task
 * @return struct task* Pointer to the newly created task, or an error code cast to a pointer on failure
 */
struct task* task_new(struct process *process) {
    if (!process) {
        return ERROR(-EINVARG);
    }

    int res = ALL_GOOD;

    struct task* task = kzalloc(sizeof(struct task));
    if (!task) {
        res = -ENOMEM;
        goto out;
    }

    res = task_init(task, process);
    if (res != ALL_GOOD) {
        goto out;
    }

    if (task_head == NULL) {
        task_head = task;
        task_tail = task;
        current_task = task;
        goto out;
    }

    task_tail->next = task;
    task->prev = task_tail;
    task_tail = task;

out:
    if (ISERROR(res)) {
        task_free(task);
        return ERROR(res);
    }

    return task;
}

/**
 * @brief Retrieves the next task in the linked list
 * 
 * @return struct task* Pointer to the next task structure, or task_head if at the end of the list
 */
struct task* task_get_next(void) {
    if (!current_task->next) {
        return task_head;
    }

    return current_task->next;
}

/**
 * @brief Removes a task from the task linked list
 * 
 * @param task The task to remove
 */
static void task_list_remove(struct task* task) {
    if (!task) {
        return;
    }

    if (task->prev) {
        task->prev->next = task->next;
    }

    if (task == task_head) {
        task_head = task->next;
    }

    if (task == task_tail) {
        task_tail = task->prev;
    }

    if (task == current_task) {
        current_task = task_get_next();
    }
}

/**
 * @brief Frees the resources associated with a task
 * 
 * @param task The task to free
 * @return int Returns 0 on success or a negative error code on failure
 */
int task_free(struct task* task) {
    if (!task) {
        return -EINVARG;
    }

    paging_free_4gb(task->page_directory);
    task_list_remove(task);

    // Finally free the task data
    kfree(task);
    return ALL_GOOD;
}

/**
 * @brief Saves the state of the current task
 * 
 * @param task The task whose state is being saved
 * @param frame The interrupt frame containing the CPU state
 */
void task_save_state(struct task* task, struct interrupt_frame* frame) {
    if (!task || !frame) {
        return;
    }

    task->registers.ip = frame->ip;
    task->registers.cs = frame->cs;
    task->registers.flags = frame->flags;
    task->registers.esp = frame->esp;
    task->registers.ss = frame->ss;
    task->registers.eax = frame->eax;
    task->registers.ebp = frame->ebp;
    task->registers.ebx = frame->ebx;
    task->registers.ecx = frame->ecx;
    task->registers.edi = frame->edi;
    task->registers.edx = frame->edx;
    task->registers.esi = frame->esi;
}

/**
 * @brief Initializes a task structure with a given process
 * 
 * @param task The task structure to initialize
 * @param process The process associated with the task
 * @return int Returns 0 on success, negative value on failure
 */
static int task_init(struct task* task, struct process* process) {
    if (!task || !process) {
        return -EINVARG;
    }

    memset(task, 0, sizeof(struct task));

    // Map the entire 4GB address space to its self
    task->page_directory = paging_new_4gb(PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);
    if (!task->page_directory) {
        return -EIO;
    }

    task->registers.ss = TOYOS_USER_DATA_SEGMENT;
    task->registers.cs = TOYOS_USER_CODE_SEGMENT;
    task->registers.esp = TOYOS_PROGRAM_VIRTUAL_STACK_ADDRESS_START;

    task->process = process;

    return ALL_GOOD;
}
