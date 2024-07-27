[BITS 32]
section .asm

global restore_general_purpose_registers
global task_return
global user_registers

; void task_return(struct registers* regs);
; Restores the state of a task and returns to user mode
task_return:
    mov ebp, esp
    ; Access the registers structure
    mov ebx, [ebp+4]
    ; Push the data/stack segment selector
    push dword [ebx+44]
    ; Push the stack pointer (ESP)
    push dword [ebx+40]
    ; Push EFLAGS with interrupts enabled
    pushf
    pop eax
    or eax, 0x200
    push eax
    ; Push the code segment (CS)
    push dword [ebx+32]
    ; Push the instruction pointer (EIP)
    push dword [ebx+28]

    ; Set the segment registers
    mov ax, [ebx+44]
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Call to restore general-purpose registers
    push dword [ebp+4]
    call restore_general_purpose_registers
    add esp, 4

    ; Return from interrupt, transitioning to user mode
    iretd
    
; void restore_general_purpose_registers(struct registers* regs);
; Restores general-purpose registers from a saved state
restore_general_purpose_registers:
    push ebp
    mov ebp, esp
    mov ebx, [ebp+8]
    mov edi, [ebx]       ; Restore EDI
    mov esi, [ebx+4]     ; Restore ESI
    mov ebp, [ebx+8]     ; Restore EBP
    mov edx, [ebx+16]    ; Restore EDX
    mov ecx, [ebx+20]    ; Restore ECX
    mov eax, [ebx+24]    ; Restore EAX
    mov ebx, [ebx+12]    ; Restore EBX
    add esp, 4
    ret

; void user_registers()
; Sets segment registers to user mode segments
user_registers:
    mov ax, 0x23         ; User data segment selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    ret
