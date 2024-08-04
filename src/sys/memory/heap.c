#include "heap.h"
#include "task/task.h"
#include "task/process.h"
#include <stddef.h>

/**
 * Allocates memory on the heap.
 * 
 * @param frame The interrupt frame.
 * @return void* The address of the allocated memory.
 */
void* sys_cmd4_malloc(struct interrupt_frame* frame) {
    size_t size = (int)task_get_stack_item(task_current(), 0);
    return process_malloc(task_current()->process, size);
}

/**
 * Frees memory on the heap.
 * 
 * @param frame The interrupt frame.
 * @return void* The address of the allocated memory.
 */
void* sys_cmd5_free(struct interrupt_frame* frame) {
    void* ptr_to_free = task_get_stack_item(task_current(), 0);
    process_free(task_current()->process, ptr_to_free);
    return 0;
}
