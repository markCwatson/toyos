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
#include <stdbool.h>

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
 * Maps the ELF file to the process's virtual address space.
 * 
 * @details This function takes an ELF file associated with a process and maps its segments 
 * into the process's virtual address space. This is a crucial step in setting up the process 
 * for execution, as it ensures that the necessary code and data segments are accessible in 
 * memory according to the specifications provided in the ELF headers.
 * 
 * The function iterates over each program header in the ELF file, extracting the physical 
 * memory address and other properties like permissions. It aligns the virtual and physical 
 * addresses to page boundaries and sets appropriate flags (such as writeable if the segment 
 * can be written to). The mapped regions in memory are then accessible to the process, 
 * allowing it to execute the code or access data contained in these segments.
 * 
 * @param process The process structure that includes the ELF file to be mapped.
 * @return 0 on success, an error code on failure.
 */
static int process_map_elf(struct process* process) {
    int res = OK;
    struct elf_file* elf_file = process->elf_file;
    struct elf_header* header = elf_header(elf_file);
    struct elf32_phdr* phdrs = elf_pheader(header);
    
    // Map the ELF file into the process's address space by mapping each program header
    for (int i = 0; i < header->e_phnum; i++) {
        // Get the program header from the table
        struct elf32_phdr* phdr = &phdrs[i];
        void* phdr_phys_address = elf_phdr_phys_address(elf_file, phdr);
        
        int flags = PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL;
        if (phdr->p_flags & PF_W) {
            flags |= PAGING_IS_WRITEABLE;
        }

        // Align the virtual base address to the lower page boundary
        void* virt = paging_align_to_lower_page((void*)phdr->p_vaddr);

        // Get the physical base address of the ELF file
        void* phys = paging_align_to_lower_page(phdr_phys_address);

        // Align the physical end address to the page boundary
        void* phys_end = paging_align_address(phdr_phys_address + phdr->p_memsz);
        
        // Map the ELF file into the process's virtual address space
        res = paging_map_to(process->task->page_directory, virt, phys, phys_end, flags);
        if (res < 0) {
            break;
        }
    }

    return res;
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
 * Finds a free allocation index for a process.
 * 
 * @param process The process to find an allocation index for.
 * @return The index of the free allocation, or -ENOMEM if no free allocations are available.
 */
static int process_find_free_allocation_index(struct process* process) {
    int res = -ENOMEM;
    for (int i = 0; i < TOYOS_MAX_PROGRAM_ALLOCATIONS; i++) {
        if (process->allocations[i].ptr == NULL) {
            res = i;
            break;
        }
    }

    return res;
}

/**
 * Checks if a pointer is allocated to a process.
 * 
 * @param process The process to check.
 * @param ptr The pointer to check.
 * @return true if the pointer is allocated to the process, false otherwise.
 */
static bool process_is_process_pointer(struct process* process, void* ptr) {
    for (int i = 0; i < TOYOS_MAX_PROGRAM_ALLOCATIONS; i++) {
        if (process->allocations[i].ptr == ptr)
            return true;
    }

    return false;
}

/**
 * Unjoins an allocation from a process.
 * 
 * @param process The process to unjoin the allocation from.
 * @param ptr The pointer to unjoin.
 */
static void process_allocation_unjoin(struct process* process, void* ptr) {
    for (int i = 0; i < TOYOS_MAX_PROGRAM_ALLOCATIONS; i++) {
        if (process->allocations[i].ptr == ptr) {
            process->allocations[i].ptr = 0x00;;
            process->allocations[i].size = 0;
        }
    }
}

/**
 * Retrieves an allocation by its address.
 * 
 * @param process The process to retrieve the allocation from.
 * @param addr The address of the allocation.
 * @return The allocation structure, or NULL if not found.
 */
static struct process_allocation* process_get_allocation_by_addr(struct process* process, void* addr) {
    for (int i = 0; i < TOYOS_MAX_PROGRAM_ALLOCATIONS; i++) {
        if (process->allocations[i].ptr == addr)
            return &process->allocations[i];
    }

    return 0;
}

/**
 * Frees memory allocated for a process.
 * 
 * @param process The process to free memory for.
 * @param ptr The pointer to the memory to free.
 * @return void
 */
void process_free(struct process* process, void* ptr) {
    // Unlink the pages from the process for the given address
    struct process_allocation* allocation = process_get_allocation_by_addr(process, ptr);
    if (!allocation) {
        // Not our pointer.
        return;
    }

    int res = paging_map_to(process->task->page_directory, allocation->ptr, allocation->ptr, paging_align_address(allocation->ptr + allocation->size), 0x00);
    if (res < 0) {
        return;
    }

    // Unjoin the allocation
    process_allocation_unjoin(process, ptr);

    // We can now free the memory.
    kfree(ptr);
}

/**
 * Allocates memory for a process.
 * 
 * This function allocates memory for a process. The memory is allocated from the process's
 * memory space, and is not shared with other processes.
 * 
 * @param process The process to allocate memory for.
 * @param size The size of the memory to allocate.
 * @return void* The address of the allocated memory or NULL if allocation failed.
 */
void* process_malloc(struct process* process, size_t size) {
    void* ptr = kzalloc(size);
    if (!ptr) {
        goto out_err;
    }

    int index = process_find_free_allocation_index(process);
    if (index < 0) {
        goto out_err;
    }

    // Map the memory to the process's address space
    int res = paging_map_to(process->task->page_directory, ptr, ptr, paging_align_address(ptr + size), PAGING_IS_WRITEABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);
    if (res < 0) {
        goto out_err;
    }

    process->allocations[index].ptr = ptr;
    process->allocations[index].size = size;
    return ptr;

out_err:
    if(ptr) {
        kfree(ptr);
    }

    return NULL;
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

/**
 * @brief Terminates the allocations for a process.
 * 
 * @details This function terminates the allocations for a process, freeing the memory
 * associated with each allocation.
 * 
 * @param process The process to terminate allocations for.
 * @return 0 on success, error code on failure.
 */
int process_terminate_allocations(struct process* process) {
    for (int i = 0; i < TOYOS_MAX_PROGRAM_ALLOCATIONS; i++) {
        process_free(process, process->allocations[i].ptr);
    }

    return OK;
}

/**
 * @brief Frees the binary data associated with a process.
 * 
 * @details This function frees the binary data associated with a process, deallocating the memory
 * used to store the binary data.
 * 
 * @param process The process to free binary data for.
 * @return 0 on success, error code on failure.
 */
int process_free_binary_data(struct process* process) {
    kfree(process->ptr);
    return OK;
}

/**
 * @brief Frees the ELF data associated with a process.
 * 
 * @details This function frees the ELF data associated with a process, deallocating the memory
 * used to store the ELF file structure.
 * 
 * @param process The process to free ELF data for.
 * @return 0 on success, error code on failure.
 */
int process_free_elf_data(struct process* process) {
    elf_close(process->elf_file);
    return OK;
}

/**
 * @brief Frees the program data associated with a process.
 * 
 * @details This function frees the program data associated with a process, including the binary
 * data or ELF data.
 * 
 * @param process The process to free program data for.
 * @return 0 on success, error code on failure.
 */
int process_free_program_data(struct process* process) {
    int res = OK;

    switch(process->filetype) {
        case PROCESS_FILETYPE_BINARY:
            res = process_free_binary_data(process);
            break;

        case PROCESS_FILETYPE_ELF:
            res = process_free_elf_data(process);
            break;

        default:
            res = -EINVARG;
    }

    return res;
}

/**
 * @brief Switches to the next process in the process array.
 * 
 * @details This function switches to the next process in the process array. If the current process
 * is the last process in the array, it switches to the first process in the array.
 */
void process_switch_to_any(void) {
    for (int i = 0; i < TOYOS_MAX_PROCESSES; i++) {
        if (processes[i]) {
            process_switch(processes[i]);
            return;
        }
    }

    panick("No processes to switch too\n");
}

/**
 * @brief Unlinks a process from the process array.
 * 
 * @details This function unlinks a process from the process array, setting the slot to NULL.
 * 
 * @param process The process to unlink.
 */
static void process_unlink(struct process* process) {
    processes[process->id] = NULL;

    if (current_process == process) {
        process_switch_to_any();
    }
}

/**
 * @brief Terminates a process.
 * 
 * @details This function terminates a process, freeing the memory associated with the process.
 * 
 * @param process The process to terminate.
 * @return 0 on success, error code on failure.
 */
int process_terminate(struct process* process) {
    int res = OK;

    res = process_terminate_allocations(process);
    if (res < 0) {
        goto out;
    }

    res = process_free_program_data(process);
    if (res < 0) {
        goto out;
    }

    // Free the process stack memory.
    kfree(process->stack);
    // Free the task
    task_free(process->task);
    // Unlink the process from the process array.
    process_unlink(process);

out:
    return res;
}
