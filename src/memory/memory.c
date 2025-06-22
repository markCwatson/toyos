#include "memory.h"

/**
 * @brief Sets a block of memory to a specified value.
 *
 * This function sets each byte in the block of memory pointed to by 'ptr' to the value of 'c'.
 *
 * @param ptr Pointer to the block of memory to fill.
 * @param c The byte value to set. Only the lower 8 bits are used.
 * @param size Number of bytes to set to the value.
 * @return The original value of 'ptr'.
 */
void *memset(void *ptr, int c, size_t size) {
    char *c_ptr = (char *)ptr;

    for (size_t i = 0; i < size; i++) {
        c_ptr[i] = (char)c;
    }

    return ptr;
}

/**
 * @brief Compares two blocks of memory.
 *
 * This function compares 'count' bytes of the memory areas 's1' and 's2'.
 * The comparison is done lexicographically and stops as soon as a difference is found.
 *
 * @param s1 Pointer to the first block of memory.
 * @param s2 Pointer to the second block of memory.
 * @param count Number of bytes to compare.
 * @return An integer indicating the result of the comparison:
 *         - 0 if the blocks are equal,
 *         - A negative value if the first differing byte in 's1' is less than that in 's2',
 *         - A positive value if the first differing byte in 's1' is greater than that in 's2'.
 */
int memcmp(void *s1, void *s2, int count) {
    char *c1 = s1;
    char *c2 = s2;

    while (count-- > 0) {
        if (*c1++ != *c2++) {
            return c1[-1] < c2[-1] ? -1 : 1;
        }
    }

    return 0;
}

/**
 * @brief Copies a block of memory.
 *
 * This function copies 'len' bytes from the source memory area 'src' to the destination memory area 'dest'.
 * The memory areas should not overlap, as the behavior in that case is undefined.
 *
 * @param dest Pointer to the destination memory area.
 * @param src Pointer to the source memory area.
 * @param len Number of bytes to copy.
 * @return Pointer to the destination memory area 'dest'.
 */
void *memcpy(void *dest, void *src, int len) {
    char *d = dest;
    char *s = src;

    for (int i = 0; i < len; i++) {
        d[i] = s[i];
    }

    return dest;
}
