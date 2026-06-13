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
global toyos_check_done:function
global toyos_done:function
global toyos_fork:function
global toyos_kill:function
global toyos_socket:function
global toyos_bind:function
global toyos_sendto:function
global toyos_recvfrom:function

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

; void toyos_check_done(void)
toyos_check_done:
    push ebp
    mov ebp, esp
    mov eax, 12 ; Command 12 check lock
    int 0x80
    pop ebp
    ret

; void toyos_done(void)
toyos_done:
    push ebp
    mov ebp, esp
    mov eax, 13 ; Command 13 done
    int 0x80
    pop ebp
    ret

; int toyos_fork(void)
toyos_fork:
    push ebp
    mov ebp, esp
    mov eax, 14 ; Command 14 fork
    int 0x80
    pop ebp
    ret

; int toyos_kill(int pid)
; Kills the process with the given pid.
toyos_kill:
    push ebp
    mov ebp, esp
    mov eax, 15 ; Command 15 kill
    push dword[ebp+8] ; Variable "pid"
    int 0x80
    add esp, 4
    pop ebp
    ret

; int toyos_socket(int type)
; Creates a new socket. type=2 for UDP (SOCK_DGRAM).
; Returns socket descriptor (>= 0) or -1 on error.
toyos_socket:
    push ebp
    mov ebp, esp
    mov eax, 16 ; Command 16 socket
    push dword[ebp+8] ; Variable "type"
    int 0x80
    add esp, 4
    pop ebp
    ret

; int toyos_bind(int sockfd, int port)
; Binds a socket to a UDP port.
; Returns 0 on success, -1 on error.
toyos_bind:
    push ebp
    mov ebp, esp
    mov eax, 17 ; Command 17 bind
    push dword[ebp+12] ; Variable "port" (pushed first = stack item 1)
    push dword[ebp+8]  ; Variable "sockfd" (pushed second = stack item 0)
    int 0x80
    add esp, 8
    pop ebp
    ret

; int toyos_sendto(struct sendto_args *args)
; Sends a UDP packet. args is a pointer to a sendto_args struct.
; Returns bytes sent or -1 on error.
toyos_sendto:
    push ebp
    mov ebp, esp
    mov eax, 18 ; Command 18 sendto
    push dword[ebp+8] ; Variable "args" (pointer to struct)
    int 0x80
    add esp, 4
    pop ebp
    ret

; int toyos_recvfrom(struct recvfrom_args *args)
; Receives a UDP packet. args is a pointer to a recvfrom_args struct.
; Returns bytes received, 0 if nothing available, -1 on error.
toyos_recvfrom:
    push ebp
    mov ebp, esp
    mov eax, 19 ; Command 19 recvfrom
    push dword[ebp+8] ; Variable "args" (pointer to struct)
    int 0x80
    add esp, 4
    pop ebp
    ret