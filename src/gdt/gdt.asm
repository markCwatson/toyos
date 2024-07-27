section .asm
global gdt_load

; gdt_load:
; Loads the Global Descriptor Table (GDT) using the lgdt instruction.
; This function takes two arguments: the address of the GDT and its size.
; It sets up the GDT descriptor with these values and loads the GDT.
;
; Parameters:
;   - esp+4: Address of the GDT
;   - esp+8: Size of the GDT
gdt_load:
    ; Load the address of the GDT into the GDT descriptor
    mov eax, [esp+4]
    mov [gdt_descriptor + 2], eax
    
    ; Load the size of the GDT into the GDT descriptor
    mov ax, [esp+8]
    mov [gdt_descriptor], ax

    ; Load the GDT using the lgdt instruction
    lgdt [gdt_descriptor]
    
    ret

section .data
; gdt_descriptor:
; This data structure holds the size and address of the GDT.
; It is used by the lgdt instruction to set up the GDT.
gdt_descriptor:
    dw 0x00 ; Size of the GDT
    dd 0x00 ; Starting address of the GDT
