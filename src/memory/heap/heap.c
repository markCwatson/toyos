#include "heap.h"
#include "kernel.h"
#include "status.h"
#include "memory/memory.h"
#include <stdbool.h>

/**
 * @brief Validates the heap table size.
 *
 * This function checks if the heap table size matches the total number of blocks
 * that can fit between the provided start and end addresses.
 *
 * @param ptr Start address of the heap.
 * @param end End address of the heap.
 * @param table Pointer to the heap table structure.
 * @return 0 if the table size is valid, or a negative error code otherwise.
 */
static int heap_validate_table(void* ptr, void* end, struct heap_table* table) {
    size_t table_size = (size_t)(end - ptr);
    size_t total_blocks = table_size / TOYOS_HEAP_BLOCK_SIZE;

    if (table->total != total_blocks) {
        return -EINVARG;
    }

    return ALL_GOOD;
}

/**
 * @brief Checks if a pointer is aligned to the heap block size.
 *
 * This function ensures that the pointer is aligned to the start of a block,
 * which is necessary for proper heap management.
 *
 * @param ptr The pointer to check.
 * @return True if the pointer is aligned, false otherwise.
 */
static bool heap_validate_alignment(void* ptr) {
    return ((unsigned int)ptr % TOYOS_HEAP_BLOCK_SIZE) == 0;
}

/**
 * @brief Creates a heap.
 *
 * Initializes a heap structure with a specified memory range and a block table.
 * This function sets up the heap's block table and prepares the heap for allocations.
 *
 * @param heap Pointer to the heap structure to initialize.
 * @param ptr Start address of the heap memory range.
 * @param end End address of the heap memory range.
 * @param table Pointer to the heap block table.
 * @return 0 on success, or a negative error code on failure.
 */
int heap_create(struct heap* heap, void* ptr, void* end, struct heap_table* table) {
    if (!heap_validate_alignment(ptr) || !heap_validate_alignment(end)) {
        return -EINVARG;
    }

    memset(heap, 0, sizeof(struct heap));
    heap->saddr = ptr;
    heap->table = table;

    int res = heap_validate_table(ptr, end, table);
    if (res < 0) {
        return res;
    }

    size_t table_size = sizeof(heap_block_table_entry) * table->total;
    memset(table->entries, HEAP_BLOCK_TABLE_ENTRY_FREE, table_size);

    return ALL_GOOD;
}

/**
 * @brief Aligns a value to the next upper block boundary.
 *
 * This function rounds up the given value to the nearest block size boundary,
 * which is necessary for block-aligned memory allocations.
 *
 * @param val The value to align.
 * @return The aligned value.
 */
static uint32_t heap_align_value_to_upper(uint32_t val) {
    if ((val % TOYOS_HEAP_BLOCK_SIZE) == 0) {
        return val;
    }

    val = (val - ( val % TOYOS_HEAP_BLOCK_SIZE));
    val += TOYOS_HEAP_BLOCK_SIZE;
    return val;
}

/**
 * @brief Retrieves the type of a heap block entry.
 *
 * This function extracts the block entry type from the block table entry, 
 * which indicates whether the block is free, taken, the first block in an allocation, etc.
 *
 * @param entry The heap block table entry.
 * @return The type of the block entry.
 */
static int heap_get_entry_type(heap_block_table_entry entry) {
    return entry & 0x0f;
}

/**
 * @brief Finds the start block for an allocation.
 *
 * This function searches the heap for a sequence of free blocks that can accommodate
 * the requested number of blocks. It returns the index of the first block in the sequence.
 *
 * @param heap Pointer to the heap structure.
 * @param total_blocks The total number of blocks needed.
 * @return The index of the first block in the free sequence, or a negative error code if no sufficient block is found.
 */
int heap_get_start_block(struct heap* heap, uint32_t total_blocks) {
    struct heap_table* table = heap->table;
    int curr_block = 0;
    int start_block = -1;

    for (size_t i = 0; i < table->total; i++) {
        if (heap_get_entry_type(table->entries[i]) != HEAP_BLOCK_TABLE_ENTRY_FREE) {
            curr_block = 0;
            start_block = -1;
            continue;
        }

        if (start_block == -1) {
            start_block = i;
        }

        if (++curr_block == total_blocks) {
            break;
        }
    }
    
    return start_block == -1 ? -ENOMEM : start_block;
}

/**
 * @brief Converts a block index to a memory address.
 *
 * This function calculates the memory address corresponding to a given block index
 * within the heap.
 *
 * @param heap Pointer to the heap structure.
 * @param block The block index to convert.
 * @return The memory address corresponding to the block index.
 */
void* heap_block_to_address(struct heap* heap, int block) {
    return heap->saddr + (block * TOYOS_HEAP_BLOCK_SIZE);
}

/**
 * @brief Marks a series of blocks as taken.
 *
 * This function updates the heap block table to mark a sequence of blocks as taken,
 * indicating that they are part of an allocated memory region.
 *
 * @param heap Pointer to the heap structure.
 * @param start_block The index of the first block in the sequence.
 * @param total_blocks The total number of blocks in the sequence.
 */
void heap_mark_blocks_taken(struct heap* heap, int start_block, int total_blocks) {
    int end_block = (start_block + total_blocks) - 1;
    
    heap_block_table_entry entry = HEAP_BLOCK_TABLE_ENTRY_TAKEN | HEAP_BLOCK_IS_FIRST;

    if (total_blocks > 1) {
        entry |= HEAP_BLOCK_HAS_NEXT;
    }

    for (int i = start_block; i <= end_block; i++) {
        heap->table->entries[i] = entry;
        entry = HEAP_BLOCK_TABLE_ENTRY_TAKEN;

        if (i != end_block - 1) {
            entry |= HEAP_BLOCK_HAS_NEXT;
        }
    }
}

/**
 * @brief Allocates a sequence of blocks from the heap.
 *
 * This function finds a sequence of free blocks in the heap and marks them as taken.
 * It returns the memory address corresponding to the first block in the sequence.
 *
 * @param heap Pointer to the heap structure.
 * @param total_blocks The total number of blocks to allocate.
 * @return A pointer to the allocated memory, or NULL if the allocation fails.
 */
void* heap_malloc_blocks(struct heap* heap, uint32_t total_blocks) {
    void* address = NULL;

    int start_block = heap_get_start_block(heap, total_blocks);
    if (start_block < 0) {
        goto out;
    }

    address = heap_block_to_address(heap, start_block);
    if (!address) {
        goto out;
    }

    // Mark the blocks as taken
    heap_mark_blocks_taken(heap, start_block, total_blocks);

out:
    return address;
}

/**
 * @brief Frees a series of blocks in the heap.
 *
 * This function marks a sequence of blocks as free, making them available for future allocations.
 * It starts from the specified block index and continues until a block without the HEAP_BLOCK_HAS_NEXT flag is found.
 *
 * @param heap Pointer to the heap structure.
 * @param starting_block The index of the first block to free.
 */
void heap_mark_blocks_free(struct heap* heap, int starting_block) {
    struct heap_table* table = heap->table;
    for (int i = starting_block; i < (int)table->total; i++) {
        heap_block_table_entry entry = table->entries[i];
        table->entries[i] = HEAP_BLOCK_TABLE_ENTRY_FREE;

        if (!(entry & HEAP_BLOCK_HAS_NEXT)) {
            break;
        }
    }
}

/**
 * @brief Converts a memory address to a block index.
 *
 * This function calculates the block index corresponding to a given memory address
 * within the heap.
 *
 * @param heap Pointer to the heap structure.
 * @param address The memory address to convert.
 * @return The block index corresponding to the memory address.
 */
int heap_address_to_block(struct heap* heap, void* address) {
    return ((int)(address - heap->saddr)) / TOYOS_HEAP_BLOCK_SIZE;
}

/**
 * @brief Allocates a block of memory from the heap.
 *
 * This function allocates a block of memory of the specified size from the heap.
 * The size is aligned to the nearest upper block size boundary.
 *
 * @param heap Pointer to the heap structure.
 * @param size The size of the memory block to allocate, in bytes.
 * @return A pointer to the allocated memory block, or NULL if the allocation fails.
 */
void* malloc(struct heap* heap, size_t size) {
    size_t aligned_size = heap_align_value_to_upper(size);
    uint32_t total_blocks = aligned_size / TOYOS_HEAP_BLOCK_SIZE;
    return heap_malloc_blocks(heap, total_blocks);
}

/**
 * @brief Frees a previously allocated block of memory.
 *
 * This function releases a block of memory back to the heap, making it available for future allocations.
 * The block must have been previously allocated using malloc.
 *
 * @param heap Pointer to the heap structure.
 * @param ptr Pointer to the memory block to free.
 */
void free(struct heap* heap, void* ptr) {
    heap_mark_blocks_free(heap, heap_address_to_block(heap, ptr));
}
