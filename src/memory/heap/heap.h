#ifndef HEAP_H
#define HEAP_H

#include "config.h"
#include <stdint.h>
#include <stddef.h>

#define HEAP_BLOCK_TABLE_ENTRY_TAKEN 0x01
#define HEAP_BLOCK_TABLE_ENTRY_FREE 0x00

#define HEAP_BLOCK_HAS_NEXT 0b10000000
#define HEAP_BLOCK_IS_FIRST  0b01000000

typedef unsigned char heap_block_table_entry;

struct heap_table {
    heap_block_table_entry* entries;
    size_t total;
};

struct heap {
    struct heap_table* table;

    // Start address of the heap data pool
    void* saddr;
};

int heap_create(struct heap* heap, void* ptr, void* end, struct heap_table* table);
void* malloc(struct heap* heap, size_t size);
void free(struct heap* heap, void* ptr);

#endif
