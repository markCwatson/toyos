#include "heap.h"
#include "kernel.h"
#include "status.h"
#include "memory/memory.h"
#include <stdbool.h>

static int heap_validate_table(void* ptr, void* end, struct heap_table* table)
{
    size_t table_size = (size_t)(end - ptr);
    size_t total_blocks = table_size / TOYOS_HEAP_BLOCK_SIZE;

    if (table->total != total_blocks)
    {
        return -EINVARG;
    }

    return ALL_GOOD;
}

static bool heap_validate_alignment(void* ptr)
{
    return ((unsigned int)ptr % TOYOS_HEAP_BLOCK_SIZE) == 0;
}

int heap_create(struct heap* heap, void* ptr, void* end, struct heap_table* table)
{
    if (!heap_validate_alignment(ptr) || !heap_validate_alignment(end))
    {
        return -EINVARG;
    }

    memset(heap, 0, sizeof(struct heap));
    heap->saddr = ptr;
    heap->table = table;

    int res = heap_validate_table(ptr, end, table);
    if (res < 0)
    {
        return res;
    }

    size_t table_size = sizeof(heap_block_table_entry) * table->total;
    memset(table->entries, HEAP_BLOCK_TABLE_ENTRY_FREE, table_size);

    return ALL_GOOD;
}

static uint32_t heap_align_value_to_upper(uint32_t val)
{
    if ((val % TOYOS_HEAP_BLOCK_SIZE) == 0)
    {
        return val;
    }

    val = (val - ( val % TOYOS_HEAP_BLOCK_SIZE));
    val += TOYOS_HEAP_BLOCK_SIZE;
    return val;
}

static int heap_get_entry_type(heap_block_table_entry entry)
{
    return entry & 0x0f;
}

int heap_get_start_block(struct heap* heap, uint32_t total_blocks)
{
    struct heap_table* table = heap->table;
    int curr_block = 0;
    int start_block = -1;

    for (size_t i = 0; i < table->total; i++)
    {
        if (heap_get_entry_type(table->entries[i]) != HEAP_BLOCK_TABLE_ENTRY_FREE)
        {
            curr_block = 0;
            start_block = -1;
            continue;
        }

        if (start_block == -1)
        {
            start_block = i;
        }

        if (++curr_block == total_blocks)
        {
            break;
        }
    }
    
    return start_block == -1 ? -ENOMEM : start_block;
}

void* heap_block_to_address(struct heap* heap, int block)
{
    return heap->saddr + (block * TOYOS_HEAP_BLOCK_SIZE);
}

void heap_mark_blocks_taken(struct heap* heap, int start_block, int total_blocks)
{
    int end_block = (start_block + total_blocks) - 1;
    
    heap_block_table_entry entry = HEAP_BLOCK_TABLE_ENTRY_TAKEN | HEAP_BLOCK_IS_FIRST;

    if (total_blocks > 1)
    {
        entry |= HEAP_BLOCK_HAS_NEXT;
    }

    for (int i = start_block; i <= end_block; i++)
    {
        heap->table->entries[i] = entry;
        entry = HEAP_BLOCK_TABLE_ENTRY_TAKEN;

        if (i != end_block - 1)
        {
            entry |= HEAP_BLOCK_HAS_NEXT;
        }
    }
}

void* heap_malloc_blocks(struct heap* heap, uint32_t total_blocks)
{
    void* address = 0;

    int start_block = heap_get_start_block(heap, total_blocks);
    if (start_block < 0)
    {
        goto out;
    }

    address = heap_block_to_address(heap, start_block);

    // Mark the blocks as taken
    heap_mark_blocks_taken(heap, start_block, total_blocks);

out:
    return address;
}

void heap_mark_blocks_free(struct heap* heap, int starting_block)
{
    struct heap_table* table = heap->table;
    for (int i = starting_block; i < (int)table->total; i++)
    {
        heap_block_table_entry entry = table->entries[i];
        table->entries[i] = HEAP_BLOCK_TABLE_ENTRY_FREE;

        if (!(entry & HEAP_BLOCK_HAS_NEXT))
        {
            break;
        }
    }
}

int heap_address_to_block(struct heap* heap, void* address)
{
    return ((int)(address - heap->saddr)) / TOYOS_HEAP_BLOCK_SIZE;
}

void* malloc(struct heap* heap, size_t size)
{
    size_t aligned_size = heap_align_value_to_upper(size);
    uint32_t total_blocks = aligned_size / TOYOS_HEAP_BLOCK_SIZE;
    return heap_malloc_blocks(heap, total_blocks);
}

void free(struct heap* heap, void* ptr)
{
    heap_mark_blocks_free(heap, heap_address_to_block(heap, ptr));
}
