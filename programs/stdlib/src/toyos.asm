[BITS 32]

section .asm

global print:function
global toyos_getkey:function
global toyos_malloc:function
global toyos_free:function
global toyos_putchar:function
global toyos_process_load_start:function
global toyos_exit:function
global toyos_process_get_arguments:function
global toyos_system:function
global toyos_clear_terminal:function
global toyos_get_processes:function

; void print(const char* filename)
print:
    push ebp
    mov ebp, esp
    push dword[ebp+8]
    mov eax, 1 ; Command print
    int 0x80
    add esp, 4 ; undo push
    pop ebp
    ret

; int toyos_getkey()
toyos_getkey:
    push ebp
    mov ebp, esp
    mov eax, 2 ; Command getkey
    int 0x80
    pop ebp
    ret

; void* toyos_get_processes(void)
toyos_get_processes:
    push ebp
    mov ebp, esp
    mov eax, 11 ; Command 11 display process list
    int 0x80
    pop ebp
    ret
    
; void* toyos_malloc(size_t size)
toyos_malloc:
    push ebp
    mov ebp, esp
    mov eax, 4 ; Command malloc (Allocates memory for the process)
    push dword[ebp+8] ; Variable "size"
    int 0x80
    add esp, 4
    pop ebp
    ret

; void toyos_free(void* ptr)
toyos_free:
    push ebp
    mov ebp, esp
    mov eax, 5 ; Command 5 free (Frees the allocated memory for this process)
    push dword[ebp+8] ; Variable "ptr"
    int 0x80
    add esp, 4
    pop ebp
    ret

; void toyos_putchar(char c)
toyos_putchar:
    push ebp
    mov ebp, esp
    mov eax, 3 ; Command putchar
    push dword [ebp+8] ; Variable "c"
    int 0x80
    add esp, 4
    pop ebp
    ret

; void toyos_process_load_start(const char* filename)
toyos_process_load_start:
    push ebp
    mov ebp, esp
    mov eax, 6 ; Command 6 process load start ( stars a process )
    push dword[ebp+8] ; Variable "filename"
    int 0x80
    add esp, 4
    pop ebp
    ret

; void toyos_exit(void)
toyos_exit:
    push ebp
    mov ebp, esp
    mov eax, 7 ; Command 7 process exit
    int 0x80
    pop ebp
    ret

; void toyos_process_get_arguments(struct process_arguments* arguments)
toyos_process_get_arguments:
    push ebp
    mov ebp, esp
    mov eax, 8 ; Command 8 Gets the process arguments
    push dword[ebp+8] ; Variable arguments
    int 0x80
    add esp, 4
    pop ebp
    ret

; int toyos_system(struct command_argument* arguments)
toyos_system:
    push ebp
    mov ebp, esp
    mov eax, 9 ; Command 9 process_system ( runs a system command based on the arguments)
    push dword[ebp+8] ; Variable "arguments"
    int 0x80
    add esp, 4
    pop ebp
    ret

; void toyos_clear_terminal(void)
toyos_clear_terminal:
    push ebp
    mov ebp, esp
    mov eax, 10 ; Command 10 clear terminal
    int 0x80
    pop ebp
    ret
