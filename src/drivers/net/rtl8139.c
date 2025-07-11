// RealTek RTL8129/RTL8139 PCI NIC network driver
//
// Written 1997-2002 by Donald Becker.
// Ported to sanos 2002 by Michael Ringgaard.
// Ported to toyos 2025 by Mark Watson (with the help of Claude-4-Sonnet in Cursor...
// ...actually, it should be the other way around)

#include "rtl8139.h"
#include "memory/memory.h"

// The user-configurable values

// Maximum events (Rx packets, etc.) to handle at each interrupt
static int max_interrupt_work = 20;

// Maximum number of multicast addresses to filter (vs. Rx-all-multicast).
// The RTL chips use a 64 element hash table based on the Ethernet CRC.  It
// is efficient to update the hardware filter, but recalculating the table
// for a long filter list is painful
static int multicast_filter_limit = 32;

// Define capability flags
#define RTL8139_CAPS (HAS_CHIP_XCVR | HAS_LNK_CHNG)

// Twister tuning parameters from RealTek.
// Completely undocumented, but required to tune bad links.

#define PARA78_default 0x78fa8388
#define PARA7c_default 0xcb38de43
#define PARA7c_xxx 0xcb38de43

unsigned long param[4][4] = {{0xcb39de43, 0xcb39ce43, 0xfb38de03, 0xcb38de43},
                             {0xcb39de43, 0xcb39ce43, 0xcb39ce83, 0xcb39ce83},
                             {0xcb39de43, 0xcb39ce43, 0xcb39ce83, 0xcb39ce83},
                             {0xbb39de43, 0xbb39ce43, 0xbb39ce83, 0xbb39ce83}};

//
// Serial EEPROM section
//

//  EEPROM_Ctrl bits

#define EE_SHIFT_CLK 0x04   // EEPROM shift clock
#define EE_CS 0x08          // EEPROM chip select
#define EE_DATA_WRITE 0x02  // EEPROM chip data in
#define EE_WRITE_0 0x00
#define EE_WRITE_1 0x02
#define EE_DATA_READ 0x01  // EEPROM chip data out
#define EE_ENB (0x80 | EE_CS)

// Delay between EEPROM clock transitions.
// No extra delay is needed with 33Mhz PCI, but 66Mhz may change this.

#define eeprom_delay() insl(ee_addr)

// The EEPROM commands include the alway-set leading bit
#define EE_WRITE_CMD (5)
#define EE_READ_CMD (6)
#define EE_ERASE_CMD (7)

/**
 * @brief RTL8139 private data structure
 *
 * This replaces the original struct nic
 */
struct rtl8139 {
    struct netdev *netdev;  // Associated network device

    // Hardware information
    uint16_t iobase;  // I/O base address
    uint8_t irq;      // IRQ number

    // Interrupt handling
    int interrupt_registered;  // Whether interrupt is registered

    // Receive state
    uint8_t *rx_ring;     // Receive ring buffer
    uint32_t cur_rx;      // Index into the Rx buffer of next Rx pkt
    uint32_t rx_buf_len;  // Size (8K 16K 32K or 64KB) of the Rx ring

    // Transmit state
    struct spinlock_t tx_lock;            // Transmit lock (replaces semaphore)
    uint32_t cur_tx;                      // Current transmit descriptor
    uint32_t dirty_tx;                    // Dirty transmit descriptors
    uint32_t tx_flag;                     // Transmit configuration flags
    struct netbuf *tx_bufs[NUM_TX_DESC];  // Saved transmit packets
    uint8_t *tx_buffer[NUM_TX_DESC];      // Tx bounce buffers
    uint8_t *tx_bufs_mem;                 // Tx bounce buffer region
    uint32_t trans_start;                 // Last transmission start time

    // Receiver filter state
    uint32_t rx_config;     // Receive configuration
    uint32_t mc_filter[2];  // Multicast hash filter

    // Transceiver state
    uint8_t phys[4];       // MII device addresses
    uint16_t advertising;  // NWay media advertisement
    uint8_t full_duplex;   // Full-duplex operation requested
    uint8_t duplex_lock;   // Duplex locked
    uint8_t link_speed;    // Link speed

    // Configuration
    uint8_t config1;                  // Configuration register 1 value
    uint32_t max_interrupt_work;      // Maximum interrupt work
    uint32_t multicast_filter_limit;  // Multicast filter limit

    // Flags
    uint32_t flags;  // Board capability flags

    // Stats (basic - netdev handles the rest)
    uint32_t tx_timeout_count;  // Transmit timeout counter
};

static int rtl8139_read_eeprom(uint16_t iobase, int location, int addr_len) {
    int i;
    unsigned retval = 0;
    uint16_t ee_addr = iobase + Cfg9346;
    int read_cmd = location | (EE_READ_CMD << addr_len);

    outb(ee_addr, EE_ENB & ~EE_CS);
    outb(ee_addr, EE_ENB);

    // Shift the read command bits out
    for (i = 4 + addr_len; i >= 0; i--) {
        int dataval = (read_cmd & (1 << i)) ? EE_DATA_WRITE : 0;
        outb(ee_addr, EE_ENB | dataval);
        eeprom_delay();
        outb(ee_addr, EE_ENB | dataval | EE_SHIFT_CLK);
        eeprom_delay();
    }
    outb(ee_addr, EE_ENB);
    eeprom_delay();

    for (i = 16; i > 0; i--) {
        outb(ee_addr, EE_ENB | EE_SHIFT_CLK);
        eeprom_delay();
        retval = (retval << 1) | ((insb(ee_addr) & EE_DATA_READ) ? 1 : 0);
        outb(ee_addr, EE_ENB);
        eeprom_delay();
    }

    // Terminate the EEPROM access
    outb(ee_addr, ~EE_CS);
    return retval;
}

// MII serial management

// Read and write the MII management registers using software-generated
// serial MDIO protocol.
// The maximum data clock rate is 2.5 Mhz.  The minimum timing is usually
// met by back-to-back PCI I/O cycles, but we insert a delay to avoid
// "overclocking" issues

#define MDIO_DIR 0x80
#define MDIO_DATA_OUT 0x04
#define MDIO_DATA_IN 0x02
#define MDIO_CLK 0x01
#define MDIO_WRITE0 (MDIO_DIR)
#define MDIO_WRITE1 (MDIO_DIR | MDIO_DATA_OUT)

#define mdio_delay(mdio_addr) insl(mdio_addr)

static char mii_2_8139_map[8] = {MII_BMCR, MII_BMSR, 0, 0, NWayAdvert, NWayLPAR, NWayExpansion, 0};

// Synchronize the MII management interface by shifting 32 one bits out
static void mdio_sync(uint16_t mdio_addr) {
    int i;

    for (i = 32; i >= 0; i--) {
        outb(mdio_addr, MDIO_WRITE1);
        mdio_delay(mdio_addr);
        outb(mdio_addr, MDIO_WRITE1 | MDIO_CLK);
        mdio_delay(mdio_addr);
    }
}

static int rtl8139_mdio_read(struct rtl8139 *rtl, int phy_id, int location) {
    uint16_t mdio_addr = rtl->iobase + MII_SMI;
    int mii_cmd = (0xf6 << 10) | (phy_id << 5) | location;
    int retval = 0;
    int i;

    if (phy_id > 31) {
        // Really a 8139.  Use internal registers
        return location < 8 && mii_2_8139_map[location] ? insw(rtl->iobase + mii_2_8139_map[location]) : 0;
    }

    mdio_sync(mdio_addr);

    // Shift the read command bits out
    for (i = 14; i >= 0; i--) {
        int dataval = (mii_cmd & (1 << i)) ? MDIO_DATA_OUT : 0;
        outb(mdio_addr, MDIO_DIR | dataval);
        mdio_delay(mdio_addr);
        outb(mdio_addr, MDIO_DIR | dataval | MDIO_CLK);
        mdio_delay(mdio_addr);
    }

    // Read the two transition bits
    for (i = 19; i > 0; i--) {
        outb(mdio_addr, 0);
        mdio_delay(mdio_addr);
        retval = (retval << 1) | ((insb(mdio_addr) & MDIO_DATA_IN) ? 1 : 0);
        outb(mdio_addr, MDIO_CLK);
        mdio_delay(mdio_addr);
    }

    return (retval >> 1) & 0xffff;
}

static void rtl8139_mdio_write(struct rtl8139 *rtl, int phy_id, int location, int value) {
    uint16_t mdio_addr = rtl->iobase + MII_SMI;
    int mii_cmd = (0x5002 << 16) | (phy_id << 23) | (location << 18) | value;
    int i;

    if (phy_id > 31) {
        // Really a 8139.  Use internal registers.
        uint16_t ioaddr = rtl->iobase;
        if (location == 0) {
            outb(ioaddr + Cfg9346, 0xC0);
            outw(ioaddr + MII_BMCR, value);
            outb(ioaddr + Cfg9346, 0x00);
        } else if (location < 8 && mii_2_8139_map[location]) {
            outw(ioaddr + mii_2_8139_map[location], value);
        }
    } else {
        mdio_sync(mdio_addr);

        // Shift the command bits out
        for (i = 31; i >= 0; i--) {
            int dataval = (mii_cmd & (1 << i)) ? MDIO_WRITE1 : MDIO_WRITE0;
            outb(mdio_addr, dataval);
            mdio_delay(mdio_addr);
            outb(mdio_addr, dataval | MDIO_CLK);
            mdio_delay(mdio_addr);
        }

        // Clear out extra bits
        for (i = 2; i > 0; i--) {
            outb(mdio_addr, 0);
            mdio_delay(mdio_addr);
            outb(mdio_addr, MDIO_CLK);
            mdio_delay(mdio_addr);
        }
    }
}

static struct netdev_stats *rtl8139_get_stats(struct netdev *dev) {
    struct rtl8139 *rtl = (struct rtl8139 *)dev->driver_data;
    uint16_t ioaddr = rtl->iobase;

    dev->stats.rx_dropped += insl(ioaddr + RxMissed);
    outl(ioaddr + RxMissed, 0);

    return &dev->stats;
}

// Set or clear the multicast filter

static int rtl8139_set_rx_mode(struct netdev *dev) {
    struct rtl8139 *rtl = (struct rtl8139 *)dev->driver_data;
    uint16_t ioaddr = rtl->iobase;
    uint32_t mc_filter[2];  // Multicast hash filter
    int rx_mode;

    // For now, just accept all packets (simplified for ToyOS)
    // TODO: Implement proper multicast filtering when network stack is ready
    rx_mode = AcceptBroadcast | AcceptMulticast | AcceptMyPhys;
    mc_filter[1] = mc_filter[0] = 0xffffffff;

    // We can safely update without stopping the chip
    outl(ioaddr + RxConfig, rtl->rx_config | rx_mode);
    rtl->mc_filter[0] = mc_filter[0];
    rtl->mc_filter[1] = mc_filter[1];
    outl(ioaddr + MAR0 + 0, mc_filter[0]);
    outl(ioaddr + MAR0 + 4, mc_filter[1]);

    return 0;
}

// Initialize the Rx and Tx rings

static void rtl8139_init_ring(struct rtl8139 *rtl) {
    int i;

    rtl->dirty_tx = rtl->cur_tx = 0;
    rtl->tx_lock.locked = 0;

    for (i = 0; i < NUM_TX_DESC; i++) {
        rtl->tx_bufs[i] = NULL;
        rtl->tx_buffer[i] = &rtl->tx_bufs_mem[i * TX_BUF_SIZE];
    }
}

// Start the hardware at open or resume

static int rtl8139_hw_start(struct rtl8139 *rtl) {
    uint16_t ioaddr = rtl->iobase;
    int i;

    // Soft reset the chip
    outb(ioaddr + ChipCmd, CmdReset);

    // Check that the chip has finished the reset
    for (i = 1000; i > 0; i--) {
        if ((insb(ioaddr + ChipCmd) & CmdReset) == 0)
            break;
        // TODO: Add delay here if needed
    }

    // Restore our idea of the MAC address
    outb(ioaddr + Cfg9346, 0xC0);
    outl(ioaddr + MAC0 + 0, *(uint32_t *)(rtl->netdev->hwaddr.addr + 0));
    outl(ioaddr + MAC0 + 4, *(uint32_t *)(rtl->netdev->hwaddr.addr + 4));

    // Initialize current receive pointer
    rtl->cur_rx = 0;

    // Must enable Tx/Rx before setting transfer thresholds!
    outb(ioaddr + ChipCmd, CmdRxEnb | CmdTxEnb);
    outl(ioaddr + RxConfig, rtl->rx_config);

    // Configure for full-duplex operation
    outl(ioaddr + TxConfig, TX_DMA_BURST << 8);

    // Check for link and duplex
    if (rtl->phys[0] >= 0 || (rtl->flags & HAS_MII_XCVR)) {
        uint16_t mii_reg5 = rtl8139_mdio_read(rtl, rtl->phys[0], 5);
        if (mii_reg5 != 0xffff) {
            if ((mii_reg5 & 0x0100) == 0x0100 || (mii_reg5 & 0x00C0) == 0x0040) {
                rtl->full_duplex = 1;
            }
        }

        printf("%s: Setting %s%s-duplex based on auto-negotiated partner ability %x\n", rtl->netdev->name,
               mii_reg5 == 0         ? ""
               : (mii_reg5 & 0x0180) ? "100mbps "
                                     : "10mbps ",
               rtl->full_duplex ? "full" : "half", mii_reg5);
    }

    if (rtl->flags & HAS_MII_XCVR) {
        // RTL8129 chip
        outb(ioaddr + Config1, rtl->full_duplex ? 0x60 : 0x20);
    }
    outb(ioaddr + Cfg9346, 0x00);

    // Convert virtual address to physical for DMA
    // Note: ToyOS might need a different approach for virtual-to-physical conversion
    outl(ioaddr + RxBuf, (uint32_t)rtl->rx_ring);

    // Start the chip's Tx and Rx process
    outl(ioaddr + RxMissed, 0);
    rtl8139_set_rx_mode(rtl->netdev);
    outb(ioaddr + ChipCmd, CmdRxEnb | CmdTxEnb);

    // Enable all known interrupts by setting the interrupt mask
    uint16_t intr_mask = PCIErr | PCSTimeout | RxUnderrun | RxOverflow | RxFIFOOver | TxErr | TxOK | RxErr | RxOK;
    printf("RTL8139: Attempting to write interrupt mask: 0x%x\n", intr_mask);
    outw(ioaddr + IntrMask, intr_mask);

    // verify
    uint16_t actual_mask = insw(ioaddr + IntrMask);
    printf("RTL8139: Actual interrupt mask after write: 0x%x\n", actual_mask);

    return 0;
}

static void rtl8139_get_mac_address(struct rtl8139 *rtl) {
    uint16_t ioaddr = rtl->iobase;
    int i;

    // Read MAC address from EEPROM
    for (i = 0; i < 3; i++) {
        uint16_t eeprom_data = rtl8139_read_eeprom(ioaddr, i + 7, 8);
        rtl->netdev->hwaddr.addr[i * 2] = eeprom_data & 0xff;
        rtl->netdev->hwaddr.addr[i * 2 + 1] = (eeprom_data >> 8) & 0xff;
    }
}

static int rtl8139_rx(struct rtl8139 *rtl) {
    uint16_t ioaddr = rtl->iobase;
    uint8_t *rx_ring = rtl->rx_ring;
    uint16_t cur_rx = rtl->cur_rx;

    while ((insb(ioaddr + ChipCmd) & RxBufEmpty) == 0) {
        uint32_t ring_offset = cur_rx % rtl->rx_buf_len;
        uint32_t rx_status = *(uint32_t *)(rx_ring + ring_offset);
        uint32_t rx_size = rx_status >> 16;  // Includes the CRC

        if (rx_status & 0x8000) {
            // Error condition
            if (rx_status & (RxBadSymbol | RxRunt | RxTooLong | RxCRCErr | RxBadAlign)) {
                printf("%s: Ethernet frame had errors, status %08x\n", rtl->netdev->name, rx_status);
                rtl->netdev->stats.rx_errors++;
                if (rx_status & (RxBadSymbol | RxBadAlign))
                    rtl->netdev->stats.rx_errors++;
                if (rx_status & (RxRunt | RxTooLong))
                    rtl->netdev->stats.rx_errors++;
                if (rx_status & RxCRCErr)
                    rtl->netdev->stats.rx_crc_errors++;
            }
        } else {
            // Good packet
            int pkt_size = rx_size - 4;
            struct netbuf *netbuf = netbuf_alloc(pkt_size);
            if (netbuf == NULL) {
                printf("%s: Memory squeeze, deferring packet.\n", rtl->netdev->name);
                rtl->netdev->stats.rx_dropped++;
                break;
            }

            if (ring_offset + rx_size > rtl->rx_buf_len) {
                int semi_count = rtl->rx_buf_len - ring_offset - 4;
                memcpy(netbuf->data, &rx_ring[ring_offset + 4], semi_count);
                memcpy((char *)netbuf->data + semi_count, rx_ring, pkt_size - semi_count);
            } else {
                memcpy(netbuf->data, &rx_ring[ring_offset + 4], pkt_size);
            }

            netbuf->len = pkt_size;
            netbuf->total_len = pkt_size;

            // Send packet to upper layer
            if (netdev_rx(rtl->netdev, netbuf) < 0) {
                netbuf_free(netbuf);
            }

            rtl->netdev->stats.rx_bytes += pkt_size;
            rtl->netdev->stats.rx_packets++;
        }

        cur_rx = (cur_rx + rx_size + 4 + 3) & ~3;
        outw(ioaddr + RxBufPtr, cur_rx - 16);
    }

    rtl->cur_rx = cur_rx;
    return 0;
}

static void rtl8139_tx_clear(struct rtl8139 *rtl) {
    uint16_t ioaddr = rtl->iobase;
    uint32_t dirty_tx = rtl->dirty_tx;

    while (rtl->cur_tx - dirty_tx > 0) {
        int entry = dirty_tx % NUM_TX_DESC;
        int txstatus = insl(ioaddr + TxStatus0 + entry * 4);

        if (!(txstatus & (TxStatOK | TxUnderrun | TxAborted))) {
            break;  // Not finished yet
        }

        if (txstatus & (TxOutOfWindow | TxAborted)) {
            // Major error, log it
            printf("%s: Transmit error, Tx status %08x\n", rtl->netdev->name, txstatus);
            rtl->netdev->stats.tx_errors++;
            if (txstatus & TxAborted) {
                rtl->netdev->stats.tx_errors++;
                outl(ioaddr + TxConfig, TX_DMA_BURST << 8);
            }
            if (txstatus & TxCarrierLost)
                rtl->netdev->stats.tx_errors++;
            if (txstatus & TxOutOfWindow)
                rtl->netdev->stats.tx_errors++;
        } else {
            if (txstatus & TxUnderrun) {
                // Add 64 to the Tx FIFO threshold
                if (rtl->tx_flag < 0x00300000)
                    rtl->tx_flag += 0x00020000;
                rtl->netdev->stats.tx_fifo_errors++;
            }
            rtl->netdev->stats.collisions += (txstatus >> 24) & 15;
            rtl->netdev->stats.tx_bytes += txstatus & 0x7ff;
            rtl->netdev->stats.tx_packets++;
        }

        // Free the original buffer
        if (rtl->tx_bufs[entry]) {
            netbuf_free(rtl->tx_bufs[entry]);
            rtl->tx_bufs[entry] = NULL;
        }

        dirty_tx++;
    }

    rtl->dirty_tx = dirty_tx;
}

static void rtl8139_tx_timeout(struct rtl8139 *rtl) {
    uint16_t ioaddr = rtl->iobase;
    int status = insw(ioaddr + IntrStatus);
    int i;

    printf("%s: Transmit timeout, status %x %x media %x\n", rtl->netdev->name, insb(ioaddr + ChipCmd), status,
           insb(ioaddr + GPPinData));

    if (status & (TxOK | RxOK)) {
        printf("%s: RTL8139 Interrupt line blocked, status %x\n", rtl->netdev->name, status);
    }

    // Disable interrupts by clearing the interrupt mask
    outw(ioaddr + IntrMask, 0x0000);

    // Emit info to figure out what went wrong
    printf("%s: Tx queue start entry %i  dirty entry %i\n", rtl->netdev->name, rtl->cur_tx, rtl->dirty_tx);

    for (i = 0; i < NUM_TX_DESC; i++) {
        printf("%s:  Tx descriptor %i is %08x.%s\n", rtl->netdev->name, i, insl(ioaddr + TxStatus0 + i * 4),
               i == rtl->dirty_tx % NUM_TX_DESC ? " (queue head)" : "");
    }

    // Dump the unsent Tx packets
    for (i = 0; i < NUM_TX_DESC; i++) {
        if (rtl->tx_bufs[i]) {
            netbuf_free(rtl->tx_bufs[i]);
            rtl->tx_bufs[i] = NULL;
            rtl->netdev->stats.tx_dropped++;
        }
    }

    rtl8139_hw_start(rtl);

    // Clear tx ring
    rtl->dirty_tx = rtl->cur_tx = 0;
    rtl->tx_timeout_count++;
}

static void rtl8139_interrupt(struct interrupt_frame *frame) {
    printf("RTL8139: INTERRUPT RECEIVED!\n");

    // TODO: For now we need a way to get the device from the interrupt
    // This is a simplified version - normally we'd get the device from interrupt registration

    // For now, find the RTL8139 device (this is a hack until proper interrupt registration)
    struct pci_device *rtl_dev = pci_find_device(RTL8139_VENDOR_ID, RTL8139_DEVICE_ID);
    if (!rtl_dev)
        return;

    // Get the netdev (this assumes only one RTL8139 - also a hack)
    struct netdev *netdev = netdev_get_by_name("eth0");
    if (!netdev)
        return;

    struct rtl8139 *rtl = (struct rtl8139 *)netdev->driver_data;
    if (!rtl)
        return;

    uint16_t ioaddr = rtl->iobase;
    int max_work = rtl->max_interrupt_work;

    while (max_work-- > 0) {
        uint16_t status = insw(ioaddr + IntrStatus);

        if (status == 0 || status == 0xFFFF) {
            break;  // No more interrupts or device removed
        }

        // Acknowledge interrupts
        outw(ioaddr + IntrStatus, status);

        // Handle receive interrupts
        if (status & (RxOK | RxErr)) {
            rtl8139_rx(rtl);
        }

        // Handle transmit interrupts
        if (status & (TxOK | TxErr)) {
            rtl8139_tx_clear(rtl);
        }

        // Handle errors
        if (status & (RxOverflow | RxFIFOOver | RxUnderrun)) {
            printf("%s: RX error, status=0x%x\n", netdev->name, status);
            // TODO: Proper error recovery
        }

        if (status & PCIErr) {
            printf("%s: PCI error, status=0x%x\n", netdev->name, status);
            // TODO: Proper error recovery
        }
    }
}

// Network device operations structure
static struct netdev_ops rtl8139_netdev_ops = {
    .open = rtl8139_open,
    .close = rtl8139_close,
    .transmit = rtl8139_transmit,
    .set_rx_mode = rtl8139_set_rx_mode,
    .get_stats = rtl8139_get_stats,
};

int rtl8139_open(struct netdev *dev) {
    struct rtl8139 *rtl = (struct rtl8139 *)dev->driver_data;
    int rx_buf_len_idx;

    // Register interrupt handler (IRQ numbers start at 0x20)
    idt_register_interrupt_callback(0x20 + rtl->irq, rtl8139_interrupt);
    printf("%s: Registered interrupt handler for IRQ %i (vector 0x%x)\n", dev->name, rtl->irq, 0x20 + rtl->irq);

    // Allocate receive buffer
    rx_buf_len_idx = RX_BUF_LEN_IDX;
    do {
        rtl->rx_buf_len = 8192 << rx_buf_len_idx;
        rtl->rx_ring = kzalloc(rtl->rx_buf_len + 16 + (TX_BUF_SIZE * NUM_TX_DESC));
    } while (rtl->rx_ring == NULL && --rx_buf_len_idx >= 0);

    if (rtl->rx_ring == NULL) {
        printf("%s: Failed to allocate RX buffer\n", dev->name);
        return -1;
    }

    rtl->tx_bufs_mem = (uint8_t *)rtl->rx_ring + rtl->rx_buf_len + 16;

    rtl8139_init_ring(rtl);
    rtl->full_duplex = rtl->duplex_lock;
    rtl->tx_flag = (TX_FIFO_THRESH << 11) & 0x003f0000;
    rtl->rx_config = (RX_FIFO_THRESH << 13) | (rx_buf_len_idx << 11) | (RX_DMA_BURST << 8);

    rtl8139_hw_start(rtl);

    // TODO: Set up timer when timer system is ready
    // init_timer(&rtl->timer, rtl8139_timer, dev);
    // mod_timer(&rtl->timer, get_ticks() + 3 * HZ);

    printf("%s: RTL8139 opened successfully\n", dev->name);
    printf("RTL8139: Device state - Interrupt Status: 0x%x, Chip Command: 0x%x\n", insw(rtl->iobase + IntrStatus),
           insb(rtl->iobase + ChipCmd));
    return 0;
}

int rtl8139_close(struct netdev *dev) {
    struct rtl8139 *rtl = (struct rtl8139 *)dev->driver_data;
    uint16_t ioaddr = rtl->iobase;
    int i;

    printf("%s: Shutting down ethercard, status was 0x%x\n", dev->name, insw(ioaddr + IntrStatus));

    // Disable interrupts by clearing the interrupt mask
    outw(ioaddr + IntrMask, 0x0000);

    // Stop the chip's Tx and Rx DMA processes
    outb(ioaddr + ChipCmd, 0x00);

    // Update the error counts
    dev->stats.rx_dropped += insl(ioaddr + RxMissed);
    outl(ioaddr + RxMissed, 0);

    // TODO: Delete timer when timer system is ready
    // del_timer(&rtl->timer);

    // TODO: Disable IRQ when interrupt system is ready
    // disable_irq(rtl->irq);

    for (i = 0; i < NUM_TX_DESC; i++) {
        if (rtl->tx_bufs[i]) {
            netbuf_free(rtl->tx_bufs[i]);
        }
        rtl->tx_bufs[i] = NULL;
    }

    kfree(rtl->rx_ring);
    rtl->rx_ring = NULL;

    // Green! Put the chip in low-power mode
    outb(ioaddr + Cfg9346, 0xC0);
    outb(ioaddr + Config1, rtl->config1 | 0x03);
    outb(ioaddr + HltClk, 'H');  // 'R' would leave the clock running

    return 0;
}

int rtl8139_transmit(struct netdev *dev, struct netbuf *buf) {
    struct rtl8139 *rtl = (struct rtl8139 *)dev->driver_data;
    uint16_t ioaddr = rtl->iobase;
    int entry;
    uint32_t len = buf->len;

    // TODO: Implement proper synchronization when needed
    // For now, just use a simple check
    if (rtl->cur_tx - rtl->dirty_tx >= NUM_TX_DESC) {
        printf("%s: Transmit queue full, dropping packet\n", dev->name);
        dev->stats.tx_dropped++;
        return -1;
    }

    // Calculate the next Tx descriptor entry
    entry = rtl->cur_tx % NUM_TX_DESC;

    rtl->tx_bufs[entry] = buf;

    // Copy data to transmit buffer (RTL8139 needs contiguous buffer)
    memcpy(rtl->tx_buffer[entry], buf->data, len);

    outl(ioaddr + TxAddr0 + entry * 4, (uint32_t)rtl->tx_buffer[entry]);

    // Note: the chip doesn't have auto-pad!
    outl(ioaddr + TxStatus0 + entry * 4, rtl->tx_flag | (len >= ETH_ZLEN ? len : ETH_ZLEN));

    rtl->trans_start = 0;  // TODO: get current time when timer system is ready
    rtl->cur_tx++;

    dev->stats.tx_packets++;
    dev->stats.tx_bytes += len;

    return 0;
}

int rtl8139_init(struct pci_device *pci_dev) {
    struct rtl8139 *rtl;
    struct netdev *netdev;
    uint16_t iobase;
    uint8_t irq;

    // Get PCI configuration
    iobase = pci_dev->bar[0] & 0xFFFFFFFC;  // I/O base address
    irq = pci_dev->interrupt_line;

    printf("RTL8139: Found at I/O 0x%x, IRQ %i\n", iobase, irq);

    // Enable PCI device (read-modify-write)
    uint32_t cmd = pci_config_read_32(pci_dev->bus, pci_dev->device, pci_dev->function, PCI_COMMAND_OFFSET);
    printf("RTL8139: Original PCI command register: 0x%x\n", cmd);

    // Enable I/O space, bus mastering, and make sure INTX is NOT disabled
    cmd |= PCI_COMMAND_IO | PCI_COMMAND_MASTER;
    cmd &= ~PCI_COMMAND_INTX_DISABLE;  // Clear the INTX disable bit

    pci_config_write_32(pci_dev->bus, pci_dev->device, pci_dev->function, PCI_COMMAND_OFFSET, cmd);

    uint32_t new_cmd = pci_config_read_32(pci_dev->bus, pci_dev->device, pci_dev->function, PCI_COMMAND_OFFSET);
    printf("RTL8139: New PCI command register: 0x%x\n", new_cmd);

    rtl = kzalloc(sizeof(struct rtl8139));
    if (!rtl) {
        printf("RTL8139: Failed to allocate private data\n");
        return -1;
    }

    netdev = netdev_create("eth", &rtl8139_netdev_ops, pci_dev, rtl);
    if (!netdev) {
        printf("RTL8139: Failed to create network device\n");
        kfree(rtl);
        return -1;
    }

    // Initialize private data
    rtl->netdev = netdev;
    rtl->iobase = iobase;
    rtl->irq = irq;
    rtl->flags = RTL8139_CAPS;
    rtl->full_duplex = 0;
    rtl->duplex_lock = 0;
    rtl->max_interrupt_work = max_interrupt_work;
    rtl->multicast_filter_limit = multicast_filter_limit;

    // Initialize PHY addresses
    rtl->phys[0] = 32;  // Use internal registers
    rtl->phys[1] = -1;
    rtl->phys[2] = -1;
    rtl->phys[3] = -1;

    // Read MAC address from EEPROM
    rtl8139_get_mac_address(rtl);

    printf("RTL8139: MAC address %x:%x:%x:%x:%x:%x\n", netdev->hwaddr.addr[0], netdev->hwaddr.addr[1],
           netdev->hwaddr.addr[2], netdev->hwaddr.addr[3], netdev->hwaddr.addr[4], netdev->hwaddr.addr[5]);

    // Set device state
    netdev->state = NETDEV_STATE_DOWN;
    netdev->iobase = iobase;
    netdev->irq = irq;

    printf("RTL8139: Initialized successfully as %s\n", netdev->name);
    return 0;
}

void rtl8139_cleanup(struct rtl8139 *rtl) {
    if (rtl && rtl->netdev) {
        netdev_destroy(rtl->netdev);
        kfree(rtl);
    }
}

static uint32_t rtl8139_ether_crc(int length, uint8_t *data) {
    int crc = -1;

    while (--length >= 0) {
        uint8_t current_octet = *data++;
        int bit;
        for (bit = 0; bit < 8; bit++, current_octet >>= 1) {
            crc = (crc << 1) ^ ((crc < 0) ^ (current_octet & 1) ? 0x04c11db7 : 0);
        }
    }
    return crc;
}

static void rtl8139_set_bit(uint32_t *filter, int bitnum) {
    filter[bitnum >> 5] |= (1 << (bitnum & 31));
}
