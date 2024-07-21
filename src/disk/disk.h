#ifndef DISK_H
#define DISK_H

#include "fs/file.h"

// Represents a real physical hard disk
#define DISK_TYPE_REAL 0

typedef unsigned int disk_type;

struct disk
{
    disk_type type;
    int sector_size;
    int id;
    
    // filesystem binded to disk
    struct filesystem* fs;

    // private data of filesystem
    void* fs_private;
};

int disk_read_block(struct disk* idisk, unsigned int lba, int total, void* buf);
void disk_search_and_init(void);
struct disk* disk_get(int index);

#endif
