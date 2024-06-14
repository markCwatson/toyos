ORG 0
BITS 16

jmp 0x7c0:start

start:
    cli
    mov ax, 0x7c0       ; setup data segment
    mov ds, ax
    mov es, ax
    mov ax, 0x00
    mov ss, ax
    mov sp, 0x7c00
    sti
    mov si, message
    call print
    jmp $

print:
    mov bx, 0
.loop:
    lodsb               ; si -> al
    cmp al, 0
    je .done
    call print_char
    jmp .loop
.done:
    ret

print_char:
    mov ah, 0eh         ; print to screen in bios
    int 0x10            ; invoke bios
    ret

message: db 'Hello world!', 0

times 510-($ - $$) db 0
dw 0xAA55