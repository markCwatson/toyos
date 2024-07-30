[BITS 32]

global _start

_start: 
    push message
    mov eax, 1      ; Set to envoke system command 1 (print)
    int 0x80        ; Call the kernel
    add esp, 4
_loop:
    call getkey
    push eax
    mov eax, 3      ; Set to envoke system command 3 (putchar)
    int 0x80        ; Call the kernel
    add esp, 4      ; Clean up the stack
    jmp _loop

getkey:
    mov eax, 2      ; Set to envoke system command 2 (getkey)
    int 0x80
    cmp eax, 0x00
    je getkey
    ret

section .data
message: db '$ ', 0
