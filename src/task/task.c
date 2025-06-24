#include "task.h"
#include "idt/idt.h"
#include "kernel.h"
#include "loader/formats/elfloader.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"
#include "memory/paging/paging.h"
#include "process.h"
#include "status.h"
#include "stdlib/string.h"

// The current task that is running
struct task *current_task = NULL;

// Task linked list
struct task *task_tail = NULL;
struct task *task_head = NULL;

/**
 * @brief Initializes a task structure
 *
 * @param task The task structure to initialize
 * @param process The process associated with the task
 * @return int Returns 0 on success, negative value on failure
 */
static int task_init(struct task *task, struct process *process);

/**
 * @brief Removes a task from the task linked list
 *
 * @param task The task to remove
 */
static void task_list_remove(struct task *task) {
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
 * @brief Saves the state of the current task
 *
 * @param task The task whose state is being saved
 * @param frame The interrupt frame containing the CPU state
 */
void task_save_state(struct task *task, struct interrupt_frame *frame) {
    if (!task || !frame) {
        return;
    }

    // Save the CPU state from the interrupt frame to the task
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
static int task_init(struct task *task, struct process *process) {
    if (!task || !process) {
        return -EINVARG;
    }

    memset(task, 0, sizeof(struct task));

    // Map the entire 4GB address space to its self
    task->page_directory = paging_new_4gb(PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);
    if (!task->page_directory) {
        return -EIO;
    }

    // Set the ip to the program's entry point
    task->registers.ip = TOYOS_PROGRAM_VIRTUAL_ADDRESS;
    if (process->filetype == PROCESS_FILETYPE_ELF) {
        task->registers.ip = elf_header(process->elf_file)->e_entry;
    }

    task->registers.ss = TOYOS_USER_DATA_SEGMENT;
    task->registers.cs = TOYOS_USER_CODE_SEGMENT;
    task->registers.esp = TOYOS_PROGRAM_VIRTUAL_STACK_ADDRESS_START;

    task->process = process;

    return OK;
}

/**
 * @brief Switches to page directory for a given task
 *
 * @param task The task to switch to
 * @return int Returns 0 on success, negative value on failure
 */
static int task_page_task(struct task *task) {
    user_registers();
    paging_switch(task->page_directory);
    return OK;
}

struct task *task_current(void) {
    return current_task;
}

struct task *task_new(struct process *process) {
    if (!process) {
        return ERROR(-EINVARG);
    }

    int res = OK;

    struct task *task = kzalloc(sizeof(struct task));
    if (!task) {
        res = -ENOMEM;
        goto out;
    }

    res = task_init(task, process);
    if (res != OK) {
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

struct task *task_get_next(void) {
    if (!current_task->next) {
        return task_head;
    }

    return current_task->next;
}

int copy_string_from_task(struct task *task, void *virtual, void *phys, int max) {
    if (max >= PAGING_PAGE_SIZE || !task || !virtual || !phys) {
        return -EINVARG;
    }

    // Allocate memory in the kernel space
    char *tmp = kzalloc(max);
    if (!tmp) {
        return -ENOMEM;
    }

    // Get the task's page directory and the old entry for the virtual address
    uint32_t *task_directory = task->page_directory->directory_entry;
    uint32_t old_entry = paging_get(task_directory, tmp);

    // Map the memory in the task's page directory to the kernel space using the temporary buffer
    paging_map(task->page_directory, tmp, tmp, PAGING_IS_WRITEABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);

    // Switch to the task's page directory and copy the string to the kernel space
    paging_switch(task->page_directory);
    strncpy(tmp, virtual, max);

    // Switch back to the kernel page directory
    kernel_page();

    // Unmap the memory in the task's page directory
    int res = paging_set(task_directory, tmp, old_entry);
    if (res < 0) {
        res = -EIO;
        goto out;
    }

    // Copy the string to the physical address
    strncpy(phys, tmp, max);

out:
    kfree(tmp);
    return res;
}

int task_free(struct task *task) {
    if (!task) {
        return -EINVARG;
    }

    paging_free_4gb(task->page_directory);
    task_list_remove(task);

    // Finally free the task data
    kfree(task);
    return OK;
}

void task_current_save_state(struct interrupt_frame *frame) {
    struct task *task = task_current();
    if (!task) {
        panick("[task_current_save_state] No current task exists!\n");
    }

    task_save_state(task, frame);
}

int task_switch(struct task *task) {
    current_task = task;
    paging_switch(task->page_directory);
    return OK;
}

int task_page(void) {
    user_registers();
    task_switch(current_task);
    return OK;
}

void task_run_first_ever_task(void) {
    if (!current_task) {
        panick("[task_run_first_ever_task] No current task exists!\n");
    }

    task_switch(task_head);
    task_return(&task_head->registers);
}

void *task_get_stack_item(struct task *task, int index) {
    if (index < 0) {
        alertk("[task_get_stack_item] Invalid index provided!\n");
        return NULL;
    }

    if (!task) {
        alertk("[task_get_stack_item] No task provided!\n");
        return NULL;
    }

    void *result = 0;

    // Get the stack pointer from the task
    uint32_t *sp_ptr = (uint32_t *)task->registers.esp;

    // Switch to the given tasks page
    task_page_task(task);

    result = (void *)sp_ptr[index];

    // Switch back to the kernel page
    kernel_page();

    return result;
}

void task_next(void) {
    struct task *next_task = task_get_next();
    if (!next_task) {
        panick("No more tasks!\n");
    }

    task_switch(next_task);
    task_return(&next_task->registers);
}

void *task_virtual_address_to_physical(struct task *task, void *virtual_address) {
    return paging_get_physical_address(task->page_directory->directory_entry, virtual_address);
}
