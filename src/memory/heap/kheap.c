#include "kheap.h"
#include "config.h"
#include "heap.h"
#include "kernel.h"
#include "memory/memory.h"

// Structure representing the kernel heap and its table
struct heap kernel_heap;
struct heap_table kernel_heap_table;

void kheap_init() {
    // Calculate the total number of blocks in the heap
    int total_table_entries = TOYOS_HEAP_SIZE_BYTES / TOYOS_HEAP_BLOCK_SIZE;

    // Set the base address for the heap table
    kernel_heap_table.entries = (heap_block_table_entry *)(TOYOS_HEAP_TABLE_ADDRESS);
    kernel_heap_table.total = total_table_entries;

    // Determine the end of the heap based on the configured size
    void *end = (void *)(TOYOS_HEAP_ADDRESS + TOYOS_HEAP_SIZE_BYTES);

    // Create the heap structure within the specified memory range
    int res = heap_create(&kernel_heap, (void *)(TOYOS_HEAP_ADDRESS), end, &kernel_heap_table);
    if (res < 0) {
        panick("Failed to create heap\n");
    }
}

void *kmalloc(size_t size) {
    return malloc(&kernel_heap, size);
}

void *kzalloc(size_t size) {
    void *ptr = kmalloc(size);
    if (!ptr) {
        return NULL;  // Return NULL if allocation fails
    }

    memset(ptr, 0x00, size);  // Zero the allocated memory
    return ptr;
}

void kfree(void *ptr) {
    free(&kernel_heap, ptr);
}
