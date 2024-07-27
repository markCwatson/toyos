#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include <stdbool.h>
#include "task.h"
#include "config.h"

/**
 * @struct process
 * @brief Represents a process in the system.
 */
struct process {
    uint16_t id;                                        /**< The process ID. */
    char filename[TOYOS_MAX_PATH];                      /**< The filename of the executable. */
    struct task* task;                                  /**< The main task associated with the process. */
    void* allocations[TOYOS_MAX_PROGRAM_ALLOCATIONS];   /**< Memory allocations. */
    void* ptr;                                          /**< Physical pointer to the process memory. */
    void* stack;                                        /**< Physical pointer to the stack memory. */
    uint32_t size;                                      /**< Size of the data pointed to by 'ptr'. */
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

#endif
