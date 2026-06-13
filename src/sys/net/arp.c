/*
 * ARP — Address Resolution Protocol implementation for ToyOS
 *
 * This is a minimal "reactive" ARP implementation: we only reply to ARP
 * requests targeting our IP address. We don't maintain an ARP cache or
 * send ARP requests ourselves (that comes later when we need to send
 * IP packets outbound).
 *
 * The flow when someone pings ToyOS:
 *
 *   Host: "Who has 10.0.2.15? Tell 10.0.2.1"   (ARP REQUEST, broadcast)
 *     |
 *     v
 *   RTL8139 receives frame via DMA → interrupt fires
 *     |
 *     v
 *   rtl8139_rx() → netdev_rx() → ethernet_rx() → arp_rx()  [we are here]
 *     |
 *     v
 *   We check: is 10.0.2.15 our IP? YES → build ARP REPLY
 *     |
 *     v
 *   "10.0.2.15 is at <our MAC>"  (ARP REPLY, unicast back to sender)
 *     |
 *     v
 *   ethernet_tx() → rtl8139_transmit() → DMA sends frame out
 *     |
 *     v
 *   Host now knows our MAC → can send ICMP ping (or UDP, etc.)
 */

#include "sys/net/arp.h"
#include "memory/memory.h"
#include "stdlib/printf.h"
#include "sys/net/byteorder.h"
#include "sys/net/ethernet.h"

/*
 * Our static IP address: 10.0.2.15
 *
 * QEMU's default guest IP for user-mode networking is 10.0.2.15.
 * We use the same address for TAP networking so the setup is consistent.
 * In a real OS, this would come from DHCP or manual configuration.
 *
 * Stored as 4 bytes in network order (big-endian): 10, 0, 2, 15
 */
uint8_t our_ip[4] = {10, 0, 2, 15};

/*
 * ARP CACHE
 *
 * A simple fixed-size table mapping IP addresses to MAC addresses.
 * Entries are populated when we receive ARP packets (requests or replies).
 * New entries overwrite the oldest when the table is full.
 *
 * This cache is essential for sending packets: when ip_tx() needs to
 * send an IP packet, it calls arp_cache_lookup() to find the destination
 * MAC address for the Ethernet frame.
 */
#define ARP_CACHE_SIZE 16

struct arp_cache_entry {
    uint8_t ip[4];
    uint8_t mac[6];
    int valid;
};

static struct arp_cache_entry arp_cache[ARP_CACHE_SIZE];
static int arp_cache_next = 0; /* Next slot to overwrite when full */

void arp_cache_update(uint8_t *ip, uint8_t *mac) {
    /* Check if entry already exists — update it */
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (arp_cache[i].valid && arp_cache[i].ip[0] == ip[0] && arp_cache[i].ip[1] == ip[1] &&
            arp_cache[i].ip[2] == ip[2] && arp_cache[i].ip[3] == ip[3]) {
            memcpy(arp_cache[i].mac, mac, 6);
            return;
        }
    }

    /* New entry — use next available slot (circular) */
    struct arp_cache_entry *entry = &arp_cache[arp_cache_next];
    memcpy(entry->ip, ip, 4);
    memcpy(entry->mac, mac, 6);
    entry->valid = 1;
    arp_cache_next = (arp_cache_next + 1) % ARP_CACHE_SIZE;

    printf("ARP: Cached %i.%i.%i.%i -> %x:%x:%x:%x:%x:%x\n", ip[0], ip[1], ip[2], ip[3], mac[0], mac[1], mac[2], mac[3],
           mac[4], mac[5]);
}

int arp_cache_lookup(uint8_t *ip, uint8_t *mac_out) {
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (arp_cache[i].valid && arp_cache[i].ip[0] == ip[0] && arp_cache[i].ip[1] == ip[1] &&
            arp_cache[i].ip[2] == ip[2] && arp_cache[i].ip[3] == ip[3]) {
            memcpy(mac_out, arp_cache[i].mac, 6);
            return 0;
        }
    }
    return -1; /* Not found */
}

/**
 * @brief Check if an IP address matches ours
 *
 * Compares 4 bytes. Could use memcmp, but this makes the intent clear
 * and avoids any ambiguity about signed/unsigned comparison.
 */
static int is_our_ip(uint8_t *ip) {
    return ip[0] == our_ip[0] && ip[1] == our_ip[1] && ip[2] == our_ip[2] && ip[3] == our_ip[3];
}

/**
 * @brief Print an ARP packet for debugging
 */
static void arp_print(const char *prefix, struct arp_header *arp) {
    printf("%s op=%i %i.%i.%i.%i (%x:%x:%x:%x:%x:%x) -> %i.%i.%i.%i (%x:%x:%x:%x:%x:%x)\n", prefix, ntohs(arp->opcode),
           arp->sender_ip[0], arp->sender_ip[1], arp->sender_ip[2], arp->sender_ip[3], arp->sender_mac[0],
           arp->sender_mac[1], arp->sender_mac[2], arp->sender_mac[3], arp->sender_mac[4], arp->sender_mac[5],
           arp->target_ip[0], arp->target_ip[1], arp->target_ip[2], arp->target_ip[3], arp->target_mac[0],
           arp->target_mac[1], arp->target_mac[2], arp->target_mac[3], arp->target_mac[4], arp->target_mac[5]);
}

/**
 * @brief Send an ARP reply
 *
 * When we receive an ARP request for our IP, we need to tell the sender
 * our MAC address so they can send us Ethernet frames directly.
 *
 * An ARP reply is basically the request with:
 *   - opcode changed from REQUEST (1) to REPLY (2)
 *   - sender fields set to OUR addresses (we are now the sender)
 *   - target fields set to the ORIGINAL sender (we're replying to them)
 *
 * The reply is sent as a unicast Ethernet frame (not broadcast) because
 * we know exactly who asked — their MAC is in the request.
 */
static int arp_send_reply(struct netdev *dev, struct arp_header *request) {
    /*
     * Allocate a buffer for the ARP reply payload.
     * An ARP packet for Ethernet/IPv4 is always 28 bytes.
     */
    struct netbuf *reply_buf = netbuf_alloc(sizeof(struct arp_header));
    if (!reply_buf) {
        printf("ARP: Failed to allocate reply buffer\n");
        return -1;
    }

    struct arp_header *reply = (struct arp_header *)reply_buf->data;

    /*
     * Fill in the ARP reply header.
     *
     * hwtype and protype stay the same (Ethernet, IPv4).
     * hwsize and prosize stay the same (6, 4).
     * All multi-byte fields must be in network byte order (big-endian).
     */
    reply->hwtype = htons(ARP_HW_ETHERNET);
    reply->protype = htons(ARP_PRO_IPV4);
    reply->hwsize = 6;  /* MAC address is 6 bytes */
    reply->prosize = 4; /* IPv4 address is 4 bytes */
    reply->opcode = htons(ARP_OP_REPLY);

    /*
     * Sender = us (we're the one replying)
     * This is the answer to "Who has 10.0.2.15?" — "It's me, here's my MAC"
     */
    memcpy(reply->sender_mac, dev->hwaddr.addr, 6);
    memcpy(reply->sender_ip, our_ip, 4);

    /*
     * Target = whoever sent the request
     * We copy their MAC and IP from the request's sender fields.
     */
    memcpy(reply->target_mac, request->sender_mac, 6);
    memcpy(reply->target_ip, request->sender_ip, 4);

    reply_buf->len = sizeof(struct arp_header);

    arp_print("ARP: TX reply", reply);

    /*
     * Send via ethernet_tx(), which wraps our ARP payload in an Ethernet
     * frame with:
     *   - dest MAC = the requester's MAC (unicast)
     *   - src MAC  = our MAC
     *   - ethertype = 0x0806 (ARP)
     *
     * Then ethernet_tx() hands it to rtl8139_transmit() for DMA transmission.
     */
    int res = ethernet_tx(dev, request->sender_mac, ETH_TYPE_ARP, reply_buf);

    netbuf_free(reply_buf);
    return res;
}

int arp_rx(struct netdev *dev, struct netbuf *buf) {
    /*
     * Validate the packet. An ARP packet for Ethernet/IPv4 is exactly
     * 28 bytes. If we got less, the packet is malformed.
     */
    if (buf->len < sizeof(struct arp_header)) {
        printf("ARP: Packet too short (%i bytes, need %i)\n", buf->len, sizeof(struct arp_header));
        return -1;
    }

    struct arp_header *arp = (struct arp_header *)buf->data;

    arp_print("ARP: RX", arp);

    /*
     * Validate hardware and protocol types.
     * We only handle Ethernet (hwtype=1) + IPv4 (protype=0x0800).
     * Other combinations exist (e.g., ARP over Token Ring) but we'll
     * never see them in QEMU.
     */
    if (ntohs(arp->hwtype) != ARP_HW_ETHERNET) {
        printf("ARP: Unsupported hardware type %i\n", ntohs(arp->hwtype));
        return -1;
    }

    if (ntohs(arp->protype) != ARP_PRO_IPV4) {
        printf("ARP: Unsupported protocol type 0x%x\n", ntohs(arp->protype));
        return -1;
    }

    /*
     * Cache the sender's IP→MAC mapping from every valid ARP packet.
     * This is important: when we later need to send a packet (e.g., ICMP
     * echo reply), ip_tx() will call arp_cache_lookup() to find the
     * destination MAC. By caching here, the entry is already present
     * by the time we need it.
     */
    arp_cache_update(arp->sender_ip, arp->sender_mac);

    switch (ntohs(arp->opcode)) {
    case ARP_OP_REQUEST:
        /*
         * Someone is asking "Who has <target_ip>?"
         * If the target IP is ours, we reply with our MAC address.
         * If it's not for us, we silently ignore it — that's normal,
         * ARP requests are broadcast so every host on the network sees them.
         */
        if (is_our_ip(arp->target_ip)) {
            printf("ARP: Request for our IP, sending reply\n");
            return arp_send_reply(dev, arp);
        } else {
            printf("ARP: Request for %i.%i.%i.%i (not us), ignoring\n", arp->target_ip[0], arp->target_ip[1],
                   arp->target_ip[2], arp->target_ip[3]);
        }
        break;

    case ARP_OP_REPLY:
        /*
         * Someone is telling us their MAC address in response to a
         * request we sent. We'd store this in an ARP cache for future
         * use when sending packets. Not implemented yet since we don't
         * send ARP requests ourselves.
         */
        printf("ARP: Reply from %i.%i.%i.%i (caching not implemented)\n", arp->sender_ip[0], arp->sender_ip[1],
               arp->sender_ip[2], arp->sender_ip[3]);
        break;

    default:
        printf("ARP: Unknown opcode %i\n", ntohs(arp->opcode));
        return -1;
    }

    return 0;
}
