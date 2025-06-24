#include "streamer.h"
#include "config.h"
#include "memory/heap/kheap.h"
#include "status.h"
#include <stdbool.h>

struct disk_stream *streamer_new(int disk_id) {
    struct disk *disk = disk_get(disk_id);
    if (!disk) {
        return NULL;
    }

    struct disk_stream *streamer = kzalloc(sizeof(struct disk_stream));
    if (!streamer) {
        return NULL;
    }

    streamer->pos = 0;
    streamer->disk = disk;

    return streamer;
}

int streamer_seek(struct disk_stream *stream, int pos) {
    if (!stream || pos < 0) {
        return -EINVARG;
    }

    stream->pos = pos;
    return OK;
}

int streamer_read(struct disk_stream *stream, void *out, int total) {
    if (!stream || !out || total < 0) {
        return -EINVARG;
    }

    int sector = stream->pos / TOYOS_SECTOR_SIZE;
    int offset = stream->pos % TOYOS_SECTOR_SIZE;
    int total_to_read = total;
    bool overflow = (offset + total_to_read) >= TOYOS_SECTOR_SIZE;
    char buf[TOYOS_SECTOR_SIZE];

    if (overflow) {
        total_to_read -= (offset + total_to_read) - TOYOS_SECTOR_SIZE;
    }

    int res = disk_read_block(stream->disk, sector, 1, buf);
    if (res < 0) {
        goto out;
    }

    // Copy the read data from the buffer to the output
    for (int i = 0; i < total_to_read; i++) {
        *(char *)out++ = buf[offset + i];
    }

    // Update the stream position
    stream->pos += total_to_read;
    if (overflow) {
        res = streamer_read(stream, out, total - total_to_read);
    }

out:
    return res;
}

int streamer_write(struct disk_stream *stream, const void *in, int total) {
    if (!stream || !in || total < 0) {
        return -EINVARG;
    }

    int sector = stream->pos / TOYOS_SECTOR_SIZE;
    int offset = stream->pos % TOYOS_SECTOR_SIZE;
    int total_to_write = total;
    bool overflow = (offset + total_to_write) >= TOYOS_SECTOR_SIZE;
    char buf[TOYOS_SECTOR_SIZE];

    // If the write spans multiple sectors, handle the overflow
    if (overflow) {
        total_to_write -= (offset + total_to_write) - TOYOS_SECTOR_SIZE;
    }

    // Read the current sector into the buffer if not writing an entire sector
    if (offset != 0 || total_to_write != TOYOS_SECTOR_SIZE) {
        int res = disk_read_block(stream->disk, sector, 1, buf);
        if (res < 0) {
            goto out;
        }
    }

    // Copy data into the buffer
    for (int i = 0; i < total_to_write; i++) {
        buf[offset + i] = *(const char *)in++;
    }

    // Write the buffer back to the disk
    int res = disk_write_block(stream->disk, sector, 1, buf);
    if (res < 0) {
        goto out;
    }

    // Update the stream position
    stream->pos += total_to_write;
    if (overflow) {
        res = streamer_write(stream, in, total - total_to_write);
    }

out:
    return res;
}

void streamer_close(struct disk_stream *stream) {
    kfree(stream);
}
