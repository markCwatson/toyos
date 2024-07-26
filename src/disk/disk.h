#ifndef _DISK_H_
#define _DISK_H_

#include "fs/file.h"

/**
 * @brief Represents the type of a physical hard disk.
 */
#define DISK_TYPE_REAL 0

typedef unsigned int disk_type;

/**
 * @brief Structure representing a disk.
 */
struct disk {
    disk_type type;             /**< Type of the disk. */
    int sector_size;            /**< Size of a sector in bytes. */
    int id;                     /**< Identifier for the disk. */
    
    /**
     * @brief Filesystem associated with the disk.
     */
    struct filesystem* fs;

    /**
     * @brief Private data for the filesystem.
     */
    void* fs_private;
};

/**
 * @brief Reads a block of data from the disk.
 *
 * @param idisk Pointer to the disk structure.
 * @param lba Logical Block Addressing (LBA) address to read from.
 * @param total Number of sectors to read.
 * @param buf Buffer to store the read data.
 * @return 0 on success, error code otherwise.
 */
int disk_read_block(struct disk* idisk, unsigned int lba, int total, void* buf);

/**
 * @brief Writes a block of data to the disk.
 *
 * @param idisk Pointer to the disk structure.
 * @param lba LBA address to write to.
 * @param total Number of sectors to write.
 * @param buf Buffer containing the data to write.
 * @return 0 on success, error code otherwise.
 */
int disk_write_block(struct disk* idisk, unsigned int lba, int total, void* buf);

/**
 * @brief Searches for available disks and initializes them.
 */
void disk_search_and_init(void);

/**
 * @brief Retrieves a disk by its index.
 *
 * @param index Index of the disk.
 * @return Pointer to the disk structure, or NULL if not found.
 */
struct disk* disk_get(int index);

#endif
