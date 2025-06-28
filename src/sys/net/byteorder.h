#ifndef __BYTEORDER_H
#define __BYTEORDER_H

#include "stdint.h"

/**
* Every multi-byte field in network protocols needs conversion:
- Ethernet ethertype (16-bit)
- IP addresses (32-bit)
- Port numbers (16-bit)
- Packet lengths (16-bit)
- Checksums (16-bit)

 * Critical for correct protocol handling! Without these conversions, your network stack won't work correctly. 🌐
 */

// Convert 16-bit network to host byte order
static inline uint16_t ntohs(uint16_t netshort) {
    return ((netshort & 0xFF) << 8) | ((netshort >> 8) & 0xFF);
}

// Convert 16-bit host to network byte order
static inline uint16_t htons(uint16_t hostshort) {
    return ((hostshort & 0xFF) << 8) | ((hostshort >> 8) & 0xFF);
}

// Convert 32-bit network to host byte order
static inline uint32_t ntohl(uint32_t netlong) {
    return ((netlong & 0xFF) << 24) | (((netlong >> 8) & 0xFF) << 16) | (((netlong >> 16) & 0xFF) << 8) |
           ((netlong >> 24) & 0xFF);
}

// Convert 32-bit host to network byte order
static inline uint32_t htonl(uint32_t hostlong) {
    return ((hostlong & 0xFF) << 24) | (((hostlong >> 8) & 0xFF) << 16) | (((hostlong >> 16) & 0xFF) << 8) |
           ((hostlong >> 24) & 0xFF);
}

#endif