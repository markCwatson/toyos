ENTRY(_start)
OUTPUT_FORMAT(binary)

SECTIONS
{
    . = 0x400000;              # Set the starting address of the output file in memory to 4 MB for user programs. See TOYOS_PROGRAM_VIRTUAL_ADDRESS in config.h.

    .text : ALIGN(4096)
    {
        *(.text)
    }

    .asm : ALIGN(4096)
    {
        *(.asm)
    }

    .rodata : ALIGN(4096)
    {
        *(.rodata)
    }

    .data : ALIGN(4096)
    {
        *(.data)
    }

    .bss : ALIGN(4096)
    {
        *(COMMON)
        *(.bss)
    }
}
