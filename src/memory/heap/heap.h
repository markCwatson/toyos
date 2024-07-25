#ifndef HEAP_H
#define HEAP_H

#include "config.h"
#include <stdint.h>
#include <stddef.h>

// Definitions for heap block table entries
#define HEAP_BLOCK_TABLE_ENTRY_TAKEN 0x01     /**< Indicates a block is allocated (taken). */
#define HEAP_BLOCK_TABLE_ENTRY_FREE 0x00      /**< Indicates a block is free. */

#define HEAP_BLOCK_HAS_NEXT 0b10000000        /**< Indicates that the block has a subsequent block. */
#define HEAP_BLOCK_IS_FIRST 0b01000000        /**< Indicates that the block is the first in a series. */

typedef unsigned char heap_block_table_entry; /**< Type definition for heap block table entries. */

/**
 * @brief Structure representing a heap's block table.
 * 
 * The block table keeps track of the status of blocks in the heap, such as whether they are free or allocated.
 */
struct heap_table {
    heap_block_table_entry* entries; /**< Pointer to the array of block entries. */
    size_t total;                    /**< Total number of entries in the table. */
};

/**
 * @brief Structure representing a heap.
 * 
 * This structure holds information about the heap, including its block table and the start address of the heap memory.
 */
struct heap {
    struct heap_table* table; /**< Pointer to the heap's block table. */

    // Start address of the heap data pool
    void* saddr;              /**< Start address of the memory managed by the heap. */
};

/**
 * @brief Initializes a heap structure.
 * 
 * Sets up a heap with a specified memory range and block table. This function initializes the heap's
 * block table and prepares the heap for memory allocations.
 * 
 * @param heap Pointer to the heap structure to initialize.
 * @param ptr Start address of the heap memory range.
 * @param end End address of the heap memory range.
 * @param table Pointer to the heap block table.
 * @return 0 on success, or a negative error code on failure.
 */
int heap_create(struct heap* heap, void* ptr, void* end, struct heap_table* table);

/**
 * @brief Allocates a block of memory from the heap.
 * 
 * This function allocates a block of memory of the specified size from the given heap. The allocated memory
 * is not initialized.
 * 
 * @param heap Pointer to the heap from which to allocate memory.
 * @param size The size of the memory block to allocate, in bytes.
 * @return A pointer to the allocated memory block, or NULL if the allocation fails.
 */
void* malloc(struct heap* heap, size_t size);

/**
 * @brief Frees a previously allocated block of memory.
 * 
 * Releases a block of memory back to the heap, making it available for future allocations.
 * The block must have been previously allocated using malloc.
 * 
 * @param heap Pointer to the heap from which the memory was allocated.
 * @param ptr Pointer to the memory block to free.
 */
void free(struct heap* heap, void* ptr);

#endif
