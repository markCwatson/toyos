#ifndef __SOCKET_H
#define __SOCKET_H

#include <stdint.h>

/*
 * KERNEL SOCKET LAYER
 *
 * This is the bridge between user programs and the kernel network stack.
 *
 * We create "socket" objects in the kernel when a user program:
 *   1. Calls socket() to get a socket descriptor (like a file descriptor)
 *   2. Calls bind() to associate the socket with a port (e.g., port 7)
 *   3. Calls recvfrom() to block until a packet arrives — the kernel
 *      copies the data into the user's buffer
 *   4. Calls sendto() to send data — the kernel builds UDP/IP/Ethernet
 *      headers and transmits
 *
 * WHY A TABLE INSTEAD OF A LINKED LIST?
 * Simplicity. We support a small fixed number of sockets (16). The
 * socket descriptor is just the index into this table — no dynamic
 * allocation needed. Real OSes use hash tables and linked lists for
 * scalability, but for a hobby OS with a handful of sockets, a flat
 * array is fine.
 */

/* Maximum number of simultaneous sockets */
#define MAX_SOCKETS 16

/* Maximum packets queued per socket before new ones are dropped */
#define SOCKET_RECV_QUEUE_SIZE 8

/* Maximum bytes of payload per queued packet */
#define SOCKET_MAX_PACKET_SIZE 1500

/* Socket types (only UDP for now) */
#define SOCK_DGRAM 2 /* UDP — matches the POSIX value */

/*
 * A queued packet in the receive buffer.
 *
 * When a UDP packet arrives for a bound socket, we copy the payload
 * and the sender's address info into one of these structures. The
 * user retrieves it later via recvfrom().
 */
struct socket_packet {
    uint8_t data[SOCKET_MAX_PACKET_SIZE]; /* Packet payload (no headers) */
    int len;                              /* Length of data */
    uint8_t src_ip[4];                    /* Sender's IP address */
    uint16_t src_port;                    /* Sender's port number */
};

/*
 * A socket.
 *
 * Each socket represents one endpoint for communication. It has a
 * type (UDP), an optional bound port, and a ring buffer for received
 * packets.
 *
 * The ring buffer uses head/tail indices:
 *   - head: next slot to WRITE into (where new packets go)
 *   - tail: next slot to READ from (what recvfrom returns)
 *   - count: number of packets currently queued
 *
 * When head == tail and count == 0, the queue is empty.
 * When count == SOCKET_RECV_QUEUE_SIZE, the queue is full.
 */
struct socket {
    int in_use;                                              /* 1 if this socket slot is allocated */
    int type;                                                /* SOCK_DGRAM (UDP) */
    uint16_t bound_port;                                     /* Port this socket is bound to (0 = unbound) */
    struct socket_packet recv_queue[SOCKET_RECV_QUEUE_SIZE]; /* Ring buffer */
    int recv_head;                                           /* Write index */
    int recv_tail;                                           /* Read index */
    int recv_count;                                          /* Packets in queue */
};

/**
 * @brief Allocate a new socket
 *
 * Finds a free slot in the socket table and marks it as in use.
 *
 * @param type Socket type (must be SOCK_DGRAM for now)
 * @return Socket descriptor (0-15) on success, -1 if table full or invalid type
 */
int socket_create(int type);

/**
 * @brief Bind a socket to a UDP port
 *
 * After binding, UDP packets arriving on this port will be queued
 * to this socket's receive buffer instead of being dropped.
 *
 * @param sockfd Socket descriptor from socket_create()
 * @param port   Port number to bind to (host byte order)
 * @return 0 on success, -1 on error (bad fd, already bound, port in use)
 */
int socket_bind(int sockfd, uint16_t port);

/**
 * @brief Send a UDP packet
 *
 * Builds and sends a UDP packet through the network stack.
 *
 * @param sockfd   Socket descriptor
 * @param buf      Data to send
 * @param len      Length of data
 * @param dst_ip   Destination IP (4 bytes)
 * @param dst_port Destination port (host byte order)
 * @return Number of bytes sent, or -1 on error
 */
int socket_sendto(int sockfd, void *buf, int len, uint8_t *dst_ip, uint16_t dst_port);

/**
 * @brief Receive a UDP packet
 *
 * Pulls the oldest packet from the socket's receive queue.
 * Non-blocking: returns 0 immediately if no packet is available.
 *
 * @param sockfd     Socket descriptor
 * @param buf        Buffer to copy packet data into
 * @param max_len    Maximum bytes to copy
 * @param src_ip_out Filled with sender's IP (4 bytes), or NULL to ignore
 * @param src_port_out Filled with sender's port, or NULL to ignore
 * @return Number of bytes received, 0 if queue empty, -1 on error
 */
int socket_recvfrom(int sockfd, void *buf, int max_len, uint8_t *src_ip_out, uint16_t *src_port_out);

/**
 * @brief Queue a received UDP packet to the appropriate socket
 *
 * Called from udp_rx() when a packet arrives on a bound port.
 * This is the kernel-internal interface — not a syscall.
 *
 * @param port     Destination port the packet arrived on
 * @param data     Packet payload (past UDP header)
 * @param len      Payload length
 * @param src_ip   Sender's IP address (4 bytes)
 * @param src_port Sender's port number
 * @return 0 if delivered to a socket, -1 if no socket bound to this port
 */
int socket_deliver_udp(uint16_t port, void *data, int len, uint8_t *src_ip, uint16_t src_port);

/**
 * @brief Close a socket and free its slot
 *
 * @param sockfd Socket descriptor
 * @return 0 on success, -1 on error
 */
int socket_close(int sockfd);

#endif
