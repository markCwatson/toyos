# WIP - ToyOS Networking Implementation Plan

## Table of Contents

1. [Overview](#overview)
2. [Architecture](#architecture)
3. [Phase 1: PCI Enumeration](#phase-1-pci-enumeration) ✅
4. [Phase 2: RTL8139 Driver](#phase-2-rtl8139-driver) ✅
5. [Phase 3: Network Stack](#phase-3-network-stack) ✅
6. [Phase 4: System Call Interface](#phase-4-system-call-interface) ✅
7. [Phase 5: User Space Integration](#phase-5-user-space-integration) ✅
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

## Phase 1: PCI Enumeration ✅

> **Status**: Complete. PCI configuration space access and device enumeration are implemented in `src/drivers/pci/pci.c`. RTL8139 devices are detected and their I/O base and IRQ are read from BARs.
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

## Phase 2: RTL8139 Driver ✅

> **Status**: Complete. The sanos RTL8139 driver has been ported to ToyOS. The driver handles hardware init, DMA-based packet TX/RX, and interrupt handling. Integrated with the `netdev` abstraction layer.
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

## Phase 3: Network Stack ✅

> **Status**: Complete. Ethernet, ARP, IPv4, ICMP, and UDP are implemented. ToyOS responds to `ping` and echoes UDP packets on port 7.

Data flow:
```
Packet arrives → RTL8139 hardware → Interrupt → rtl8139_interrupt() → rtl8139_rx() → netdev_rx() → ethernet_rx() → [Protocol layers]
```

### Step 3.1: Ethernet Layer ✅

> **Status**: Complete. Implemented in `src/sys/net/ethernet.h` and `src/sys/net/ethernet.c`.

- Ethernet header structure and frame parsing
- Protocol demultiplexing by EtherType (IPv4 0x0800, ARP 0x0806)
- `ethernet_rx()` strips header and dispatches to protocol handlers
- `ethernet_tx()` builds Ethernet frames and sends via driver

### Step 3.2: ARP Protocol ✅

> **Status**: Complete. Implemented in `src/sys/net/arp.h` and `src/sys/net/arp.c`.

- ARP request/reply handling for Ethernet + IPv4
- ARP cache (16 entries, circular overwrite) for IP→MAC lookups
- Replies to ARP requests targeting our static IP (10.0.2.15)
- Cache is populated from all received ARP packets
- `arp_cache_lookup()` used by `ip_tx()` for destination MAC resolution

### Step 3.3: IPv4 Layer ✅

> **Status**: Complete. Implemented in `src/sys/net/ip.h` and `src/sys/net/ip.c`.

- IPv4 header parsing with version, IHL, TTL, protocol, and checksum validation
- `ip_rx()` validates headers and demultiplexes by protocol (ICMP=1, UDP=17, TCP=6)
- `ip_tx()` builds IP headers (TTL=64, Don't Fragment) and sends via ARP cache + `ethernet_tx()`
- `ip_checksum()` implements RFC 1071 internet checksum (shared with ICMP)
- Static IP configuration (10.0.2.15)

### Step 3.4: ICMP (Ping) ✅

> **Status**: Complete. Implemented in `src/sys/net/icmp.h` and `src/sys/net/icmp.c`. ToyOS responds to `ping` from the host.

- ICMP Echo Request (type 8) → Echo Reply (type 0)
- Copies ID, sequence number, and data payload verbatim
- Checksum calculation and validation
- Full round trip: host `ping 10.0.2.15` → ARP → ICMP request → ICMP reply → host sees response

### Step 3.5: UDP Protocol ✅

> **Status**: Complete. Implemented in `src/sys/net/udp.h` and `src/sys/net/udp.c`. Echo server on port 7 works with `echo "test" | nc -u -w1 10.0.2.15 7`.

- UDP header parsing with port and length validation
- `udp_rx()` dispatches by destination port
- `udp_tx()` builds UDP header and sends via `ip_tx()`
- Echo server (RFC 862) on port 7: echoes received data back to sender
- Checksum set to 0 (optional in IPv4); pseudo-header verification is a TODO

**Files**: `src/sys/net/udp.h` and `src/sys/net/udp.c`

## Phase 4: System Call Interface ✅

> **Status**: Complete. Four network syscalls implemented (socket, bind, sendto, recvfrom) with kernel socket layer for packet queuing.

**System Call Concepts**
System calls provide a controlled interface between user space and kernel space. They allow user programs to request services from the operating system while maintaining security and stability.

**Key Benefits**:

1. **Security**: User programs cannot directly access hardware or kernel data
2. **Abstraction**: Provides clean APIs that hide implementation details
3. **Portability**: Standard interfaces work across different hardware
4. **Resource Management**: Kernel controls access to shared resources

### Step 4.1: Network System Call Definitions ✅

> **Status**: Complete. Syscall numbers 16-19 added to `src/sys/sys.h`.

| Syscall | Number | Purpose |
|---------|--------|---------|
| `SYSTEM_COMMAND16_SOCKET` | 16 | Create a UDP socket |
| `SYSTEM_COMMAND17_BIND` | 17 | Bind a socket to a port |
| `SYSTEM_COMMAND18_SENDTO` | 18 | Send a UDP packet |
| `SYSTEM_COMMAND19_RECVFROM` | 19 | Receive a UDP packet |

`sendto` and `recvfrom` use argument structs (`struct sendto_args`, `struct recvfrom_args`) passed by pointer, since they have too many parameters to push individually on the stack.

### Step 4.2: Network System Call Implementation ✅

> **Status**: Complete. Handlers in `src/sys/net/sys_net.c`, socket layer in `src/sys/net/socket.c`.

**Kernel Socket Layer** (`src/sys/net/socket.h` / `src/sys/net/socket.c`):
- Fixed-size socket table (16 slots), indexed by descriptor
- Each socket has a receive ring buffer (8 packets deep)
- `socket_create()` — allocates a slot, returns descriptor
- `socket_bind()` — associates a socket with a UDP port
- `socket_sendto()` — builds a netbuf and calls `udp_tx()`
- `socket_recvfrom()` — pulls oldest packet from ring buffer (non-blocking)
- `socket_deliver_udp()` — called from `udp_rx()` to queue incoming packets

**Syscall Handlers** (`src/sys/net/sys_net.h` / `src/sys/net/sys_net.c`):
- Read arguments from user stack via `task_get_stack_item()`
- Convert user-space pointers to physical addresses via `task_virtual_address_to_physical()`
- Call kernel socket functions and return results in EAX

**UDP changes** (`src/sys/net/udp.c`):
- Removed hardcoded `udp_echo()` function
- `udp_rx()` now calls `socket_deliver_udp()` to route packets to bound sockets

### Step 4.3: Register Network System Calls ✅

> **Status**: Complete. All four syscalls registered in `src/sys/sys.c` via `sys_register_commands()`.

---

## Phase 5: User Space Integration ✅

> **Status**: Complete. User-space stdlib extended with socket functions. UDP echo server runs as a user program.

### Step 5.1: Network Library for User Programs ✅

> **Status**: Complete. Assembly stubs and C declarations added to user stdlib.

**Assembly stubs** (`programs/stdlib/src/toyos.asm`):
- `toyos_socket(type)` — pushes type, `mov eax, 16`, `int 0x80`
- `toyos_bind(sockfd, port)` — pushes port then sockfd, `mov eax, 17`, `int 0x80`
- `toyos_sendto(args)` — pushes pointer to `sendto_args`, `mov eax, 18`, `int 0x80`
- `toyos_recvfrom(args)` — pushes pointer to `recvfrom_args`, `mov eax, 19`, `int 0x80`

**C declarations** (`programs/stdlib/src/toyos.h`):
- `struct sendto_args` and `struct recvfrom_args` (packed, matching kernel-side layout)
- `SOCK_DGRAM` constant (value 2)
- Function prototypes for all four socket functions

### Step 5.2: Example UDP Echo Server ✅

> **Status**: Complete. Implemented in `programs/udpecho/`.

The echo server runs as a user-space process:
1. `toyos_socket(SOCK_DGRAM)` — create a UDP socket
2. `toyos_bind(sock, 7)` — listen on port 7
3. Poll loop: `toyos_recvfrom()` returns 0 when empty, >0 when a packet arrives
4. `toyos_sendto()` sends the data back to the sender

**Testing:**
```bash
# In ToyOS shell:
udpecho

# From host:
echo "hello" | nc -u -w1 10.0.2.15 7
```

**Note:** `recvfrom()` is non-blocking (returns 0 immediately if no data). The user program polls in a loop. Blocking I/O (sleeping until data arrives) is a future improvement.

### Step 5.3: Example UDP Client

> **Status**: Not started. A client program could send UDP packets to a host service.

---

## Phase 6: Testing and Validation with QEMU

ToyOS uses TAP networking for direct Layer 2 connectivity between the host and guest.

**Setup:**
```bash
./setup_tap.sh    # one-time TAP interface setup (requires sudo)
./run.sh          # run ToyOS
./teardown_tap.sh # remove TAP interface when done
```

**Network topology:**
- Host (`tap0`): `10.0.2.1/24`
- Guest (ToyOS): `10.0.2.15/24`

**Testing:**
```bash
ping 10.0.2.15                    # ICMP ping (working)
echo "test" | nc -u -w1 10.0.2.15 7  # UDP echo (working)
sudo tcpdump -i tap0 -n           # capture traffic for debugging
```

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
