[BITS 32]

global _start

_start:
    mov eax, 0  ; Set to envoke system command 0
    int 0x80    ; Call the kernel
    jmp $
