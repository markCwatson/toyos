#ifndef __ARP_H
#define __ARP_H

#include "sys/net/netdev.h"
#include <stdint.h>

/*
 * ARP — Address Resolution Protocol (RFC 826)
 *
 * THE PROBLEM ARP SOLVES:
 * Ethernet frames need a destination MAC address (6 bytes, Layer 2), but
 * higher-level protocols like IP use IP addresses (4 bytes, Layer 3).
 * When a host wants to send an IP packet to another host on the same local
 * network, it needs to discover the MAC address associated with that IP.
 *
 * HOW IT WORKS:
 * 1. Host A wants to send to IP 10.0.2.15 but doesn't know its MAC.
 * 2. Host A broadcasts an ARP REQUEST: "Who has 10.0.2.15? Tell 10.0.2.1"
 *    - Broadcast means dest MAC is FF:FF:FF:FF:FF:FF — every host receives it.
 * 3. Host B (10.0.2.15) sees the request is for its IP, and sends an ARP REPLY:
 *    "10.0.2.15 is at AA:BB:CC:DD:EE:FF" — sent directly (unicast) back to A.
 * 4. Host A caches this mapping and can now send Ethernet frames to B.
 *
 * FOR TOYOS:
 * When the host runs `ping 10.0.2.15`, the host's network stack sends an ARP
 * request through the TAP interface. ToyOS receives it via the RTL8139
 * interrupt → rtl8139_rx() → netdev_rx() → ethernet_rx() → arp_rx().
 * We need to reply so the host learns our MAC, then it can send the actual
 * ICMP ping packet.
 */

/*
 * ARP header structure (for Ethernet + IPv4)
 *
 * This is the payload inside an Ethernet frame when ethertype = 0x0806.
 * The format is generic (supports different hardware/protocol types), but
 * in practice it's almost always Ethernet + IPv4 with the sizes below.
 *
 * Wire format (28 bytes for Ethernet/IPv4):
 *
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |         Hardware Type         |         Protocol Type         |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  HW Addr Len  | Proto Addr Len|           Operation           |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                  Sender Hardware Address (6)                  |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  (cont.)      |          Sender Protocol Address (4)          |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  (cont.)      |       Target Hardware Address (6)             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                  (cont.)      |  Target Protocol Address (4)  |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
struct arp_header {
    uint16_t hwtype;       /* Hardware type (1 = Ethernet) */
    uint16_t protype;      /* Protocol type (0x0800 = IPv4) */
    uint8_t hwsize;        /* Hardware address length (6 for Ethernet MAC) */
    uint8_t prosize;       /* Protocol address length (4 for IPv4) */
    uint16_t opcode;       /* Operation: 1 = request, 2 = reply */
    uint8_t sender_mac[6]; /* Sender hardware (MAC) address */
    uint8_t sender_ip[4];  /* Sender protocol (IP) address */
    uint8_t target_mac[6]; /* Target hardware (MAC) address */
    uint8_t target_ip[4];  /* Target protocol (IP) address */
} __attribute__((packed));

/* Hardware types */
#define ARP_HW_ETHERNET 1 /* We only support Ethernet */

/* Protocol types (same values as Ethernet ethertype) */
#define ARP_PRO_IPV4 0x0800 /* We only support IPv4 */

/* ARP opcodes */
#define ARP_OP_REQUEST 1 /* "Who has <target_ip>? Tell <sender_ip>" */
#define ARP_OP_REPLY 2   /* "<sender_ip> is at <sender_mac>" */

/* Ethertype value for ARP (used when building Ethernet frames) */
#define ETH_TYPE_ARP 0x0806

/**
 * @brief Handle an incoming ARP packet
 *
 * Called from ethernet_rx() when ethertype == 0x0806.
 * The Ethernet header has already been stripped — buf->data points
 * directly at the ARP header.
 *
 * For ARP requests targeting our IP, we send back an ARP reply.
 *
 * @param dev The network device that received the packet
 * @param buf The packet buffer (data starts at ARP header)
 * @return 0 on success, negative on error
 */
int arp_rx(struct netdev *dev, struct netbuf *buf);

/*
 * ARP CACHE
 *
 * Every host that talks to us via ARP reveals its IP→MAC mapping.
 * We store these so that when we need to SEND a packet (e.g., ICMP
 * echo reply), we can look up the destination MAC without sending
 * our own ARP request.
 *
 * Real OS implementations have timeouts, eviction policies, and
 * send ARP requests for unknown IPs. Ours is static and simple.
 */

/**
 * @brief Look up a MAC address for a given IP in the ARP cache
 *
 * @param ip   4-byte IP address to look up
 * @param mac_out  6-byte buffer to receive the MAC address
 * @return 0 if found, -1 if not in cache
 */
int arp_cache_lookup(uint8_t *ip, uint8_t *mac_out);

/**
 * @brief Add or update an entry in the ARP cache
 *
 * @param ip   4-byte IP address
 * @param mac  6-byte MAC address
 */
void arp_cache_update(uint8_t *ip, uint8_t *mac);

/*
 * Our static IP address, defined in arp.c.
 * Used by ARP (to decide which requests to answer) and IP (to validate
 * that incoming packets are addressed to us).
 * In a real OS, this would be per-interface and set via DHCP or config.
 */
extern uint8_t our_ip[4];

#endif
