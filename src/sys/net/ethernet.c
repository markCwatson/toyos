#include "sys/net/ethernet.h"
#include "memory/memory.h"
#include "stdlib/printf.h"
#include "stdlib/string.h"
#include "sys/net/byteorder.h"
#include "sys/net/netdev.h"

// todo: incomplete
int ethernet_rx(struct netdev *dev, struct netbuf *buf) {
    struct ethernet_header *eth = (struct ethernet_header *)buf->data;

    printf("ETH: %x:%x:%x:%x:%x:%x -> %x:%x:%x:%x:%x:%x type=0x%x\n", eth->src[0], eth->src[1], eth->src[2],
           eth->src[3], eth->src[4], eth->src[5], eth->dest[0], eth->dest[1], eth->dest[2], eth->dest[3], eth->dest[4],
           eth->dest[5], ntohs(eth->ethertype));

    // Strip ethernet header and pass to higher layers
    buf->data = (uint8_t *)buf->data + sizeof(struct ethernet_header);
    buf->len -= sizeof(struct ethernet_header);

    switch (ntohs(eth->ethertype)) {
    case 0x0800:  // IPv4
        printf("ETH: IPv4 packet\n");
        // todo: implement ip_rx
        // return ip_rx(dev, buf);
        return 0;
    case 0x0806:  // ARP
        printf("ETH: ARP packet\n");
        // todo: implement arp_rx
        // return arp_rx(dev, buf);
        return 0;
    default:
        printf("ETH: Unknown ethertype 0x%04x\n", ntohs(eth->ethertype));
        return -1;
    }
}

int ethernet_tx(struct netdev *dev, uint8_t *dest_mac, uint16_t ethertype, struct netbuf *payload) {
    // Allocate buffer for frame
    struct netbuf *frame_buf = netbuf_alloc(sizeof(struct ethernet_header) + payload->len);

    struct ethernet_frame *frame = (struct ethernet_frame *)frame_buf->data;

    // Build ethernet header
    memcpy(frame->header.dest, dest_mac, 6);
    memcpy(frame->header.src, dev->hwaddr.addr, 6);
    frame->header.ethertype = htons(ethertype);

    // Copy payload
    memcpy(frame->payload, payload->data, payload->len);

    // Send via driver
    return dev->ops->transmit(dev, frame_buf);
}
