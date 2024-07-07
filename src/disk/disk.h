#ifndef DISK_H
#define DISK_H

// Represents a real physical hard disk
#define DISK_TYPE_REAL 0


typedef unsigned int disk_type;

struct disk
{
    disk_type type;
    int sector_size;
    int id;
};

int disk_read_block(struct disk* idisk, unsigned int lba, int total, void* buf);
void disk_search_and_init();
struct disk* disk_get(int index);

#endif
