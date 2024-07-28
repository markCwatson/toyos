#include "streamer.h"
#include "memory/heap/kheap.h"
#include "config.h"
#include "status.h"
#include <stdbool.h>

/**
 * @brief Creates a new disk stream for the specified disk.
 *
 * Allocates and initializes a disk_stream structure for the disk with the given ID.
 *
 * @param disk_id The identifier of the disk to stream from.
 * @return A pointer to the newly created disk_stream structure, or NULL if the disk is not found or allocation fails.
 */
struct disk_stream* streamer_new(int disk_id) {
    struct disk* disk = disk_get(disk_id);
    if (!disk) {
        return NULL;
    }

    struct disk_stream* streamer = kzalloc(sizeof(struct disk_stream));
    if (!streamer) {
        return NULL;
    }

    streamer->pos = 0;
    streamer->disk = disk;

    return streamer;
}

/**
 * @brief Sets the position in the disk stream.
 *
 * Changes the position in the stream to the specified byte offset, enabling subsequent read/write operations to begin at this position.
 *
 * @param stream The disk stream to modify.
 * @param pos The new byte offset in the disk stream.
 * @return 0 on success, or an error code if the stream is invalid or the position is out of bounds.
 */
int streamer_seek(struct disk_stream* stream, int pos) {
    if (!stream || pos < 0) {
        return -EINVARG;
    }

    stream->pos = pos;
    return OK;
}

/**
 * @brief Reads data from the disk into a buffer.
 *
 * Reads `total` bytes from the current position in the disk stream into the provided buffer.
 * If the read operation crosses sector boundaries, it handles the overflow by reading the next sector.
 *
 * @param stream The disk stream to read from.
 * @param out The buffer to store the read data.
 * @param total The total number of bytes to read.
 * @return 0 on success, or an error code on failure.
 */
int streamer_read(struct disk_stream* stream, void* out, int total) {
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
        *(char*)out++ = buf[offset + i];
    }

    // Update the stream position
    stream->pos += total_to_read;
    if (overflow) {
        res = streamer_read(stream, out, total - total_to_read);
    }

out:
    return res;
}

/**
 * @brief Writes data from a buffer to the disk.
 *
 * Writes `total` bytes from the provided buffer into the disk stream starting at the current position.
 * Handles writes that span multiple sectors by managing overflow appropriately.
 *
 * @param stream The disk stream to write to.
 * @param in The buffer containing the data to write.
 * @param total The total number of bytes to write.
 * @return 0 on success, or an error code on failure.
 */
int streamer_write(struct disk_stream* stream, const void* in, int total) {
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
        buf[offset + i] = *(const char*)in++;
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

/**
 * @brief Closes the disk stream and frees its resources.
 *
 * This function frees the memory allocated for the disk stream structure.
 *
 * @param stream The disk stream to close.
 */
void streamer_close(struct disk_stream* stream) {
    kfree(stream);
}
