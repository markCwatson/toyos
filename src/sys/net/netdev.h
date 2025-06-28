#ifndef _NETDEV_H_
#define _NETDEV_H_

#include "drivers/pci/pci.h"
#include <stddef.h>
#include <stdint.h>

// Maximum device name length
#define NETDEV_NAME_MAX 32

// Network device states
#define NETDEV_STATE_DOWN 0
#define NETDEV_STATE_UP 1

// Ethernet address length
#define ETH_ADDR_LEN 6

/**
 * @brief Ethernet MAC address structure
 */
struct ethaddr {
    uint8_t addr[ETH_ADDR_LEN];
};

/**
 * @brief Network packet buffer structure
 *
 * This is a simplified version of pbuf for ToyOS.
 * For now, we'll use a single contiguous buffer.
 */
struct netbuf {
    void *data;           // Packet data
    uint16_t len;         // Data length
    uint16_t total_len;   // Total length (same as len for simple buffers)
    struct netbuf *next;  // For chained buffers (unused for now)
};

/**
 * @brief Network device statistics
 */
struct netdev_stats {
    uint32_t rx_packets;      // Received packets
    uint32_t tx_packets;      // Transmitted packets
    uint32_t rx_bytes;        // Received bytes
    uint32_t tx_bytes;        // Transmitted bytes
    uint32_t rx_errors;       // Receive errors
    uint32_t tx_errors;       // Transmit errors
    uint32_t rx_dropped;      // Dropped received packets
    uint32_t tx_dropped;      // Dropped transmitted packets
    uint32_t rx_crc_errors;   // CRC errors
    uint32_t rx_fifo_errors;  // FIFO overrun errors
    uint32_t tx_fifo_errors;  // FIFO underrun errors
    uint32_t collisions;      // Collision count
};

/**
 * @brief Network device structure
 *
 * This serves as ToyOS's equivalent to struct dev from the original driver.
 */
struct netdev {
    char name[NETDEV_NAME_MAX];  // Device name (e.g., "eth0")
    uint32_t flags;              // Device flags
    uint32_t state;              // Device state (up/down)

    // Hardware information
    struct pci_device *pci_dev;  // Associated PCI device
    uint16_t iobase;             // I/O base address
    uint8_t irq;                 // IRQ number
    struct ethaddr hwaddr;       // Hardware (MAC) address

    // Driver operations
    struct netdev_ops *ops;  // Device operations
    void *driver_data;       // Driver private data (struct nic)

    // Statistics
    struct netdev_stats stats;  // Device statistics

    // Future networking integration points
    void *netif;      // Future: network interface
    void *ip_config;  // Future: IP configuration
};

/**
 * @brief Network device operations structure
 */
struct netdev_ops {
    int (*open)(struct netdev *dev);
    int (*close)(struct netdev *dev);
    int (*transmit)(struct netdev *dev, struct netbuf *buf);
    int (*set_rx_mode)(struct netdev *dev);
    struct netdev_stats *(*get_stats)(struct netdev *dev);
};

/**
 * @brief Allocate a network buffer
 *
 * This function will allocate a new network buffer for the network device on the heap. It will also allocate the data
 * buffer for the network buffer.
 *
 * @param size Size of buffer to allocate
 * @return Allocated network buffer or NULL on failure
 */
struct netbuf *netbuf_alloc(uint16_t size);

/**
 * @brief Free a network buffer
 *
 * This function will free the network buffer and the data buffer for the network buffer.
 *
 * @param buf Buffer to free
 */
void netbuf_free(struct netbuf *buf);

/**
 * @brief Create and register a new network device
 *
 * This function will create a new network device and register it with the system.
 * The device name will be generated based on the template string. It requires a PCI device to be associated with the
 * network device.
 *
 * @param name Device name template (e.g., "eth")
 * @param ops Device operations
 * @param pci_dev Associated PCI device
 * @param driver_data Driver private data
 * @return Pointer to allocated device or NULL on failure
 */
struct netdev *netdev_create(const char *name, struct netdev_ops *ops, struct pci_device *pci_dev, void *driver_data);

/**
 * @brief Destroy and unregister a network device
 *
 * @param dev Device to destroy
 */
void netdev_destroy(struct netdev *dev);

/**
 * @brief Handle received packet (called by drivers)
 *
 * This function will be called by the driver to handle received packets. It will update the network device statistics
 * and pass the packet to the network stack.
 *
 * TODO: we do not have a network stack yet, so we will just print the packet info and discard the packet.
 *
 * @param dev Network device
 * @param buf Received packet buffer
 * @return 0 on success, negative on error
 */
int netdev_rx(struct netdev *dev, struct netbuf *buf);

/**
 * @brief Get network device by name
 *
 * @param name Device name to search for
 * @return Pointer to device or NULL if not found
 */
struct netdev *netdev_get_by_name(const char *name);

/**
 * @brief Get network device by index
 *
 * @param index Device index
 * @return Pointer to device or NULL if not found
 */
struct netdev *netdev_get_by_index(int index);

/**
 * @brief Get count of registered network devices
 *
 * @return Number of registered devices
 */
int netdev_get_count(void);

/**
 * @brief Bring up all registered network devices
 *
 * This function attempts to open all registered network devices.
 * It's typically called during system initialization.
 *
 * @return Number of devices successfully brought up
 */
int netdev_bring_all_up(void);

/**
 * @brief Bring up a specific network device by name
 *
 * @param name Device name (e.g., "eth0")
 * @return 0 on success, negative on error
 */
int netdev_bring_up(const char *name);

/**
 * @brief Bring down a specific network device by name
 *
 * @param name Device name (e.g., "eth0")
 * @return 0 on success, negative on error
 */
int netdev_bring_down(const char *name);

#endif