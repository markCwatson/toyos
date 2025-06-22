#include "elfloader.h"
#include "config.h"
#include "fs/file.h"
#include "kernel.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"
#include "memory/paging/paging.h"
#include "status.h"
#include "stdlib/string.h"
#include <stdbool.h>

const char elf_signature[] = {0x7f, 'E', 'L', 'F'};

/**
 * @brief Checks if the ELF signature is valid.
 *
 * This function compares the buffer against the known ELF signature to verify
 * the file's format.
 *
 * @param buffer The buffer containing the file data.
 * @return true if the signature is valid, false otherwise.
 */
static bool elf_valid_signature(void *buffer) {
    return memcmp(buffer, (void *)elf_signature, sizeof(elf_signature)) == 0;
}

/**
 * @brief Checks if the ELF class is supported.
 *
 * This function checks whether the ELF file is 32-bit, which is the only supported
 * class.
 *
 * @param header The ELF header structure.
 * @return true if the class is supported, false otherwise.
 */
static bool elf_valid_class(struct elf_header *header) {
    // We only support 32-bit binaries.
    return header->e_ident[EI_CLASS] == ELFCLASSNONE || header->e_ident[EI_CLASS] == ELFCLASS32;
}

/**
 * @brief Checks if the ELF data encoding is supported.
 *
 * This function ensures that the ELF file uses a supported data encoding.
 *
 * @param header The ELF header structure.
 * @return true if the encoding is supported, false otherwise.
 */
static bool elf_valid_encoding(struct elf_header *header) {
    return header->e_ident[EI_DATA] == ELFDATANONE || header->e_ident[EI_DATA] == ELFDATA2LSB;
}

/**
 * @brief Checks if the ELF file is executable.
 *
 * This function checks if the ELF file is of type executable and has a valid entry point.
 *
 * @param header The ELF header structure.
 * @return true if the file is executable, false otherwise.
 */
static bool elf_is_executable(struct elf_header *header) {
    return header->e_type == ET_EXEC && header->e_entry >= TOYOS_PROGRAM_VIRTUAL_ADDRESS;
}

/**
 * @brief Checks if the ELF file has a program header.
 *
 * This function verifies the presence of a program header table in the ELF file.
 *
 * @param header The ELF header structure.
 * @return true if the file has a program header, false otherwise.
 */
static bool elf_has_program_header(struct elf_header *header) {
    return header->e_phoff != 0;
}

/**
 * @brief Retrieves the memory location of the ELF file.
 *
 * @param file The ELF file structure.
 * @return Pointer to the ELF memory location.
 */
void *elf_memory(struct elf_file *file) {
    return file->elf_memory;
}

/**
 * @brief Retrieves the ELF header from the ELF file.
 *
 * @param file The ELF file structure.
 * @return Pointer to the ELF header.
 */
struct elf_header *elf_header(struct elf_file *file) {
    return file->elf_memory;
}

/**
 * @brief Retrieves the section header table from the ELF header.
 *
 * @param header The ELF header structure.
 * @return Pointer to the section header table.
 */
struct elf32_shdr *elf_sheader(struct elf_header *header) {
    return (struct elf32_shdr *)((int)header + header->e_shoff);
}

/**
 * @brief Retrieves the program header table from the ELF header.
 *
 * @param header The ELF header structure.
 * @return Pointer to the program header table, or NULL if not present.
 */
struct elf32_phdr *elf_pheader(struct elf_header *header) {
    if (header->e_phoff == 0) {
        return NULL;
    }

    return (struct elf32_phdr *)((int)header + header->e_phoff);
}

/**
 * @brief Retrieves a specific program header from the ELF header.
 *
 * @param header The ELF header structure.
 * @param index The index of the program header to retrieve.
 * @return Pointer to the program header at the specified index.
 */
struct elf32_phdr *elf_program_header(struct elf_header *header, int index) {
    return &elf_pheader(header)[index];
}

/**
 * @brief Retrieves a specific section header from the ELF header.
 *
 * @param header The ELF header structure.
 * @param index The index of the section header to retrieve.
 * @return Pointer to the section header at the specified index.
 */
struct elf32_shdr *elf_section(struct elf_header *header, int index) {
    return &elf_sheader(header)[index];
}

/**
 * @brief Retrieves the string table from the ELF header.
 *
 * @param header The ELF header structure.
 * @return Pointer to the string table.
 */
char *elf_str_table(struct elf_header *header) {
    struct elf32_shdr *shdr = elf_section(header, header->e_shstrndx);
    return (char *)header + shdr->sh_offset;
}

/**
 * @brief Retrieves the virtual base address of the ELF file.
 *
 * @param file The ELF file structure.
 * @return Pointer to the virtual base address.
 */
void *elf_virtual_base(struct elf_file *file) {
    return file->virtual_base_address;
}

/**
 * @brief Retrieves the virtual end address of the ELF file.
 *
 * @param file The ELF file structure.
 * @return Pointer to the virtual end address.
 */
void *elf_virtual_end(struct elf_file *file) {
    return file->virtual_end_address;
}

/**
 * @brief Retrieves the physical base address of the ELF file.
 *
 * @param file The ELF file structure.
 * @return Pointer to the physical base address.
 */
void *elf_phys_base(struct elf_file *file) {
    return file->physical_base_address;
}

/**
 * @brief Retrieves the physical end address of the ELF file.
 *
 * @param file The ELF file structure.
 * @return Pointer to the physical end address.
 */
void *elf_phys_end(struct elf_file *file) {
    return file->physical_end_address;
}

/**
 * @brief Retrieves the physical address of a program header.
 *
 * @param file The ELF file structure.
 * @param phdr The program header structure.
 * @return Pointer to the physical address of the program header.
 */
void *elf_phdr_phys_address(struct elf_file *file, struct elf32_phdr *phdr) {
    return elf_memory(file) + phdr->p_offset;
}

/**
 * @brief Validates that the ELF file is properly loaded.
 *
 * This function checks the ELF file for a valid signature, class, encoding, and presence
 * of a program header.
 *
 * @param header The ELF header structure.
 * @return 0 if valid, otherwise negative.
 */
static int elf_validate_loaded(struct elf_header *header) {
    return (elf_valid_signature(header) && elf_valid_class(header) && elf_valid_encoding(header) &&
            elf_has_program_header(header))
               ? OK
               : -EINFORMAT;
}

/**
 * @brief Processes the PT_LOAD segment in the program header.
 *
 * This function updates the virtual and physical base and end addresses of the ELF file
 * based on the PT_LOAD segment's virtual address and size.
 *
 * @param elf_file Pointer to the ELF file structure.
 * @param phdr Pointer to the program header.
 * @return 0 on success, error code otherwise.
 */
static int elf_process_phdr_pt_load(struct elf_file *elf_file, struct elf32_phdr *phdr) {
    if (elf_file->virtual_base_address >= (void *)phdr->p_vaddr || elf_file->virtual_base_address == 0x00) {
        elf_file->virtual_base_address = (void *)phdr->p_vaddr;
        elf_file->physical_base_address = elf_memory(elf_file) + phdr->p_offset;
    }

    unsigned int end_virtual_address = phdr->p_vaddr + phdr->p_filesz;
    if (elf_file->virtual_end_address <= (void *)(end_virtual_address) || elf_file->virtual_end_address == 0x00) {
        elf_file->virtual_end_address = (void *)end_virtual_address;
        elf_file->physical_end_address = elf_memory(elf_file) + phdr->p_offset + phdr->p_filesz;
    }

    return OK;
}

/**
 * @brief Processes a single program header entry.
 *
 * This function processes the specified program header entry, handling different types
 * of segments such as PT_LOAD.
 *
 * @param elf_file Pointer to the ELF file structure.
 * @param phdr Pointer to the program header entry.
 * @return 0 on success, error code otherwise.
 */
static int elf_process_pheader(struct elf_file *elf_file, struct elf32_phdr *phdr) {
    int res = 0;
    switch (phdr->p_type) {
    case PT_LOAD:
        res = elf_process_phdr_pt_load(elf_file, phdr);
        break;
    }
    return res;
}

/**
 * @brief Processes all program headers in the ELF file.
 *
 * This function iterates over all program header entries in the ELF file and processes each one.
 *
 * @param elf_file Pointer to the ELF file structure.
 * @return 0 on success, error code otherwise.
 */
static int elf_process_pheaders(struct elf_file *elf_file) {
    int res = 0;
    struct elf_header *header = elf_header(elf_file);
    for (int i = 0; i < header->e_phnum; i++) {
        struct elf32_phdr *phdr = elf_program_header(header, i);
        res = elf_process_pheader(elf_file, phdr);
        if (res < 0) {
            break;
        }
    }
    return res;
}

/**
 * @brief Validates and processes the loaded ELF file.
 *
 * This function checks the validity of the ELF file and processes its program headers.
 *
 * @param elf_file Pointer to the ELF file structure.
 * @return 0 on success, error code otherwise.
 */
int elf_process_loaded(struct elf_file *elf_file) {
    int res = 0;
    struct elf_header *header = elf_header(elf_file);
    res = elf_validate_loaded(header);
    if (res < 0) {
        goto out;
    }

    res = elf_process_pheaders(elf_file);
    if (res < 0) {
        goto out;
    }

out:
    return res;
}

/**
 * @brief Loads an ELF file from the filesystem.
 *
 * This function opens the specified ELF file, reads its contents into memory, and processes it.
 *
 * @param filename The name of the ELF file to load.
 * @param file_out Pointer to store the loaded ELF file structure.
 * @return 0 on success, error code otherwise.
 */
int elf_load(const char *filename, struct elf_file **file_out) {
    struct elf_file *elf_file = kzalloc(sizeof(struct elf_file));
    int fd = 0;
    int res = fopen(filename, "r");
    if (res <= 0) {
        res = -EIO;
        goto out;
    }

    fd = res;
    struct file_stat stat;
    res = fstat(fd, &stat);
    if (res < 0) {
        goto out;
    }

    elf_file->elf_memory = kzalloc(stat.filesize);
    res = fread(elf_file->elf_memory, stat.filesize, 1, fd);
    if (res < 0) {
        goto out;
    }

    res = elf_process_loaded(elf_file);
    if (res < 0) {
        goto out;
    }

    *file_out = elf_file;

out:
    fclose(fd);
    return res;
}

/**
 * @brief Frees the resources associated with an ELF file.
 *
 * This function deallocates the memory used by the ELF file structure and its contents.
 *
 * @param file Pointer to the ELF file structure to free.
 */
void elf_close(struct elf_file *file) {
    if (!file) {
        return;
    }

    kfree(file->elf_memory);
    kfree(file);
}
