#ifndef _FAT16_H_
#define _FAT16_H_

#include "file.h"

/**
 * @brief Initializes the FAT16 filesystem.
 *
 * This function sets up the FAT16 filesystem structure and registers it
 * with the system. It prepares the filesystem for mounting and file operations.
 *
 * @return A pointer to the initialized filesystem structure.
 */
struct filesystem* fat16_init(void);

#endif
