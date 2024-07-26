#ifndef _KHEAP_H_
#define _KHEAP_H_

#include <stdint.h>
#include <stddef.h>

/**
 * @brief Initializes the kernel heap.
 *
 * This function sets up the kernel heap, preparing it for dynamic memory allocation.
 * It must be called before any allocations are made using kmalloc or kzalloc.
 */
void kheap_init(void);

/**
 * @brief Allocates a block of memory from the kernel heap.
 *
 * Allocates a block of memory of the specified size. The allocated memory is not initialized.
 *
 * @param size The size of the memory block to allocate, in bytes.
 * @return A pointer to the allocated memory block, or NULL if the allocation fails.
 */
void* kmalloc(size_t size);

/**
 * @brief Allocates and zeroes a block of memory from the kernel heap.
 *
 * Similar to kmalloc, but additionally fills the allocated memory with zeros.
 *
 * @param size The size of the memory block to allocate, in bytes.
 * @return A pointer to the allocated and zeroed memory block, or NULL if the allocation fails.
 */
void* kzalloc(size_t size);

/**
 * @brief Frees a previously allocated block of memory.
 *
 * Releases the memory block pointed to by ptr, which must have been returned by a previous
 * call to kmalloc or kzalloc. If ptr is NULL, no operation is performed.
 *
 * @param ptr Pointer to the memory block to free.
 */
void kfree(void* ptr);

#endif
