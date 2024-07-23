#ifndef DISK_STREAMER_H
#define DISK_STREAMER_H

#include "disk.h"

struct disk_stream {
    int pos;
    struct disk* disk;
};

struct disk_stream* streamer_new(int disk_id);
int streamer_seek(struct disk_stream* stream, int pos);
int streamer_read(struct disk_stream* stream, void* out, int total);
int streamer_write(struct disk_stream* stream, const void* in, int total);
void streamer_close(struct disk_stream* stream);

#endif
