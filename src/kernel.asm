[BITS 32]

extern kernel_main
global _start

CODE_SEG equ 0x08
DATA_SEG equ 0x10

_start:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov ebp, 0x00200000
    mov esp, ebp

    ; enable (fast) A20 line
    in al, 0x92
    or al, 2
    out 0x92, al

    call kernel_main
    jmp $

times 512-($ - $$) db 0