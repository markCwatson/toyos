#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Bit flags for setting up page tables and entries, used for controlling page properties.
#define PAGING_CACHE_DISABLED  0b00010000
#define PAGING_WRITE_THROUGH   0b00001000
#define PAGING_ACCESS_FROM_ALL 0b00000100
#define PAGING_IS_WRITEABLE    0b00000010
#define PAGING_IS_PRESENT      0b00000001

// Constants for paging structures.
#define PAGING_TOTAL_ENTRIES_PER_TABLE  1024
#define PAGING_PAGE_SIZE                4096

/**
 * @brief Represents a 4GB paging chunk, including a page directory.
 * 
 * This structure is used to manage 4GB chunks of virtual memory.
 */
struct paging_4gb_chunk {
    uint32_t* directory_entry; /**< Pointer to the page directory entry array. */
};

/**
 * @brief Creates a new 4GB paging chunk.
 * 
 * Allocates and initializes a new paging structure with the specified flags for each entry.
 * 
 * @param flags Flags to set for each entry in the page tables.
 * @return A pointer to the new paging structure.
 */
struct paging_4gb_chunk* paging_new_4gb(uint8_t flags);

/**
 * @brief Switches to a different paging directory.
 * 
 * This function changes the current page directory to the one provided.
 * 
 * @param directory Pointer to the new page directory to switch to.
 */
void paging_switch(struct paging_4gb_chunk* directory);

/**
 * @brief Enables paging by setting the appropriate control register.
 */
void enable_paging(void);

/**
 * @brief Sets a specific entry in the page directory.
 * 
 * Maps a virtual address to a physical address in the page directory.
 * 
 * @param directory Pointer to the page directory.
 * @param virt The virtual address to map.
 * @param val The value to set in the page table entry (includes physical address and flags).
 * @return 0 on success, negative error code on failure.
 */
int paging_set(uint32_t* directory, void* virt, uint32_t val);

/**
 * @brief Checks if an address is aligned to the page size.
 * 
 * @param addr The address to check.
 * @return True if the address is page-aligned, false otherwise.
 */
bool paging_is_aligned(void* addr);

/**
 * @brief Retrieves the directory entry for a 4GB paging chunk.
 *
 * This function returns a pointer to the page directory associated with the
 * given 4GB paging chunk.
 *
 * @param chunk Pointer to the paging chunk.
 * @return Pointer to the page directory.
 */
uint32_t *paging_4gb_chunk_get_directory(struct paging_4gb_chunk *chunk);

/**
 * @brief Gets the directory and table index for a given virtual address.
 * 
 * @param virtual_addr The virtual address to get the indexes for.
 * @param directory_index Pointer to store the directory index.
 * @param table_index Pointer to store the table index.
 * @return 0 on success, negative error code on failure.
 */
int paging_get_indexes(void* virtual_addr, uint32_t* directory_index, uint32_t* table_index);

/**
 * @brief Aligns a pointer to the nearest page boundary above the current address.
 * 
 * @param ptr The pointer to align.
 * @return The aligned address.
 */
void* paging_align_address(void* ptr);

/**
 * @brief Aligns a pointer to the nearest lower page boundary.
 * 
 * @param addr The address to align.
 * @return The aligned address.
 */
void* paging_align_to_lower_page(void* addr);

/**
 * @brief Maps a virtual address to a physical address in the paging structure.
 * 
 * @param directory The paging directory to modify.
 * @param virt The virtual address to map.
 * @param phys The physical address to map to.
 * @param flags Page flags to set for this mapping.
 * @return 0 on success, negative error code on failure.
 */
int paging_map(struct paging_4gb_chunk* directory, void* virt, void* phys, int flags);

/**
 * @brief Maps a range of virtual addresses to a range of physical addresses.
 * 
 * @param directory The paging directory to modify.
 * @param virt The starting virtual address to map.
 * @param phys The starting physical address to map to.
 * @param count The number of pages to map.
 * @param flags Page flags to set for these mappings.
 * @return 0 on success, negative error code on failure.
 */
int paging_map_range(struct paging_4gb_chunk* directory, void* virt, void* phys, int count, int flags);

/**
 * @brief Maps a range of physical addresses to virtual addresses.
 * 
 * @param directory The paging directory to modify.
 * @param virt The starting virtual address.
 * @param phys The starting physical address.
 * @param phys_end The ending physical address.
 * @param flags Page flags to set for these mappings.
 * @return 0 on success, negative error code on failure.
 */
int paging_map_to(struct paging_4gb_chunk *directory, void *virt, void *phys, void *phys_end, int flags);

/**
 * @brief Retrieves the physical address corresponding to a virtual address.
 * 
 * @param directory The paging directory to search.
 * @param virt The virtual address.
 * @return The corresponding physical address.
 */
void* paging_get_physical_address(uint32_t* directory, void* virt);

/**
 * @brief Retrieves the value of a page directory entry for a given virtual address.
 * 
 * @param directory The paging directory to search.
 * @param virt The virtual address to lookup.
 * @return The value of the page directory entry.
 */
uint32_t paging_get(uint32_t* directory, void* virt);

#endif
