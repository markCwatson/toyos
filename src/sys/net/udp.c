/*
 * UDP implementation for ToyOS
 *
 * Currently implements:
 *   - UDP packet reception and validation
 *   - UDP packet transmission via ip_tx()
 *   - Echo server on port 7 (RFC 862)
 *
 * The echo protocol is the simplest possible network service:
 *   1. Receive data on port 7
 *   2. Send the exact same data back to the sender
 *   That's it. No state, no connection, no protocol negotiation.
 *
 * This is useful for testing because `nc -u` (netcat) can send/receive
 * UDP packets interactively from the host.
 */

#include "sys/net/udp.h"
#include "memory/memory.h"
#include "stdlib/printf.h"
#include "sys/net/arp.h"
#include "sys/net/byteorder.h"
#include "sys/net/ip.h"

/**
 * @brief Handle the echo protocol (RFC 862)
 *
 * Echo is dead simple: send back whatever we received.
 * The reply goes back to the sender's IP and port.
 *
 * @param dev       Network device
 * @param ip_hdr    IP header of received packet (for sender's IP)
 * @param udp       UDP header of received packet (for sender's port)
 * @param data      Pointer to the UDP payload data
 * @param data_len  Length of the payload data
 */
static int udp_echo(struct netdev *dev, struct ip_header *ip_hdr, struct udp_header *udp, void *data, int data_len) {
    printf("UDP: Echo %i bytes back to %i.%i.%i.%i:%i\n", data_len, ip_hdr->src_ip[0], ip_hdr->src_ip[1],
           ip_hdr->src_ip[2], ip_hdr->src_ip[3], ntohs(udp->src_port));

    /*
     * Build a netbuf containing just the data to echo back.
     */
    struct netbuf *reply = netbuf_alloc(data_len);
    if (!reply) {
        printf("UDP: Failed to allocate echo reply buffer\n");
        return -1;
    }

    memcpy(reply->data, data, data_len);
    reply->len = data_len;

    /*
     * Send the reply. Note the port swap:
     *   - Our source port = the port they sent TO (port 7)
     *   - Their dest port = the port they sent FROM (their ephemeral port)
     *
     * This is how the sender's `nc` knows which conversation the reply
     * belongs to — it matches the source port it used.
     */
    int res = udp_tx(dev, ip_hdr->src_ip, ntohs(udp->dst_port), /* our port (7) */
                     ntohs(udp->src_port),                      /* their port */
                     reply);

    netbuf_free(reply);
    return res;
}

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
     * Dispatch by destination port.
     * In a real OS, there would be a table of "bound sockets" and we'd
     * look up which process is listening on this port. For now, we
     * just hardcode the echo service.
     */
    switch (dst_port) {
    case UDP_PORT_ECHO:
        return udp_echo(dev, ip_hdr, udp, data, data_len);

    default:
        printf("UDP: No service on port %i, dropping\n", dst_port);
        return 0;
    }
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
