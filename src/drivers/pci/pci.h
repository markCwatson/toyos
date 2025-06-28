#ifndef _PCI_H_
#define _PCI_H_

#include <stddef.h>
#include <stdint.h>

// PCI Command Register Bits
#define PCI_COMMAND_OFFSET 0x04          // Command register offset
#define PCI_COMMAND_IO 0x0001            // Enable I/O space
#define PCI_COMMAND_MEMORY 0x0002        // Enable memory space
#define PCI_COMMAND_MASTER 0x0004        // Enable bus mastering
#define PCI_COMMAND_INTX_DISABLE 0x0400  // Disable INTx interrupts

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

/**
 * @brief Find a PCI device by vendor and device ID
 *
 * @param vendor_id Vendor ID to search for
 * @param device_id Device ID to search for
 * @return Pointer to device if found, NULL otherwise
 */
struct pci_device *pci_find_device(uint16_t vendor_id, uint16_t device_id);

/**
 * @brief Find devices by class code
 *
 * @param class_code PCI class code to search for
 * @param devices Array to store found devices
 * @param max_devices Maximum number of devices to return
 * @return Number of devices found
 */
int pci_find_devices_by_class(uint8_t class_code, struct pci_device *devices, int max_devices);

#endif