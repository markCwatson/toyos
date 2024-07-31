[BITS 32]

global _start
extern main
extern toyos_exit

section .asm

_start:
    call main
    call toyos_exit
    ret
