#ifndef _SYS_HEAP_H_
#define _SYS_HEAP_H_

// Forward declaration of interrupt_frame.
struct interrupt_frame;

/**
 * @brief Allocates memory on the heap.
 *
 * @param frame The interrupt frame.
 * @return void* The address of the allocated memory.
 */
void *sys_command4_malloc(struct interrupt_frame *frame);

/**
 * @brief Frees memory on the heap.
 *
 * @param frame The interrupt frame.
 * @return void* The address of the allocated memory.
 */
void *sys_command5_free(struct interrupt_frame *frame);

#endif
