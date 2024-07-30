#ifndef _ELF_LOADER_H_
#define _ELF_LOADER_H_

#include <stdint.h>
#include <stddef.h>

#include "elf.h"
#include "config.h"

/**
 * @brief Represents an ELF file loaded into memory.
 *
 * This structure contains information about an ELF file that has been
 * loaded into memory, including its filename, memory locations, and
 * size. It is used by the OS to manage the ELF files of running
 * processes.
 */
struct elf_file {
    char filename[TOYOS_MAX_PATH]; //< The name of the ELF file.

    int in_memory_size; //< The size of the ELF file in memory.

    /**
     * @brief The physical memory address where this ELF file is loaded.
     *
     * This pointer refers to the location in physical memory where the
     * ELF file's contents are stored.
     */
    void* elf_memory;

    /**
     * @brief The virtual base address of this binary.
     *
     * This address represents the starting virtual memory address
     * where the ELF file expects to be loaded.
     */
    void* virtual_base_address;

    /**
     * @brief The ending virtual address.
     *
     * This address represents the end of the virtual memory range used
     * by the ELF file.
     */
    void* virtual_end_address;

    /**
     * @brief The physical base address of this binary.
     *
     * This address represents the starting physical memory address
     * where the ELF file is mapped.
     */
    void* physical_base_address;

    /**
     * @brief The physical end address of this binary.
     *
     * This address represents the end of the physical memory range used
     * by the ELF file.
     */
    void* physical_end_address;
};

/**
 * @brief Loads an ELF file from the filesystem.
 *
 * This function opens the specified ELF file, reads its contents into memory, and processes it.
 *
 * @param filename The name of the ELF file to load.
 * @param file_out Pointer to store the loaded ELF file structure.
 * @return 0 on success, error code otherwise.
 */
int elf_load(const char* filename, struct elf_file** file_out);

/**
 * @brief Frees the resources associated with an ELF file.
 *
 * This function deallocates the memory used by the ELF file structure and its contents.
 *
 * @param file Pointer to the ELF file structure to free.
 */
void elf_close(struct elf_file* file);

/**
 * @brief Retrieves the virtual base address of the ELF file.
 *
 * @param file The ELF file structure.
 * @return Pointer to the virtual base address.
 */
void* elf_virtual_base(struct elf_file* file);

/**
 * @brief Retrieves the physical address of a program header.
 * 
 * @param file The ELF file structure.
 * @param phdr The program header structure.
 * @return Pointer to the physical address of the program header.
 */
void* elf_phdr_phys_address(struct elf_file* file, struct elf32_phdr* phdr);

/**
 * @brief Retrieves the virtual end address of the ELF file.
 *
 * @param file The ELF file structure.
 * @return Pointer to the virtual end address.
 */
void* elf_virtual_end(struct elf_file* file);

/**
 * @brief Retrieves the physical base address of the ELF file.
 *
 * @param file The ELF file structure.
 * @return Pointer to the physical base address.
 */
void* elf_phys_base(struct elf_file* file);

/**
 * @brief Retrieves the physical end address of the ELF file.
 *
 * @param file The ELF file structure.
 * @return Pointer to the physical end address.
 */
void* elf_phys_end(struct elf_file* file);

/**
 * @brief Retrieves the ELF header from the ELF file.
 *
 * @param file The ELF file structure.
 * @return Pointer to the ELF header.
 */
struct elf_header* elf_header(struct elf_file* file);

/**
 * @brief Retrieves the program header table from the ELF header.
 *
 * @param header The ELF header structure.
 * @return Pointer to the program header table, or NULL if not present.
 */
struct elf32_phdr* elf_pheader(struct elf_header* header);

#endif
