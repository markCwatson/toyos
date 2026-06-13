#ifndef __UDP_H
#define __UDP_H

#include "sys/net/ip.h"
#include "sys/net/netdev.h"
#include <stdint.h>

/*
 * UDP — User Datagram Protocol (RFC 768)
 *
 * THE SIMPLEST TRANSPORT PROTOCOL:
 * UDP sits on top of IP and adds just one thing: port numbers.
 * IP gets packets to the right machine, UDP gets them to the right
 * application on that machine.
 *
 * WHY PORTS MATTER:
 * A machine might run a web server, DNS server, and echo server
 * simultaneously. They all share the same IP address. Port numbers
 * (16-bit, range 0-65535) identify which service a packet is for.
 * Well-known ports: 7=echo, 53=DNS, 80=HTTP, 443=HTTPS.
 *
 * UDP vs TCP:
 *   UDP: no connection, no guarantees, no ordering. Just fire-and-forget.
 *   TCP: connection-oriented, reliable, ordered. Much more complex.
 *   UDP is perfect for simple request/reply protocols like echo and DNS.
 *
 * HEADER FORMAT (8 bytes — that's it!):
 *
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |          Source Port          |       Destination Port        |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |            Length             |           Checksum            |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                         Data (variable)                       |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * - Source Port: sender's port (so the receiver can reply)
 * - Destination Port: which service this packet is for
 * - Length: total size of UDP header + data (minimum 8)
 * - Checksum: optional in IPv4 (0 means "not computed"). Covers a
 *   "pseudo-header" that includes src/dst IP from the IP layer — this
 *   catches packets delivered to the wrong host.
 *
 * FOR TOYOS:
 * We implement a simple echo server on port 7. When we receive a UDP
 * packet on port 7, we send back the same data to the sender.
 * Flow: `echo "test" | nc -u 10.0.2.15 7` on host →
 *   ethernet_rx → ip_rx → udp_rx → echo data back → ip_tx → ethernet_tx
 */

struct udp_header {
    uint16_t src_port; /* Source port number */
    uint16_t dst_port; /* Destination port number */
    uint16_t length;   /* Length of UDP header + data */
    uint16_t checksum; /* Checksum (0 = not computed) */
} __attribute__((packed));

/* Well-known port numbers */
#define UDP_PORT_ECHO 7

/**
 * @brief Handle an incoming UDP packet
 *
 * Called from ip_rx() when protocol == 17 (UDP).
 * IP header has been parsed; we receive it for the source IP.
 * buf->data points at the UDP header.
 *
 * @param dev    Network device that received the packet
 * @param ip_hdr The parsed IP header (for source IP)
 * @param buf    UDP payload (data starts at UDP header)
 * @return 0 on success, negative on error
 */
int udp_rx(struct netdev *dev, struct ip_header *ip_hdr, struct netbuf *buf);

/**
 * @brief Send a UDP packet
 *
 * Builds a UDP header, wraps the payload, and sends via ip_tx().
 *
 * @param dev      Network device to send through
 * @param dst_ip   4-byte destination IP address
 * @param src_port Source port number (host byte order)
 * @param dst_port Destination port number (host byte order)
 * @param payload  Data to send
 * @return 0 on success, negative on error
 */
int udp_tx(struct netdev *dev, uint8_t *dst_ip, uint16_t src_port, uint16_t dst_port, struct netbuf *payload);

#endif
