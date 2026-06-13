/*
 * Kernel socket implementation for ToyOS
 *
 * This provides the kernel side of the socket API. User programs
 * interact with this via system calls (INT 0x80), which call the
 * functions here after validating parameters.
 *
 * Data flow with sockets:
 *
 *   RECEIVE (inbound):
 *     RTL8139 → ethernet_rx → ip_rx → udp_rx → socket_deliver_udp()
 *       → packet queued in socket's recv_queue
 *       → user calls recvfrom() syscall → socket_recvfrom()
 *       → kernel copies data to user buffer
 *
 *   SEND (outbound):
 *     User calls sendto() syscall → socket_sendto()
 *       → udp_tx() → ip_tx() → ethernet_tx() → rtl8139_transmit()
 *
 * This replaces the old udp_echo() which did everything in kernel space.
 * Now the kernel just delivers packets to sockets, and user programs
 * decide what to do with them.
 */

#include "sys/net/socket.h"
#include "memory/memory.h"
#include "stdlib/printf.h"
#include "sys/net/netdev.h"
#include "sys/net/udp.h"

/* The global socket table. Simple flat array indexed by descriptor. */
static struct socket sockets[MAX_SOCKETS];

int socket_create(int type) {
    /*
     * Only UDP (SOCK_DGRAM) is supported.
     * TCP would require connection state machines, sequence numbers,
     * retransmission timers — far more complex.
     */
    if (type != SOCK_DGRAM) {
        printf("socket: Only SOCK_DGRAM (UDP) is supported\n");
        return -1;
    }

    /* Find a free slot in the socket table */
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (!sockets[i].in_use) {
            memset(&sockets[i], 0, sizeof(struct socket));
            sockets[i].in_use = 1;
            sockets[i].type = type;
            printf("socket: Created socket %i (UDP)\n", i);
            return i;
        }
    }

    printf("socket: No free socket slots\n");
    return -1;
}

int socket_bind(int sockfd, uint16_t port) {
    /* Validate the socket descriptor */
    if (sockfd < 0 || sockfd >= MAX_SOCKETS || !sockets[sockfd].in_use) {
        printf("socket: Invalid socket descriptor %i\n", sockfd);
        return -1;
    }

    /* Check if this socket is already bound */
    if (sockets[sockfd].bound_port != 0) {
        printf("socket: Socket %i already bound to port %i\n", sockfd, sockets[sockfd].bound_port);
        return -1;
    }

    /*
     * Check if another socket is already bound to this port.
     * Only one socket per port — like real OSes with SO_REUSEADDR off.
     */
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (sockets[i].in_use && sockets[i].bound_port == port) {
            printf("socket: Port %i already in use by socket %i\n", port, i);
            return -1;
        }
    }

    sockets[sockfd].bound_port = port;
    printf("socket: Socket %i bound to port %i\n", sockfd, port);
    return 0;
}

int socket_sendto(int sockfd, void *buf, int len, uint8_t *dst_ip, uint16_t dst_port) {
    if (sockfd < 0 || sockfd >= MAX_SOCKETS || !sockets[sockfd].in_use) {
        printf("socket: sendto: Invalid socket %i\n", sockfd);
        return -1;
    }

    if (!buf || len <= 0 || len > SOCKET_MAX_PACKET_SIZE) {
        printf("socket: sendto: Invalid buffer or length %i\n", len);
        return -1;
    }

    /*
     * Get the source port from the socket's binding.
     * If not bound, the packet will go out with source port 0,
     * which is technically valid but unusual.
     */
    uint16_t src_port = sockets[sockfd].bound_port;

    /*
     * Get the first network device.
     * We only have one NIC (eth0), so this is fine.
     * A real OS would have routing tables to pick the right interface.
     */
    struct netdev *dev = netdev_get_by_index(0);
    if (!dev) {
        printf("socket: sendto: No network device available\n");
        return -1;
    }

    /*
     * Allocate a netbuf and copy the user data into it.
     * udp_tx() will wrap this in UDP, IP, and Ethernet headers.
     */
    struct netbuf *payload = netbuf_alloc(len);
    if (!payload) {
        printf("socket: sendto: Failed to allocate buffer\n");
        return -1;
    }

    memcpy(payload->data, buf, len);
    payload->len = len;

    int res = udp_tx(dev, dst_ip, src_port, dst_port, payload);
    netbuf_free(payload);

    if (res < 0) {
        return -1;
    }

    return len;
}

int socket_recvfrom(int sockfd, void *buf, int max_len, uint8_t *src_ip_out, uint16_t *src_port_out) {
    if (sockfd < 0 || sockfd >= MAX_SOCKETS || !sockets[sockfd].in_use) {
        printf("socket: recvfrom: Invalid socket %i\n", sockfd);
        return -1;
    }

    if (!buf || max_len <= 0) {
        return -1;
    }

    /*
     * Check if there's a packet in the receive queue.
     * Non-blocking: return 0 immediately if empty.
     *
     * We don't have a sleep/wakeup mechanism yet, so user programs
     * must poll with a loop. This is wasteful but simple.
     * A real OS would put the process to sleep and wake it when
     * a packet arrives (select/poll/epoll in Linux).
     */
    if (sockets[sockfd].recv_count == 0) {
        return 0; /* No data available */
    }

    /* Pull the oldest packet from the ring buffer */
    struct socket_packet *pkt = &sockets[sockfd].recv_queue[sockets[sockfd].recv_tail];

    /* Copy data to user buffer, truncating if necessary */
    int copy_len = pkt->len;
    if (copy_len > max_len) {
        copy_len = max_len;
    }
    memcpy(buf, pkt->data, copy_len);

    /* Copy sender address info if the caller wants it */
    if (src_ip_out) {
        memcpy(src_ip_out, pkt->src_ip, 4);
    }
    if (src_port_out) {
        *src_port_out = pkt->src_port;
    }

    /* Advance the tail pointer (ring buffer) */
    sockets[sockfd].recv_tail = (sockets[sockfd].recv_tail + 1) % SOCKET_RECV_QUEUE_SIZE;
    sockets[sockfd].recv_count--;

    return copy_len;
}

int socket_deliver_udp(uint16_t port, void *data, int len, uint8_t *src_ip, uint16_t src_port) {
    /*
     * Called from udp_rx() when a packet arrives.
     * Find the socket bound to this port and queue the packet.
     */
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (sockets[i].in_use && sockets[i].bound_port == port) {
            /* Found the socket — check if queue is full */
            if (sockets[i].recv_count >= SOCKET_RECV_QUEUE_SIZE) {
                printf("socket: Receive queue full for port %i, dropping packet\n", port);
                return -1;
            }

            /* Copy packet into the next free slot in the ring buffer */
            struct socket_packet *pkt = &sockets[i].recv_queue[sockets[i].recv_head];

            int copy_len = len;
            if (copy_len > SOCKET_MAX_PACKET_SIZE) {
                copy_len = SOCKET_MAX_PACKET_SIZE;
            }

            memcpy(pkt->data, data, copy_len);
            pkt->len = copy_len;
            memcpy(pkt->src_ip, src_ip, 4);
            pkt->src_port = src_port;

            /* Advance the head pointer */
            sockets[i].recv_head = (sockets[i].recv_head + 1) % SOCKET_RECV_QUEUE_SIZE;
            sockets[i].recv_count++;

            printf("socket: Delivered %i bytes to socket %i (port %i), queue=%i\n", copy_len, i, port,
                   sockets[i].recv_count);
            return 0;
        }
    }

    /* No socket bound to this port */
    return -1;
}

int socket_close(int sockfd) {
    if (sockfd < 0 || sockfd >= MAX_SOCKETS || !sockets[sockfd].in_use) {
        return -1;
    }

    printf("socket: Closing socket %i (port %i)\n", sockfd, sockets[sockfd].bound_port);
    memset(&sockets[sockfd], 0, sizeof(struct socket));
    return 0;
}
