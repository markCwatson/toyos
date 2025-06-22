#ifndef _MEMORY_H_
#define _MEMORY_H_

#include <stddef.h>

/**
 * @brief Sets a block of memory to a specified value.
 *
 * This function sets the first 'size' bytes of the memory area pointed to by 'ptr'
 * to the specified value 'c'.
 *
 * @param ptr Pointer to the memory area to be filled.
 * @param c Value to be set. The value is passed as an int but is converted to an unsigned char.
 * @param size Number of bytes to be set to the value.
 * @return Pointer to the memory area 'ptr'.
 */
void *memset(void *ptr, int c, size_t size);

/**
 * @brief Compares two blocks of memory.
 *
 * This function compares the first 'count' bytes of the memory areas 's1' and 's2'.
 *
 * @param s1 Pointer to the first memory area.
 * @param s2 Pointer to the second memory area.
 * @param count Number of bytes to compare.
 * @return An integer less than, equal to, or greater than zero if the first 'count' bytes
 *         of 's1' is found, respectively, to be less than, to match, or to be greater than
 *         the first 'count' bytes of 's2'.
 */
int memcmp(void *s1, void *s2, int count);

/**
 * @brief Copies a block of memory from one location to another.
 *
 * This function copies 'len' bytes from the memory area 'src' to the memory area 'dest'.
 * The memory areas must not overlap, as this may lead to undefined behavior.
 *
 * @param dest Pointer to the destination memory area.
 * @param src Pointer to the source memory area.
 * @param len Number of bytes to copy.
 * @return Pointer to the destination memory area 'dest'.
 */
void *memcpy(void *dest, void *src, int len);

#endif
