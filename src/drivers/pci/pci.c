#include "pci.h"
#include "drivers/net/rtl8139.h"
#include "io/io.h"
#include "kernel.h"
#include "stdlib/printf.h"

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

// Special Values
#define PCI_INVALID_VENDOR 0xFFFF  // Invalid vendor ID
#define PCI_MAX_BUS 256            // Maximum number of buses
#define PCI_MAX_DEVICE 32          // Maximum devices per bus
#define PCI_MAX_FUNCTION 8         // Maximum functions per device

// Target Device IDs
#define RTL8139_VENDOR_ID 0x10EC  // RealTek
#define RTL8139_DEVICE_ID 0x8139  // RTL8139 Fast Ethernet

// Global device registry
#define PCI_MAX_DEVICES 16

/**
 * @brief PCI class code names {O(1)} lookup table
 */
static const char *pci_class_names[] = {
    "Unclassified",             // 0x00
    "Mass Storage",             // 0x01
    "Network",                  // 0x02
    "Display",                  // 0x03
    "Multimedia",               // 0x04
    "Memory",                   // 0x05
    "Bridge",                   // 0x06
    "Communication",            // 0x07
    "System Peripheral",        // 0x08
    "Input Device",             // 0x09
    "Docking Station",          // 0x0A
    "Processor",                // 0x0B
    "Serial Bus",               // 0x0C
    "Wireless",                 // 0x0D
    "Intelligent I/O",          // 0x0E
    "Satellite Communication",  // 0x0F
    "Encryption/Decryption",    // 0x10
    "Data Acquisition"          // 0x11
};

#define PCI_CLASS_NAMES_COUNT (sizeof(pci_class_names) / sizeof(pci_class_names[0]))

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

// Global variables for PCI devices
struct pci_device pci_devices[PCI_MAX_DEVICES];
int pci_device_count = 0;

/**
 * @brief Get class name string for display
 *
 * This function returns a string describing the device class based on
 * the provided class code. It uses a static lookup table to map class
 * codes to human-readable names.
 *
 * @param class_code PCI class code
 * @return String describing the device class
 */
static const char *pci_get_class_name(uint8_t class_code) {
    if (class_code < PCI_CLASS_NAMES_COUNT) {
        return pci_class_names[class_code];
    }
    return "Unknown";
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

int pci_enumerate_devices(void) {
    struct pci_device device;
    pci_device_count = 0;

    printf("Enumerating PCI devices...\n");

    for (int bus = 0; bus < PCI_MAX_BUS; bus++) {
        for (int dev = 0; dev < PCI_MAX_DEVICE; dev++) {
            // Check function 0 first
            if (pci_read_device_info(bus, dev, 0, &device) == 0) {
                pci_devices[pci_device_count] = device;
                pci_device_count++;

                printf("PCI %x:%x.%x - %x:%x (%s)\n", device.bus, device.device, device.function, device.vendor_id,
                       device.device_id, pci_get_class_name(device.class_code));

                if (device.vendor_id == RTL8139_VENDOR_ID && device.device_id == RTL8139_DEVICE_ID) {
                    printf("    Initializing RTL8139 driver...\n");
                    if (rtl8139_init(&pci_devices[pci_device_count - 1]) == 0) {
                        printf("    RTL8139 driver initialized successfully\n");
                    } else {
                        printf("    RTL8139 driver initialization failed\n");
                    }
                }

                // todo: handle multifunction device, check other functions
            }
        }
    }

    printf("PCI enumeration complete. Found %i devices.\n", pci_device_count);
    return pci_device_count;
}

struct pci_device *pci_find_device(uint16_t vendor_id, uint16_t device_id) {
    for (int i = 0; i < pci_device_count; i++) {
        if (pci_devices[i].vendor_id == vendor_id && pci_devices[i].device_id == device_id) {
            return &pci_devices[i];
        }
    }
    return NULL;  // Not found
}

int pci_find_devices_by_class(uint8_t class_code, struct pci_device *devices, int max_devices) {
    int found = 0;
    for (int i = 0; i < pci_device_count && found < max_devices; i++) {
        if (pci_devices[i].class_code == class_code) {
            devices[found] = pci_devices[i];
            found++;
        }
    }
    return found;
}
