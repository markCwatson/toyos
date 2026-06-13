/*
 * ICMP implementation for ToyOS
 *
 * Currently handles only Echo Request → Echo Reply (ping).
 *
 * The full flow when someone pings ToyOS:
 *
 *   Host: ping 10.0.2.15
 *     |
 *     v
 *   [ARP exchange happens first — already implemented]
 *     |
 *     v
 *   Host sends: ETH frame → IP packet (proto=1) → ICMP Echo Request
 *     |
 *     v
 *   RTL8139 interrupt → ethernet_rx → ip_rx → icmp_rx  [we are here]
 *     |
 *     v
 *   We see type=8 (Echo Request), build type=0 (Echo Reply)
 *   with the SAME id, sequence, and data
 *     |
 *     v
 *   icmp_reply → ip_tx → ethernet_tx → rtl8139_transmit
 *     |
 *     v
 *   Host receives Echo Reply, prints "64 bytes from 10.0.2.15: ..."
 */

#include "sys/net/icmp.h"
#include "memory/memory.h"
#include "stdlib/printf.h"
#include "sys/net/byteorder.h"
#include "sys/net/ip.h"

/**
 * @brief Send an ICMP Echo Reply in response to an Echo Request
 *
 * The echo reply is essentially a mirror of the request:
 *   - Type changes from 8 (request) to 0 (reply)
 *   - Code stays 0
 *   - ID and sequence are copied verbatim
 *   - Data payload is copied verbatim
 *   - Checksum is recalculated
 *   - IP addresses are swapped (reply goes back to sender)
 *
 * @param dev     Network device to send through
 * @param ip_hdr  The IP header of the received request (for source IP)
 * @param request The received ICMP message (header + data)
 * @param icmp_len Total length of the ICMP message (header + data)
 */
static int icmp_echo_reply(struct netdev *dev, struct ip_header *ip_hdr, struct icmp_header *request, int icmp_len) {
    /*
     * Allocate buffer for the entire ICMP reply.
     * It's the same size as the request — we echo everything back.
     */
    struct netbuf *reply_buf = netbuf_alloc(icmp_len);
    if (!reply_buf) {
        printf("ICMP: Failed to allocate reply buffer\n");
        return -1;
    }

    struct icmp_header *reply = (struct icmp_header *)reply_buf->data;

    /*
     * Copy the entire request (header + data), then modify the type.
     * This preserves the ID, sequence number, and data payload exactly.
     * The ping program on the host matches replies to requests using
     * the ID and sequence number.
     */
    memcpy(reply, request, icmp_len);

    /* Change type from Echo Request (8) to Echo Reply (0) */
    reply->type = ICMP_TYPE_ECHO_REPLY;
    reply->code = 0;

    /*
     * Recalculate the ICMP checksum.
     * The checksum covers the entire ICMP message (header + data).
     * Must set the checksum field to 0 before computing.
     *
     * Note: we could be clever and incrementally adjust the checksum
     * (since we only changed the type field from 8 to 0), but
     * recalculating is clearer and we're not optimizing for speed.
     */
    reply->checksum = 0;
    reply->checksum = ip_checksum(reply, icmp_len);

    reply_buf->len = icmp_len;

    printf("ICMP: Sending echo reply (id=%i seq=%i)\n", ntohs(reply->id), ntohs(reply->sequence));

    /*
     * Send the reply back to whoever sent the request.
     * ip_tx() will:
     *   1. Wrap our ICMP message in an IP header (src=our IP, dst=sender IP)
     *   2. Look up the sender's MAC in the ARP cache
     *   3. Pass to ethernet_tx() to build the Ethernet frame
     *   4. Pass to rtl8139_transmit() for DMA transmission
     */
    int res = ip_tx(dev, ip_hdr->src_ip, IP_PROTO_ICMP, reply_buf);

    netbuf_free(reply_buf);
    return res;
}

int icmp_rx(struct netdev *dev, struct ip_header *ip_hdr, struct netbuf *buf) {
    if (buf->len < sizeof(struct icmp_header)) {
        printf("ICMP: Packet too short (%i bytes)\n", buf->len);
        return -1;
    }

    struct icmp_header *icmp = (struct icmp_header *)buf->data;

    /*
     * Verify ICMP checksum.
     * The checksum covers the entire ICMP message (header + data).
     * ip_checksum() over a correctly checksummed message returns 0.
     */
    if (ip_checksum(icmp, buf->len) != 0) {
        printf("ICMP: Bad checksum\n");
        return -1;
    }

    printf("ICMP: type=%i code=%i id=%i seq=%i len=%i\n", icmp->type, icmp->code, ntohs(icmp->id),
           ntohs(icmp->sequence), buf->len);

    switch (icmp->type) {
    case ICMP_TYPE_ECHO_REQUEST:
        /*
         * Ping! Someone wants to know if we're alive.
         * Reply with the same data so they can verify it.
         */
        return icmp_echo_reply(dev, ip_hdr, icmp, buf->len);

    case ICMP_TYPE_ECHO_REPLY:
        /* We received a reply to a ping WE sent. Not implemented yet. */
        printf("ICMP: Echo reply received (not handled)\n");
        return 0;

    default:
        printf("ICMP: Unhandled type %i\n", icmp->type);
        return 0;
    }
}
