#ifndef CONFIG_H
#define CONFIG_H

#define KERNEL_CODE_SELECTOR        0x08
#define KERNEL_DATA_SELECTOR        0x10

#define TOYOS_TOTAL_INTERRUPTS      512

#define TOYOS_HEAP_SIZE_BYTES       104857600
#define TOYOS_HEAP_BLOCK_SIZE       4096

// see https://wiki.osdev.org/Memory_Map_(x86) for free RAM
#define TOYOS_HEAP_ADDRESS          0x01000000
#define TOYOS_HEAP_TABLE_ADDRESS    0x00007e00

#define TOYOS_SECTOR_SIZE           512

#define TOYOS_MAX_FILESYSTEMS       12
#define TOYOS_MAX_FILE_DESCRIPTORS  512

#endif