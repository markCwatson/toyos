[BITS 32]               ; Assembler directive to generate 32-bit code.

extern maink            ; Declare external function `maink` defined elsewhere.

global _start           ; Define the global entry point `_start` for the linker.
global kernel_registers ; Define the global function `kernel_registers` for setting up data segment registers.

CODE_SEG equ 0x08       ; Code segment selector for protected mode (points to code segment in GDT).
DATA_SEG equ 0x10       ; Data segment selector for protected mode (points to data segment in GDT).

_start:
    mov ax, DATA_SEG    ; Load the data segment selector into AX.
    mov ds, ax          ; Set the data segment (DS) to the data segment selector.
    mov es, ax          ; Set the extra segment (ES) to the data segment selector.
    mov fs, ax          ; Set FS segment to the data segment selector.
    mov gs, ax          ; Set GS segment to the data segment selector.
    mov ss, ax          ; Set the stack segment (SS) to the data segment selector.
    mov ebp, 0x00200000 ; Set the base pointer (EBP) to 2 MB. This is the base of the stack.
    mov esp, ebp        ; Set the stack pointer (ESP) to the base of the stack.

    ; Enable the A20 line to access memory above 1 MB.
    in al, 0x92         ; Read the current status of the A20 line from port 0x92.
    or al, 2            ; Set the second bit to enable the A20 line.
    out 0x92, al        ; Write back to port 0x92 to enable the A20 line.

    ; Call the main function of the kernel, which is the entry point of the kernel's C code.
    call maink          ; Transfer control to the C kernel code.
    jmp $               ; Infinite loop to prevent returning from the kernel main function.

; Function to set up data segment registers in protected mode.
kernel_registers:
    mov ax, 0x10        ; Load the data segment selector into AX.
    mov ds, ax          ; Set the data segment (DS) to the data segment selector.
    mov es, ax          ; Set the extra segment (ES) to the data segment selector.
    mov fs, ax          ; Set FS segment to the data segment selector.
    mov gs, ax          ; Set GS segment to the data segment selector.
    ret

times 512-($ - $$) db 0 ; Pad the remainder of the sector with zeros to make the file 512 bytes.
