#include "fat16.h"
#include "status.h"
#include "config.h"
#include "kernel.h"
#include "memory/memory.h"
#include "string/string.h"
#include "disk/streamer.h"
#include "disk/disk.h"
#include "memory/heap/kheap.h"
#include <stddef.h>
#include <stdint.h>

#define TOYOS_FAT16_SIGNATURE       0x29
#define TOYOS_FAT16_FAT_ENTRY_SIZE  0x02
#define TOYOS_FAT16_BAD_SECTOR      0xff7
#define TOYOS_FAT16_UNUSED          0x00

// Fat directory entry attributes bitmask
#define FAT_FILE_READ_ONLY          0x01
#define FAT_FILE_HIDDEN             0x02
#define FAT_FILE_SYSTEM             0x04
#define FAT_FILE_VOLUME_LABEL       0x08
#define FAT_FILE_SUBDIRECTORY       0x10
#define FAT_FILE_ARCHIVED           0x20
#define FAT_FILE_DEVICE             0x40
#define FAT_FILE_RESERVED           0x80

#define FAT_ITEM_TYPE_DIRECTORY     0
#define FAT_ITEM_TYPE_FILE          1

#define DIRECTORY_ENTRY_AVAILABLE   0xe5

typedef unsigned int fat_item_type;

struct fat_header_extended
{
    uint8_t drive_number;
    uint8_t win_nt_bit;
    uint8_t signature;
    uint32_t volume_id;
    uint8_t volume_id_string[11];
    uint8_t system_id_string[8];
} __attribute__((packed));

struct fat_header
{
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

struct fat_h
{
    struct fat_header primary_header;
    union fat_h_e {
        struct fat_header_extended extended_header;
    } shared;
};

struct fat_directory_item
{
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

struct fat_directory
{
    struct fat_directory_item* item;
    int total;
    int sector_pos;
    int ending_sector_pos;
};

struct fat_item
{
    union {
        struct fat_directory_item* item;
        struct fat_directory* directory;
    };

    fat_item_type type;
};

struct fat_file_descriptor
{
    struct fat_item* item;
    uint32_t pos;
};

struct fat_private
{
    struct fat_h header;
    struct fat_directory root_directory;

    // Used to stream data clusters
    struct disk_stream* cluster_read_stream;
    // Used to stream the file allocation table
    struct disk_stream* fat_read_stream;

    // Used in situations where we stream the directory
    struct disk_stream* directory_stream;
};

int fat16_resolve(struct disk* disk);
void* fat16_open(struct disk* disk, struct path_part* path, file_mode mode);
int fat16_read(struct disk* disk, void* private_data, uint32_t size, uint32_t nmemb, char* out);

struct filesystem fat16_fs = {
    .resolve = fat16_resolve,
    .open = fat16_open,
    .read = fat16_read
};

struct filesystem* fat16_init(void) {
    strcpy(fat16_fs.name, "FAT16");
    return &fat16_fs;
}

static void fat16_init_private(struct disk *disk, struct fat_private *private) {
    memset(private, 0, sizeof(struct fat_private));
    private->cluster_read_stream = streamer_new(disk->id);
    private->fat_read_stream = streamer_new(disk->id);
    private->directory_stream = streamer_new(disk->id);
}

static inline struct disk_stream* get_disk_directory_stream(const struct disk* disk) {
    return ((const struct fat_private*)disk->fs_private)->directory_stream;
}

int fat16_get_total_items_for_directory(struct disk* disk, uint32_t directory_start_sector) {
    int total_items = 0;

    struct fat_directory_item item;
    struct fat_directory_item empty_item;
    memset(&empty_item, 0, sizeof(empty_item));

    struct disk_stream* stream = get_disk_directory_stream(disk);

    int directory_start_pos = directory_start_sector * disk->sector_size;

    if (streamer_seek(stream, directory_start_pos) != ALL_GOOD) {
        return -EIO;
    }

    while (1) {
        if (streamer_read(stream, &item, sizeof(item)) != ALL_GOOD) {
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

int fat16_sector_to_absolute(struct disk *disk, int sector) {
    return sector * disk->sector_size;
}

int fat16_get_root_directory(struct disk *disk, struct fat_private *fat_private, struct fat_directory *directory) {
    int res = ALL_GOOD;
    struct fat_directory_item* dir = NULL;
    struct fat_header* primary_header = &fat_private->header.primary_header;

    int root_dir_sector_pos = (primary_header->fat_copies * primary_header->sectors_per_fat) + primary_header->reserved_sectors;
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

    if (streamer_seek(stream, fat16_sector_to_absolute(disk, root_dir_sector_pos)) != ALL_GOOD) {
        res = -EIO;
        goto err_out;
    }

    if (streamer_read(stream, dir, root_dir_size) != ALL_GOOD) {
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

// binds fat16 fs to disk
int fat16_resolve(struct disk* disk) {
    int res = ALL_GOOD;
    struct fat_private *fat_private = kzalloc(sizeof(struct fat_private));
    if (!fat_private) {
        return -ENOMEM;
    }

    fat16_init_private(disk, fat_private);

    disk->fs_private = fat_private;
    // bind to disk
    disk->fs = &fat16_fs;

    struct disk_stream *stream = streamer_new(disk->id);
    if (!stream) {
        res = -ENOMEM;
        goto out;
    }

    if (streamer_read(stream, &fat_private->header, sizeof(fat_private->header)) != ALL_GOOD) {
        res = -EIO;
        goto out;
    }

    if (fat_private->header.shared.extended_header.signature != TOYOS_FAT16_SIGNATURE) {
        res = -EFSNOTUS;
        goto out;
    }

    if (fat16_get_root_directory(disk, fat_private, &fat_private->root_directory) != ALL_GOOD) {
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

struct fat_directory_item* fat16_clone_directory_item(struct fat_directory_item* item, int size) {
    if (size < sizeof(struct fat_directory_item)) {
        return NULL;
    }

    struct fat_directory_item* item_copy = kzalloc(size);
    if (!item_copy) {
        return NULL;
    }

    memcpy(item_copy, item, size);
    return item_copy;
}

static uint32_t fat16_get_first_cluster(struct fat_directory_item* item) {
    return item->high_16_bits_first_cluster | item->low_16_bits_first_cluster;
};

static int fat16_cluster_to_sector(struct fat_private* private, int cluster) {
    return private->root_directory.ending_sector_pos + ((cluster - 2) * private->header.primary_header.sectors_per_cluster);
}

static uint32_t fat16_get_first_fat_sector(struct fat_private* private) {
    return private->header.primary_header.reserved_sectors;
}

static int fat16_get_fat_entry(struct disk *disk, int cluster) {
    int res = -100;
    struct fat_private *private = disk->fs_private;
    struct disk_stream *stream = private->fat_read_stream;
    if (!stream) {
        goto out;
    }

    uint32_t fat_table_position = fat16_get_first_fat_sector(private) * disk->sector_size;
    res = streamer_seek(stream, fat_table_position * (cluster * TOYOS_FAT16_FAT_ENTRY_SIZE));
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

static int fat16_read_internal_from_stream(struct disk *disk, struct disk_stream *stream, int cluster, int offset, int total, void *out) {
    int res = 0;
    struct fat_private* private = disk->fs_private;

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
    if (res != ALL_GOOD) {
        goto out;
    }

    res = streamer_read(stream, out, total_to_read);
    if (res != ALL_GOOD) {
        goto out;
    }

    total -= total_to_read;
    if (total > 0) {
        // still more to read so recursively call this function
        res = fat16_read_internal_from_stream(disk, stream, cluster, offset + total_to_read, total, out + total_to_read);
    }

out:
    return res;
}

static int fat16_read_internal(struct disk* disk, int starting_cluster, int offset, int total, void* out) {
    struct fat_private* fs_private = disk->fs_private;
    struct disk_stream* stream = fs_private->cluster_read_stream;

    return fat16_read_internal_from_stream(disk, stream, starting_cluster, offset, total, out);
}

static void fat16_free_directory(struct fat_directory* directory) {
    if (!directory) {
        return;
    }

    if (directory->item) {
        kfree(directory->item);
    }

    kfree(directory);
}

static void fat16_fat_item_free(struct fat_item *item) {
    if (item->type == FAT_ITEM_TYPE_DIRECTORY) {
        fat16_free_directory(item->directory);
    } else if (item->type == FAT_ITEM_TYPE_FILE) {
        kfree(item->item);
    }

    kfree(item);
}

struct fat_directory* fat16_load_fat_directory(struct disk* disk, struct fat_directory_item* item) {
    if (!(item->attribute & FAT_FILE_SUBDIRECTORY)) {
        return NULL;
    }

    struct fat_directory* directory = NULL;
    struct fat_private* fat_private = disk->fs_private;

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
    if (res != ALL_GOOD) {
        goto out;
    }

out:
    if (res != ALL_GOOD) {
        fat16_free_directory(directory);
    }

    return directory;
}

struct fat_item* fat16_new_fat_item_for_directory_item(struct disk* disk, struct fat_directory_item* item) {
    struct fat_item* f_item = kzalloc(sizeof(struct fat_item));
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

// a filename is 8 bytes. spaces are used to pad the filename
// this function removes the padding by replacing spaces with a null terminator
void fat16_to_proper_string(char** out, const char* in, size_t size) {
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

struct fat_item *fat16_find_item_in_directory(struct disk *disk, struct fat_directory *directory, const char *name) {
    struct fat_item* f_item = NULL;
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

struct fat_item* fat16_get_directory_entry(struct disk* disk, struct path_part* path) {
    struct fat_private* fat_private = disk->fs_private;
    struct fat_item* current_item = 0;

    struct fat_item* root_item = fat16_find_item_in_directory(disk, &fat_private->root_directory, path->part);
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

        struct fat_item* tmp_item = fat16_find_item_in_directory(disk, current_item->directory, next_part->part);
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

void* fat16_open(struct disk *disk, struct path_part* path, file_mode mode) {
    if (mode != FILE_MODE_READ) {
        return ERROR(-ERDONLY);
    }

    struct fat_file_descriptor* descriptor = NULL;
    int err_code = 0;

    descriptor = kzalloc(sizeof(struct fat_file_descriptor));
    if (!descriptor) {
        err_code = -ENOMEM;
        goto err_out;
    }

    descriptor->item = fat16_get_directory_entry(disk, path);
    if (!descriptor->item) {
        err_code = -EIO;
        goto err_out;
    }

    descriptor->pos = 0;
    return descriptor;

err_out:
    if(descriptor) {
        kfree(descriptor);
    }

    return ERROR(err_code);
}

int fat16_read(struct disk* disk, void* private_data, uint32_t size, uint32_t nmemb, char* out) {
    struct fat_file_descriptor* descriptor = private_data;
    struct fat_directory_item* item = descriptor->item->item;
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
