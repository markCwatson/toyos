/*
 * Network system call handlers for ToyOS
 *
 * These bridge user-space network calls to the kernel socket layer.
 *
 * How a syscall works in ToyOS (step by step):
 *
 *   USER SPACE (ring 3):
 *     push 2           ; arg: SOCK_DGRAM
 *     mov eax, 16      ; syscall number: SYSTEM_COMMAND16_SOCKET
 *     int 0x80         ; software interrupt — CPU switches to ring 0
 *
 *   KERNEL SPACE (ring 0):
 *     int80h handler (idt.asm) saves registers, calls sys_handler()
 *       → sys_handler() saves task state, calls sys_handle_command(16, frame)
 *       → sys_handle_command() looks up sys_commands[16]
 *       → calls sys_command16_socket(frame)
 *       → reads arg from user stack: task_get_stack_item(task, 0) → 2
 *       → calls socket_create(2)
 *       → returns socket descriptor (e.g., 0) as void*
 *     int80h returns to user space with result in EAX
 *
 *   USER SPACE:
 *     ; EAX now contains 0 (the socket descriptor)
 */

#include "sys/net/sys_net.h"
#include "kernel.h"
#include "status.h"
#include "stdlib/printf.h"
#include "sys/net/socket.h"
#include "task/task.h"

/*
 * sys_command16_socket — create a socket
 *
 * User calls: int sockfd = toyos_socket(SOCK_DGRAM);
 * Stack:      [type]
 * Returns:    socket descriptor (>= 0) or error (< 0)
 */
void *sys_command16_socket(struct interrupt_frame *frame) {
    int type = (int)task_get_stack_item(task_current(), 0);
    int sockfd = socket_create(type);
    return (void *)(intptr_t)sockfd;
}

/*
 * sys_command17_bind — bind a socket to a port
 *
 * User calls: toyos_bind(sockfd, port);
 * Stack:      [port, sockfd]  (pushed right-to-left)
 * Returns:    0 on success, -1 on error
 */
void *sys_command17_bind(struct interrupt_frame *frame) {
    int sockfd = (int)task_get_stack_item(task_current(), 0);
    uint16_t port = (uint16_t)(int)task_get_stack_item(task_current(), 1);
    int res = socket_bind(sockfd, port);
    return (void *)(intptr_t)res;
}

/*
 * sys_command18_sendto — send a UDP packet
 *
 * User calls: toyos_sendto(&args);
 * Stack:      [pointer to sendto_args struct]
 *
 * We use a struct because sendto has 5 parameters — too many to push
 * individually. The user packs them into a sendto_args struct and
 * pushes a single pointer.
 *
 * Returns: bytes sent or -1 on error
 */
void *sys_command18_sendto(struct interrupt_frame *frame) {
    /*
     * Get the pointer to the args struct from the user stack.
     * This is a virtual address in the user's address space.
     * task_virtual_address_to_physical() walks the page tables
     * to find the actual physical address the kernel can read.
     */
    void *user_ptr = task_get_stack_item(task_current(), 0);
    struct sendto_args *args = task_virtual_address_to_physical(task_current(), user_ptr);
    if (!args) {
        return (void *)(intptr_t)-1;
    }

    /*
     * The buf pointer inside args is ALSO a user virtual address.
     * Convert it too so we can read the actual data.
     */
    void *buf_phys = task_virtual_address_to_physical(task_current(), args->buf);
    if (!buf_phys) {
        return (void *)(intptr_t)-1;
    }

    int res = socket_sendto(args->sockfd, buf_phys, args->len, args->dst_ip, args->dst_port);
    return (void *)(intptr_t)res;
}

/*
 * sys_command19_recvfrom — receive a UDP packet
 *
 * User calls: int n = toyos_recvfrom(&args);
 * Stack:      [pointer to recvfrom_args struct]
 *
 * The kernel fills args->buf with data, and args->src_ip / args->src_port
 * with the sender's address. The user can read these after the call returns.
 *
 * Returns: bytes received, 0 if nothing available, -1 on error
 */
void *sys_command19_recvfrom(struct interrupt_frame *frame) {
    void *user_ptr = task_get_stack_item(task_current(), 0);
    struct recvfrom_args *args = task_virtual_address_to_physical(task_current(), user_ptr);
    if (!args) {
        return (void *)(intptr_t)-1;
    }

    void *buf_phys = task_virtual_address_to_physical(task_current(), args->buf);
    if (!buf_phys) {
        return (void *)(intptr_t)-1;
    }

    /*
     * recvfrom fills the buffer and writes the sender's IP/port
     * directly into the args struct (which is in user memory,
     * but we have the physical address so the kernel can write there).
     *
     * We read src_port into a local variable to avoid taking the
     * address of a packed struct member (which could be misaligned).
     */
    uint16_t src_port = 0;
    int res = socket_recvfrom(args->sockfd, buf_phys, args->max_len, args->src_ip, &src_port);
    args->src_port = src_port;
    return (void *)(intptr_t)res;
}
