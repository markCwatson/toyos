/*
 * IPv4 implementation for ToyOS
 *
 * Handles incoming IP packets and provides ip_tx() for sending.
 *
 * Receive path:
 *   ethernet_rx() → ip_rx() → [validate header] → icmp_rx() / udp_rx() / ...
 *
 * Send path:
 *   icmp_reply() → ip_tx() → [build IP header] → ethernet_tx() → rtl8139_transmit()
 */

#include "sys/net/ip.h"
#include "memory/memory.h"
#include "stdlib/printf.h"
#include "sys/net/arp.h"
#include "sys/net/byteorder.h"
#include "sys/net/ethernet.h"
#include "sys/net/icmp.h"
#include "sys/net/udp.h"

/*
 * IP identification counter.
 * Each IP packet we send gets a unique ID. This is used by the receiver
 * to reassemble fragmented packets. Since we set Don't Fragment, this
 * is mainly for protocol correctness.
 */
static uint16_t ip_id_counter = 0;

uint16_t ip_checksum(void *data, int len) {
    /*
     * Internet checksum algorithm (RFC 1071):
     *
     * This is used everywhere in TCP/IP — IP headers, ICMP, UDP, TCP.
     * The algorithm is designed to be simple to implement in hardware
     * and has a nice property: it's endian-independent (the same bytes
     * produce the same checksum on big-endian and little-endian machines).
     *
     * Steps:
     * 1. Treat the data as an array of 16-bit words
     * 2. Add them all up in a 32-bit accumulator (to catch carries)
     * 3. Fold the carries: add the high 16 bits to the low 16 bits
     *    (repeat until no more carries)
     * 4. Take the ones' complement (flip all bits)
     *
     * To verify a checksum: compute over the data INCLUDING the checksum
     * field. If correct, the result is 0 (or 0xFFFF before complementing).
     */
    uint32_t sum = 0;
    uint16_t *ptr = (uint16_t *)data;

    /* Sum all 16-bit words */
    while (len > 1) {
        sum += *ptr++;
        len -= 2;
    }

    /* If there's a leftover byte, pad it with zero and add */
    if (len == 1) {
        sum += *(uint8_t *)ptr;
    }

    /* Fold 32-bit sum to 16 bits: add carries back in */
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    /* Ones' complement */
    return (uint16_t)~sum;
}

int ip_rx(struct netdev *dev, struct netbuf *buf) {
    /*
     * Minimum IP header is 20 bytes (IHL=5, no options).
     * Reject anything shorter.
     */
    if (buf->len < sizeof(struct ip_header)) {
        printf("IP: Packet too short (%i bytes)\n", buf->len);
        return -1;
    }

    struct ip_header *ip = (struct ip_header *)buf->data;

    /* Verify this is IPv4 */
    if (IP_VERSION(ip) != 4) {
        printf("IP: Not IPv4 (version=%i)\n", IP_VERSION(ip));
        return -1;
    }

    /* Get header length (IHL is in 32-bit words, so multiply by 4) */
    int hdr_len = IP_HDR_LEN(ip);
    if (hdr_len < 20) {
        printf("IP: Invalid header length (%i bytes)\n", hdr_len);
        return -1;
    }

    /*
     * Verify header checksum.
     * ip_checksum() over a correctly checksummed header returns 0.
     */
    if (ip_checksum(ip, hdr_len) != 0) {
        printf("IP: Bad header checksum\n");
        return -1;
    }

    /*
     * Check if this packet is addressed to us.
     * In a real OS you'd also accept broadcast (255.255.255.255) and
     * multicast addresses. For now, only exact match.
     */
    if (ip->dst_ip[0] != our_ip[0] || ip->dst_ip[1] != our_ip[1] || ip->dst_ip[2] != our_ip[2] ||
        ip->dst_ip[3] != our_ip[3]) {
        printf("IP: Not for us (%i.%i.%i.%i)\n", ip->dst_ip[0], ip->dst_ip[1], ip->dst_ip[2], ip->dst_ip[3]);
        return -1;
    }

    uint16_t total_len = ntohs(ip->total_length);

    printf("IP: %i.%i.%i.%i -> %i.%i.%i.%i proto=%i len=%i\n", ip->src_ip[0], ip->src_ip[1], ip->src_ip[2],
           ip->src_ip[3], ip->dst_ip[0], ip->dst_ip[1], ip->dst_ip[2], ip->dst_ip[3], ip->protocol, total_len);

    /*
     * Strip IP header and pass payload to the upper-layer protocol.
     * The payload starts at offset hdr_len, and its length is
     * total_length - hdr_len.
     */
    struct netbuf payload;
    payload.data = (uint8_t *)buf->data + hdr_len;
    payload.len = total_len - hdr_len;
    payload.total_len = payload.len;
    payload.next = NULL;

    switch (ip->protocol) {
    case IP_PROTO_ICMP:
        return icmp_rx(dev, ip, &payload);

    case IP_PROTO_UDP:
        return udp_rx(dev, ip, &payload);

    case IP_PROTO_TCP:
        printf("IP: TCP packet (not implemented)\n");
        return 0;

    default:
        printf("IP: Unknown protocol %i\n", ip->protocol);
        return -1;
    }
}

int ip_tx(struct netdev *dev, uint8_t *dst_ip, uint8_t protocol, struct netbuf *payload) {
    /*
     * Build a complete IP packet: 20-byte header + payload.
     *
     * Memory layout of what we're building:
     * +--------------------+-------------------+
     * | IP Header (20B)    | Payload (ICMP etc) |
     * +--------------------+-------------------+
     */
    uint16_t total_len = sizeof(struct ip_header) + payload->len;
    struct netbuf *ip_buf = netbuf_alloc(total_len);
    if (!ip_buf) {
        printf("IP: Failed to allocate TX buffer\n");
        return -1;
    }

    struct ip_header *ip = (struct ip_header *)ip_buf->data;

    /*
     * Fill in the IP header fields.
     */

    /* Version=4, IHL=5 (20 bytes, no options). Combined into one byte. */
    ip->version_ihl = (4 << 4) | 5;

    /* Type of Service: 0 = normal/default */
    ip->tos = 0;

    /* Total length of IP packet (header + payload), in network byte order */
    ip->total_length = htons(total_len);

    /* Identification: unique per packet, incremented each time we send */
    ip->id = htons(ip_id_counter++);

    /*
     * Flags + Fragment Offset:
     * We set the "Don't Fragment" (DF) flag. We don't support IP
     * fragmentation/reassembly, so we tell routers not to fragment.
     * If a packet is too large, the router will drop it and send us
     * an ICMP "Fragmentation Needed" message.
     */
    ip->flags_frag = htons(IP_FLAG_DF);

    /* Time to Live: 64 is the standard default (Linux uses 64) */
    ip->ttl = IP_DEFAULT_TTL;

    /* Protocol: what's inside this IP packet (ICMP, UDP, etc.) */
    ip->protocol = protocol;

    /* Source = our IP, Destination = provided */
    memcpy(ip->src_ip, our_ip, 4);
    memcpy(ip->dst_ip, dst_ip, 4);

    /*
     * Checksum: must be computed with the checksum field set to 0.
     * The checksum covers ONLY the IP header, not the payload.
     * (Each upper layer has its own checksum.)
     */
    ip->checksum = 0;
    ip->checksum = ip_checksum(ip, sizeof(struct ip_header));

    /* Copy payload after the IP header */
    memcpy((uint8_t *)ip_buf->data + sizeof(struct ip_header), payload->data, payload->len);
    ip_buf->len = total_len;

    /*
     * Look up the destination MAC address in the ARP cache.
     * This should already be there because:
     *   1. The host sent us an ARP request (which we cached)
     *   2. Then sent us the IP packet (which we're now replying to)
     *
     * If it's not there, we'd need to send an ARP request ourselves
     * and queue the packet — that's not implemented yet.
     */
    uint8_t dst_mac[6];
    if (arp_cache_lookup(dst_ip, dst_mac) < 0) {
        printf("IP: No ARP entry for %i.%i.%i.%i, cannot send\n", dst_ip[0], dst_ip[1], dst_ip[2], dst_ip[3]);
        netbuf_free(ip_buf);
        return -1;
    }

    /* Send the IP packet wrapped in an Ethernet frame */
    int res = ethernet_tx(dev, dst_mac, ETH_TYPE_IPV4, ip_buf);

    netbuf_free(ip_buf);
    return res;
}
