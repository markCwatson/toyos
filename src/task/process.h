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
 * @struct process
 * @brief Represents a process in the system.
 */
struct process {
    uint16_t id;                                        /**< The process ID. */
    char filename[TOYOS_MAX_PATH];                      /**< The filename of the executable. */
    struct task* task;                                  /**< The main task associated with the process. */
    void* allocations[TOYOS_MAX_PROGRAM_ALLOCATIONS];   /**< Memory allocations. */
    void* stack;                                        /**< Physical pointer to the stack memory. */
    uint32_t size;                                      /**< Size of the data pointed to by 'ptr'. */
    struct keyboard_buffer {                            /**< Keyboard buffer for the process. */
        char buffer[TOYOS_KEYBOARD_BUFFER_SIZE];        /**< The buffer. */
        int tail;                                       /**< Tail index. */
        int head;                                       /**< Head index. */
    } keyboard;
    process_filetype filetype;                          /**< The type of file the process is. */
    union {                                             /**< File data. */
        void* ptr;                                      /**< Pointer to the process memory. */
        struct elf_file* elf_file;                      /**< Pointer to the ELF file structure. */
    };
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

#endif
