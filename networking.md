# WIP - ToyOS Networking Implementation Plan

## Table of Contents

1. [Overview](#overview)
2. [Architecture](#architecture)
3. [Phase 1: PCI Enumeration](#phase-1-pci-enumeration)
4. [Phase 2: RTL8139 Driver](#phase-2-rtl8139-driver)
5. [Phase 3: Network Stack](#phase-3-network-stack)
6. [Phase 4: System Call Interface](#phase-4-system-call-interface)
7. [Phase 5: User Space Integration](#phase-5-user-space-integration)
8. [Phase 6: Testing and Validation](#phase-6-testing-and-validation)
9. [References](#references)

---

## Overview

This document provides a rough (and WIP) plan (which is subject to change) for implementing UDP networking capabilities in ToyOS using the RTL8139 network interface controller. The implementation follows a layered approach, building from hardware detection through a complete UDP networking stack, attempting to follow an OSI-like model.

### Goals

- **Primary**: Enable UDP packet transmission and reception
- **Secondary**: Create reusable networking infrastructure
- **Educational**: learn and demonstrate fundamental networking concepts

### Target Hardware

**RTL8139 Network Interface Controller**

- **Vendor ID**: 0x10EC (RealTek)
- **Device ID**: 0x8139
- **Bus Type**: PCI
- **Speeds**: 10/100 Mbps Ethernet
- **Features**: DMA-based packet processing, interrupt-driven operation

### Why RTL8139?

1. **Simplicity**: Fewer registers and simpler DMA model than modern NICs
2. **Documentation**: Well-documented with available datasheets
3. **Emulation**: Supported in QEMU
4. **Educational Value**: Good balance of complexity and functionality
5. **Legacy Support**: Understanding helps with newer hardware
6. **existing driver example**: see refs

---

## Architecture

### Network Stack Layers

```
┌─────────────────────────────────────────┐
│           User Applications             │
│        (ping, echo server, etc.)        │
├─────────────────────────────────────────┤
│         Socket API (BSD-style)          │
│     (socket, bind, sendto, recvfrom)    │
├─────────────────────────────────────────┤
│         System Call Interface           │
│    (INT 0x80 with network commands)     │
├─────────────────────────────────────────┤
│              UDP Layer                  │
│        (Port multiplexing)              │
├─────────────────────────────────────────┤
│              IP Layer                   │
│       (Internet addressing)             │
├─────────────────────────────────────────┤
│              ARP Layer                  │
│      (Address resolution)               │
├─────────────────────────────────────────┤
│            Ethernet Layer               │
│         (Frame handling)                │
├─────────────────────────────────────────┤
│           RTL8139 Driver                │
│        (Hardware abstraction)           │
├─────────────────────────────────────────┤
│             PCI Subsystem               │
│        (Device enumeration)             │
├─────────────────────────────────────────┤
│           Hardware (RTL8139)            │
└─────────────────────────────────────────┘
```

### Data Flow

**Outbound (Transmit)**:

1. User calls `sendto()`
2. System call marshalls data to kernel
3. UDP layer adds UDP header
4. IP layer adds IP header and handles routing
5. ARP resolves destination MAC address
6. Ethernet layer creates frame
7. RTL8139 driver transmits via DMA

**Inbound (Receive)**:

1. RTL8139 receives frame via DMA
2. Interrupt handler processes packet
3. Ethernet layer parses frame
4. IP layer validates and routes packet
5. UDP layer demultiplexes by port
6. Data queued for user application
7. User calls `recvfrom()` to retrieve data

---

## Phase 1: PCI Enumeration

**What is PCI?**
The Peripheral Component Interconnect (PCI) bus is a standard for connecting peripheral devices to a computer's motherboard. Unlike older buses that used fixed addresses, PCI devices are detected and configured dynamically.

**Why PCI Enumeration?**

- Modern systems can have hundreds of PCI devices
- Each device has a unique vendor/device ID combination
- Network cards are PCI devices with configurable addresses
- Configuration space contains resource requirements (I/O ports, memory, IRQ)

### Implementation Plan

#### Step 1.1: PCI Configuration Space Access

**Requirements:**

- Extend I/O subsystem with 32-bit operations (`insl` and `outl`)
- Define PCI configuration space constants (ports 0xCF8/0xCFC, register offsets)
- Create PCI data structures:
  - `pci_device` struct holds essential information about each device
  - `pci_config_header` gives exact layout of the configuration space
  - `pci_config_address` union makes it easy to construct addresses for configuration space access

**Implementation:**

- `pci_config_read_32()` - constructs address, writes to 0xCF8, reads from 0xCFC
- `pci_config_write_32()` - constructs address, writes to 0xCF8, writes to 0xCFC
- Proper bit field handling for PCI address format (enable bit, bus/device/function encoding)

#### Step 1.2: PCI Device Enumeration

**Strategy:**

- Brute force scan all possible bus/device/function combinations
- Check vendor ID to determine if device exists (0xFFFF = no device)
- Read essential device information into `pci_device` structure
- Handle multifunction devices (check header type bit 7)
- Specifically watch for RTL8139 (vendor 0x10EC, device 0x8139)

**Implementation:**

- `pci_read_device_info()` - populates device structure from config space
- `pci_enumerate_devices()` - main enumeration loop
- `pci_get_class_name()` - translates class codes to human-readable names
- Triple nested loop: buses (0-255) -> devices (0-31) -> functions (0-7)
- Special handling for RTL8139 detection with I/O base and IRQ reporting

### Notes

**PCI Configuration Mechanism**:

- Uses two 32-bit I/O ports: 0xCF8 (address) and 0xCFC (data)
- Address format encodes bus, device, function, and register offset
- Allows access to 256 bytes of configuration space per function
- The PCI configuration space layout is fixed by specification:

| Offset  | Size | Register Name                  |
| ------- | ---- | ------------------------------ |
| 0x00    | 16   | Vendor ID                      |
| 0x02    | 16   | Device ID                      |
| 0x04    | 16   | Command                        |
| 0x06    | 16   | Status                         |
| 0x08    | 8    | Revision ID                    |
| 0x09    | 8    | Programming Interface          |
| 0x0A    | 8    | Subclass                       |
| 0x0B    | 8    | Class Code                     |
| 0x0C    | 8    | Cache Line Size                |
| 0x0D    | 8    | Latency Timer                  |
| 0x0E    | 8    | Header Type                    |
| 0x0F    | 8    | BIST                           |
| 0x10    | 32   | Base Address Register 0 (BAR0) |
| 0x14    | 32   | Base Address Register 1 (BAR1) |
| 0x18    | 32   | Base Address Register 2 (BAR2) |
| 0x1C    | 32   | Base Address Register 3 (BAR3) |
| 0x20    | 32   | Base Address Register 4 (BAR4) |
| 0x24    | 32   | Base Address Register 5 (BAR5) |
| 0x28    | 32   | Cardbus CIS Pointer            |
| 0x2C    | 16   | Subsystem Vendor ID            |
| 0x2E    | 16   | Subsystem Device ID            |
| 0x30    | 32   | Expansion ROM Base Address     |
| 0x34    | 8    | Capabilities Pointer           |
| 0x35-3B | 7    | Reserved                       |
| 0x3C    | 8    | Interrupt Line                 |
| 0x3D    | 8    | Interrupt Pin                  |
| 0x3E    | 8    | Min Grant                      |
| 0x3F    | 8    | Max Latency                    |

**Bus Mastering**:

- Required for DMA operations
- Allows device to initiate memory transfers without CPU intervention
- Critical for network card performance

---

## Phase 2: RTL8139 Driver

**RTL8139 Hardware Overview**
The RTL8139 is a PCI Fast Ethernet controller that implements:

- **DMA Engine**: Direct memory access for packet transfers
- **Ring Buffers**: Circular buffers for efficient packet handling
- **Interrupt System**: Hardware notifications for events
- **Configuration Registers**: Control device behavior

**Key Concepts**:

1. **DMA (Direct Memory Access)**: Hardware transfers data directly to/from memory
2. **Ring Buffers**: Circular data structures that wrap around when full
3. **Memory Mapping**: Device registers accessible via I/O ports or memory addresses
4. **Interrupt Handling**: Asynchronous notifications from hardware

### Implementation Plan

#### Step 2.1: ToyOS Network Device Abstraction

The original RTL8139 driver was designed for a different OS (sanos) with a `struct dev` abstractions. We will create a ToyOS-specific network device abstraction that provides equivalent functionality while integrating with toyos THis shows how specialized subsystems work in real operating systems (Linux has both `struct device` and `struct net_device`).

- `src/sys/net/netdev.h` - Network device abstraction header
- `src/sys/net/netdev.c` - Network device management implementation

**Integration:**
- Direct integration with toyos PCI subsystem
- Uses toyos memory management (`kzalloc`/`kfree`)
- Compatible with ToyOS I/O system (`insl`/`outl` functions)
- Designed for future network stack integration

#### Step 2.2: RTL8139 Driver Structure Adaptation

**Mapping Original Driver to ToyOS:**

| Original Driver | ToyOS Equivalent | Purpose |
|----------------|------------------|----------|
| `struct dev` | `struct netdev` | Device representation |
| `struct pbuf` | `struct netbuf` | Packet buffers |
| `dev->privdata` | `netdev->driver_data` | Private driver data |
| `dev->name` | `netdev->name` | Device name |
| `dev_receive()` | `netdev_rx()` | Receive packet handling |

**Required Changes:**
1. Replace `#include <os/krnl.h>` with ToyOS headers
2. Update `struct nic` to work with `struct netdev` instead of `struct dev`
3. Map I/O function names (e.g., `outp()` → `outb()`, `inpw()` → `insw()`)
4. Replace missing functions with ToyOS equivalents

#### Step 2.3: RTL8139 Hardware Definitions

**Register Definitions:**
- RTL8139 register offsets and bit masks
- Ethernet frame format constants
- DMA configuration parameters
- Interrupt status and control bits

**Files:**
- Update `src/drivers/net/rtl8129.h` with ToyOS compatibility

#### Step 2.4: Core Driver Implementation

Based on the reference implementation from the [sanos RTL8139 driver](http://www.jbox.dk/sanos/source/sys/dev/rtl8139.c.html)

**Key Functions to Adapt:**
- `rtl8139_open()` - Initialize hardware and allocate buffers
- `rtl8139_close()` - Shutdown hardware and free resources
- `rtl8139_transmit()` - Send packets via DMA
- `rtl8139_rx()` - Receive packet processing
- `rtl8139_interrupt()` - Interrupt service routine

**ToyOS-Specific Adaptations:**
1. **Memory Management**: Use `kzalloc()` instead of `alloc_pages_linear()`
2. **Interrupt Handling**: Integrate with ToyOS interrupt system
3. **Timer Management**: Implement or stub timer functions
4. **DPC (Deferred Procedure Call)**: Replace with ToyOS equivalent or simplify

#### Step 2.5: Driver Registration and Initialization

**Driver Detection:**
- Integrate with existing PCI enumeration in `pci_enumerate_devices()`
- Detect RTL8139 cards (vendor 0x10EC, device 0x8139)
- Create `struct netdev` for each detected card

**Initialization Sequence:**
1. PCI enumeration finds RTL8139 device
2. Driver creates `struct netdev` via `netdev_create()`
3. Driver allocates private `struct nic` data
4. Hardware initialization (reset, MAC address reading, buffer allocation)
5. Device registered and ready for network stack integration

**Integration with Kernel:**
- Add driver initialization to kernel startup sequence
- Register interrupt handlers
- Make device available to network stack (future phases)

### Notes

**DMA and Buffer Management**:

- RTL8139 uses DMA to transfer packets directly to/from memory
- Receive buffer is a ring buffer that wraps around when full
- Transmit uses 4 descriptors in round-robin fashion for better throughput

**Interrupt Handling**:

- Must acknowledge interrupts by writing to ISR register
- Critical to acknowledge BEFORE processing packets in QEMU
- Different interrupts indicate different events (RX, TX, errors)

**Memory Considerations**:

- Buffers must be physically contiguous
- Physical addresses required for DMA programming
- Proper alignment important for performance

---

## Phase 3: Network Stack

The network stack implements a layered architecture where each layer provides services to the layer above and uses services from the layer below. This separation of concerns makes the system more maintainable and allows protocols to be developed independently.

Data flow:
```
Packet arrives → RTL8139 hardware → Interrupt → rtl8139_interrupt() → rtl8139_rx() → netdev_rx() → ethernet_rx() → [Protocol layers]
```

### Step 3.1: Ethernet Layer Implementation

- `src/sys/net/ethernet.h` - Ethernet header structure definitions  
- `src/sys/net/ethernet.c` - Basic frame parsing and protocol demultiplexing
- Integration with netdev layer (`netdev_rx()` calls `ethernet_rx()`)
- EtherType recognition (IPv4 0x0800, ARP 0x0806)
- ethernet frame transmission (`ethernet_tx()`)
- ethernet frame validation (minimum length, CRC checking)
- support for VLAN tags (802.1Q)

### Step 3.2: ARP Protocol Implementation

Address Resolution Protocol (ARP) maps IP addresses to MAC addresses on the local network segment. This is essential for IP communication as Ethernet frames require MAC addresses, but higher layers work with IP addresses.

- **Address Resolution**: When sending to an IP address, ARP determines the corresponding MAC address
- **ARP Cache**: Maintains a table of recently resolved IP→MAC mappings to avoid repeated requests
- **ARP Request/Reply**: Uses broadcast requests and unicast replies for address resolution
- **Gratuitous ARP**: Announces IP address ownership and updates neighbor caches

**Implementation Strategy**:
Create ARP packet header structure with hardware/protocol types, operation codes, and address fields. Implement ARP table for caching IP→MAC mappings with timeout support. Handle incoming ARP requests by checking if target IP matches our configuration. Send ARP replies when requests target our IP address. Provide lookup function for higher layers to resolve IP addresses.

**Files to Create**: `src/sys/net/arp.h` and `src/sys/net/arp.c`

### Step 3.3: IPv4 Layer Implementation

**Purpose**: Implements the Internet Protocol version 4, handling packet routing, fragmentation, and delivery between networks. This layer provides the foundation for higher-level protocols like UDP, TCP, and ICMP.

**Key Concepts**:
- **Packet Routing**: Determines if packets are for local delivery or need forwarding
- **Header Processing**: Validates IP headers, checksums, and packet integrity
- **Protocol Demultiplexing**: Routes packets to appropriate upper layer protocols
- **IP Configuration**: Manages local IP address, netmask, and gateway settings

**Implementation Strategy**:
Define IPv4 header structure with version, length, TTL, protocol, and address fields. Implement packet reception handling that validates headers and routes to upper protocols. Create packet transmission functions that build IP headers and integrate with ARP for address resolution. Support basic IP configuration for static addressing.

**Files to Create**: `src/sys/net/ip.h` and `src/sys/net/ip.c`

### Step 3.4: ICMP Implementation

**Purpose**: Internet Control Message Protocol (ICMP) provides error reporting and diagnostic functionality for IP networks. Most importantly, it enables ping responses for network connectivity testing.

**Key Concepts**:
- **Echo Request/Reply**: Implements ping functionality for network diagnostics
- **Error Reporting**: Provides feedback for unreachable destinations and other network issues
- **Checksum Validation**: Ensures message integrity through checksum calculation
- **Message Types**: Supports different ICMP message types (echo, destination unreachable, etc.)

**Implementation Strategy**:
Create ICMP header structure with type, code, checksum, and data fields. Implement echo request handling that generates appropriate echo replies. Support checksum calculation and validation for received packets. Integrate with IP layer for packet transmission and reception.

**Files to Create**: `src/sys/net/icmp.h` and `src/sys/net/icmp.c`

### Step 3.5: UDP Protocol Implementation

**Purpose**: User Datagram Protocol (UDP) provides a simple, connectionless transport layer service. It's ideal for applications that prioritize speed over reliability, such as DNS queries, gaming, and real-time communications.

**Key Concepts**:
- **Connectionless Transport**: No connection establishment required before data transmission
- **Port-Based Multiplexing**: Uses port numbers to identify different applications and services
- **Minimal Overhead**: Simple header structure with minimal processing requirements
- **Best-Effort Delivery**: No guarantees for packet delivery, ordering, or duplicate prevention

**Implementation Strategy**:
Define UDP header structure with source/destination ports, length, and checksum fields. Implement packet reception that validates headers and delivers data to appropriate services. Create transmission functions that build UDP headers and integrate with IP layer. Support port-based application multiplexing for service identification.

**Files to Create**: `src/sys/net/udp.h` and `src/sys/net/udp.c`

## Phase 4: System Call Interface

**System Call Concepts**
System calls provide a controlled interface between user space and kernel space. They allow user programs to request services from the operating system while maintaining security and stability.

**Key Benefits**:

1. **Security**: User programs cannot directly access hardware or kernel data
2. **Abstraction**: Provides clean APIs that hide implementation details
3. **Portability**: Standard interfaces work across different hardware
4. **Resource Management**: Kernel controls access to shared resources

### Step 4.1: Network System Call Definitions

### Step 4.2: Network System Call Implementation

### Step 4.3: Register Network System Calls

---

## Phase 5: User Space Integration

### Step 5.1: Network Library for User Programs

### Step 5.2: Example UDP Echo Server

### Step 5.3: Example UDP Client

---

## Phase 6: Testing and Validation

### Step 6.1: Integration and Build System Updates

### Step 6.2: Kernel Initialization Updates

### Step 6.3: Network Packet Processing Loop

### Step 6.4: QEMU

Run QEMU with RTL8139 networking

```bash
qemu-system-i386 \
    -hda ./bin/os.bin \
    -netdev user,id=net0,hostfwd=udp::8080-:7 \
    -device rtl8139,netdev=net0 \
    -monitor stdio \
    -m 32M
```

The `hostfwd` option forwards UDP port 8080 on the host to port 7 in the guest. This allows testing the echo server from the host machine.

---

## References

- [OSDev PCI](https://wiki.osdev.org/PCI)
- [PCI-SIG Specification](https://pcisig.com/specifications)
- [OSDev Wiki RTL8139](https://wiki.osdev.org/RTL8139)
- [OSDev Wiki networking stack](https://wiki.osdev.org/Network_Stack)
- [RTL8139 Programming Guide](http://www.cs.usfca.edu/~cruse/cs326f04/RTL8139_ProgrammersGuide.pdf)
- [RTL8139 Datasheet](http://www.cs.usfca.edu/~cruse/cs326f04/RTL8139D_DataSheet.pdf)
- [Sanos RTL8139 Driver](http://www.jbox.dk/sanos/source/sys/dev/rtl8139.c.html)
- [Linux kernel source for RTL8139](https://github.com/torvalds/linux/blob/master/drivers/net/ethernet/realtek/8139cp.c)
