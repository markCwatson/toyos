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

### Step 2.1: RTL8139 Hardware Definitions

### Step 2.2: Core Driver Implementation

Based on the reference implementation from the [sanos RTL8139 driver](http://www.jbox.dk/sanos/source/sys/dev/rtl8139.c.html)

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

**Network Layering Concepts**
The network stack implements a layered architecture where each layer provides services to the layer above and uses services from the layer below. This separation of concerns makes the system more maintainable and allows protocols to be developed independently.

**Key Principles**:

1. **Encapsulation**: Each layer adds its own header to the data
2. **Abstraction**: Upper layers don't need to know lower layer details
3. **Modularity**: Layers can be replaced or modified independently
4. **Standard Compliance**: Following RFC specifications ensures interoperability

### Step 3.1: Ethernet Layer Implementation

The Ethernet layer handles local network communication and frame formatting.

### Step 3.2: ARP Protocol Implementation

ARP (Address Resolution Protocol) maps IP addresses to MAC addresses on the local network.

### Step 3.3: IPv4 Layer Implementation

The IP layer handles internet addressing and packet routing.

### Network Stack Layers

**Layer Benefits**:

1. **Separation of Concerns**: Each layer handles specific responsibilities
2. **Reusability**: Lower layers can support multiple upper layer protocols
3. **Maintainability**: Changes to one layer don't affect others
4. **Testability**: Each layer can be tested independently

**Protocol Processing**:

- **Outbound**: Data flows down the stack, each layer adding headers
- **Inbound**: Data flows up the stack, each layer removing and processing headers
- **Encapsulation**: Higher layer data becomes payload for lower layers

### Step 3.4: UDP Protocol Implementation

UDP provides a simple, connectionless transport protocol suitable for applications that don't require reliability guarantees.

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

- [RTL8139 Programming Guide](http://www.cs.usfca.edu/~cruse/cs326f04/RTL8139_ProgrammersGuide.pdf)
- [RTL8139 Datasheet](http://www.cs.usfca.edu/~cruse/cs326f04/RTL8139D_DataSheet.pdf)
- [OSDev Wiki RTL8139](https://wiki.osdev.org/RTL8139)
- [Sanos RTL8139 Driver](http://www.jbox.dk/sanos/source/sys/dev/rtl8139.c.html)
