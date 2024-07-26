#include "kheap.h"
#include "heap.h"
#include "config.h"
#include "kernel.h"
#include "memory/memory.h"

// Structure representing the kernel heap and its table
struct heap kernel_heap;
struct heap_table kernel_heap_table;

/**
 * @brief Initializes the kernel heap.
 * 
 * This function sets up the kernel heap by creating a heap structure at a specified memory
 * region defined by the configuration. It calculates the total number of entries in the heap table
 * and initializes the heap with this table.
 */
void kheap_init() {
    // Calculate the total number of blocks in the heap
    int total_table_entries = TOYOS_HEAP_SIZE_BYTES / TOYOS_HEAP_BLOCK_SIZE;
    
    // Set the base address for the heap table
    kernel_heap_table.entries = (heap_block_table_entry*)(TOYOS_HEAP_TABLE_ADDRESS);
    kernel_heap_table.total = total_table_entries;

    // Determine the end of the heap based on the configured size
    void* end = (void*)(TOYOS_HEAP_ADDRESS + TOYOS_HEAP_SIZE_BYTES);
    
    // Create the heap structure within the specified memory range
    int res = heap_create(&kernel_heap, (void*)(TOYOS_HEAP_ADDRESS), end, &kernel_heap_table);
    if (res < 0) {
        panick("Failed to create heap\n");
    }
}

/**
 * @brief Allocates a block of memory from the kernel heap.
 * 
 * This function allocates a block of memory of the specified size from the kernel heap.
 * 
 * @param size The size of the memory block to allocate, in bytes.
 * @return A pointer to the allocated memory block, or NULL if the allocation fails.
 */
void* kmalloc(size_t size) {
    return malloc(&kernel_heap, size);
}

/**
 * @brief Allocates and zeroes a block of memory from the kernel heap.
 * 
 * This function allocates memory and initializes it to zero. It's useful when the allocated memory
 * needs to be in a known state.
 * 
 * @param size The size of the memory block to allocate, in bytes.
 * @return A pointer to the allocated and zeroed memory block, or NULL if the allocation fails.
 */
void* kzalloc(size_t size) {
    void* ptr = kmalloc(size);
    if (!ptr) {
        return NULL;  // Return NULL if allocation fails
    }

    memset(ptr, 0x00, size);  // Zero the allocated memory
    return ptr;
}

/**
 * @brief Frees a previously allocated block of memory.
 * 
 * This function frees the memory block pointed to by ptr, returning it to the kernel heap.
 * 
 * @param ptr Pointer to the memory block to free.
 */
void kfree(void* ptr) {
    free(&kernel_heap, ptr);
}
