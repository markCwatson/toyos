#include "fat16.h"
#include "config.h"
#include "disk/disk.h"
#include "disk/streamer.h"
#include "kernel.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"
#include "status.h"
#include "stdlib/string.h"
#include <stddef.h>
#include <stdint.h>

// FAT16 specific constants and definitions
#define TOYOS_FAT16_SIGNATURE 0x29
#define TOYOS_FAT16_FAT_ENTRY_SIZE 0x02
#define TOYOS_FAT16_BAD_SECTOR 0xff7
#define TOYOS_FAT16_UNUSED 0x00

// FAT directory entry attributes bitmask
#define FAT_FILE_READ_ONLY 0x01
#define FAT_FILE_HIDDEN 0x02
#define FAT_FILE_SYSTEM 0x04
#define FAT_FILE_VOLUME_LABEL 0x08
#define FAT_FILE_SUBDIRECTORY 0x10
#define FAT_FILE_ARCHIVED 0x20
#define FAT_FILE_DEVICE 0x40
#define FAT_FILE_RESERVED 0x80

#define FAT_ITEM_TYPE_DIRECTORY 0
#define FAT_ITEM_TYPE_FILE 1

#define DIRECTORY_ENTRY_AVAILABLE 0xe5

typedef unsigned int fat_item_type;

/**
 * @struct fat_header_extended
 * @brief Extended header structure for FAT16, includes additional metadata.
 */
struct fat_header_extended {
    uint8_t drive_number;
    uint8_t win_nt_bit;
    uint8_t signature;
    uint32_t volume_id;
    uint8_t volume_id_string[11];
    uint8_t system_id_string[8];
} __attribute__((packed));

/**
 * @struct fat_header
 * @brief Primary header structure for FAT16, describes the general disk layout.
 */
struct fat_header {
    uint8_t short_jmp_ins[3];
    uint8_t oem_identifier[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t fat_copies;
    uint16_t root_dir_entries;
    uint16_t number_of_sectors;
    uint8_t media_type;
    uint16_t sectors_per_fat;
    uint16_t sectors_per_track;
    uint16_t number_of_heads;
    uint32_t hidden_setors;
    uint32_t sectors_big;
} __attribute__((packed));

/**
 * @struct fat_h
 * @brief Combined header structure for FAT16, including primary and extended headers.
 */
struct fat_h {
    struct fat_header primary_header;
    union fat_h_e {
        struct fat_header_extended extended_header;
    } shared;
};

/**
 * @struct fat_directory_item
 * @brief Structure representing a directory entry in FAT16.
 */
struct fat_directory_item {
    uint8_t filename[8];
    uint8_t ext[3];
    uint8_t attribute;
    uint8_t reserved;
    uint8_t creation_time_tenths_of_a_sec;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access;
    uint16_t high_16_bits_first_cluster;
    uint16_t last_mod_time;
    uint16_t last_mod_date;
    uint16_t low_16_bits_first_cluster;
    uint32_t filesize;
} __attribute__((packed));

/**
 * @struct fat_directory
 * @brief Represents a directory in FAT16, containing multiple directory items.
 */
struct fat_directory {
    struct fat_directory_item *item; /**< Pointer to array of directory items */
    int total;                       /**< Total number of items in the directory */
    int sector_pos;                  /**< Starting sector of the directory */
    int ending_sector_pos;           /**< Ending sector of the directory */
};

/**
 * @struct fat_item
 * @brief Union structure representing either a file or a directory in FAT16.
 */
struct fat_item {
    union {
        struct fat_directory_item *item;
        struct fat_directory *directory;
    };
    fat_item_type type; /**< Type of the item (file or directory) */
};

/**
 * @struct fat_file_descriptor
 * @brief Descriptor for open files in FAT16, tracks the position within the file.
 */
struct fat_file_descriptor {
    struct fat_item *item; /**< Associated FAT item */
    uint32_t pos;          /**< Current position within the file */
};

/**
 * @struct fat_private
 * @brief Private data for FAT16 filesystem, used for maintaining filesystem state.
 */
struct fat_private {
    struct fat_h header;                 /**< Combined header for FAT16 */
    struct fat_directory root_directory; /**< Root directory of the filesystem */

    // Streams for reading data clusters and FAT table
    struct disk_stream *cluster_read_stream;
    struct disk_stream *fat_read_stream;

    // Stream for directory data
    struct disk_stream *directory_stream;
};

// Function declarations for FAT16 filesystem operations
int fat16_resolve(struct disk *disk);
void *fat16_open(struct disk *disk, struct path_part *path, file_mode mode);
int fat16_read(struct disk *disk, void *private_data, uint32_t size, uint32_t nmemb, char *out);
int fat16_write(struct disk *disk, void *private_data, uint32_t size, uint32_t nmemb, char *in);
int fat16_seek(void *private_data, uint32_t offset, file_seek_mode seek_mode);
int fat16_stat(struct disk *disk, void *private_data, struct file_stat *stat);
int fat16_close(void *private_data);

/**
 * @brief FAT16 filesystem structure with function pointers to filesystem operations.
 */
struct filesystem fat16_fs = {.resolve = fat16_resolve,
                              .open = fat16_open,
                              .read = fat16_read,
                              .write = fat16_write,
                              .seek = fat16_seek,
                              .stat = fat16_stat,
                              .close = fat16_close};

/**
 * @brief Initializes the FAT16 filesystem and registers it.
 * @return Pointer to the filesystem structure.
 */
struct filesystem *fat16_init(void) {
    strcpy(fat16_fs.name, "FAT16");
    return &fat16_fs;
}

/**
 * @brief Initializes the private FAT16 structure for a disk.
 * @param disk Pointer to the disk structure.
 * @param private Pointer to the FAT16 private structure.
 */
static void fat16_init_private(struct disk *disk, struct fat_private *private) {
    memset(private, 0, sizeof(struct fat_private));
    private->cluster_read_stream = streamer_new(disk->id);
    private->fat_read_stream = streamer_new(disk->id);
    private->directory_stream = streamer_new(disk->id);
}

/**
 * @brief Retrieves the directory stream for the disk.
 * @param disk Pointer to the disk structure.
 * @return Pointer to the directory stream.
 */
static inline struct disk_stream *get_disk_directory_stream(const struct disk *disk) {
    return ((const struct fat_private *)disk->fs_private)->directory_stream;
}

/**
 * @brief Counts the total items in a directory starting from a given sector.
 * @param disk Pointer to the disk structure.
 * @param directory_start_sector Sector where the directory starts.
 * @return Total number of items in the directory.
 */
int fat16_get_total_items_for_directory(struct disk *disk, uint32_t directory_start_sector) {
    int total_items = 0;

    struct fat_directory_item item;
    struct fat_directory_item empty_item;
    memset(&empty_item, 0, sizeof(empty_item));

    struct disk_stream *stream = get_disk_directory_stream(disk);

    int directory_start_pos = directory_start_sector * disk->sector_size;

    if (streamer_seek(stream, directory_start_pos) != OK) {
        return -EIO;
    }

    while (1) {
        if (streamer_read(stream, &item, sizeof(item)) != OK) {
            return -EIO;
        }

        if (item.filename[0] == 0x00) {
            break;
        }

        if (item.filename[0] == DIRECTORY_ENTRY_AVAILABLE) {
            continue;
        }

        total_items += 1;
    }

    return total_items;
}

/**
 * @brief Converts a logical sector number to an absolute byte offset.
 * @param disk Pointer to the disk structure.
 * @param sector The logical sector number.
 * @return The absolute byte offset.
 */
int fat16_sector_to_absolute(struct disk *disk, int sector) {
    return sector * disk->sector_size;
}

/**
 * @brief Retrieves the root directory of the FAT16 filesystem.
 * @param disk Pointer to the disk structure.
 * @param fat_private Pointer to the FAT16 private structure.
 * @param directory Pointer to the directory structure to be filled.
 * @return Status code indicating success or failure.
 */
int fat16_get_root_directory(struct disk *disk, struct fat_private *fat_private, struct fat_directory *directory) {
    int res = OK;
    struct fat_directory_item *dir = NULL;
    struct fat_header *primary_header = &fat_private->header.primary_header;

    int root_dir_sector_pos =
        (primary_header->fat_copies * primary_header->sectors_per_fat) + primary_header->reserved_sectors;
    int root_dir_entries = fat_private->header.primary_header.root_dir_entries;
    int root_dir_size = (root_dir_entries * sizeof(struct fat_directory_item));
    int total_sectors = root_dir_size / disk->sector_size;

    if (root_dir_size % disk->sector_size) {
        total_sectors += 1;
    }

    int total_items = fat16_get_total_items_for_directory(disk, root_dir_sector_pos);

    dir = kzalloc(root_dir_size);
    if (!dir) {
        return -ENOMEM;
    }

    struct disk_stream *stream = fat_private->directory_stream;
    if (!stream) {
        res = -EIO;
        goto err_out;
    }

    if (streamer_seek(stream, fat16_sector_to_absolute(disk, root_dir_sector_pos)) != OK) {
        res = -EIO;
        goto err_out;
    }

    if (streamer_read(stream, dir, root_dir_size) != OK) {
        res = -EIO;
        goto err_out;
    }

    directory->item = dir;
    directory->total = total_items;
    directory->sector_pos = root_dir_sector_pos;
    directory->ending_sector_pos = root_dir_sector_pos + (root_dir_size / disk->sector_size);

out:
    return res;

err_out:
    if (dir) {
        kfree(dir);
    }

    return res;
}

/**
 * @brief Resolves the FAT16 filesystem on the given disk.
 * @param disk Pointer to the disk structure.
 * @return Status code indicating success or failure.
 */
int fat16_resolve(struct disk *disk) {
    int res = OK;
    struct fat_private *fat_private = kzalloc(sizeof(struct fat_private));
    if (!fat_private) {
        return -ENOMEM;
    }

    fat16_init_private(disk, fat_private);

    disk->fs_private = fat_private;
    disk->fs = &fat16_fs;

    struct disk_stream *stream = streamer_new(disk->id);
    if (!stream) {
        res = -ENOMEM;
        goto out;
    }

    if (streamer_read(stream, &fat_private->header, sizeof(fat_private->header)) != OK) {
        res = -EIO;
        goto out;
    }

    if (fat_private->header.shared.extended_header.signature != TOYOS_FAT16_SIGNATURE) {
        res = -EFSNOTUS;
        goto out;
    }

    if (fat16_get_root_directory(disk, fat_private, &fat_private->root_directory) != OK) {
        res = -EIO;
        goto out;
    }

out:
    if (stream) {
        streamer_close(stream);
    }

    if (res < 0) {
        kfree(fat_private);
        disk->fs_private = NULL;
    }

    return res;
}

/**
 * @brief Clones a FAT16 directory item.
 * @param item Pointer to the directory item to clone.
 * @param size Size of the item structure.
 * @return Pointer to the cloned item.
 */
struct fat_directory_item *fat16_clone_directory_item(struct fat_directory_item *item, int size) {
    if (size < sizeof(struct fat_directory_item)) {
        return NULL;
    }

    struct fat_directory_item *item_copy = kzalloc(size);
    if (!item_copy) {
        return NULL;
    }

    memcpy(item_copy, item, size);
    return item_copy;
}

/**
 * @brief Gets the first cluster number from a FAT16 directory item.
 * @param item Pointer to the directory item.
 * @return First cluster number.
 */
static uint32_t fat16_get_first_cluster(struct fat_directory_item *item) {
    return item->high_16_bits_first_cluster | item->low_16_bits_first_cluster;
}

/**
 * @brief Converts a cluster number to an absolute sector number.
 * @param private Pointer to the FAT16 private structure.
 * @param cluster Cluster number.
 * @return Absolute sector number.
 */
static int fat16_cluster_to_sector(struct fat_private *private, int cluster) {
    return private->root_directory.ending_sector_pos +
           ((cluster - 2) * private->header.primary_header.sectors_per_cluster);
}

/**
 * @brief Gets the starting sector of the FAT table.
 * @param private Pointer to the FAT16 private structure.
 * @return Starting sector of the FAT table.
 */
static uint32_t fat16_get_first_fat_sector(struct fat_private *private) {
    return private->header.primary_header.reserved_sectors;
}

/**
 * @brief Retrieves a FAT table entry for a given cluster.
 * @param disk Pointer to the disk structure.
 * @param cluster Cluster number.
 * @return FAT table entry value.
 */
static int fat16_get_fat_entry(struct disk *disk, int cluster) {
    int res = -100;
    struct fat_private *private = disk->fs_private;
    struct disk_stream *stream = private->fat_read_stream;
    if (!stream) {
        goto out;
    }

    uint32_t fat_table_position = fat16_get_first_fat_sector(private) * disk->sector_size;
    res = streamer_seek(stream, fat_table_position + (cluster * TOYOS_FAT16_FAT_ENTRY_SIZE));
    if (res < 0) {
        goto out;
    }

    uint16_t result = 0;
    res = streamer_read(stream, &result, sizeof(result));
    if (res < 0) {
        goto out;
    }

    res = result;

out:
    return res;
}

/**
 * @brief Finds the cluster corresponding to a given offset in a file.
 * @param disk Pointer to the disk structure.
 * @param starting_cluster Starting cluster of the file.
 * @param offset Byte offset within the file.
 * @return Cluster number or error code.
 */
static int fat16_get_cluster_for_offset(struct disk *disk, int starting_cluster, int offset) {
    int res = 0;
    struct fat_private *private = disk->fs_private;

    int size_of_cluster_bytes = private->header.primary_header.sectors_per_cluster * disk->sector_size;
    int cluster_to_use = starting_cluster;
    int clusters_ahead = offset / size_of_cluster_bytes;

    for (int i = 0; i < clusters_ahead; i++) {
        int entry = fat16_get_fat_entry(disk, cluster_to_use);
        if (entry == 0xff8 || entry == 0xfff) {
            // We are at the last entry in the file
            res = -EIO;
            goto out;
        }

        if (entry == TOYOS_FAT16_BAD_SECTOR) {
            res = -EIO;
            goto out;
        }

        if (entry == 0xff0 || entry == 0xff6) {
            // reserved sector
            res = -EIO;
            goto out;
        }

        if (entry == 0x00) {
            // corrupted sector
            res = -EIO;
            goto out;
        }

        cluster_to_use = entry;
    }

    res = cluster_to_use;

out:
    return res;
}

/**
 * @brief Reads data from a FAT16 filesystem into a buffer using a disk stream.
 * @param disk Pointer to the disk structure.
 * @param stream Pointer to the disk stream.
 * @param cluster Starting cluster of the file.
 * @param offset Offset within the file.
 * @param total Total bytes to read.
 * @param out Output buffer.
 * @return Number of bytes read or error code.
 */
static int fat16_read_internal_from_stream(struct disk *disk, struct disk_stream *stream, int cluster, int offset,
                                           int total, void *out) {
    int res = 0;
    struct fat_private *private = disk->fs_private;

    int size_of_cluster_bytes = private->header.primary_header.sectors_per_cluster * disk->sector_size;
    int cluster_to_use = fat16_get_cluster_for_offset(disk, cluster, offset);
    if (cluster_to_use < 0) {
        res = cluster_to_use;
        goto out;
    }

    int starting_sector = fat16_cluster_to_sector(private, cluster_to_use);
    int offset_from_cluster = offset % size_of_cluster_bytes;
    int starting_pos = (starting_sector * disk->sector_size) + offset_from_cluster;
    int total_to_read = total > size_of_cluster_bytes ? size_of_cluster_bytes : total;

    res = streamer_seek(stream, starting_pos);
    if (res != OK) {
        goto out;
    }

    res = streamer_read(stream, out, total_to_read);
    if (res != OK) {
        goto out;
    }

    total -= total_to_read;
    if (total > 0) {
        // still more to read so recursively call this function
        res =
            fat16_read_internal_from_stream(disk, stream, cluster, offset + total_to_read, total, out + total_to_read);
    }

out:
    return res;
}

/**
 * @brief Reads data from a FAT16 file into a buffer.
 * @param disk Pointer to the disk structure.
 * @param starting_cluster Starting cluster of the file.
 * @param offset Offset within the file.
 * @param total Total bytes to read.
 * @param out Output buffer.
 * @return Number of bytes read or error code.
 */
static int fat16_read_internal(struct disk *disk, int starting_cluster, int offset, int total, void *out) {
    struct fat_private *fs_private = disk->fs_private;
    struct disk_stream *stream = fs_private->cluster_read_stream;

    return fat16_read_internal_from_stream(disk, stream, starting_cluster, offset, total, out);
}

/**
 * @brief Frees a FAT16 directory structure.
 * @param directory Pointer to the directory structure to free.
 */
static void fat16_free_directory(struct fat_directory *directory) {
    if (!directory) {
        return;
    }

    if (directory->item) {
        kfree(directory->item);
    }

    kfree(directory);
}

/**
 * @brief Frees a FAT16 item structure (file or directory).
 * @param item Pointer to the item structure to free.
 */
static void fat16_fat_item_free(struct fat_item *item) {
    if (item->type == FAT_ITEM_TYPE_DIRECTORY) {
        fat16_free_directory(item->directory);
    } else if (item->type == FAT_ITEM_TYPE_FILE) {
        kfree(item->item);
    }

    kfree(item);
}

/**
 * @brief Loads a FAT16 directory structure from a directory item.
 * @param disk Pointer to the disk structure.
 * @param item Pointer to the directory item structure.
 * @return Pointer to the loaded directory structure.
 */
struct fat_directory *fat16_load_fat_directory(struct disk *disk, struct fat_directory_item *item) {
    if (!(item->attribute & FAT_FILE_SUBDIRECTORY)) {
        return NULL;
    }

    struct fat_directory *directory = NULL;
    struct fat_private *fat_private = disk->fs_private;

    directory = kzalloc(sizeof(struct fat_directory));
    if (!directory) {
        return NULL;
    }

    int res = 0;

    int cluster = fat16_get_first_cluster(item);
    int cluster_sector = fat16_cluster_to_sector(fat_private, cluster);

    directory->total = fat16_get_total_items_for_directory(disk, cluster_sector);
    int directory_size = directory->total * sizeof(struct fat_directory_item);

    // make room to load the directory into memory
    directory->item = kzalloc(directory_size);
    if (!directory->item) {
        res = -ENOMEM;
        goto out;
    }

    // read the directory into memory
    res = fat16_read_internal(disk, cluster, 0x00, directory_size, directory->item);
    if (res != OK) {
        goto out;
    }

out:
    if (res != OK) {
        fat16_free_directory(directory);
    }

    return directory;
}

/**
 * @brief Creates a new FAT16 item structure for a directory item.
 * @param disk Pointer to the disk structure.
 * @param item Pointer to the directory item structure.
 * @return Pointer to the new FAT16 item structure.
 */
struct fat_item *fat16_new_fat_item_for_directory_item(struct disk *disk, struct fat_directory_item *item) {
    struct fat_item *f_item = kzalloc(sizeof(struct fat_item));
    if (!f_item) {
        return NULL;
    }

    if (item->attribute & FAT_FILE_SUBDIRECTORY) {
        f_item->directory = fat16_load_fat_directory(disk, item);
        f_item->type = FAT_ITEM_TYPE_DIRECTORY;
        return f_item;
    }

    f_item->type = FAT_ITEM_TYPE_FILE;
    f_item->item = fat16_clone_directory_item(item, sizeof(struct fat_directory_item));

    return f_item;
}

/**
 * @brief Converts a FAT16 filename to a proper string, removing padding spaces.
 * @param out Pointer to the output buffer.
 * @param in Pointer to the input FAT16 filename.
 * @param size Size of the input filename.
 */
void fat16_to_proper_string(char **out, const char *in, size_t size) {
    int i = 0;

    while (*in != 0x00 && *in != 0x20) {
        **out = *in;
        *out += 1;
        in += 1;

        // We cant process anymore since we have exceeded the input buffer size
        if (i >= size - 1) {
            break;
        }

        i++;
    }

    **out = 0x00;
}

/**
 * @brief Retrieves the full relative filename for a FAT16 directory item.
 * @param item Pointer to the directory item.
 * @param out Output buffer for the filename.
 * @param max_len Maximum length of the output buffer.
 */
void fat16_get_full_relative_filename(struct fat_directory_item *item, char *out, int max_len) {
    memset(out, 0x00, max_len);
    char *out_tmp = out;

    fat16_to_proper_string(&out_tmp, (const char *)item->filename, sizeof(item->filename));

    if (item->ext[0] != 0x00 && item->ext[0] != 0x20) {
        // add a dot then the extension
        *out_tmp++ = '.';
        fat16_to_proper_string(&out_tmp, (const char *)item->ext, sizeof(item->ext));
    }
}

/**
 * @brief Finds a FAT16 item in a directory by its name.
 * @param disk Pointer to the disk structure.
 * @param directory Pointer to the directory structure.
 * @param name Name of the item to find.
 * @return Pointer to the found item structure.
 */
struct fat_item *fat16_find_item_in_directory(struct disk *disk, struct fat_directory *directory, const char *name) {
    struct fat_item *f_item = NULL;
    char tmp_filename[TOYOS_MAX_PATH];

    for (int i = 0; i < directory->total; i++) {
        fat16_get_full_relative_filename(&directory->item[i], tmp_filename, sizeof(tmp_filename));

        if (istrncmp(tmp_filename, name, sizeof(tmp_filename)) == 0) {
            // Found it let's create a new fat_item
            f_item = fat16_new_fat_item_for_directory_item(disk, &directory->item[i]);
        }
    }

    return f_item;
}

/**
 * @brief Retrieves the FAT16 item for a directory entry from a given path.
 * @param disk Pointer to the disk structure.
 * @param path Pointer to the parsed path structure.
 * @return Pointer to the found FAT16 item structure.
 */
struct fat_item *fat16_get_directory_entry(struct disk *disk, struct path_part *path) {
    struct fat_private *fat_private = disk->fs_private;
    struct fat_item *current_item = 0;

    struct fat_item *root_item = fat16_find_item_in_directory(disk, &fat_private->root_directory, path->part);
    if (!root_item) {
        return NULL;
    }

    // we have root, now we need to traverse the path
    struct path_part *next_part = path->next;
    current_item = root_item;

    while (next_part != NULL) {
        if (current_item->type != FAT_ITEM_TYPE_DIRECTORY) {
            return NULL;
        }

        struct fat_item *tmp_item = fat16_find_item_in_directory(disk, current_item->directory, next_part->part);
        if (!tmp_item) {
            return NULL;
        }

        fat16_fat_item_free(current_item);
        current_item = tmp_item;

        // will be NULL if we are at the end of the path
        next_part = next_part->next;
    }

    return current_item;
}

/**
 * @brief Sets a FAT entry value for a given cluster in the FAT table.
 * @param disk Pointer to the disk structure.
 * @param cluster Cluster number.
 * @param value Value to set in the FAT entry.
 * @return Status code indicating success or failure.
 */
static int fat16_set_fat_entry(struct disk *disk, int cluster, int value) {
    if (!disk) {
        return -EINVARG;
    }

    struct fat_private *private = disk->fs_private;
    struct disk_stream *stream = private->fat_read_stream;
    if (!stream) {
        return -EIO;
    }

    uint32_t fat_table_position = fat16_get_first_fat_sector(private) * disk->sector_size;
    if (streamer_seek(stream, fat_table_position + (cluster * TOYOS_FAT16_FAT_ENTRY_SIZE)) != OK) {
        return -EIO;
    }

    uint16_t entry = value;
    if (streamer_write(stream, &entry, sizeof(entry)) != OK) {
        return -EIO;
    }

    return OK;
}

/**
 * @brief Allocates a new cluster in the FAT table.
 * @param disk Pointer to the disk structure.
 * @param current_cluster Current cluster number.
 * @return The new cluster number or an error code.
 */
static int fat16_allocate_cluster(struct disk *disk, int current_cluster) {
    if (!disk) {
        return -EINVARG;
    }

    struct fat_private *fs_private = disk->fs_private;

    // Find a free cluster
    for (int i = 2; i < fs_private->header.primary_header.number_of_sectors; i++) {
        int entry = fat16_get_fat_entry(disk, i);
        if (entry == TOYOS_FAT16_UNUSED) {
            // Mark the new cluster as used and link it to the current cluster
            if (fat16_set_fat_entry(disk, current_cluster, i) != OK) {
                return -EIO;
            }

            if (fat16_set_fat_entry(disk, i, 0xfff) != OK) {
                return -EIO;
            }

            return i;
        }
    }

    return -ENOMEM;
}

/**
 * @brief Gets the sector number for a given directory item.
 * @param private Pointer to the FAT16 private structure.
 * @param item Pointer to the directory item.
 * @return Sector number or an error code.
 */
static int fat16_get_directory_sector(struct fat_private *private, struct fat_directory_item *item) {
    if (!item || !private) {
        return -EINVARG;
    }

    if (item->attribute & FAT_FILE_SUBDIRECTORY) {
        // For subdirectories, calculate the sector based on the cluster
        int cluster = fat16_get_first_cluster(item);
        return fat16_cluster_to_sector(private, cluster);
    }

    // For the root directory, return the starting sector of the root directory
    struct fat_header *primary_header = &private->header.primary_header;
    int root_dir_sector =
        primary_header->reserved_sectors + (primary_header->fat_copies * primary_header->sectors_per_fat);
    return root_dir_sector;
}

/**
 * @brief Gets the offset within a directory for a given item.
 * @param private Pointer to the FAT16 private structure.
 * @param item Pointer to the directory item.
 * @return Offset or an error code.
 */
static int fat16_get_directory_offset(struct fat_private *private, struct fat_directory_item *item) {
    if (!item || !private) {
        return -EINVARG;
    }

    // For subdirectories, the offset is within the cluster
    if (item->attribute & FAT_FILE_SUBDIRECTORY) {
        // Subdirectories start at the beginning of their cluster
        return 0;
    }

    // For the root directory, calculate the offset based on the item's position
    struct fat_directory *root_directory = &private->root_directory;
    for (int i = 0; i < root_directory->total; i++) {
        if (&root_directory->item[i] == item) {
            return i * sizeof(struct fat_directory_item);
        }
    }

    // Item not found in the root directory
    return -EIO;
}

/**
 * @brief Opens a file or directory in the FAT16 filesystem.
 * @param disk Pointer to the disk structure.
 * @param path Pointer to the parsed path structure.
 * @param mode Mode to open the file (read or write).
 * @return Pointer to the file descriptor or error code.
 */
void *fat16_open(struct disk *disk, struct path_part *path, file_mode mode) {
    if (!disk || !path) {
        return ERROR(-EINVARG);
    }

    if (mode != FILE_MODE_READ && mode != FILE_MODE_WRITE) {
        // FILE_MODE_APPEND is not supported
        return ERROR(-EINVARG);
    }

    struct fat_file_descriptor *descriptor = NULL;
    int err_code = 0;

    descriptor = kzalloc(sizeof(struct fat_file_descriptor));
    if (!descriptor) {
        return ERROR(-ENOMEM);
    }

    descriptor->item = fat16_get_directory_entry(disk, path);
    if (!descriptor->item) {
        err_code = -EIO;
        goto err_out;
    }

    descriptor->pos = mode == FILE_MODE_APPEND ? descriptor->item->item->filesize : 0;
    return descriptor;

err_out:
    kfree(descriptor);
    return ERROR(err_code);
}

/**
 * @brief Reads data from a FAT16 file.
 * @param disk Pointer to the disk structure.
 * @param private_data Pointer to the file descriptor.
 * @param size Size of each element to read.
 * @param nmemb Number of elements to read.
 * @param out Output buffer.
 * @return Number of elements read or error code.
 */
int fat16_read(struct disk *disk, void *private_data, uint32_t size, uint32_t nmemb, char *out) {
    if (!disk || !private_data || !out) {
        return -EINVARG;
    }

    struct fat_file_descriptor *descriptor = private_data;
    struct fat_directory_item *item = descriptor->item->item;
    int offset = descriptor->pos;

    for (int i = 0; i < nmemb; i++) {
        int res = fat16_read_internal(disk, fat16_get_first_cluster(item), offset, size, out);
        if (res < 0) {
            return 0;
        }

        offset += size;
        offset += res;
    }

    return nmemb;
}

/**
 * @brief Seeks to a position within a FAT16 file.
 * @param private_data Pointer to the file descriptor.
 * @param offset Offset to seek to.
 * @param seek_mode Seek mode (SEEK_SET, SEEK_CUR, SEEK_END).
 * @return Status code indicating success or failure.
 */
int fat16_seek(void *private_data, uint32_t offset, file_seek_mode seek_mode) {
    if (!private_data) {
        return -EINVARG;
    }

    struct fat_file_descriptor *descriptor = private_data;
    struct fat_item *item = descriptor->item;

    if (item->type == FAT_ITEM_TYPE_DIRECTORY) {
        return -EINVARG;
    }

    struct fat_directory_item *dir_item = item->item;
    if (offset > dir_item->filesize) {
        return -EIO;
    }

    switch (seek_mode) {
    case SEEK_SET:
        descriptor->pos = offset;
        break;
    case SEEK_CUR:
        descriptor->pos += offset;
        break;
    case SEEK_END:
        descriptor->pos = dir_item->filesize + offset;
        break;
    default:
        return -EINVARG;
    }

    return OK;
}

/**
 * @brief Retrieves the status (metadata) of a FAT16 file.
 * @param disk Pointer to the disk structure.
 * @param private_data Pointer to the file descriptor.
 * @param stat Output structure for the file status.
 * @return Status code indicating success or failure.
 */
int fat16_stat(struct disk *disk, void *private_data, struct file_stat *stat) {
    if (!stat || !private_data || !disk) {
        return -EINVARG;
    }

    struct fat_file_descriptor *descriptor = private_data;
    struct fat_item *item = descriptor->item;
    if (!item) {
        return -EINVARG;
    }

    if (item->type == FAT_ITEM_TYPE_DIRECTORY) {
        return -EINVARG;
    }

    struct fat_directory_item *dir_item = item->item;
    stat->filesize = dir_item->filesize;
    stat->flags = 0;

    if (dir_item->attribute & FAT_FILE_READ_ONLY) {
        stat->flags |= FILE_STAT_READ_ONLY;
    }

    return OK;
}

/**
 * @brief Writes data to a FAT16 file.
 * @param disk Pointer to the disk structure.
 * @param private_data Pointer to the file descriptor.
 * @param size Size of each element to write.
 * @param nmemb Number of elements to write.
 * @param in Input buffer.
 * @return Number of elements written or error code.
 */
int fat16_write(struct disk *disk, void *private_data, uint32_t size, uint32_t nmemb, char *in) {
    if (!private_data || !disk || !in) {
        return -EINVARG;
    }

    struct fat_file_descriptor *descriptor = private_data;
    struct fat_directory_item *item = descriptor->item->item;
    struct fat_private *fs_private = disk->fs_private;

    if (item->attribute & FAT_FILE_READ_ONLY) {
        return -ERDONLY;
    }

    uint32_t total_bytes = size * nmemb;
    uint32_t bytes_written = 0;
    int cluster = fat16_get_first_cluster(item);
    int offset = descriptor->pos;

    while (total_bytes > 0) {
        int current_cluster = fat16_get_cluster_for_offset(disk, cluster, offset);
        if (current_cluster < 0) {
            return -EIO;
        }

        // Calculate the sector to write to
        int starting_sector = fat16_cluster_to_sector(fs_private, current_cluster);
        int offset_from_cluster = offset % (fs_private->header.primary_header.sectors_per_cluster * disk->sector_size);
        int starting_pos = (starting_sector * disk->sector_size) + offset_from_cluster;
        int bytes_to_write = total_bytes > (disk->sector_size - offset_from_cluster)
                                 ? (disk->sector_size - offset_from_cluster)
                                 : total_bytes;

        // Write to the disk
        struct disk_stream *stream = fs_private->cluster_read_stream;
        if (streamer_seek(stream, starting_pos) != OK) {
            return -EIO;
        }

        if (streamer_write(stream, in + bytes_written, bytes_to_write) != OK) {
            return -EIO;
        }

        total_bytes -= bytes_to_write;
        bytes_written += bytes_to_write;
        offset += bytes_to_write;

        // If we reach the end of the current cluster, get the next cluster
        if (offset_from_cluster + bytes_to_write >=
            disk->sector_size * fs_private->header.primary_header.sectors_per_cluster) {
            int next_cluster = fat16_get_fat_entry(disk, current_cluster);

            if (next_cluster == 0xfff || next_cluster == 0xff8) {
                // Allocate a new cluster if necessary
                next_cluster = fat16_allocate_cluster(disk, current_cluster);
                if (next_cluster < 0) {
                    return -EIO;
                }
            }

            cluster = next_cluster;
        }
    }

    if (descriptor->pos + bytes_written > item->filesize) {
        item->filesize = descriptor->pos + bytes_written;
    }

    // Update the directory entry on the disk
    int dir_sector = fat16_get_directory_sector(fs_private, item);
    if (dir_sector < 0) {
        return -EIO;
    }

    int dir_offset = fat16_get_directory_offset(fs_private, item);
    if (dir_offset < 0) {
        return -EIO;
    }

    if (streamer_seek(fs_private->directory_stream, dir_sector * disk->sector_size + dir_offset) != OK) {
        return -EIO;
    }

    if (streamer_write(fs_private->directory_stream, item, sizeof(*item)) != OK) {
        return -EIO;
    }

    descriptor->pos += bytes_written;
    return bytes_written;
}

/**
 * @brief Closes a FAT16 file or directory.
 * @param private_data Pointer to the file descriptor.
 * @return Status code indicating success or failure.
 */
int fat16_close(void *private_data) {
    if (!private_data) {
        return -EINVARG;
    }

    struct fat_file_descriptor *descriptor = private_data;
    fat16_fat_item_free(descriptor->item);
    kfree(descriptor);

    return OK;
}
