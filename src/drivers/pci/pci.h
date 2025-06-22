#ifndef _PCI_H_
#define _PCI_H_

#include <stddef.h>
#include <stdint.h>

// PCI Configuration Space I/O Ports
#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA 0xCFC

// PCI Configuration Space Offsets
#define PCI_VENDOR_ID 0x00
#define PCI_DEVICE_ID 0x02
#define PCI_COMMAND 0x04
#define PCI_STATUS 0x06
#define PCI_REVISION_ID 0x08
#define PCI_CLASS_CODE 0x0B
#define PCI_HEADER_TYPE 0x0E
#define PCI_BAR0 0x10
#define PCI_BAR1 0x14
#define PCI_BAR2 0x18
#define PCI_BAR3 0x1C
#define PCI_BAR4 0x20
#define PCI_BAR5 0x24
#define PCI_INTERRUPT_LINE 0x3C
#define PCI_INTERRUPT_PIN 0x3D

// PCI Command Register Bits
#define PCI_COMMAND_IO 0x0001            // Enable I/O space
#define PCI_COMMAND_MEMORY 0x0002        // Enable memory space
#define PCI_COMMAND_MASTER 0x0004        // Enable bus mastering
#define PCI_COMMAND_INTX_DISABLE 0x0400  // Disable INTx interrupts

// Special Values
#define PCI_INVALID_VENDOR 0xFFFF  // Invalid vendor ID
#define PCI_MAX_BUS 256            // Maximum number of buses
#define PCI_MAX_DEVICE 32          // Maximum devices per bus
#define PCI_MAX_FUNCTION 8         // Maximum functions per device

// Target Device IDs
#define RTL8139_VENDOR_ID 0x10EC  // RealTek
#define RTL8139_DEVICE_ID 0x8139  // RTL8139 Fast Ethernet

/**
 * @brief PCI device information structure
 *
 * This structure holds the basic information about a PCI device
 * that we discover during enumeration.
 */
struct pci_device {
    uint8_t bus;       // PCI bus number (0-255)
    uint8_t device;    // PCI device number (0-31)
    uint8_t function;  // PCI function number (0-7)

    uint16_t vendor_id;  // Vendor ID (e.g., 0x10EC for RealTek)
    uint16_t device_id;  // Device ID (e.g., 0x8139 for RTL8139)
    uint16_t command;    // Command register
    uint16_t status;     // Status register

    uint8_t revision_id;  // Revision ID
    uint8_t prog_if;      // Programming interface
    uint8_t subclass;     // Device subclass
    uint8_t class_code;   // Device class code

    uint8_t header_type;     // Header type (0x00 for normal devices)
    uint8_t interrupt_line;  // Interrupt line
    uint8_t interrupt_pin;   // Interrupt pin

    uint32_t bar[6];  // Base Address Registers (I/O ports, memory addresses)
};

/**
 * @brief PCI Configuration Space Header (Type 0)
 *
 * This represents the first 64 bytes of PCI configuration space
 * for a normal device (header type 0x00).
 */
struct pci_config_header {
    uint16_t vendor_id;         // 0x00
    uint16_t device_id;         // 0x02
    uint16_t command;           // 0x04
    uint16_t status;            // 0x06
    uint8_t revision_id;        // 0x08
    uint8_t prog_if;            // 0x09
    uint8_t subclass;           // 0x0A
    uint8_t class_code;         // 0x0B
    uint8_t cache_line_size;    // 0x0C
    uint8_t latency_timer;      // 0x0D
    uint8_t header_type;        // 0x0E
    uint8_t bist;               // 0x0F
    uint32_t bar0;              // 0x10
    uint32_t bar1;              // 0x14
    uint32_t bar2;              // 0x18
    uint32_t bar3;              // 0x1C
    uint32_t bar4;              // 0x20
    uint32_t bar5;              // 0x24
    uint32_t cardbus_cis;       // 0x28
    uint16_t subsystem_vendor;  // 0x2C
    uint16_t subsystem_device;  // 0x2E
    uint32_t expansion_rom;     // 0x30
    uint8_t capabilities;       // 0x34
    uint8_t reserved[7];        // 0x35-0x3B
    uint8_t interrupt_line;     // 0x3C
    uint8_t interrupt_pin;      // 0x3D
    uint8_t min_grant;          // 0x3E
    uint8_t max_latency;        // 0x3F
} __attribute__((packed));

/**
 * @brief PCI configuration address format
 *
 * This structure represents the address format used for PCI
 * configuration space access via port 0xCF8.
 */
union pci_config_address {
    uint32_t raw;
    struct {
        uint32_t offset : 8;    // Register offset (bits 0-7)
        uint32_t function : 3;  // Function number (bits 8-10)
        uint32_t device : 5;    // Device number (bits 11-15)
        uint32_t bus : 8;       // Bus number (bits 16-23)
        uint32_t reserved : 7;  // Reserved (bits 24-30)
        uint32_t enable : 1;    // Enable bit (bit 31)
    } __attribute__((packed));
};

/**
 * @brief Enumerate all PCI devices
 *
 * This function scans all PCI buses, devices, and functions to discover
 * available PCI devices in the system. Found devices are printed to
 * the console for debugging.
 *
 * @return Number of devices found
 */
int pci_enumerate_devices(void);

/**
 * @brief Read a 32-bit value from PCI configuration space
 *
 * @param bus PCI bus number
 * @param device PCI device number
 * @param function PCI function number
 * @param offset Register offset (must be 4-byte aligned)
 * @return 32-bit value from configuration space
 */
uint32_t pci_config_read_32(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);

/**
 * @brief Write a 32-bit value to PCI configuration space
 *
 * @param bus PCI bus number
 * @param device PCI device number
 * @param function PCI function number
 * @param offset Register offset (must be 4-byte aligned)
 * @param value 32-bit value to write
 */
void pci_config_write_32(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value);

#endif