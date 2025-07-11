# QEMU Network Testing Command Explanation

## Overview

This document explains the QEMU command used to test networking functionality in ToyOS and how to interpret the debug output.

## QEMU Command Breakdown

```bash
qemu-system-i386 \
    -hda ./bin/os.bin \
    -netdev user,id=net0,hostfwd=udp::8080-:7 \
    -device rtl8139,netdev=net0 \
    -monitor stdio \
    -m 32M \
    -d int,cpu_reset \
    -no-reboot \
    -no-shutdown \
    2>&1 | tee network_debug.log
```

### Parameter Explanation

| Parameter | Purpose | Details |
|-----------|---------|---------|
| `qemu-system-i386` | QEMU emulator for i386 architecture | Emulates a 32-bit x86 system |
| `-hda ./bin/os.bin` | Hard drive A | Uses the ToyOS binary as the boot disk |
| `-netdev user,id=net0,hostfwd=udp::8080-:7` | Network backend | Creates user-mode networking with port forwarding |
| `-device rtl8139,netdev=net0` | Network device | Adds RTL8139 NIC connected to the network backend |
| `-monitor stdio` | QEMU monitor | Enables QEMU monitor on standard input/output |
| `-m 32M` | Memory allocation | Allocates 32 MB of RAM to the guest |
| `-d int,cpu_reset` | Debug logging | Logs all interrupts and CPU resets |
| `-no-reboot` | Reboot behavior | Prevents automatic reboot on triple fault |
| `-no-shutdown` | Shutdown behavior | Keeps QEMU running when guest halts |

## Network Configuration Details

### User-Mode Networking (`-netdev user`)

QEMU's user-mode networking creates a virtual network with these default settings:

- **Guest IP**: `10.0.2.15`
- **Host IP**: `10.0.2.2` (from guest perspective)
- **Gateway**: `10.0.2.2`
- **DNS**: `10.0.2.3`
- **Network**: `10.0.2.0/24`

### Port Forwarding (`hostfwd=udp::8080-:7`)

- **Protocol**: UDP
- **Host Port**: 8080 (empty host IP means localhost)
- **Guest Port**: 7
- **Purpose**: Allows testing UDP echo servers

Example: `echo "test" | nc -u localhost 8080` on host → UDP packet to guest port 7

### RTL8139 Network Card

- **Vendor ID**: 0x10EC (RealTek)
- **Device ID**: 0x8139
- **Type**: PCI Fast Ethernet controller
- **Speed**: 10/100 Mbps
- **Features**: DMA-based packet processing, interrupt-driven operation

## Debug Output Explanation

The `-d int,cpu_reset` flag generates detailed interrupt logs. Here's how to interpret them:

### Sample Output
```
10618: v=80 e=0000 i=1 cpl=3 IP=001b:00401025 pc=00401025 SP=0023:003feb48 env->regs[R_EAX]=00000002
EAX=00000002 EBX=00000000 ECX=003fefd0 EDX=00000000
ESI=00000000 EDI=00000000 EBP=003feb48 ESP=003feb48
EIP=00401025 EFL=00000246 [---Z-P-] CPL=3 II=0 A20=1 SMM=0 HLT=0
ES =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
CS =001b 00000000 ffffffff 00cff800 DPL=3 CS32 [---]
SS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
...
```

### Header Line Breakdown

| Field | Example | Meaning |
|-------|---------|---------|
| Instruction Count | `10618:` | Number of instructions executed |
| Vector | `v=80` | Interrupt vector (0x80 = system call) |
| Error Code | `e=0000` | Error code (0 = no error) |
| Interrupt Flag | `i=1` | 1 = interrupts enabled, 0 = disabled |
| Privilege Level | `cpl=3` | Current Privilege Level (0=kernel, 3=user) |
| Instruction Pointer | `IP=001b:00401025` | Segment:Offset format |
| Program Counter | `pc=00401025` | Linear address |
| Stack Pointer | `SP=0023:003feb48` | Stack segment:offset |
| Register Value | `env->regs[R_EAX]=00000002` | EAX register contents |

### Register Dump

```
EAX=00000002  # System call number or return value
EBX=00000000  # Function parameter 1
ECX=003fefd0  # Function parameter 2 (often a pointer)
EDX=00000000  # Function parameter 3
ESI=00000000  # Source index register
EDI=00000000  # Destination index register
EBP=003feb48  # Base pointer (stack frame)
ESP=003feb48  # Stack pointer
```

### Flags Register (EFL)

```
EFL=00000246 [---Z-P-]
```

Common flags:
- `Z` = Zero flag set
- `P` = Parity flag set
- `C` = Carry flag set
- `S` = Sign flag set
- `O` = Overflow flag set

### Segment Registers

```
ES =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
CS =001b 00000000 ffffffff 00cff800 DPL=3 CS32 [---]
SS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
```

Format: `SELECTOR BASE LIMIT ACCESS_RIGHTS DPL TYPE ATTRIBUTES`

- **0023/001b**: Segment selectors (0x23 = user data, 0x1B = user code)
- **DPL=3**: Descriptor Privilege Level (user mode)
- **CS32**: 32-bit code segment
- **[-WA]**: Attributes (Writable, Accessed)

## Interrupt Vector Analysis

### Common Interrupt Vectors

| Vector | Purpose | Source |
|--------|---------|---------|
| 0x80 | System calls | Software interrupt (INT 0x80) |
| 0x20 | Timer interrupt | PIT (Programmable Interval Timer) |
| 0x21 | Keyboard | PS/2 keyboard controller |
| 0x2A-0x2F | Network/PCI | RTL8139 typically uses IRQ 10-15 |

### What to Look For

**Normal Operation:**
- Mostly 0x80 (system calls) during user program execution
- Occasional 0x20 (timer) for task scheduling

**Network Activity:**
- Look for vectors 0x2A-0x2F when pinging or sending packets
- Absence of these vectors indicates no network interrupts

## Testing Network Functionality

### From Host Machine

1. **Ping the guest:**
   ```bash
   ping 10.0.2.15
   ```

2. **Send UDP packet to echo server:**
   ```bash
   echo "test message" | nc -u 10.0.2.15 7
   ```

3. **Test port forwarding:**
   ```bash
   echo "forwarded" | nc -u localhost 8080
   ```

### Expected Debug Output

If networking works, you should see:
- Interrupt vectors other than 0x80
- Vectors around 0x2A-0x2F for RTL8139 interrupts
- Register states showing interrupt handling in kernel mode (cpl=0)

### Troubleshooting

**Only seeing 0x80 vectors?**
- Network interrupts aren't firing
- RTL8139 driver may not be properly initialized
- Interrupts might not be enabled for the device
- PCI interrupt routing issues

**No response to ping?**
- Check if RTL8139 device is detected during boot
- Verify IP configuration in guest
- Ensure network interface is brought up

## QEMU Monitor Commands

With `-monitor stdio`, you can use these commands:

```
(qemu) info network          # Show network status
(qemu) info pci              # Show PCI devices
(qemu) info registers        # Show CPU registers
(qemu) info irq              # Show interrupt statistics
```

### Real Example Output

Here's actual output from a ToyOS session:

#### Network Status
```
(qemu) info network
rtl8139.0: index=0,type=nic,model=rtl8139,macaddr=b0:43:e7:04:e7:04
 \ net0: index=0,type=user,net=10.0.2.0,restrict=off
```

**Analysis:**
- RTL8139 device is properly created with MAC address `b0:43:e7:04:e7:04`
- Connected to user-mode network `10.0.2.0/24`
- Guest IP will be `10.0.2.15` (default for QEMU user networking)

#### PCI Device Information
```
(qemu) info pci
...
  Bus  0, device   3, function 0:
    Ethernet controller: PCI device 10ec:8139
      PCI subsystem 1af4:1100
      IRQ 11, pin A
      BAR0: I/O at 0xc000 [0xc0ff].
      BAR1: 32 bit memory at 0xfeb91000 [0xfeb910ff].
      BAR6: 32 bit memory at 0xffffffffffffffff [0x0007fffe].
      id ""
```

**Critical Information:**
- **Vendor:Device ID**: `10ec:8139` (RealTek RTL8139) ✓
- **IRQ**: **11** (this is key for interrupt handling!)
- **I/O Base**: `0xc000` (for register access)
- **Memory Base**: `0xfeb91000` (memory-mapped registers)

#### IRQ Statistics
```
(qemu) info irq
IRQ statistics for ioapic:
 0: 1770
 4: 1
14: 266
IRQ statistics for isa-i8259:
 0: 1770
 2: 2
 4: 1
14: 266
```

**Analysis:**
- **IRQ 0**: Timer interrupts (1770 occurrences) ✓
- **IRQ 4**: Serial port interrupts (1 occurrence)
- **IRQ 14**: IDE controller (266 occurrences)
- **IRQ 11**: **MISSING!** This is a problem!!!

## Writing Output to Files

To capture debug output, redirect stderr:

```bash
qemu-system-i386 [options] 2> debug_output.log
```

Or use tee to see output and save it:

```bash
qemu-system-i386 [options] 2>&1 | tee debug_output.log
```
