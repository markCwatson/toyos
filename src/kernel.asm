[BITS 32]                ; Assembler directive to generate 32-bit code.

extern kernel_main       ; Declare external function `kernel_main` defined elsewhere.
global _start            ; Define the global entry point `_start` for the linker.

CODE_SEG equ 0x08        ; Code segment selector for protected mode (points to code segment in GDT).
DATA_SEG equ 0x10        ; Data segment selector for protected mode (points to data segment in GDT).

_start:
    mov ax, DATA_SEG     ; Load the data segment selector into AX.
    mov ds, ax           ; Set the data segment (DS) to the data segment selector.
    mov es, ax           ; Set the extra segment (ES) to the data segment selector.
    mov fs, ax           ; Set FS segment to the data segment selector.
    mov gs, ax           ; Set GS segment to the data segment selector.
    mov ss, ax           ; Set the stack segment (SS) to the data segment selector.
    mov ebp, 0x00200000  ; Set the base pointer (EBP) to 2 MB. This is the base of the stack.
    mov esp, ebp         ; Set the stack pointer (ESP) to the base of the stack.

    ; Enable the A20 line to access memory above 1 MB.
    in al, 0x92          ; Read the current status of the A20 line from port 0x92.
    or al, 2             ; Set the second bit to enable the A20 line.
    out 0x92, al         ; Write back to port 0x92 to enable the A20 line.

    ; Remap the Programmable Interrupt Controller (PIC) to avoid conflicts with CPU exceptions.
    mov al, 00010001b    ; Start the initialization sequence for the master PIC.
    out 0x20, al         ; Send initialization command to master PIC command port (0x20).
    mov al, 0x20         ; Set the master PIC vector offset to 0x20 (ISR start vector).
    out 0x21, al         ; Send the vector offset to the master PIC data port (0x21).
    mov al, 00000001b    ; Finalize the initialization with 8086/88 mode.
    out 0x21, al         ; Send the command to the master PIC data port (0x21).

    ; Call the main function of the kernel, which is the entry point of the kernel's C code.
    call kernel_main     ; Transfer control to the C kernel code.

    jmp $                ; Infinite loop to prevent returning from the kernel main function.

times 512-($ - $$) db 0  ; Pad the remainder of the sector with zeros to make the file 512 bytes.
