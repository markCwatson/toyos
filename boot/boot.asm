ORG 0x7c00
BITS 16

start:
    mov si, message
    call print
    jmp $

print:
    mov bx, 0
.loop:
    lodsb  ; si -> al
    cmp al, 0
    je .done
    call print_char
    jmp .loop
.done:
    ret

print_char:
    mov ah, 0eh ; print to screen in bios
    int 0x10 ; invoke bios
    ret

message: db 'Hello world!', 0

times 510-($ - $$) db 0
dw 0xAA55