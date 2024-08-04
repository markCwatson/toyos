#ifndef _SYS_HEAP_H_
#define _SYS_HEAP_H_

// Forward declaration of interrupt_frame.
struct interrupt_frame;

/**
 * Allocates memory on the heap.
 * 
 * @param frame The interrupt frame.
 * @return void* The address of the allocated memory.
 */
void* sys_cmd4_malloc(struct interrupt_frame* frame);

/**
 * Frees memory on the heap.
 * 
 * @param frame The interrupt frame.
 * @return void* The address of the allocated memory.
 */
void* sys_cmd5_free(struct interrupt_frame* frame);

#endif
