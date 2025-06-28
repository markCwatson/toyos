#ifndef __ETHERNET_H
#define __ETHERNET_H

#include "sys/net/netdev.h"

struct ethernet_header {
    uint8_t dest[6];     // Destination MAC
    uint8_t src[6];      // Source MAC
    uint16_t ethertype;  // Protocol (IP=0x0800, ARP=0x0806)
} __attribute__((packed));

struct ethernet_frame {
    struct ethernet_header header;
    uint8_t payload[];
} __attribute__((packed));

int ethernet_rx(struct netdev *dev, struct netbuf *buf);
int ethernet_tx(struct netdev *dev, struct netbuf *buf);

#endif
