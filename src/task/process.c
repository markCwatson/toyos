#include "process.h"
#include "config.h"
#include "status.h"
#include "task/task.h"
#include "memory/memory.h"
#include "memory/heap/kheap.h"
#include "fs/file.h"
#include "string/string.h"
#include "kernel.h"
#include "loader/formats/elfloader.h"

// The current process that is running
struct process* current_process = NULL;

// Array of processes
static struct process* processes[TOYOS_MAX_PROCESSES] = {};

/**
 * Loads a binary file into memory.
 * 
 * @param filename The name of the file to load.
 * @param process The process structure to store the loaded binary.
 * @return 0 on success, error code on failure.
 */
static int process_load_binary(const char* filename, struct process* process) {
   int res = OK;
   int fd = fopen(filename, "r");
    if (fd < 0) {
         return -EIO;
    }

    struct file_stat stat;
    res = fstat(fd, &stat);
    if (res < 0) {
        res = -EIO;
        goto out;
    }

    void* program_data_ptr = kzalloc(stat.filesize);
    if (!program_data_ptr) {
        res = -ENOMEM;
        goto out;
    }

    if (fread(program_data_ptr, stat.filesize, 1, fd) != 1) {
        res = -EIO;
        goto out;
    }

    process->filetype = PROCESS_FILETYPE_BINARY;
    process->ptr = program_data_ptr;
    process->size = stat.filesize;

out:
    fclose(fd);
    return res;
}

/**
 * Loads an ELF file into memory.
 * 
 * @param filename The name of the file to load.
 * @param process The process structure to store the loaded ELF file.
 * @return 0 on success, error code on failure.
 */
static int process_load_elf(const char* filename, struct process* process) {
    int res = OK;
    struct elf_file* elf_file = NULL;
    res = elf_load(filename, &elf_file);
    if (ISERROR(res)) {
        goto out;
    }

    process->filetype = PROCESS_FILETYPE_ELF;
    process->elf_file = elf_file;

out:
    return res;
}

/**
 * Maps the binary data to memory.
 * 
 * @details This function maps the binary data to the virtual address space of the process.
 * 
 * @param process The process to map.
 * @return 0 on success, error code on failure.
 */
static int process_map_binary(struct process* process) {
    int res = OK;
    res = paging_map_to(process->task->page_directory,
                        (void*)TOYOS_PROGRAM_VIRTUAL_ADDRESS,
                        process->ptr,
                        paging_align_address(process->ptr + process->size),
                        PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL | PAGING_IS_WRITEABLE);
    return res;
}

/**
 * Loads data for a process.
 * 
 * @details This function loads data for a process, including ELF files and binary files.
 * 
 * @param filename The name of the file to load.
 * @param process The process structure to store the loaded data.
 * @return 0 on success, error code on failure.
 */
static int process_load_data(const char* filename, struct process* process) {
    int res = process_load_elf(filename, process);
    if (res == -EINFORMAT) {
        res = process_load_binary(filename, process);
    }

    return res;
}

/**
 * Maps the ELF file to memory.
 * 
 * @details This function maps the ELF file to the virtual address space of the process.
 * 
 * @param process The process to map the ELF file for.
 * @return 0 on success, error code on failure.
 */
static int process_map_elf(struct process* process) {
    struct elf_file* elf_file = process->elf_file;

    // Align the virtual base address to the lower page boundary
    void* virtual_base_aligned = paging_align_to_lower_page(elf_virtual_base(elf_file));

    // Get the physical base address of the ELF file
    void* physical_base = elf_phys_base(elf_file);

    // Align the physical end address to the page boundary
    void* physical_end_aligned = paging_align_address(elf_phys_end(elf_file));

    // Set the flags for the paging entry
    uint32_t paging_flags = PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL | PAGING_IS_WRITEABLE;

    // Map the ELF file into the process's address space
    return paging_map_to(process->task->page_directory, virtual_base_aligned, physical_base, physical_end_aligned, paging_flags);
}

/**
 * Initializes a process structure.
 * 
 * @param process The process structure to initialize.
 */
static void process_init(struct process* process) {
    memset(process, 0, sizeof(struct process));
}

/**
 * @brief Maps the memory for a process.
 * 
 * @details This function maps the memory for a process, including the ELF file and stack.
 * 
 * @param process The process to map memory for.
 * @return 0 on success, error code on failure.
 */
static int process_map_memory(struct process* process) {
    int res = 0;

    switch(process->filetype) {
        case PROCESS_FILETYPE_ELF:
            res = process_map_elf(process);
            break;
        case PROCESS_FILETYPE_BINARY:
            res = process_map_binary(process);
            break;
        default:
            panick("process_map_memory: Invalid filetype\n");
    }

    if (res < 0) {
        return res;
    }

    // Map the stack
    res = paging_map_to(process->task->page_directory,
                        (void*)TOYOS_PROGRAM_VIRTUAL_STACK_ADDRESS_END,
                        process->stack,
                        paging_align_address(process->stack + TOYOS_USER_PROGRAM_STACK_SIZE), 
                        PAGING_IS_PRESENT | PAGING_IS_WRITEABLE | PAGING_ACCESS_FROM_ALL);
    if (res < 0) {
        return res;
    }

    return OK;
}

/**
 * Retrieves a free slot in the process array.
 * 
 * @return The index of the free slot, or -EISTKN if no free slots are available.
 */
static int process_get_free_slot(void) {
    for (int i = 0; i < TOYOS_MAX_PROCESSES; i++) {
        if (!processes[i]) {
            return i;
        }
    }

    return -EISTKN;
}

/**
 * Loads a process from the given filename.
 * @param filename The name of the file to load.
 * @param process A pointer to the process structure to store the loaded process.
 * @return 0 on success, error code on failure.
 */
int process_load(const char* filename, struct process** process) {
    int res = OK;

    int process_slot = process_get_free_slot();
    if (process_slot < 0) {
        res = -EISTKN;
        goto out;
    }

    res = process_load_for_slot(filename, process, process_slot);

out:
    return res;
}

/**
 * Switches to the given process.
 * @param process The process to switch to.
 * @return 0 on success, error code on failure.
 */
int process_switch(struct process* process) {
    current_process = process;
    return OK;
}

/**
 * Loads a process from the given filename and switches to it.
 * @param filename The name of the file to load.
 * @param process A pointer to the process structure to store the loaded process.
 * @return 0 on success, error code on failure.
 */
int process_load_switch(const char* filename, struct process** process) {
    if (!process || !filename) {
        return -EINVARG;
    }

    int res = process_load(filename, process);
    if (res == OK) {
        process_switch(*process);
    }

    return res;
}

/**
 * Retrieves the current running process.
 * @return The current process.
 */
struct process* process_current(void) {
    return current_process;
}

/**
 * Retrieves a process by its process ID.
 * @param process_id The process ID.
 * @return The process with the given ID, or NULL if not found.
 */
struct process* process_get(int process_id) {
    if (process_id < 0 || process_id >= TOYOS_MAX_PROCESSES) {
        return NULL;
    }

    return processes[process_id];
}

/**
 * @brief Loads a process into a specific slot.
 * 
 * @details This function loads a process into a specific slot in the process array.
 * 
 * @param filename The name of the file to load.
 * @param process A pointer to the process structure to store the loaded process.
 * @param process_slot The slot to load the process into.
 * @return 0 on success, error code on failure.
 */
int process_load_for_slot(const char* filename, struct process** process, int process_slot) {
    int res = OK;
    struct task* task = NULL;
    struct process* _process = NULL;
    void* program_stack_ptr = NULL;

    if (process_get(process_slot) != OK) {
        res = -EISTKN;
        goto out;
    }

    // Allocate memory for the process
    _process = kzalloc(sizeof(struct process));
    if (!_process) {
        res = -ENOMEM;
        goto out;
    }

    // Load the data for the process
    process_init(_process);
    res = process_load_data(filename, _process);
    if (res < 0) {
        goto out;
    }

    // Allocate a stack for the process
    program_stack_ptr = kzalloc(TOYOS_USER_PROGRAM_STACK_SIZE);
    if (!program_stack_ptr) {
        res = -ENOMEM;
        goto out;
    }

    // Set the process properties
    strncpy(_process->filename, filename, sizeof(_process->filename));
    _process->stack = program_stack_ptr;
    _process->id = process_slot;

    // Create a task
    task = task_new(_process);
    if (task == NULL) {
        res = ERROR_I(task);
        goto out;
    }

    // Set as the main task of the process
    _process->task = task;

    // Map the memory for the process
    res = process_map_memory(_process);
    if (res < 0) {
        goto out;
    }

    *process = _process;

    // Add the process to the array
    processes[process_slot] = _process;

out:
    if (ISERROR(res)) {
        if (_process && _process->task) {
            task_free(_process->task);
        }

        // \todo: see if better way to free the memory
        kfree(_process);
        kfree(program_stack_ptr);
    }

    return res;
}
