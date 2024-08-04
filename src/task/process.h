#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include <stdbool.h>
#include "task.h"
#include "config.h"

// File types supported by the process loader
#define PROCESS_FILETYPE_ELF    0
#define PROCESS_FILETYPE_BINARY 1

typedef unsigned char process_filetype;

/**
 * @struct process_allocation
 * Represents a memory allocation for a process.
 */
struct process_allocation {
    void* ptr;
    size_t size;
};

/**
 * @struct command_argument
 * Represents an argument in a command.
 */
struct command_argument {
    char argument[512];
    struct command_argument* next;
};

/**
 * @struct process_arguments
 * Represents the arguments of a process.
 */
struct process_arguments {
    int argc;
    char** argv;
};

/**
 * @struct process_info
 * Represents information about a process.
 */
struct process_info {
    int id;
    char filename[64];
};

/**
 * @struct process
 * Represents a process in the system.
 */
struct process {
    uint16_t id;                                                            /**< The process ID. */
    char filename[TOYOS_MAX_PATH];                                          /**< The filename of the executable. */
    struct task* task;                                                      /**< The main task associated with the process. */
    struct process_allocation allocations[TOYOS_MAX_PROGRAM_ALLOCATIONS];   /**< Memory allocations. */
    void* stack;                                                            /**< Physical pointer to the stack memory. */
    uint32_t size;                                                          /**< Size of the data pointed to by 'ptr'. */
    struct keyboard_buffer {                                                /**< Keyboard buffer for the process. */
        char buffer[TOYOS_KEYBOARD_BUFFER_SIZE];                            /**< The buffer. */
        int tail;                                                           /**< Tail index. */
        int head;                                                           /**< Head index. */
    } keyboard;
    process_filetype filetype;                                              /**< The type of file the process is. */
    union {                                                                 /**< File data. */
        void* ptr;                                                          /**< Pointer to the process memory. */
        struct elf_file* elf_file;                                          /**< Pointer to the ELF file structure. */
    };
    struct process_arguments arguments;                                     /**< The arguments of the process. */
};

/**
 * Loads a process from the given filename.
 * @param filename The name of the file to load.
 * @param process A pointer to the process structure to store the loaded process.
 * @return 0 on success, error code on failure.
 */
int process_load(const char* filename, struct process** process);

/**
 * Loads a process into a specific slot.
 * @param filename The name of the file to load.
 * @param process A pointer to the process structure to store the loaded process.
 * @param process_slot The slot to load the process into.
 * @return 0 on success, error code on failure.
 */
int process_load_for_slot(const char* filename, struct process** process, int process_slot);

/**
 * Retrieves the current running process.
 * @return The current process.
 */
struct process* process_current(void);

/**
 * Retrieves a process by its process ID.
 * @param process_id The process ID.
 * @return The process with the given ID, or NULL if not found.
 */
struct process* process_get(int process_id);

/**
 * Switches to the given process.
 * @param process The process to switch to.
 * @return 0 on success, error code on failure.
 */
int process_switch(struct process* process);

/**
 * Loads a process from the given filename and switches to it.
 * @param filename The name of the file to load.
 * @param process A pointer to the process structure to store the loaded process.
 * @return 0 on success, error code on failure.
 */
int process_load_switch(const char* filename, struct process** process);

/**
 * Allocates memory for a process.
 * 
 * This function allocates memory for a process. The memory is allocated from the process's
 * memory space, and is not shared with other processes.
 * 
 * @param process The process to allocate memory for.
 * @param size The size of the memory to allocate.
 * @return void* The address of the allocated memory.
 */
void* process_malloc(struct process* process, size_t size);

/**
 * Frees memory allocated for a process.
 * 
 * @param process The process to free memory for.
 * @param ptr The pointer to the memory to free.
 * @return void
 */
void process_free(struct process* process, void* ptr);

/**
 * Terminates a process.
 * 
 * This function terminates a process and frees all resources associated with it.
 * 
 * @param process The process to terminate.
 * @return The status of the operation.
 */
int process_terminate(struct process* process);

/**
 * Retrieves the arguments for a process.
 * 
 * @param process The process to retrieve arguments for.
 * @param argc A pointer to store the number of arguments.
 * @param argv A pointer to store the arguments.
 */
void process_get_arguments(struct process* process, int* argc, char*** argv);

/**
 * Injects arguments into a process.
 * 
 * @param process The process to inject arguments into.
 * @param root_argument The root argument in the list.
 * @return 0 on success, error code on failure.
 */
int process_inject_arguments(struct process* process, struct command_argument* root_argument);

/**
 * Forks a new process from the given parent process.
 *
 * This function creates a new child process by duplicating the state of the
 * parent process. It allocates necessary memory for the child process structure,
 * loads the process data, creates a new task for the process, and maps the memory.
 * It also copies the parent's memory allocations to the child process.
 *
 * @param parent The parent process from which to fork the new process.
 * @param[out] child A pointer to store the address of the newly created child process.
 * @return Returns 0 (OK) on success, or a negative error code on failure.
 */
int process_fork(struct process* parent, struct process** child);

/**
 * Waits for a process to finish execution.
 * 
 * This function waits for a process to finish execution. It blocks the current process
 * until the target process has finished execution.
 * 
 * @param process The process to wait for.
 * @return 0 on success, error code on failure.
 */
int process_wait(struct process* process);

/**
 * Kills a process.
 * 
 * This function kills a process. It terminates the process and frees all resources
 * associated with it.
 * 
 * @param process The process to kill.
 * @return void
 */
void process_kill(struct process* process);

/**
 * Replaces the current process with a new program.
 * 
 * This function replaces the current process with a new program. It loads the new program
 * into memory, maps the memory for the new program, and frees the memory and state of the
 * current process.
 * 
 * @param process The process to replace.
 * @param path The path to the new program.
 * @return 0 on success, error code on failure.
 */
int process_load_replace(struct process* process, const char* path);

#endif
