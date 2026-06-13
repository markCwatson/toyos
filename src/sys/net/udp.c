/*
 * UDP implementation for ToyOS
 *
 * Implements:
 *   - UDP packet reception and validation
 *   - UDP packet transmission via ip_tx()
 *   - Delivery to user-space sockets via socket_deliver_udp()
 *
 * The echo server used to live here (udp_echo), but has been moved
 * to user space as programs/udpecho. Now udp_rx() delivers packets
 * to whatever socket is bound to the destination port.
 */

#include "sys/net/udp.h"
#include "memory/memory.h"
#include "stdlib/printf.h"
#include "sys/net/arp.h"
#include "sys/net/byteorder.h"
#include "sys/net/ip.h"
#include "sys/net/socket.h"

int udp_rx(struct netdev *dev, struct ip_header *ip_hdr, struct netbuf *buf) {
    if (buf->len < sizeof(struct udp_header)) {
        printf("UDP: Packet too short (%i bytes)\n", buf->len);
        return -1;
    }

    struct udp_header *udp = (struct udp_header *)buf->data;

    uint16_t src_port = ntohs(udp->src_port);
    uint16_t dst_port = ntohs(udp->dst_port);
    uint16_t udp_len = ntohs(udp->length);

    /*
     * Sanity check: UDP length field includes the 8-byte header.
     * It should be >= 8 and <= what IP told us the payload was.
     */
    if (udp_len < sizeof(struct udp_header) || udp_len > buf->len) {
        printf("UDP: Invalid length %i (buf=%i)\n", udp_len, buf->len);
        return -1;
    }

    /*
     * UDP checksum is optional in IPv4 (a value of 0 means "not computed").
     * Many simple tools like `nc` do compute it, but we skip verification
     * for now since our ip_checksum() would need a pseudo-header to
     * verify UDP checksums properly. The IP layer already validated the
     * IP header checksum, so we have some protection against corruption.
     *
     * TODO: implement UDP checksum verification with pseudo-header.
     */

    /* Payload starts right after the 8-byte UDP header */
    void *data = (uint8_t *)buf->data + sizeof(struct udp_header);
    int data_len = udp_len - sizeof(struct udp_header);

    printf("UDP: %i.%i.%i.%i:%i -> port %i (%i bytes data)\n", ip_hdr->src_ip[0], ip_hdr->src_ip[1], ip_hdr->src_ip[2],
           ip_hdr->src_ip[3], src_port, dst_port, data_len);

    /*
     * Try to deliver to a user-space socket bound to this port.
     * socket_deliver_udp() searches the socket table for a socket
     * bound to dst_port and queues the packet in its receive buffer.
     *
     * If no socket is bound, the packet is silently dropped —
     * this is normal UDP behavior (unlike TCP, which would send
     * an RST/ICMP port unreachable).
     */
    if (socket_deliver_udp(dst_port, data, data_len, ip_hdr->src_ip, src_port) < 0) {
        printf("UDP: No socket bound to port %i, dropping\n", dst_port);
    }

    return 0;
}

int udp_tx(struct netdev *dev, uint8_t *dst_ip, uint16_t src_port, uint16_t dst_port, struct netbuf *payload) {
    /*
     * Build a UDP packet: 8-byte header + payload data.
     *
     * +------------------+-------------------+
     * | UDP Header (8B)  | Data              |
     * +------------------+-------------------+
     *
     * This whole thing becomes the payload of an IP packet,
     * which becomes the payload of an Ethernet frame.
     */
    uint16_t udp_len = sizeof(struct udp_header) + payload->len;
    struct netbuf *udp_buf = netbuf_alloc(udp_len);
    if (!udp_buf) {
        printf("UDP: Failed to allocate TX buffer\n");
        return -1;
    }

    struct udp_header *udp = (struct udp_header *)udp_buf->data;

    udp->src_port = htons(src_port);
    udp->dst_port = htons(dst_port);
    udp->length = htons(udp_len);

    /*
     * UDP checksum: we set it to 0 (meaning "not computed").
     * This is valid in IPv4 — the checksum is optional.
     * A proper implementation would compute a checksum over a
     * "pseudo-header" (src IP, dst IP, protocol, UDP length) plus
     * the UDP header and data. That protects against misdelivery
     * (packet arriving at wrong IP). For our simple echo server
     * on a point-to-point TAP link, it's fine to skip.
     *
     * TODO: compute UDP checksum with pseudo-header for correctness.
     */
    udp->checksum = 0;

    /* Copy payload after the header */
    memcpy((uint8_t *)udp_buf->data + sizeof(struct udp_header), payload->data, payload->len);
    udp_buf->len = udp_len;

    /* Hand off to IP layer, which adds the IP header and sends */
    int res = ip_tx(dev, dst_ip, IP_PROTO_UDP, udp_buf);

    netbuf_free(udp_buf);
    return res;
}
