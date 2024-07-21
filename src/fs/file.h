#ifndef FILE_H
#define FILE_H

#include "path_parser.h"
#include <stdint.h>

typedef unsigned int file_seek_mode;

enum
{
    SEEK_SET,
    SEEK_CUR,
    SEEK_END
};

typedef unsigned int file_mode;

enum
{ 
    FILE_MODE_READ,
    FILE_MODE_WRITE,
    FILE_MODE_APPEND,
    FILE_MODE_INVALID
};

typedef unsigned int file_stat_flags;

enum
{
    FILE_STAT_READ_ONLY = 0b00000001
};

// forward declare struct disk
struct disk;

struct file_stat
{
    file_stat_flags flags;
    uint32_t filesize;
};

typedef void* (*fs_open_fp)(struct disk* disk, struct path_part* path, file_mode mode);
typedef int (*fs_resolve_fp)(struct disk* disk);
typedef int (*fs_read_fp)(struct disk* disk, void* private_data, uint32_t size, uint32_t nmemb, char* out);
typedef int (*fs_close_fp)(void* private_data);
typedef int (*fs_seek_fp)(void* private_data, uint32_t offset, file_seek_mode seek_mode);
typedef int (*fs_stat_fp)(struct disk* disk, void* private_data, struct file_stat* stat);

// interface for a file system
struct filesystem
{
    char name[20];
    // Filesystem should return zero from resolve if the provided disk is using its filesystem
    fs_resolve_fp resolve;
    fs_open_fp open;
    fs_read_fp read;
    fs_seek_fp seek;
    fs_stat_fp stat;
    fs_close_fp close;
};

struct file_descriptor
{
    // The descriptor index
    int index;
    struct filesystem* fs;

    // Private data for internal file descriptor
    void* private_data;

    // The disk that the file descriptor should be used on
    struct disk* disk;
};

void fs_init(void);
int fopen(const char* filename, const char* mode_str);
int fseek(int fd, int offset, file_seek_mode whence);
int fread(void* ptr, uint32_t size, uint32_t nmemb, int fd);
int fstat(int fd, struct file_stat* stat);
int fclose(int fd);
void fs_insert_filesystem(struct filesystem* filesystem);
struct filesystem* fs_resolve(struct disk* disk);

#endif
