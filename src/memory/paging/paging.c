#include "paging.h"
#include "memory/heap/kheap.h"
#include "status.h"

void paging_load_directory(uint32_t *directory);

static uint32_t *current_directory = 0;

struct paging_4gb_chunk *paging_new_4gb(uint8_t flags) {
    uint32_t *directory = kzalloc(sizeof(uint32_t) * PAGING_TOTAL_ENTRIES_PER_TABLE);
    int offset = 0;

    for (int i = 0; i < PAGING_TOTAL_ENTRIES_PER_TABLE; i++) {
        uint32_t *entry = kzalloc(sizeof(uint32_t) * PAGING_TOTAL_ENTRIES_PER_TABLE);

        for (int j = 0; j < PAGING_TOTAL_ENTRIES_PER_TABLE; j++) {
            // note: the upper 20 bits are the address, and the lower bits are flags.
            entry[j] = (offset + (j * PAGING_PAGE_SIZE)) | flags;
        }

        offset += (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE);
        directory[i] = (uint32_t)entry | flags | PAGING_IS_WRITEABLE;
    }

    struct paging_4gb_chunk *chunk_4gb = kzalloc(sizeof(struct paging_4gb_chunk));
    chunk_4gb->directory_entry = directory;
    return chunk_4gb;
}

void paging_switch(struct paging_4gb_chunk *directory) {
    paging_load_directory(directory->directory_entry);
    current_directory = directory->directory_entry;
}

void paging_free_4gb(struct paging_4gb_chunk *chunk) {
    for (int i = 0; i < PAGING_TOTAL_ENTRIES_PER_TABLE; i++) {
        uint32_t entry = chunk->directory_entry[i];
        uint32_t *table = (uint32_t *)(entry & 0xfffff000);
        kfree(table);
    }

    kfree(chunk->directory_entry);
    kfree(chunk);
}

uint32_t *paging_4gb_chunk_get_directory(struct paging_4gb_chunk *chunk) {
    return chunk->directory_entry;
}

bool paging_is_aligned(void *addr) {
    return ((uint32_t)addr % PAGING_PAGE_SIZE) == 0;
}

int paging_get_indexes(void *virtual_addr, uint32_t *directory_index, uint32_t *table_index) {
    if (!paging_is_aligned(virtual_addr)) {
        return -EINVARG;
    }

    *directory_index = ((uint32_t)virtual_addr / (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE));
    *table_index = ((uint32_t)virtual_addr % (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE) / PAGING_PAGE_SIZE);

    return OK;
}

void *paging_align_address(void *ptr) {
    if ((uint32_t)ptr % PAGING_PAGE_SIZE) {
        return (void *)((uint32_t)ptr + PAGING_PAGE_SIZE - ((uint32_t)ptr % PAGING_PAGE_SIZE));
    }

    return ptr;
}

void *paging_align_to_lower_page(void *addr) {
    uint32_t _addr = (uint32_t)addr;
    _addr -= (_addr % PAGING_PAGE_SIZE);
    return (void *)_addr;
}

int paging_map(struct paging_4gb_chunk *directory, void *virt, void *phys, int flags) {
    if (((unsigned int)virt % PAGING_PAGE_SIZE) || ((unsigned int)phys % PAGING_PAGE_SIZE)) {
        return -EINVARG;
    }

    return paging_set(directory->directory_entry, virt, (uint32_t)phys | flags);
}

int paging_map_range(struct paging_4gb_chunk *directory, void *virt, void *phys, int count, int flags) {
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

int paging_map_to(struct paging_4gb_chunk *directory, void *virt, void *phys, void *phys_end, int flags) {
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

int paging_set(uint32_t *directory, void *virt, uint32_t val) {
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
    uint32_t *table = (uint32_t *)(entry & 0xfffff000);
    table[table_index] = val;

    return OK;
}

void *paging_get_physical_address(uint32_t *directory, void *virt) {
    if (!directory || !virt) {
        return NULL;
    }

    void *virt_addr_new = (void *)paging_align_to_lower_page(virt);
    void *difference = (void *)((uint32_t)virt - (uint32_t)virt_addr_new);
    return (void *)((paging_get(directory, virt_addr_new) & 0xfffff000) + difference);
}

uint32_t paging_get(uint32_t *directory, void *virt) {
    if (!directory || !virt) {
        return -EINVARG;
    }

    uint32_t directory_index = 0;
    uint32_t table_index = 0;
    paging_get_indexes(virt, &directory_index, &table_index);

    uint32_t entry = directory[directory_index];
    uint32_t *table = (uint32_t *)(entry & 0xfffff000);
    return table[table_index];
}
