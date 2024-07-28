section .asm                    ; This section defines assembly code that will be assembled into machine code.

extern int21h_handler           ; External declaration for the handler function for interrupt 0x21 (INT 21h).
extern no_interrupt_handler     ; External declaration for a default or placeholder interrupt handler.
extern isr80h_handler           ; External declaration for the handler function for interrupt 0x80 (INT 80h).

global no_interrupt             ; This is a generic handler for unexpected or unhandled interrupts.
global int21h                   ; This is an interrupt service routine (ISR) for INT 21h.
global int80h                   ; This is a wrapper for the ISR for interrupt 0x80 (INT 80h).
global idt_load                 ; This function loads the Interrupt Descriptor Table (IDT).
global enable_interrupt         ; This function enables hardware interrupts.
global disable_interrupt        ; This function disables hardware interrupts.

enable_interrupt:
    sti                         ; Set Interrupt Flag (IF) to enable hardware interrupts.
    ret                         ; Return from the function.

disable_interrupt:
    cli                         ; Clear Interrupt Flag (IF) to disable hardware interrupts.
    ret                         ; Return from the function.

; The IDT descriptor is a structure that specifies the base address and limit of the IDT.
idt_load:
    push ebp                    ; Save the base pointer.
    mov ebp, esp                ; Set the base pointer to the current stack pointer.
    mov ebx, [ebp+8]            ; Load the pointer to the IDT descriptor from the stack.
    lidt [ebx]                  ; Load the IDT using the lidt instruction, which takes the address of the IDT descriptor.
    pop ebp                     ; Restore the base pointer.
    ret                         ; Return from the function.

; Interrupt service routines (ISRs) for specific interrupts.

no_interrupt:
    cli                         ; Disable interrupts to prevent nesting of ISRs.
    pushad                      ; Push all general-purpose registers onto the stack.
    call no_interrupt_handler   ; Call the external handler for unexpected or unhandled interrupts.
    popad                       ; Pop all general-purpose registers from the stack, restoring their values.
    sti                         ; Re-enable interrupts.
    iret                        ; Return from the interrupt, restoring the state saved by the CPU on interrupt entry.

int21h:
    cli                         ; Disable interrupts to prevent nesting of ISRs.
    pushad                      ; Push all general-purpose registers onto the stack.
    call int21h_handler         ; Call the external handler for interrupt 0x21.
    popad                       ; Pop all general-purpose registers from the stack, restoring their values.
    sti                         ; Re-enable interrupts.
    iret                        ; Return from the interrupt, restoring the state saved by the CPU on interrupt entry.

int80h:
    cli                         ; Disable interrupts to prevent nesting of ISRs.
    pushad                      ; Push all general-purpose registers onto the stack.
    push esp                    ; Push the stack pointer onto the stack to pass it as an argument to the handler.
    push eax                    ; Push the return value register onto the stack to pass it as an argument to the handler.
    call isr80h_handler         ; Call the external handler for interrupt 0x80.
    mov dword[tmp_res], eax     ; Store the return value from the handler in a temporary variable.
    add esp, 8                  ; Adjust the stack pointer to remove the arguments pushed earlier.
    popad                       ; Pop all general-purpose registers from the stack, restoring their values.
    mov eax, dword[tmp_res]     ; Move the return value from the temporary variable to the return value register.
    sti                         ; Re-enable interrupts.
    iretd                       ; Return from the interrupt, restoring the state saved by the CPU on interrupt entry.

section .data                   ; This section defines initialized data that will be stored in memory.

tmp_res dd 0                   ; Define a temporary variable to store the return value from the ISR.
