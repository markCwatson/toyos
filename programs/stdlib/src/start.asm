[BITS 32]

global _start
extern c_start
extern toyos_exit

section .asm

_start:
    call c_start
    call toyos_exit
    ret
