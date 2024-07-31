#ifndef _TOYOS_STDLIB_H_
#define _TOYOS_STDLIB_H_

#include <stddef.h>

/**
 * @brief Allocates memory on the heap
 * 
 * @param size The size of the memory to allocate.
 * @return void* A pointer to the allocated memory.
 */
void* malloc(size_t size);

/**
 * @brief Frees memory on the heap
 * 
 * @param ptr The pointer to the memory to free.
 */
void free(void* ptr);

/**
 * @brief Converts an integer to a string.
 * 
 * @param i The integer to convert.
 * @return char* The string representation of the integer.
 */
char* itoa(int i);

#endif
