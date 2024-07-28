#include "paging.h"
#include "memory/heap/kheap.h"
#include "status.h"

void paging_load_directory(uint32_t* directory);

static uint32_t* current_directory = 0;

/**
 * @brief Creates a new 4GB paging chunk with specified flags.
 *
 * Allocates a page directory and associated page tables, setting each entry with
 * the given flags. This setup is necessary for creating a paging environment that
 * maps virtual memory to physical memory.
 *
 * @param flags The flags to apply to each page table entry.
 * @return A pointer to the newly created 4GB paging chunk, or NULL on failure.
 */
struct paging_4gb_chunk* paging_new_4gb(uint8_t flags) {
    uint32_t* directory = kzalloc(sizeof(uint32_t) * PAGING_TOTAL_ENTRIES_PER_TABLE);
    int offset = 0;

    for (int i = 0; i < PAGING_TOTAL_ENTRIES_PER_TABLE; i++) {
        uint32_t* entry = kzalloc(sizeof(uint32_t) * PAGING_TOTAL_ENTRIES_PER_TABLE);

        for (int j = 0; j < PAGING_TOTAL_ENTRIES_PER_TABLE; j++) {
            // The upper 20 bits are the address, and the lower bits are flags.
            entry[j] = (offset + (j * PAGING_PAGE_SIZE)) | flags;
        }

        offset += (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE);
        directory[i] = (uint32_t)entry | flags | PAGING_IS_WRITEABLE;
    }

    struct paging_4gb_chunk* chunk_4gb = kzalloc(sizeof(struct paging_4gb_chunk));
    chunk_4gb->directory_entry = directory;
    return chunk_4gb;
}

/**
 * @brief Switches the current page directory.
 *
 * @details This function switches the currently active page directory to the one provided.
 * It updates the CR3 register with the new directory base address, effectively changing
 * the virtual-to-physical address mapping.
 *
 * @param directory Pointer to the paging structure containing the new page directory.
 */
void paging_switch(struct paging_4gb_chunk* directory) {
    paging_load_directory(directory->directory_entry);
    current_directory = directory->directory_entry;
}

/**
 * @brief Frees a 4GB paging chunk.
 *
 * This function releases the memory allocated for a 4GB paging chunk, including
 * all its page tables and the page directory.
 *
 * @param chunk Pointer to the paging chunk to free.
 */
void paging_free_4gb(struct paging_4gb_chunk* chunk) {
    for (int i = 0; i < PAGING_TOTAL_ENTRIES_PER_TABLE; i++) {
        uint32_t entry = chunk->directory_entry[i];
        uint32_t* table = (uint32_t*)(entry & 0xfffff000);
        kfree(table);
    }

    kfree(chunk->directory_entry);
    kfree(chunk);
}

/**
 * @brief Retrieves the directory entry for a 4GB paging chunk.
 *
 * This function returns a pointer to the page directory associated with the
 * given 4GB paging chunk.
 *
 * @param chunk Pointer to the paging chunk.
 * @return Pointer to the page directory.
 */
uint32_t* paging_4gb_chunk_get_directory(struct paging_4gb_chunk* chunk) {
    return chunk->directory_entry;
}

/**
 * @brief Checks if an address is aligned to the page size.
 *
 * This function checks whether a given address is aligned to the system's page size,
 * which is typically 4 KB.
 *
 * @param addr The address to check.
 * @return True if the address is aligned, false otherwise.
 */
bool paging_is_aligned(void* addr) {
    return ((uint32_t)addr % PAGING_PAGE_SIZE) == 0;
}

/**
 * @brief Gets the directory and table indexes from a virtual address.
 *
 * This function calculates the indexes into the page directory and page table
 * for a given virtual address. These indexes are used to locate the corresponding
 * page table entry.
 *
 * @param virtual_addr The virtual address to translate.
 * @param directory_index Pointer to store the directory index.
 * @param table_index Pointer to store the table index.
 * @return 0 on success, or a negative error code on failure.
 */
int paging_get_indexes(void* virtual_addr, uint32_t* directory_index, uint32_t* table_index) {
    if (!paging_is_aligned(virtual_addr)) {
        return -EINVARG;
    }

    *directory_index = ((uint32_t)virtual_addr / (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE));
    *table_index = ((uint32_t)virtual_addr % (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE) / PAGING_PAGE_SIZE);

    return OK;
}

/**
 * @brief Aligns a pointer to the nearest page boundary.
 *
 * This function adjusts a pointer to ensure it is aligned to the start of the next
 * page boundary. If the pointer is already aligned, it is returned unchanged.
 *
 * @param ptr The pointer to align.
 * @return The aligned pointer.
 */
void* paging_align_address(void* ptr) {
    if ((uint32_t)ptr % PAGING_PAGE_SIZE) {
        return (void*)((uint32_t)ptr + PAGING_PAGE_SIZE - ((uint32_t)ptr % PAGING_PAGE_SIZE));
    }
    
    return ptr;
}

/**
 * @brief Aligns a pointer down to the nearest lower page boundary.
 *
 * This function takes a pointer and aligns it downwards to the start of the
 * nearest page boundary.
 *
 * @param addr The address to align.
 * @return The aligned address.
 */
void* paging_align_to_lower_page(void* addr) {
    uint32_t _addr = (uint32_t) addr;
    _addr -= (_addr % PAGING_PAGE_SIZE);
    return (void*) _addr;
}

/**
 * @brief Maps a virtual address to a physical address.
 *
 * This function maps a given virtual address to a specified physical address in the
 * paging directory, with the given flags. The addresses must be page-aligned.
 *
 * @param directory The paging directory to modify.
 * @param virt The virtual address to map.
 * @param phys The physical address to map to.
 * @param flags Page table entry flags.
 * @return 0 on success, or a negative error code on failure.
 */
int paging_map(struct paging_4gb_chunk* directory, void* virt, void* phys, int flags) {
    if (((unsigned int)virt % PAGING_PAGE_SIZE) || ((unsigned int) phys % PAGING_PAGE_SIZE)) {
        return -EINVARG;
    }

    return paging_set(directory->directory_entry, virt, (uint32_t) phys | flags);
}

/**
 * @brief Maps a range of virtual addresses to physical addresses.
 *
 * This function maps a sequence of virtual addresses to corresponding physical addresses
 * in the paging structure, for a specified number of pages.
 *
 * @param directory The paging directory to modify.
 * @param virt The starting virtual address.
 * @param phys The starting physical address.
 * @param count The number of pages to map.
 * @param flags Page table entry flags.
 * @return 0 on success, or a negative error code on failure.
 */
int paging_map_range(struct paging_4gb_chunk* directory, void* virt, void* phys, int count, int flags) {
    for (int i = 0; i < count; i++) {
        int res = paging_map(directory, virt, phys, flags);
        if (res < 0) {
            return res;
        }

        virt += PAGING_PAGE_SIZE;
        phys += PAGING_PAGE_SIZE;
    }

    return OK;
}

/**
 * @brief Maps a range of physical addresses to virtual addresses.
 *
 * This function maps a specified range of physical addresses to virtual addresses in the
 * paging structure. The addresses and the size of the range must be page-aligned.
 *
 * @param directory The paging directory to modify.
 * @param virt The starting virtual address.
 * @param phys The starting physical address.
 * @param phys_end The ending physical address.
 * @param flags Page table entry flags.
 * @return 0 on success, or a negative error code on failure.
 */
int paging_map_to(struct paging_4gb_chunk* directory, void* virt, void* phys, void* phys_end, int flags) {
    if ((uint32_t)virt % PAGING_PAGE_SIZE) {
        return -EINVARG;
    }

    if ((uint32_t)phys % PAGING_PAGE_SIZE) {
        return -EINVARG;
    }

    if ((uint32_t)phys_end % PAGING_PAGE_SIZE) {
        return -EINVARG;
    }

    if ((uint32_t)phys_end < (uint32_t)phys) {
        return -EINVARG;
    }

    uint32_t total_bytes = phys_end - phys;
    int total_pages = total_bytes / PAGING_PAGE_SIZE;
    int res = paging_map_range(directory, virt, phys, total_pages, flags);
    return res;
}

/**
 * @brief Sets a page table entry for a given virtual address.
 *
 * This function updates the page directory to map a virtual address to a physical address
 * with specified flags. It requires that the address is page-aligned.
 *
 * @param directory The page directory to update.
 * @param virt The virtual address to set.
 * @param val The value for the page table entry (physical address and flags).
 * @return 0 on success, or a negative error code on failure.
 */
int paging_set(uint32_t* directory, void* virt, uint32_t val) {
    if (!virt || !paging_is_aligned(virt) || !directory) {
        return -EINVARG;
    }

    uint32_t directory_index = 0;
    uint32_t table_index = 0;

    int res = paging_get_indexes(virt, &directory_index, &table_index);
    if (res < 0) {
        return res;
    }

    uint32_t entry = directory[directory_index];
    uint32_t* table = (uint32_t* )(entry & 0xfffff000);
    table[table_index] = val;

    return OK;
}

/**
 * @brief Retrieves the physical address corresponding to a virtual address.
 *
 * This function calculates the physical address that corresponds to a given virtual address
 * using the provided page directory.
 *
 * @param directory The paging directory to search.
 * @param virt The virtual address to translate.
 * @return The corresponding physical address.
 */
void* paging_get_physical_address(uint32_t* directory, void* virt) {
    if (!directory || !virt) {
        return NULL;
    }

    void* virt_addr_new = (void*)paging_align_to_lower_page(virt);
    void* difference = (void*)((uint32_t)virt - (uint32_t)virt_addr_new);
    return (void*)((paging_get(directory, virt_addr_new) & 0xfffff000) + difference);
}

/**
 * @brief Retrieves the page table entry for a given virtual address.
 *
 * This function returns the page table entry value for a given virtual address from the
 * specified page directory. This value includes the physical address and the flags set for the page.
 *
 * @param directory The page directory to search.
 * @param virt The virtual address to lookup.
 * @return The page table entry value.
 */
uint32_t paging_get(uint32_t* directory, void* virt) {
    if (!directory || !virt) {
        return -EINVARG;
    }

    uint32_t directory_index = 0;
    uint32_t table_index = 0;
    paging_get_indexes(virt, &directory_index, &table_index);
    
    uint32_t entry = directory[directory_index];
    uint32_t* table = (uint32_t*)(entry & 0xfffff000);
    return table[table_index];
}
