#include "netdev.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"
#include "stdlib/printf.h"
#include "stdlib/string.h"
#include "sys/net/ethernet.h"

// Global device registry (simple for now)
#define MAX_NETDEVS 8
static struct netdev *netdevs[MAX_NETDEVS];
static int netdev_count = 0;

/**
 * @brief Generate unique device name
 *
 * If the template string is "eth", then the device name will be "eth0", "eth1", etc.
 * Otherwise, the device name will be the template string.
 *
 * @param dest Destination buffer
 * @param template Template string
 */
static void generate_device_name(char *dest, const char *template) {
    static int eth_counter = 0;

    if (strncmp(template, "eth", 3) == 0) {
        strcpy(dest, template);
        strcat(dest, itoa(eth_counter++));
    } else {
        strncpy(dest, template, NETDEV_NAME_MAX - 1);
        dest[NETDEV_NAME_MAX - 1] = '\0';
    }
}

struct netbuf *netbuf_alloc(uint16_t size) {
    struct netbuf *buf = kzalloc(sizeof(struct netbuf));
    if (!buf) {
        return NULL;
    }

    buf->data = kzalloc(size);
    if (!buf->data) {
        kfree(buf);
        return NULL;
    }

    buf->len = 0;
    buf->total_len = size;
    buf->next = NULL;

    return buf;
}

void netbuf_free(struct netbuf *buf) {
    if (!buf) {
        return;
    }

    if (buf->data) {
        kfree(buf->data);
    }

    kfree(buf);
}

struct netdev *netdev_create(const char *name, struct netdev_ops *ops, struct pci_device *pci_dev, void *driver_data) {
    if (netdev_count >= MAX_NETDEVS) {
        printf("netdev: Maximum number of network devices reached\n");
        return NULL;
    }

    struct netdev *dev = kzalloc(sizeof(struct netdev));
    if (!dev) {
        printf("netdev: Failed to allocate device structure\n");
        return NULL;
    }

    generate_device_name(dev->name, name);

    dev->flags = 0;
    dev->state = NETDEV_STATE_DOWN;
    dev->pci_dev = pci_dev;
    dev->ops = ops;
    dev->driver_data = driver_data;

    // Extract hardware information from PCI device
    if (pci_dev) {
        dev->iobase = pci_dev->bar[0] & 0xFFFFFFFC;  // Mask off flags
        dev->irq = pci_dev->interrupt_line;
    }

    // Initialize statistics
    memset(&dev->stats, 0, sizeof(dev->stats));

    // Register device
    netdevs[netdev_count] = dev;
    netdev_count++;

    printf("netdev: Created network device '%s' (I/O base: 0x%x, IRQ: %i)\n", dev->name, dev->iobase, dev->irq);

    return dev;
}

void netdev_destroy(struct netdev *dev) {
    if (!dev) {
        return;
    }

    // Find and remove from registry
    for (int i = 0; i < netdev_count; i++) {
        if (netdevs[i] == dev) {
            // Shift remaining devices down
            for (int j = i; j < netdev_count - 1; j++) {
                netdevs[j] = netdevs[j + 1];
            }
            netdev_count--;
            break;
        }
    }

    printf("netdev: Destroyed network device '%s'\n", dev->name);
    kfree(dev);
}

int netdev_rx(struct netdev *dev, struct netbuf *buf) {
    if (!dev || !buf) {
        printf("netdev: Invalid parameters\n");
        return -1;
    }

    // Update statistics
    dev->stats.rx_packets++;
    dev->stats.rx_bytes += buf->len;

    printf("netdev: %s received %i byte packet\n", dev->name, buf->len);
    int res = ethernet_rx(dev, buf);
    if (res < 0) {
        printf("netdev: Failed to process packet\n");
        return -1;
    }

    return 0;
}

struct netdev *netdev_get_by_name(const char *name) {
    for (int i = 0; i < netdev_count; i++) {
        if (strncmp(netdevs[i]->name, name, NETDEV_NAME_MAX) == 0) {
            return netdevs[i];
        }
    }
    return NULL;
}

struct netdev *netdev_get_by_index(int index) {
    if (index < 0 || index >= netdev_count) {
        return NULL;
    }
    return netdevs[index];
}

int netdev_get_count(void) {
    return netdev_count;
}

int netdev_bring_all_up(void) {
    int successful = 0;

    for (int i = 0; i < netdev_count; i++) {
        struct netdev *dev = netdevs[i];
        if (dev && dev->ops && dev->ops->open) {
            int result = dev->ops->open(dev);
            if (result == 0) {
                dev->state = NETDEV_STATE_UP;
                printf("Network interface %s is UP\n", dev->name);
                successful++;
            } else {
                printf("Failed to bring up network interface %s\n", dev->name);
            }
        }
    }

    return successful;
}

int netdev_bring_up(const char *name) {
    struct netdev *dev = netdev_get_by_name(name);
    if (!dev) {
        printf("Network device '%s' not found\n", name);
        return -1;
    }

    if (!dev->ops || !dev->ops->open) {
        printf("Network device '%s' has no open function\n", name);
        return -1;
    }

    if (dev->state == NETDEV_STATE_UP) {
        printf("Network device '%s' is already up\n", name);
        return 0;
    }

    int result = dev->ops->open(dev);
    if (result == 0) {
        dev->state = NETDEV_STATE_UP;
        printf("Network interface %s is UP\n", dev->name);
    } else {
        printf("Failed to bring up network interface %s\n", dev->name);
    }

    return result;
}

int netdev_bring_down(const char *name) {
    struct netdev *dev = netdev_get_by_name(name);
    if (!dev) {
        printf("Network device '%s' not found\n", name);
        return -1;
    }

    if (!dev->ops || !dev->ops->close) {
        printf("Network device '%s' has no close function\n", name);
        return -1;
    }

    if (dev->state == NETDEV_STATE_DOWN) {
        printf("Network device '%s' is already down\n", name);
        return 0;
    }

    int result = dev->ops->close(dev);
    if (result == 0) {
        dev->state = NETDEV_STATE_DOWN;
        printf("Network interface %s is DOWN\n", dev->name);
    } else {
        printf("Failed to bring down network interface %s\n", dev->name);
    }

    return result;
}