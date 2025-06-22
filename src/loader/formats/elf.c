#include "elf.h"

/**
 * @brief Retrieves the entry point address from the ELF header.
 *
 * This function extracts the entry point address from the given ELF header.
 * The entry point is where execution will begin when the ELF file is run.
 *
 * @param elf_header A pointer to the ELF header structure.
 * @return A void pointer to the entry point address.
 */
void *elf_get_entry_ptr(struct elf_header *elf_header) {
    return (void *)elf_header->e_entry;
}

/**
 * @brief Retrieves the entry point address as a uint32_t from the ELF header.
 *
 * This function returns the entry point address from the ELF header
 * as a 32-bit unsigned integer. The entry point is the starting address
 * where the program will begin execution.
 *
 * @param elf_header A pointer to the ELF header structure.
 * @return The entry point address as a 32-bit unsigned integer.
 */
uint32_t elf_get_entry(struct elf_header *elf_header) {
    return elf_header->e_entry;
}
