#include "stdlib.h"
#include "toyos.h"

/**
 * @brief Allocates memory on the heap
 * 
 * @param size The size of the memory to allocate.
 * @return void* A pointer to the allocated memory.
 */
void* malloc(size_t size) {
    return toyos_malloc(size);
}

/**
 * @brief Frees memory on the heap
 * 
 * @param ptr The pointer to the memory to free.
 */
void free(void* ptr) {
    toyos_free(ptr);
}

/**
 * @brief Converts an integer to a string.
 * 
 * @param i The integer to convert.
 * @return char* The string representation of the integer.
 */
char* itoa(int i) {
    static char text[12];
    int loc = 11;
    text[11] = 0;
    char neg = 1;

    if (i >= 0) {
        neg = 0;
        i = -i;
    }

    while(i) {
        text[--loc] = '0' - (i % 10);
        i /= 10;
    }

    if (loc == 11) {
        text[--loc] = '0';
    }

    if (neg) {
        text[--loc] = '-';
    }

    return &text[loc];
}
