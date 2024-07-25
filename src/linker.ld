ENTRY(_start)                  # Define the entry point of the program. This tells the linker where the program's execution should begin.
OUTPUT_FORMAT(binary)          # Specify the output format as a raw binary. This means the output file will not contain any headers or metadata, just the raw machine code.

SECTIONS
{
    . = 1M;                    # Set the starting address of the output file in memory to 1 MB. This is a common convention in many operating systems to leave the first 1 MB for BIOS and real mode code.

    .text : ALIGN(4096)        # Define the .text section, which contains the executable code.
    {
        *(.text)               # Include all input sections named .text from the object files. ALIGN(4096) aligns the section to a 4096-byte boundary (4 KB), which is typical for memory pages.
    }

    .rodata : ALIGN(4096)      # Define the .rodata section for read-only data, such as string literals.
    {
        *(.rodata)             # Include all input sections named .rodata, ensuring they are aligned to a 4 KB boundary.
    }

    .data : ALIGN(4096)        # Define the .data section for initialized global and static variables.
    {
        *(.data)               # Include all input sections named .data, with 4 KB alignment for efficient memory access.
    }

    .bss : ALIGN(4096)         # Define the .bss section for uninitialized global and static variables. These are zero-initialized at runtime.
    {
        *(COMMON)              # Include all COMMON symbols (uninitialized data), typically grouped here for simplicity.
        *(.bss)                # Include all input sections named .bss, aligned to 4 KB.
    }

    .asm : ALIGN(4096)         # Define a custom .asm section for assembly code, if any is separate from .text.
    {
        *(.asm)                # Include all input sections named .asm, aligning to a 4 KB boundary.
    }
}
