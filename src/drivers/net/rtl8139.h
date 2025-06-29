#ifndef __RTL8139_H__
#define __RTL8139_H__

// ToyOS includes
#include "drivers/pci/pci.h"
#include "idt/idt.h"
#include "io/io.h"
#include "locks/spinlock.h"
#include "memory/heap/kheap.h"
#include "stdlib/printf.h"
#include "stdlib/string.h"
#include "sys/net/netdev.h"
#include <stddef.h>
#include <stdint.h>

// RTL8139 specific constants
#define NUM_TX_DESC 4
#define ETH_ZLEN 60  // Min. octets in frame sans FCS

// RTL8139 PCI identification
#define RTL8139_VENDOR_ID 0x10EC
#define RTL8139_DEVICE_ID 0x8139

// Receive buffer sizes
#define RX_BUF_LEN_IDX 2  // 0=8K, 1=16K, 2=32K, 3=64K
#define TX_BUF_SIZE 1536

// PCI Tuning Parameters
#define TX_FIFO_THRESH 256  // In bytes, rounded down to 32 byte units
#define RX_FIFO_THRESH 4    // Rx buffer level before first PCI xfer
#define RX_DMA_BURST 4      // Maximum PCI burst, '4' is 256 bytes
#define TX_DMA_BURST 4      // Calculate as 16 << val

// Timeout values
#define TX_TIMEOUT_TICKS 200  // Simplified timeout (no HZ conversion)

// Board capability flags
enum board_capability_flags { HAS_MII_XCVR = 0x01, HAS_CHIP_XCVR = 0x02, HAS_LNK_CHNG = 0x04, HAS_DESC = 0x08 };

// Symbolic offsets to registers
enum RTL8139_registers {
    MAC0 = 0x00,       // Ethernet hardware address
    MAR0 = 0x08,       // Multicast filter
    TxStatus0 = 0x10,  // Transmit status (Four 32bit registers)
    TxAddr0 = 0x20,    // Tx descriptors (also four 32bit)
    RxBuf = 0x30,
    RxEarlyCnt = 0x34,
    RxEarlyStatus = 0x36,
    ChipCmd = 0x37,
    RxBufPtr = 0x38,
    RxBufAddr = 0x3A,
    IntrMask = 0x3C,
    IntrStatus = 0x3E,
    TxConfig = 0x40,
    RxConfig = 0x44,
    Timer = 0x48,     // A general-purpose counter
    RxMissed = 0x4C,  // 24 bits valid, write clears
    Cfg9346 = 0x50,
    Config0 = 0x51,
    Config1 = 0x52,
    FlashReg = 0x54,
    GPPinData = 0x58,
    GPPinDir = 0x59,
    MII_SMI = 0x5A,
    HltClk = 0x5B,
    MultiIntr = 0x5C,
    TxSummary = 0x60,
    MII_BMCR = 0x62,
    MII_BMSR = 0x64,
    NWayAdvert = 0x66,
    NWayLPAR = 0x68,
    NWayExpansion = 0x6A,

    // Undocumented registers, but required for proper operation
    FIFOTMS = 0x70,  // FIFO Control and test
    CSCR = 0x74,     // Chip Status and Configuration Register
    PARA78 = 0x78,
    PARA7c = 0x7c,  // Magic transceiver parameter register
};

// Chip command bits
enum ChipCmdBits {
    RxBufEmpty = 0x01,
    CmdTxEnb = 0x04,
    CmdRxEnb = 0x08,
    CmdReset = 0x10,
};

// Interrupt register bits
enum IntrStatusBits {
    RxOK = 0x0001,
    RxErr = 0x0002,
    TxOK = 0x0004,
    TxErr = 0x0008,
    RxOverflow = 0x0010,
    RxUnderrun = 0x0020,
    RxFIFOOver = 0x0040,
    PCSTimeout = 0x4000,
    PCIErr = 0x8000,
};

// Transmit status bits
enum TxStatusBits {
    TxHostOwns = 0x00002000,
    TxUnderrun = 0x00004000,
    TxStatOK = 0x00008000,
    TxOutOfWindow = 0x20000000,
    TxAborted = 0x40000000,
    TxCarrierLost = 0x80000000,
};

// Receive status bits
enum RxStatusBits {
    RxStatusOK = 0x0001,
    RxBadAlign = 0x0002,
    RxCRCErr = 0x0004,
    RxTooLong = 0x0008,
    RxRunt = 0x0010,
    RxBadSymbol = 0x0020,
    RxBroadcast = 0x2000,
    RxPhysical = 0x4000,
    RxMulticast = 0x8000,
};

// Bits in RxConfig
enum RxConfigBits {
    AcceptAllPhys = 0x01,
    AcceptMyPhys = 0x02,
    AcceptMulticast = 0x04,
    AcceptRunt = 0x10,
    AcceptErr = 0x20,
    AcceptBroadcast = 0x08,
};

// CSCR bits
enum CSCRBits {
    CSCR_LinkOKBit = 0x00400,
    CSCR_LinkDownOffCmd = 0x003c0,
    CSCR_LinkChangeBit = 0x00800,
    CSCR_LinkStatusBits = 0x0f000,
    CSCR_LinkDownCmd = 0x0f3c0,
};

// forward declaration
struct rtl8139;

int rtl8139_init(struct pci_device *pci_dev);
void rtl8139_cleanup(struct rtl8139 *rtl);
int rtl8139_open(struct netdev *dev);
int rtl8139_close(struct netdev *dev);
int rtl8139_transmit(struct netdev *dev, struct netbuf *buf);

#endif
