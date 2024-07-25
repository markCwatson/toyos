; This file, io.asm, is written in assembly language to provide low-level access 
; to hardware I/O ports. Assembly is used here because it allows direct execution 
; of CPU instructions like 'in' and 'out', which are necessary for interacting 
; with hardware components at the system level. These operations are critical 
; for tasks such as reading from and writing to ports, controlling hardware 
; devices, and managing system resources, which cannot be efficiently or 
; directly expressed in high-level languages like C.

section .asm

global insb               ; Make the insb function accessible from other files.
global insw               ; Make the insw function accessible from other files.
global outb               ; Make the outb function accessible from other files.
global outw               ; Make the outw function accessible from other files.

; Function: insb
; Description: Reads a byte from the specified I/O port.
; Parameters: port - The I/O port address.
; Returns: The byte read from the port (in the AL register).
insb:
    push ebp              ; Save the base pointer.
    mov ebp, esp          ; Set the base pointer to the current stack pointer.

    xor eax, eax          ; Clear EAX register (ensuring AL is zeroed).
    mov edx, [ebp+8]      ; Load the port number from the stack into EDX.
    in al, dx             ; Read a byte from the port into AL.

    pop ebp               ; Restore the base pointer.
    ret                   ; Return, with the result in AL.

; Function: insw
; Description: Reads a word (2 bytes) from the specified I/O port.
; Parameters: port - The I/O port address.
; Returns: The word read from the port (in the AX register).
insw:
    push ebp              ; Save the base pointer.
    mov ebp, esp          ; Set the base pointer to the current stack pointer.

    xor eax, eax          ; Clear EAX register (ensuring AX is zeroed).
    mov edx, [ebp+8]      ; Load the port number from the stack into EDX.
    in ax, dx             ; Read a word from the port into AX.

    pop ebp               ; Restore the base pointer.
    ret                   ; Return, with the result in AX.

; Function: outb
; Description: Writes a byte to the specified I/O port.
; Parameters: port - The I/O port address.
;             val  - The byte to write.
outb:
    push ebp              ; Save the base pointer.
    mov ebp, esp          ; Set the base pointer to the current stack pointer.

    mov eax, [ebp+12]     ; Load the value to output from the stack into EAX.
    mov edx, [ebp+8]      ; Load the port number from the stack into EDX.
    out dx, al            ; Write the byte in AL to the port.

    pop ebp               ; Restore the base pointer.
    ret                   ; Return.

; Function: outw
; Description: Writes a word (2 bytes) to the specified I/O port.
; Parameters: port - The I/O port address.
;             val  - The word to write.
outw:
    push ebp              ; Save the base pointer.
    mov ebp, esp          ; Set the base pointer to the current stack pointer.

    mov eax, [ebp+12]     ; Load the value to output from the stack into EAX.
    mov edx, [ebp+8]      ; Load the port number from the stack into EDX.
    out dx, ax            ; Write the word in AX to the port.

    pop ebp               ; Restore the base pointer.
    ret                   ; Return.
