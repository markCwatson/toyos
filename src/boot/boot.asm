ORG 0x7c00
BITS 16

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

; bios parameter block
_start:
    jmp short start
    nop

times 33 db 0

; start of bootloader
start:
    jmp 0:step2

step2:
    cli
    mov ax, 0x00            ; setup data segment
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00
    sti

.load_Protected:
    cli
    lgdt[gdt_descriptor]    ; loads gdt
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax
    jmp CODE_SEG:load32

; Global Descriptor Table (specific to the IA-32 and x86-64 architectures)
; contains entries telling the CPU about memory segments (default config used here)
gdt_start:
gdt_null:
    dd 0x0
    dd 0x0

gdt_code:                   ; cs should point to this
    dw 0xffff
    dw 0
    db 0
    db 0x9a
    db 11001111b
    db 0

gdt_data:                   ; ds, ss, es, fs, gs should point to this
    dw 0xffff
    dw 0
    db 0
    db 0x92
    db 11001111b
    db 0

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

[BITS 32]
load32:
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

    jmp $

times 510-($ - $$) db 0
dw 0xAA55
