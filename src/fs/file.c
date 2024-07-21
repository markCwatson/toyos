#include "file.h"
#include "fat/fat16.h"
#include "config.h"
#include "kernel.h"
#include "memory/memory.h"
#include "memory/heap/kheap.h"
#include "status.h"
#include "disk/disk.h"
#include "string/string.h"

struct filesystem* filesystems[TOYOS_MAX_FILESYSTEMS] = { NULL };
struct file_descriptor* file_descriptors[TOYOS_MAX_FILE_DESCRIPTORS] = { NULL };

static struct filesystem** fs_get_free_filesystem(void) {
    for (int i = 0; i < TOYOS_MAX_FILESYSTEMS; i++) {
        if (filesystems[i] == NULL) {
            return &filesystems[i];
        }
    }

    return NULL;
}

void fs_insert_filesystem(struct filesystem* filesystem) {
    struct filesystem** fs = fs_get_free_filesystem();
    if (!fs) {
        panick("Problem inserting filesystem"); 
    }

    // add to list of file systems
    *fs = filesystem;
}

static void fs_static_load(void) {
    fs_insert_filesystem(fat16_init());
}

void fs_load(void) {
    fs_static_load();
}

void fs_init(void) {
    memset(filesystems, 0, sizeof(filesystems));
    memset(file_descriptors, 0, sizeof(file_descriptors));
    fs_load();
}

static file_mode file_get_mode_by_string(const char* str) {
    if (strncmp(str, "r", 1) == 0) {
        return FILE_MODE_READ;
    } else if (strncmp(str, "w", 1) == 0) {
        return FILE_MODE_WRITE;
    } else if (strncmp(str, "a", 1) == 0) {
        return FILE_MODE_APPEND;
    }

    return FILE_MODE_INVALID;
}

static int file_new_descriptor(struct file_descriptor** file_desc) {
    for (int i = 0; i < TOYOS_MAX_FILE_DESCRIPTORS; i++) {
        if (file_descriptors[i] == NULL) {
            struct file_descriptor* new_file_desc = kzalloc(sizeof(struct file_descriptor));
            if (!new_file_desc) {
                return -ENOMEM;
            }

            // Descriptors start at 1
            new_file_desc->index = i + 1;
            file_descriptors[i] = new_file_desc;

            *file_desc = new_file_desc;

            return ALL_GOOD;
        }
    }

    return -ENOFILEMEM;
}

static struct file_descriptor* file_get_descriptor(int file_desc_id) {
    if (file_desc_id <= 0 || file_desc_id >= TOYOS_MAX_FILE_DESCRIPTORS) {
        return NULL;
    }

    // Descriptors start at 1
    return file_descriptors[file_desc_id - 1];
}

struct filesystem* fs_resolve(struct disk* disk) {
    for (int i = 0; i < TOYOS_MAX_FILESYSTEMS; i++) {
        if (filesystems[i] != NULL && filesystems[i]->resolve(disk) == ALL_GOOD) {
            return filesystems[i];
        }
    }

    return NULL;
}

int fopen(const char* filename, const char* mode_str) {
    int res = 0;
    struct path_root* root_path = path_parser_parse(filename, NULL);
    if (!root_path || !root_path->first) {
        // We cannot have just a root path 0:/ 0:/test.txt
        res = -EINVARG;
        goto out;
    }

    // Ensure the disk we are reading from exists
    struct disk* disk = disk_get(root_path->drive_no);
    if (!disk || !disk->fs) {
        res = -EIO;
        goto out;
    }

    file_mode mode = file_get_mode_by_string(mode_str);
    if (mode == FILE_MODE_INVALID) {
        res = -EIO;
        goto out;
    }

    void* descriptor_private_data = disk->fs->open(disk, root_path->first, mode);
    if (ISERROR(descriptor_private_data)) {
        res = ERROR_I(descriptor_private_data);
        goto out;
    }

    struct file_descriptor* desc = NULL;
    res = file_new_descriptor(&desc);
    if (res < 0) {
        goto out;
    }

    desc->fs = disk->fs;
    desc->private_data = descriptor_private_data;
    desc->disk = disk;
    res = desc->index;

out:
    // fopen shouldnt return negative values
    if (res < 0) {
        return 0;
    }

    return res;
}

int fread(void* ptr, uint32_t size, uint32_t nmemb, int fd) {
    if (size == 0 || nmemb == 0 || fd < 0) {
        return -EINVARG;
    }

    struct file_descriptor* desc = file_get_descriptor(fd);
    if (!desc) {
        return -EINVARG;
    }

    return desc->fs->read(desc->disk, desc->private_data, size, nmemb, (char*)ptr);
}

int fseek(int fd, int offset, file_seek_mode whence) {
    if (fd < 0) {
        return -EINVARG;
    }

    struct file_descriptor* desc = file_get_descriptor(fd);
    if (!desc) {
        return -EINVARG;
    }

    return desc->fs->seek(desc->private_data, offset, whence);
}

int fstat(int fd, struct file_stat* stat) {
    if (fd < 0) {
        return -EINVARG;
    }

    struct file_descriptor* desc = file_get_descriptor(fd);
    if (!desc) {
        return -EINVARG;
    }

    return desc->fs->stat(desc->disk, desc->private_data, stat);
}

int fclose(int fd) {
    if (fd < 0) {
        return -EINVARG;
    }

    struct file_descriptor* desc = file_get_descriptor(fd);
    if (!desc) {
        return -EINVARG;
    }

    int res = desc->fs->close(desc->private_data);
    if (res < 0) {
        return res;
    }

    kfree(desc);
    file_descriptors[fd - 1] = NULL;

    return ALL_GOOD;
}
