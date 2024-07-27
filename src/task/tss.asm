section .asm

global tss_load

; tss_load:
; This function loads the Task State Segment (TSS) using the `ltr` (Load Task Register) instruction.
; The TSS is used to switch the CPU's state, including registers and stack pointers, when changing tasks.
; The `ltr` instruction loads the TSS descriptor into the Task Register (TR), which then points to the TSS.
; The segment selector for the TSS is passed to this function in the AX register.
tss_load:
    push ebp          ; Save the base pointer
    mov ebp, esp      ; Set the base pointer to the current stack pointer
    mov ax, [ebp+8]   ; Load the TSS segment selector into AX
    ltr ax            ; Load the TSS segment selector into the Task Register (TR)
    pop ebp           ; Restore the base pointer
    ret               ; Return from the function
