#ifndef __IP_H
#define __IP_H

#include "sys/net/netdev.h"
#include <stdint.h>

/*
 * IPv4 — Internet Protocol version 4 (RFC 791)
 *
 * THE PROBLEM IP SOLVES:
 * Ethernet only works on a local network (LAN). IP provides addressing
 * and routing across networks — it's what makes "the internet" work.
 * Each host gets a globally (or locally) unique IP address, and routers
 * forward packets between networks based on these addresses.
 *
 * WHERE IT SITS:
 *   Ethernet (Layer 2) carries IP packets as its payload.
 *   IP (Layer 3) carries upper-layer protocols (ICMP, UDP, TCP) as its payload.
 *
 * FOR TOYOS:
 * When the host pings us, the flow is:
 *   Ethernet frame (type=0x0800) → IP packet (protocol=1) → ICMP echo request
 *
 * We parse the IP header to:
 *   1. Verify the packet is for us (destination IP matches ours)
 *   2. Identify the upper-layer protocol (ICMP=1, UDP=17, TCP=6)
 *   3. Strip the IP header and pass the payload to the right handler
 */

/*
 * IPv4 header structure (RFC 791)
 *
 * Every IP packet starts with this 20-byte header (without options).
 * All multi-byte fields are in network byte order (big-endian).
 *
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |Version|  IHL  |    TOS        |          Total Length         |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |         Identification        |Flags|      Fragment Offset    |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  Time to Live |    Protocol   |         Header Checksum       |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                       Source Address                          |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                    Destination Address                        |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * - Version (4 bits): Always 4 for IPv4.
 * - IHL (4 bits): Header length in 32-bit words. Minimum 5 (= 20 bytes).
 *   We use a single byte for version+IHL and extract with bit shifts
 *   to avoid C bitfield endianness issues.
 * - TOS: Type of Service / DSCP. We ignore this.
 * - Total Length: Entire packet size (header + payload) in bytes.
 * - Identification: Used for reassembling fragmented packets. We use a counter.
 * - Flags + Fragment Offset: Fragmentation control. We set "Don't Fragment".
 * - TTL: Decremented by each router. Packet is discarded when it hits 0.
 *   Prevents packets from looping forever.
 * - Protocol: Identifies the payload type (1=ICMP, 6=TCP, 17=UDP).
 * - Header Checksum: Error detection for the header only (not payload).
 *   Each layer has its own checksum.
 * - Source/Destination Address: 32-bit IP addresses.
 */
struct ip_header {
    uint8_t version_ihl;   /* Version (high nibble) + IHL (low nibble) */
    uint8_t tos;           /* Type of service */
    uint16_t total_length; /* Total packet length (header + payload) */
    uint16_t id;           /* Identification (for fragmentation) */
    uint16_t flags_frag;   /* Flags (3 bits) + Fragment offset (13 bits) */
    uint8_t ttl;           /* Time to live */
    uint8_t protocol;      /* Upper-layer protocol (ICMP=1, TCP=6, UDP=17) */
    uint16_t checksum;     /* Header checksum */
    uint8_t src_ip[4];     /* Source IP address */
    uint8_t dst_ip[4];     /* Destination IP address */
} __attribute__((packed));

/* Helper macros to extract version and IHL from the combined byte */
#define IP_VERSION(hdr) (((hdr)->version_ihl >> 4) & 0x0F)
#define IP_IHL(hdr) ((hdr)->version_ihl & 0x0F)

/* Header length in bytes = IHL * 4 */
#define IP_HDR_LEN(hdr) (IP_IHL(hdr) * 4)

/* IP protocol numbers (the "protocol" field) */
#define IP_PROTO_ICMP 1
#define IP_PROTO_TCP 6
#define IP_PROTO_UDP 17

/* Ethertype for IPv4 (used when building Ethernet frames) */
#define ETH_TYPE_IPV4 0x0800

/* Default TTL for packets we originate */
#define IP_DEFAULT_TTL 64

/* Don't Fragment flag (bit 1 of the 3-bit flags field) */
#define IP_FLAG_DF 0x4000

/**
 * @brief Calculate internet checksum (RFC 1071)
 *
 * Used by both IP and ICMP. The algorithm:
 *   1. Sum all 16-bit words in the data
 *   2. Fold any carry bits back into the 16-bit sum
 *   3. Take the ones' complement (bitwise NOT)
 *
 * A correct checksum over data + its checksum field == 0.
 * When computing, set the checksum field to 0 first.
 *
 * @param data Pointer to data to checksum
 * @param len  Length in bytes
 * @return 16-bit checksum in host byte order
 */
uint16_t ip_checksum(void *data, int len);

/**
 * @brief Handle an incoming IPv4 packet
 *
 * Called from ethernet_rx() when ethertype == 0x0800.
 * Ethernet header is already stripped — buf->data points at the IP header.
 *
 * @param dev The network device that received the packet
 * @param buf The packet buffer (data starts at IP header)
 * @return 0 on success, negative on error
 */
int ip_rx(struct netdev *dev, struct netbuf *buf);

/**
 * @brief Send an IPv4 packet
 *
 * Builds an IP header, wraps the payload, looks up the destination MAC
 * in the ARP cache, and sends via ethernet_tx.
 *
 * @param dev      Network device to send through
 * @param dst_ip   4-byte destination IP address
 * @param protocol Upper-layer protocol number (e.g., IP_PROTO_ICMP)
 * @param payload  Payload data (e.g., ICMP message)
 * @return 0 on success, negative on error
 */
int ip_tx(struct netdev *dev, uint8_t *dst_ip, uint8_t protocol, struct netbuf *payload);

#endif
