#ifndef _CONFIG_H_
#define _CONFIG_H_

/**
 * @brief Selector values for the kernel's code and data segments.
 * 
 * These values are used in the GDT (Global Descriptor Table) to switch between
 * code and data segments in protected mode.
 */
#define KERNEL_CODE_SELECTOR        0x08  /**< Kernel code segment selector. */
#define KERNEL_DATA_SELECTOR        0x10  /**< Kernel data segment selector. */

/**
 * @brief Total number of interrupts supported.
 * 
 * Defines the number of interrupt vectors the system can handle. This includes both
 * hardware and software interrupts.
 */
#define TOYOS_TOTAL_INTERRUPTS      512   /**< Total number of interrupt vectors. */

/**
 * @brief Configuration for the kernel heap.
 * 
 * Defines the size of the heap and the size of individual blocks used within the heap.
 * The heap is a region of memory used for dynamic memory allocation.
 */
#define TOYOS_HEAP_SIZE_BYTES       104857600  /**< Total size of the heap in bytes (100 MB). */
#define TOYOS_HEAP_BLOCK_SIZE       4096       /**< Size of each block in the heap (4 KB). */

/**
 * @brief Addresses for the heap and heap table.
 * 
 * Specifies the starting address of the heap and the address for the heap block table.
 * These addresses are chosen based on system architecture and memory map considerations.
 * 
 * For more details on memory mapping, see: https://wiki.osdev.org/Memory_Map_(x86)
 */
#define TOYOS_HEAP_ADDRESS          0x01000000  /**< Start address of the heap (16 MB). */
#define TOYOS_HEAP_TABLE_ADDRESS    0x00007e00  /**< Address for the heap block table. */

/**
 * @brief Disk sector size.
 * 
 * Defines the size of a sector on the disk. This value is commonly used in disk
 * and file system operations.
 */
#define TOYOS_SECTOR_SIZE           512   /**< Size of a disk sector in bytes. */

/**
 * @brief Maximum configuration values for file systems and descriptors.
 * 
 * Defines the maximum number of supported file systems and file descriptors, as well
 * as the maximum allowed path length for file operations.
 */
#define TOYOS_MAX_FILESYSTEMS       12    /**< Maximum number of file systems supported. */
#define TOYOS_MAX_FILE_DESCRIPTORS  512   /**< Maximum number of file descriptors supported. */
#define TOYOS_MAX_PATH              108   /**< Maximum path length for files. */

#endif
