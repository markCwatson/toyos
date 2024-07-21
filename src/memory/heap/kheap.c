#include "kheap.h"
#include "heap.h"
#include "config.h"
#include "kernel.h"
#include "memory/memory.h"

struct heap kernel_heap;
struct heap_table kernel_heap_table;

void kheap_init() {
    int total_table_entries = TOYOS_HEAP_SIZE_BYTES / TOYOS_HEAP_BLOCK_SIZE;
    kernel_heap_table.entries = (heap_block_table_entry*)(TOYOS_HEAP_TABLE_ADDRESS);
    kernel_heap_table.total = total_table_entries;

    void* end = (void*)(TOYOS_HEAP_ADDRESS + TOYOS_HEAP_SIZE_BYTES);
    int res = heap_create(&kernel_heap, (void*)(TOYOS_HEAP_ADDRESS), end, &kernel_heap_table);
    if (res < 0) {
        printk("Failed to create heap\n");
    }
}

void* kmalloc(size_t size) {
    return malloc(&kernel_heap, size);
}

void* kzalloc(size_t size) {
    void* ptr = kmalloc(size);
    if (!ptr) {
        return NULL;
    }

    memset(ptr, 0x00, size);
    return ptr;
}

void kfree(void* ptr) {
    free(&kernel_heap, ptr);
}
