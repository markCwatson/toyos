#include "disk.h"
#include "config.h"
#include "io/io.h"
#include "memory/memory.h"
#include "status.h"

struct disk disk;

/**
 * @brief Writes data to a specific sector on the disk.
 *
 * @param lba Logical Block Addressing (LBA) address of the sector to write to.
 * @param total Number of sectors to write.
 * @param buf Buffer containing the data to be written.
 * @return 0 on success, or an error code if failed.
 */
static int disk_write_sector(int lba, int total, void *buf) {
    if (!buf) {
        return -EINVARG;
    }

    outb(0x1f6, (lba >> 24) | 0xe0);
    outb(0x1f2, total);
    outb(0x1f3, (unsigned char)(lba & 0xff));
    outb(0x1f4, (unsigned char)(lba >> 8));
    outb(0x1f5, (unsigned char)(lba >> 16));
    outb(0x1f7, 0x30);  // 0x30 is the command for write

    unsigned short *ptr = (unsigned short *)buf;

    for (int i = 0; i < total; i++) {
        // Wait for the buffer to be ready
        char c = insb(0x1f7);
        while (!(c & 0x08)) {
            c = insb(0x1f7);
        }

        // Copy from memory to hard disk
        for (int j = 0; j < 256; j++) {
            outw(0x1f0, *ptr);
            ptr++;
        }
    }

    return OK;
}

/**
 * @brief Reads data from a specific sector on the disk.
 *
 * @param lba Logical Block Addressing (LBA) address of the sector to read from.
 * @param total Number of sectors to read.
 * @param buf Buffer to store the read data.
 * @return 0 on success, or an error code if failed.
 */
static int disk_read_sector(int lba, int total, void *buf) {
    if (!buf) {
        return -EINVARG;
    }

    outb(0x1f6, (lba >> 24) | 0xe0);
    outb(0x1f2, total);
    outb(0x1f3, (unsigned char)(lba & 0xff));
    outb(0x1f4, (unsigned char)(lba >> 8));
    outb(0x1f5, (unsigned char)(lba >> 16));
    outb(0x1f7, 0x20);

    unsigned short *ptr = (unsigned short *)buf;

    for (int i = 0; i < total; i++) {
        // Wait for the buffer to be ready
        char c = insb(0x1f7);
        while (!(c & 0x08)) {
            c = insb(0x1f7);
        }

        // Copy from hard disk to memory
        for (int j = 0; j < 256; j++) {
            *ptr = insw(0x1f0);
            ptr++;
        }
    }

    return OK;
}

void disk_search_and_init(void) {
    memset(&disk, 0, sizeof(disk));
    disk.id = 0;
    disk.type = DISK_TYPE_REAL;
    disk.sector_size = TOYOS_SECTOR_SIZE;
    disk.fs = fs_resolve(&disk);
}

struct disk *disk_get(int index) {
    if (index != 0) {
        return NULL;
    }

    return &disk;
}

int disk_read_block(struct disk *idisk, unsigned int lba, int total, void *buf) {
    if (idisk != &disk) {
        return -EIO;
    }

    return disk_read_sector(lba, total, buf);
}

int disk_write_block(struct disk *idisk, unsigned int lba, int total, void *buf) {
    if (idisk != &disk) {
        return -EIO;
    }

    return disk_write_sector(lba, total, buf);
}
