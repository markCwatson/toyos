section .asm              ; This section defines assembly code that will be assembled into machine code.

extern int21h_handler     ; External declaration for the handler function for interrupt 0x21 (INT 21h). This is expected to be defined in another file, likely in C or assembly.
extern no_int_handler     ; External declaration for a default or placeholder interrupt handler.

global idt_load           ; Declare `idt_load` as a global symbol so it can be called from other files. This function loads the Interrupt Descriptor Table (IDT).
global int21h             ; Declare `int21h` as a global symbol. This is an interrupt service routine (ISR) for INT 21h.
global no_int             ; Declare `no_int` as a global symbol. This is a generic handler for unexpected or unhandled interrupts.
global enable_int         ; Declare `enable_int` as a global symbol. This function enables hardware interrupts.
global disable_int        ; Declare `disable_int` as a global symbol. This function disables hardware interrupts.

enable_int:
    sti                   ; Set Interrupt Flag (IF) to enable hardware interrupts.
    ret                   ; Return from the function.

disable_int:
    cli                   ; Clear Interrupt Flag (IF) to disable hardware interrupts.
    ret                   ; Return from the function.

idt_load:
    push ebp              ; Save the base pointer.
    mov ebp, esp          ; Set the base pointer to the current stack pointer.

    mov ebx, [ebp+8]      ; Load the pointer to the IDT descriptor from the stack. The IDT descriptor is a structure that specifies the base address and limit of the IDT.
    lidt [ebx]            ; Load the IDT using the lidt instruction, which takes the address of the IDT descriptor.
    pop ebp               ; Restore the base pointer.
    ret                   ; Return from the function.

int21h:
    cli                   ; Disable interrupts to prevent nesting of ISRs.
    pushad                ; Push all general-purpose registers onto the stack.
    call int21h_handler   ; Call the external handler for interrupt 0x21.
    popad                 ; Pop all general-purpose registers from the stack, restoring their values.
    sti                   ; Re-enable interrupts.
    iret                  ; Return from the interrupt, restoring the state saved by the CPU on interrupt entry.

no_int:
    cli                   ; Disable interrupts to prevent nesting of ISRs.
    pushad                ; Push all general-purpose registers onto the stack.
    call no_int_handler   ; Call the external handler for unexpected or unhandled interrupts.
    popad                 ; Pop all general-purpose registers from the stack, restoring their values.
    sti                   ; Re-enable interrupts.
    iret                  ; Return from the interrupt, restoring the state saved by the CPU on interrupt entry.
