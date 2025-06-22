#ifndef _FILE_H_
#define _FILE_H_

#include "path_parser.h"
#include <stdint.h>

/**
 * @brief File seek modes.
 *
 * These modes define how the file pointer is moved within a file.
 */
typedef unsigned int file_seek_mode;

enum {
    SEEK_SET, /**< Set file pointer to a specific position. */
    SEEK_CUR, /**< Move file pointer relative to the current position. */
    SEEK_END  /**< Move file pointer relative to the end of the file. */
};

/**
 * @brief File access modes.
 *
 * These modes determine how a file is opened and accessed.
 */
typedef unsigned int file_mode;

enum {
    FILE_MODE_READ,   /**< Open file for reading. */
    FILE_MODE_WRITE,  /**< Open file for writing. */
    FILE_MODE_APPEND, /**< Open file for appending. */
    FILE_MODE_INVALID /**< Invalid file mode. */
};

/**
 * @brief File status flags.
 *
 * Flags providing additional status information about a file.
 */
typedef unsigned int file_stat_flags;

enum {
    FILE_STAT_READ_ONLY = 0b00000001 /**< File is read-only. */
};

/* Forward declaration of the struct disk */
struct disk;

/**
 * @brief Structure for storing file status information.
 *
 * This structure holds flags and the size of the file.
 */
struct file_stat {
    file_stat_flags flags; /**< Status flags for the file. */
    uint32_t filesize;     /**< Size of the file in bytes. */
};

/* Function pointer types for file system operations */
typedef void *(*fs_open_fp)(struct disk *disk, struct path_part *path, file_mode mode);
typedef int (*fs_resolve_fp)(struct disk *disk);
typedef int (*fs_read_fp)(struct disk *disk, void *private_data, uint32_t size, uint32_t nmemb, char *out);
typedef int (*fs_write_fp)(struct disk *disk, void *private_data, uint32_t size, uint32_t nmemb, char *in);
typedef int (*fs_close_fp)(void *private_data);
typedef int (*fs_seek_fp)(void *private_data, uint32_t offset, file_seek_mode seek_mode);
typedef int (*fs_stat_fp)(struct disk *disk, void *private_data, struct file_stat *stat);

/**
 * @brief File system interface structure.
 *
 * This structure defines the interface for a file system, including functions
 * for resolving, opening, reading, writing, seeking, getting status, and closing files.
 */
struct filesystem {
    char name[20];         /**< Name of the file system. */
    fs_resolve_fp resolve; /**< Function to resolve if the provided disk uses this file system. */
    fs_open_fp open;       /**< Function to open a file. */
    fs_read_fp read;       /**< Function to read from a file. */
    fs_write_fp write;     /**< Function to write to a file. */
    fs_seek_fp seek;       /**< Function to seek within a file. */
    fs_stat_fp stat;       /**< Function to get file status. */
    fs_close_fp close;     /**< Function to close a file. */
};

/**
 * @brief Structure representing a file descriptor.
 *
 * A file descriptor contains information about an open file, including its index,
 * the file system it belongs to, and any private data needed for file operations.
 */
struct file_descriptor {
    int index;             /**< The descriptor index. */
    struct filesystem *fs; /**< Pointer to the file system that handles this file. */
    void *private_data;    /**< Private data for internal file descriptor. */
    struct disk *disk;     /**< The disk that the file descriptor is associated with. */
};

/* File system function declarations */

/**
 * @brief Initializes the file system infrastructure.
 *
 * This function should be called during system initialization to set up
 * the necessary structures and states for file system operations.
 */
void fs_init(void);

/**
 * @brief Opens a file with the specified filename and mode.
 *
 * This function attempts to open a file and returns a file descriptor if successful.
 *
 * @param filename The name of the file to open.
 * @param mode_str The mode in which to open the file (e.g., "r" for read, "w" for write).
 * @return A file descriptor if successful, or a negative error code.
 */
int fopen(const char *filename, const char *mode_str);

/**
 * @brief Seeks to a specific position in an open file.
 *
 * This function moves the file pointer to a specified offset, based on the seek mode.
 *
 * @param fd The file descriptor of the file.
 * @param offset The offset to move the file pointer to.
 * @param whence The base from which the offset is applied.
 * @return 0 if successful, or a negative error code.
 */
int fseek(int fd, int offset, file_seek_mode whence);

/**
 * @brief Reads data from an open file.
 *
 * This function reads data from a file into a buffer.
 *
 * @param ptr The buffer to read data into.
 * @param size The size of each element to read.
 * @param nmemb The number of elements to read.
 * @param fd The file descriptor of the file.
 * @return The number of elements successfully read, or a negative error code.
 */
int fread(void *ptr, uint32_t size, uint32_t nmemb, int fd);

/**
 * @brief Writes data to an open file.
 *
 * This function writes data from a buffer to a file.
 *
 * @param ptr The buffer containing the data to write.
 * @param size The size of each element to write.
 * @param nmemb The number of elements to write.
 * @param fd The file descriptor of the file.
 * @return The number of elements successfully written, or a negative error code.
 */
int fwrite(void *ptr, uint32_t size, uint32_t nmemb, int fd);

/**
 * @brief Retrieves the status of an open file.
 *
 * This function fills a file_stat structure with information about a file, such as its size and flags.
 *
 * @param fd The file descriptor of the file.
 * @param stat The structure to fill with file status information.
 * @return 0 if successful, or a negative error code.
 */
int fstat(int fd, struct file_stat *stat);

/**
 * @brief Closes an open file.
 *
 * This function closes a file and releases any resources associated with the file descriptor.
 *
 * @param fd The file descriptor of the file to close.
 * @return 0 if successful, or a negative error code.
 */
int fclose(int fd);

/**
 * @brief Inserts a new file system into the system's list of available file systems.
 *
 * This function registers a file system, making it available for resolving and managing files.
 *
 * @param filesystem The file system to register.
 */
void fs_insert_filesystem(struct filesystem *filesystem);

/**
 * @brief Resolves the file system used by a specific disk.
 *
 * This function determines which file system is used by a disk and returns a pointer to the corresponding filesystem
 * structure.
 *
 * @param disk The disk to check.
 * @return A pointer to the filesystem structure, or NULL if no file system is found.
 */
struct filesystem *fs_resolve(struct disk *disk);

#endif
