section .asm

extern int21h_handler
extern no_int_handler

global idt_load
global int21h
global no_int

idt_load:
    push ebp
    mov ebp, esp

    mov ebx, [ebp+8]
    lidt [ebx]
    pop ebp    
    ret

int21h:
    cli
    pushad
    call int21h_handler
    popad
    sti
    iret

no_int:
    cli
    pushad
    call no_int_handler
    popad
    sti
    iret
