#ifndef DISK_STREAMER_H
#define DISK_STREAMER_H

#include "disk.h"

/**
 * @brief Structure representing a stream for reading from or writing to a disk.
 */
struct disk_stream {
    int pos;            /**< Current position in the stream (byte offset). */
    struct disk* disk;  /**< Pointer to the disk associated with this stream. */
};

/**
 * @brief Creates a new disk stream for the specified disk.
 * 
 * @param disk_id The identifier of the disk to stream from.
 * @return A pointer to the new disk_stream structure, or NULL if the disk is not found.
 */
struct disk_stream* streamer_new(int disk_id);

/**
 * @brief Moves the stream position to the specified byte offset.
 * 
 * @param stream The disk stream to modify.
 * @param pos The new position in bytes from the start of the disk.
 * @return 0 on success, or an error code on failure.
 */
int streamer_seek(struct disk_stream* stream, int pos);

/**
 * @brief Reads data from the disk stream into a buffer.
 * 
 * @param stream The disk stream to read from.
 * @param out The buffer to store the read data.
 * @param total The number of bytes to read.
 * @return 0 on success, or an error code on failure.
 */
int streamer_read(struct disk_stream* stream, void* out, int total);

/**
 * @brief Writes data from a buffer to the disk stream.
 * 
 * @param stream The disk stream to write to.
 * @param in The buffer containing the data to write.
 * @param total The number of bytes to write.
 * @return 0 on success, or an error code on failure.
 */
int streamer_write(struct disk_stream* stream, const void* in, int total);

/**
 * @brief Closes the disk stream and frees associated resources.
 * 
 * @param stream The disk stream to close.
 */
void streamer_close(struct disk_stream* stream);

#endif
