#include "pci.h"
#include "io/io.h"
#include "kernel.h"
#include "stdlib/printf.h"

/**
 * @brief Read a 32-bit value from PCI configuration space
 *
 * This function constructs the proper PCI configuration address and
 * reads the 32-bit value from the specified configuration register.
 *
 * @param bus PCI bus number (0-255)
 * @param device PCI device number (0-31)
 * @param function PCI function number (0-7)
 * @param offset Register offset (must be 4-byte aligned)
 * @return 32-bit value from configuration space
 */
uint32_t pci_config_read_32(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    union pci_config_address addr;

    // Construct the PCI configuration address
    addr.raw = 0;
    addr.enable = 1;
    addr.bus = bus;
    addr.device = device;
    addr.function = function;
    addr.offset = offset & 0xFC;  // Ensure 4-byte alignment

    // Write the address to the PCI configuration address port
    outl(PCI_CONFIG_ADDRESS, addr.raw);

    // Read the data from the PCI configuration data port
    return insl(PCI_CONFIG_DATA);
}

/**
 * @brief Write a 32-bit value to PCI configuration space
 *
 * This function constructs the proper PCI configuration address and
 * writes the 32-bit value to the specified configuration register.
 *
 * @param bus PCI bus number (0-255)
 * @param device PCI device number (0-31)
 * @param function PCI function number (0-7)
 * @param offset Register offset (must be 4-byte aligned)
 * @param value 32-bit value to write
 */
void pci_config_write_32(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value) {
    union pci_config_address addr;

    // Construct the PCI configuration address
    addr.raw = 0;
    addr.enable = 1;
    addr.bus = bus;
    addr.device = device;
    addr.function = function;
    addr.offset = offset & 0xFC;  // Ensure 4-byte alignment

    // Write the address to the PCI configuration address port
    outl(PCI_CONFIG_ADDRESS, addr.raw);

    // Write the data to the PCI configuration data port
    outl(PCI_CONFIG_DATA, value);
}

/**
 * @brief Read PCI device information
 *
 * This function reads the basic device information from PCI configuration
 * space and populates a pci_device structure.
 *
 * @param bus PCI bus number
 * @param device PCI device number
 * @param function PCI function number
 * @param dev Pointer to pci_device structure to populate
 * @return 0 on success, -1 if device not present
 */
static int pci_read_device_info(uint8_t bus, uint8_t device, uint8_t function, struct pci_device *dev) {
    uint32_t vendor_device;
    uint32_t class_info;
    uint32_t misc_info;
    uint32_t interrupt_info;

    // Read vendor and device ID
    vendor_device = pci_config_read_32(bus, device, function, PCI_VENDOR_ID);

    // Check if device is present (vendor ID 0xFFFF means no device)
    if ((vendor_device & 0xFFFF) == PCI_INVALID_VENDOR) {
        return -1;
    }

    // Fill in basic location info
    dev->bus = bus;
    dev->device = device;
    dev->function = function;

    // Extract vendor and device ID
    dev->vendor_id = vendor_device & 0xFFFF;
    dev->device_id = (vendor_device >> 16) & 0xFFFF;

    // Read command and status registers
    uint32_t cmd_status = pci_config_read_32(bus, device, function, PCI_COMMAND);
    dev->command = cmd_status & 0xFFFF;
    dev->status = (cmd_status >> 16) & 0xFFFF;

    // Read class code information
    class_info = pci_config_read_32(bus, device, function, PCI_REVISION_ID);
    dev->revision_id = class_info & 0xFF;
    dev->prog_if = (class_info >> 8) & 0xFF;
    dev->subclass = (class_info >> 16) & 0xFF;
    dev->class_code = (class_info >> 24) & 0xFF;

    // Read header type and other info
    misc_info = pci_config_read_32(bus, device, function, 0x0C);
    dev->header_type = (misc_info >> 16) & 0xFF;

    // Read interrupt information
    interrupt_info = pci_config_read_32(bus, device, function, PCI_INTERRUPT_LINE);
    dev->interrupt_line = interrupt_info & 0xFF;
    dev->interrupt_pin = (interrupt_info >> 8) & 0xFF;

    // Read Base Address Registers (BARs)
    for (int i = 0; i < 6; i++) {
        dev->bar[i] = pci_config_read_32(bus, device, function, PCI_BAR0 + (i * 4));
    }

    return 0;
}

/**
 * @brief Get class name string for display
 *
 * @param class_code PCI class code
 * @return String describing the device class
 */
static const char *pci_get_class_name(uint8_t class_code) {
    switch (class_code) {
    case 0x00:
        return "Unclassified";
    case 0x01:
        return "Mass Storage";
    case 0x02:
        return "Network";
    case 0x03:
        return "Display";
    case 0x04:
        return "Multimedia";
    case 0x05:
        return "Memory";
    case 0x06:
        return "Bridge";
    case 0x07:
        return "Communication";
    case 0x08:
        return "System Peripheral";
    case 0x09:
        return "Input Device";
    case 0x0A:
        return "Docking Station";
    case 0x0B:
        return "Processor";
    case 0x0C:
        return "Serial Bus";
    case 0x0D:
        return "Wireless";
    case 0x0E:
        return "Intelligent I/O";
    case 0x0F:
        return "Satellite Communication";
    case 0x10:
        return "Encryption/Decryption";
    case 0x11:
        return "Data Acquisition";
    default:
        return "Unknown";
    }
}

/**
 * @brief Enumerate all PCI devices
 *
 * This function scans all possible PCI bus/device/function combinations
 * to discover available PCI devices. It prints information about each
 * device found and specifically highlights the RTL8139 network card.
 *
 * @return Number of devices found
 */
int pci_enumerate_devices(void) {
    struct pci_device device;
    int device_count = 0;

    printf("Enumerating PCI devices...\n");

    for (int bus = 0; bus < PCI_MAX_BUS; bus++) {
        for (int dev = 0; dev < PCI_MAX_DEVICE; dev++) {
            // Check function 0 first
            if (pci_read_device_info(bus, dev, 0, &device) == 0) {
                device_count++;

                if (device.vendor_id == RTL8139_VENDOR_ID && device.device_id == RTL8139_DEVICE_ID) {
                    printf("*** Found RTL8139 Network Card! ***\n");
                    printf("    I/O Base: 0x%08x\n", device.bar[0] & 0xFFFFFFFC);
                    printf("    IRQ Line: %i\n", device.interrupt_line);
                }

                // todo: handle multifunction device, check other functions
            }
        }
    }

    printf("PCI enumeration complete. Found %d devices.\n", device_count);
    return device_count;
}
