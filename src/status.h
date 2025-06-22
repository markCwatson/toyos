#ifndef _STATUS_H_
#define _STATUS_H_

/**
 * @brief Status code indicating that an operation completed successfully.
 */
#define OK 0 /**< No error, operation was successful. */

/**
 * @brief Status code indicating an invalid argument error.
 *
 * This error occurs when a function receives an argument that is out of range,
 * improperly formatted, or otherwise invalid.
 */
#define EINVARG 1 /**< Invalid argument error. */

/**
 * @brief Status code indicating an out-of-memory error.
 *
 * This error occurs when a memory allocation fails because there is not enough
 * memory available to fulfill the request.
 */
#define ENOMEM 2 /**< Out of memory error. */

/**
 * @brief Status code indicating an input/output error.
 *
 * This error typically occurs during file or disk operations when an I/O failure happens,
 * such as a read or write failure.
 */
#define EIO 3 /**< Input/output error. */

/**
 * @brief Status code indicating an invalid or non-existent path.
 *
 * This error occurs when a specified file or directory path cannot be found or is invalid.
 */
#define EBADPATH 4 /**< Bad path error. */

/**
 * @brief Status code indicating insufficient memory for file operations.
 *
 * This error occurs when there is not enough memory to load or process a file.
 */
#define ENOFILEMEM 5 /**< No file memory error. */

/**
 * @brief Status code indicating that the file system is not recognized or supported.
 *
 * This error occurs when an operation is attempted on a file system that the system does
 * not recognize or support.
 */
#define EFSNOTUS 6 /**< File system not recognized error. */

/**
 * @brief Status code indicating that the operation attempted to write to a read-only resource.
 *
 * This error occurs when a write operation is attempted on a read-only file or file system.
 */
#define ERDONLY 7 /**< Read-only error. */

/**
 * @brief Status code indicating that a process slot is already taken.
 *
 * This error occurs when a process is created with an ID that is already in use by another process.
 */
#define EISTKN 8 /** < Process slot is taken */

/**
 * @brief Status code indicating an invalid format.
 *
 * This error occurs when a file or data is in an invalid or unrecognized format.
 */
#define EINFORMAT 9 /** < Invalid format */

/**
 * @brief Status code indicating that a resource is busy.
 *
 * This error occurs when a resource is already in use and cannot be accessed.
 */
#define EBUSY 10 /** < Resource is busy */

#endif
