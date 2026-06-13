#ifndef __SYS_NET_H
#define __SYS_NET_H

#include "idt/idt.h"

/*
 * NETWORK SYSTEM CALL HANDLERS
 *
 * These functions are called from sys_handler() (via INT 0x80) when
 * user programs invoke network operations.
 *
 * Each handler:
 *   1. Reads arguments from the user's stack via task_get_stack_item()
 *   2. Converts user-space pointers to physical addresses
 *   3. Calls the appropriate kernel socket function
 *   4. Returns the result (which becomes EAX in user space)
 *
 * The user-space calling convention is:
 *   push arg_N        ; arguments pushed right-to-left
 *   push arg_1
 *   mov eax, CMD_NUM  ; syscall number in EAX
 *   int 0x80          ; trap to kernel
 *   ; result in EAX
 */

/*
 * Argument structures for sendto/recvfrom.
 *
 * These have too many parameters to pass individually on the stack
 * (5 params each), so the user program packs them into a struct and
 * pushes a single pointer. The kernel dereferences the pointer after
 * converting it from the user's virtual address to a physical address.
 *
 * This is similar to how Linux handles sendto/recvfrom with the
 * socketcall() multiplexer on 32-bit x86.
 */
struct sendto_args {
    int sockfd;
    void *buf;
    int len;
    uint8_t dst_ip[4];
    uint16_t dst_port;
} __attribute__((packed));

struct recvfrom_args {
    int sockfd;
    void *buf;
    int max_len;
    uint8_t src_ip[4]; /* filled by kernel on return */
    uint16_t src_port; /* filled by kernel on return */
} __attribute__((packed));

/* Syscall handlers — registered in sys_register_commands() */
void *sys_command16_socket(struct interrupt_frame *frame);
void *sys_command17_bind(struct interrupt_frame *frame);
void *sys_command18_sendto(struct interrupt_frame *frame);
void *sys_command19_recvfrom(struct interrupt_frame *frame);

#endif
