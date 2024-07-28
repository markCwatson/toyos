[BITS 32]

global _start

_start: 
    push 20
    push 30         ; adding 20 + 30
    mov eax, 0      ; Set to envoke system command 0 (testing/sum)
    int 0x80        ; Call the kernel
    add esp, 4+2    ; Clean up the stack (+2 because of the two push instructions)
    jmp $
