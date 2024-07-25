[BITS 32]

section .asm

global paging_load_directory   ; Make the paging_load_directory function accessible from other files.
global enable_paging           ; Make the enable_paging function accessible from other files.

; Function: paging_load_directory
; Description: Loads a page directory into the CR3 register, which is used for paging.
; Parameters: 
;   - The function expects a pointer to the page directory base address on the stack.
;   - The address is stored in the EAX register and then moved to the CR3 register.
paging_load_directory:
    push ebp                    ; Save the base pointer.
    mov ebp, esp                ; Set the base pointer to the current stack pointer.
    mov eax, [ebp+8]            ; Load the address of the page directory into EAX.
    mov cr3, eax                ; Load the page directory base address into CR3.
    pop ebp                     ; Restore the base pointer.
    ret                         ; Return from the function.

; Function: enable_paging
; Description: Enables paging by setting the paging bit in the CR0 register.
; This function sets the most significant bit (bit 31) of the CR0 register, which enables paging.
enable_paging:
    push ebp                    ; Save the base pointer.
    mov ebp, esp                ; Set the base pointer to the current stack pointer.
    mov eax, cr0                ; Load the current value of CR0 into EAX.
    or eax, 0x80000000          ; Set the paging bit (bit 31) in EAX.
    mov cr0, eax                ; Store the updated value back into CR0.
    pop ebp                     ; Restore the base pointer.
    ret                         ; Return from the function.
