#ifndef __ICMP_H
#define __ICMP_H

#include "sys/net/ip.h"
#include "sys/net/netdev.h"
#include <stdint.h>

/*
 * ICMP — Internet Control Message Protocol (RFC 792)
 *
 * ICMP is IP's "error reporting and diagnostics" protocol. It rides
 * inside IP packets (protocol=1) but is considered part of the network
 * layer, not a transport protocol like UDP/TCP.
 *
 * COMMON ICMP MESSAGES:
 *   Type 0:  Echo Reply          — response to a ping
 *   Type 3:  Destination Unreachable — can't reach the target
 *   Type 8:  Echo Request        — the ping itself
 *   Type 11: Time Exceeded       — TTL hit zero (used by traceroute)
 *
 * HOW PING WORKS:
 *   1. Host sends ICMP Echo Request (type=8) with an ID and sequence number
 *   2. Target receives it, swaps src/dst IP, changes type to 0 (Echo Reply),
 *      and sends it back with the SAME ID, sequence, and data
 *   3. Host matches the reply to its request by ID+sequence, measures RTT
 *
 * The ID typically identifies the ping process, and the sequence number
 * increments with each ping. The data is arbitrary (Linux fills it with
 * a timestamp pattern).
 */

/*
 * ICMP header structure
 *
 * All ICMP messages start with this 8-byte header, but the meaning of
 * the last 4 bytes depends on the type. For Echo Request/Reply:
 *
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |     Type      |     Code      |          Checksum             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |         Identifier            |        Sequence Number        |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                         Data (variable)                       |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * - Type + Code: Together identify the message type.
 *   Echo Request = type 8, code 0.  Echo Reply = type 0, code 0.
 * - Checksum: Covers the entire ICMP message (header + data).
 *   Unlike IP checksum which only covers the IP header.
 * - Identifier: Usually the process ID of the ping program.
 * - Sequence: Increments with each ping sent.
 * - Data: Echoed back verbatim. Linux puts a timestamp here.
 */
struct icmp_header {
    uint8_t type;      /* Message type */
    uint8_t code;      /* Message code (subtype) */
    uint16_t checksum; /* Checksum over entire ICMP message */
    uint16_t id;       /* Identifier (for echo) */
    uint16_t sequence; /* Sequence number (for echo) */
} __attribute__((packed));

/* ICMP message types */
#define ICMP_TYPE_ECHO_REPLY 0   /* Ping response */
#define ICMP_TYPE_ECHO_REQUEST 8 /* Ping request */

/**
 * @brief Handle an incoming ICMP message
 *
 * Called from ip_rx() when protocol == 1 (ICMP).
 * The IP header has been parsed but we receive it for reference
 * (we need the source IP to know where to send the reply).
 *
 * @param dev    Network device that received the packet
 * @param ip_hdr The parsed IP header (for extracting source IP)
 * @param buf    The ICMP payload (data starts at ICMP header)
 * @return 0 on success, negative on error
 */
int icmp_rx(struct netdev *dev, struct ip_header *ip_hdr, struct netbuf *buf);

#endif
