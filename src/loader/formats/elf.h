#ifndef _ELF_H_
#define _ELF_H_

#include <stdint.h>
#include <stddef.h>

// ELF program header permission flags
#define PF_X            0x01            //< Executable
#define PF_W            0x02            //< Writable
#define PF_R            0x04            //< Readable

// ELF program header types
#define PT_NULL         0               //< Unused entry
#define PT_LOAD         1               //< Loadable segment
#define PT_DYNAMIC      2               //< Dynamic linking information
#define PT_INTERP       3               //< Interpreter information
#define PT_NOTE         4               //< Auxiliary information
#define PT_SHLIB        5               //< Reserved
#define PT_PHDR         6               //< Segment containing program header table itself

// ELF section header types
#define SHT_NULL        0               //< Inactive section
#define SHT_PROGBITS    1               //< Program data
#define SHT_SYMTAB      2               //< Symbol table
#define SHT_STRTAB      3               //< String table
#define SHT_RELA        4               //< Relocation entries with addends
#define SHT_HASH        5               //< Symbol hash table
#define SHT_DYNAMIC     6               //< Dynamic linking information
#define SHT_NOTE        7               //< Notes
#define SHT_NOBITS      8               //< Program space with no file data
#define SHT_REL         9               //< Relocation entries, no addends
#define SHT_SHLIB       10              //< Reserved
#define SHT_DYNSYM      11              //< Dynamic linker symbol table
#define SHT_LOPROC      12              //< Processor-specific
#define SHT_HIPROC      13              //< Processor-specific
#define SHT_LOUSER      14              //< Lower bound of OS-specific
#define SHT_HIUSER      15              //< Upper bound of OS-specific

// ELF file types
#define ET_NONE         0               //< No file type
#define ET_REL          1               //< Relocatable file
#define ET_EXEC         2               //< Executable file
#define ET_DYN          3               //< Shared object file
#define ET_CORE         4               //< Core file

// ELF identification indices
#define EI_NIDENT       16              //< Size of e_ident[] array
#define EI_CLASS        4               //< File class
#define EI_DATA         5               //< Data encoding

// ELF file classes
#define ELFCLASSNONE    0               //< Invalid class
#define ELFCLASS32      1               //< 32-bit objects
#define ELFCLASS64      2               //< 64-bit objects

// ELF data encodings
#define ELFDATANONE     0               //< Invalid data encoding
#define ELFDATA2LSB     1               //< Little-endian
#define ELFDATA2MSB     2               //< Big-endian

#define SHN_UNDEF       0               //< Undefined section

typedef uint16_t elf32_half;            //< 16-bit half
typedef uint32_t elf32_word;            //< 32-bit word
typedef int32_t elf32_sword;            //< 32-bit signed word
typedef uint32_t elf32_addr;            //< Address
typedef int32_t elf32_off;              //< Offset

/**
 * @brief ELF program header for 32-bit executables.
 *
 * This structure contains information about how to create a process image,
 * mapping from a file to the memory segments. It includes the segment type,
 * offset, virtual and physical addresses, and permissions.
 */
struct elf32_phdr {
    elf32_word p_type;                  //< Segment type
    elf32_off p_offset;                 //< Segment file offset
    elf32_addr p_vaddr;                 //< Segment virtual address
    elf32_addr p_paddr;                 //< Segment physical address
    elf32_word p_filesz;                //< Segment size in file
    elf32_word p_memsz;                 //< Segment size in memory
    elf32_word p_flags;                 //< Segment flags
    elf32_word p_align;                 //< Segment alignment
} __attribute__((packed));

/**
 * @brief ELF section header for 32-bit executables.
 *
 * This structure describes a section in an ELF file, including its name,
 * type, flags, addresses, and size.
 */
struct elf32_shdr {
    elf32_word sh_name;                 //< Section name (index into string table)
    elf32_word sh_type;                 //< Section type
    elf32_word sh_flags;                //< Section flags
    elf32_addr sh_addr;                 //< Section virtual address at execution
    elf32_off sh_offset;                //< Section file offset
    elf32_word sh_size;                 //< Section size in bytes
    elf32_word sh_link;                 //< Link to another section
    elf32_word sh_info;                 //< Additional section information
    elf32_word sh_addralign;            //< Section alignment
    elf32_word sh_entsize;              //< Entry size if section holds table
} __attribute__((packed));

/**
 * @brief ELF file header for 32-bit executables.
 *
 * This structure contains information about the ELF file, including its type,
 * architecture, entry point, and offsets to the program and section headers.
 */
struct elf_header {
    unsigned char e_ident[EI_NIDENT];   //< ELF identification
    elf32_half e_type;                  //< Object file type
    elf32_half e_machine;               //< Machine type
    elf32_word e_version;               //< Object file version
    elf32_addr e_entry;                 //< Entry point address
    elf32_off e_phoff;                  //< Program header offset
    elf32_off e_shoff;                  //< Section header offset
    elf32_word e_flags;                 //< Processor-specific flags
    elf32_half e_ehsize;                //< ELF header size
    elf32_half e_phentsize;             //< Size of program header entry
    elf32_half e_phnum;                 //< Number of program header entries
    elf32_half e_shentsize;             //< Size of section header entry
    elf32_half e_shnum;                 //< Number of section header entries
    elf32_half e_shstrndx;              //< Section name string table index
} __attribute__((packed));

/**
 * @brief ELF dynamic section entry for 32-bit executables.
 *
 * This structure defines entries in the dynamic section of an ELF file,
 * which provide information used by the dynamic linker to load and link
 * shared objects.
 */
struct elf32_dyn {
    elf32_sword d_tag; //< Dynamic entry type
    union {
        elf32_word d_val;               //< Integer value
        elf32_addr d_ptr;               //< Program virtual address
    } d_un;
} __attribute__((packed));

/**
 * @brief ELF symbol table entry for 32-bit executables.
 *
 * This structure defines entries in the symbol table, including the symbol's
 * name, value, size, and type.
 */
struct elf32_sym {
    elf32_word st_name;                 //< Symbol name (index into string table)
    elf32_addr st_value;                //< Symbol value
    elf32_word st_size;                 //< Symbol size
    unsigned char st_info;              //< Symbol type and binding
    unsigned char st_other;             //< Symbol visibility
    elf32_half st_shndx;                //< Section index
} __attribute__((packed));

/**
 * @brief Retrieves the entry point address from the ELF header.
 *
 * This function extracts the entry point address from the given ELF header.
 * The entry point is where execution will begin when the ELF file is run.
 *
 * @param elf_header A pointer to the ELF header structure.
 * @return A void pointer to the entry point address.
 */
void* elf_get_entry_ptr(struct elf_header* elf_header);

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
uint32_t elf_get_entry(struct elf_header* elf_header);

#endif
