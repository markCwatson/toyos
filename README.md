# ToyOS

ToyOS is a work-in-progress (WIP) hobby operating system meant for educational purposes. The OS is being developed with a focus on foundational operating system concepts and practical implementations. The following GIF shows ToyOS running in QEMU. In this demonstration, the shell is interacting with the kernel to execute commands (currently, only the `ps`, `echo`, and `clear` commands are supported).

![alt-text][1]

Current features include:

- **16-bit Bootloader:** A basic bootloader that initializes the system and loads the 32-bit kernel.
- **32-bit Kernel:** The core of ToyOS, built for x86 platforms, handling essential system tasks.
- **Virtual Memory with Paging:** Implements paging to manage memory, providing virtual memory support.
- **Virtual Filesystem (VFS):** An abstraction layer for file system operations, allowing different filesystem types to be used seamlessly.
- **FAT16 Filesystem:** Supports reading and writing files using the FAT16 format. Writing in append mode is under development.
- **Interactive Shell:** A user-friendly command-line interface to interact with the OS.
- **Simple Test Framework:** A basic test framework for functional testing, currently under development.
- **Multi-threading:** Support for concurrent execution of processes, enabling more complex and efficient program execution.
- **User-Level Programs:** Support for running user programs, expanding the OS's functionality beyond system-level operations.
- **Loading ELF files at runtime:** The ability to dynamically load shared libraries and user programs at runtime.
- **PCI Device driver:** PCI devices can be discovered and enumerated.
- **Network Device abstraction:** An abstraction layer for networking devices such as the RTL8139.
- **RTL8139 driver port:** The driver for sanos (originally developed by Donald Becker and Michael Ringgaard) has been ported to toyos.

Work in progress:

- **Networking**: see plan [here](./networking.md)

### Setup

On a Linux box:

```shell
sudo apt update
sudo apt upgrade
```

This project was developed for Linux. The assembler used here is `nasm`. It can be installed using

```shell
sudo apt install nasm
```

QEMU is used to emulate the x86 hardware. It can be installed using

```shell
sudo apt install qemu-system-x86
```

The makefile invokes a gcc cross-compiler with a generic target (i686-elf) custom built to not include any reminants of the host OS (stdlib, etc.). It needs to be built from source. Follow the instructions [here](https://wiki.osdev.org/GCC_Cross-Compiler). Here is what worked for me.

```shell
sudo apt install build-essential
sudo apt install bison
sudo apt install flex
sudo apt install libgmp3-dev
sudo apt install libmpc-dev
sudo apt install libmpfr-dev
sudo apt install texinfo
sudo apt install libisl-dev
```

Then we need to download the source code for gcc (10.2) and binutils (2.35):

```shell
cd ~
mkdir src
curl -O https://ftp.gnu.org/gnu/binutils/binutils-2.35.tar.xz
curl -O https://ftp.lip6.fr/pub/gcc/releases/gcc-10.2.0/gcc-10.2.0.tar.gz
tar -xf binutils-2.35.tar.xz -C ~/src/
tar -xzf gcc-10.2.0.tar.gz -C ~/src/
export PREFIX="$HOME/opt/cross"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"

# binutils
cd ~/src 
mkdir build-binutils
cd build-binutils
../binutils-2.35/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
make
make install

# gcc
cd ~/src
which -- $TARGET-as || echo $TARGET-as is not in the PATH
mkdir build-gcc
cd build-gcc
../gcc-10.2.0/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers --disable-hosted-libstdcxx
make all-gcc
make all-target-libgcc
make all-target-libstdc++-v3
make install-gcc
make install-target-libgcc
make install-target-libstdc++-v3
```

### Building

From the root of the project, invoke the make build system (will need to make build script executable beforehand: `sudo chmod +x ./build.sh`)

```shell
make clean
./build.sh
```

### Emulation (QEMU) and debugging (GDB)

To run the kernel in the QEMU emulator without debugging, simply run the 32-bit x86 emulator

```shell
qemu-system-i386 \
    -hda ./bin/os.bin \
    -netdev user,id=net0,hostfwd=udp::8080-:7 \
    -device rtl8139,netdev=net0 \
    -monitor stdio \
    -m 32M
```

For network testing and debugging, you can enable packet dumping:

```shell
qemu-system-i386 \
    -hda ./bin/os.bin \
    -netdev user,id=net0,hostfwd=udp::8080-:7 \
    -device rtl8139,netdev=net0 \
    -object filter-dump,id=dump0,netdev=net0,file=network.pcap \
    -monitor stdio \
    -m 32M
```

This will capture all network traffic to `network.pcap` which you can analyze with Wireshark.

To debug with GDB, first start GDB

```shell
$ gdb
```

Next, manually load symbol file at the specified address for debugging (because the emulator does not load symbol information from `os.bin` automatically).

```shell
(gdb) add-symbol-file "./build/kernelfull.o" 0x100000
```

Start the QEMU instance with GDB server enabled on TCP port 1234 (in a separate terminal):

```shell
qemu-system-i386 \
    -hda ./bin/os.bin \
    -netdev user,id=net0,hostfwd=udp::8080-:7 \
    -device rtl8139,netdev=net0 \
    -monitor stdio \
    -m 32M \
    -S -gdb tcp::1234
```

Then, in GDB, connect to the QEMU instance:

```shell
(gdb) target remote localhost:1234
```

To debug user programs, use address `0x400000` for user space.

```shell
(gdb) break *0x400000
```

### Tests

To run the tests (see [tests](https://github.com/markCwatson/toyos/tree/main/tests) folder at root of project), use the `--tests` flag

```shell
make clean
./build.sh --tests
```

### Elf Files

To view the content of ELF files, install `dumpelf`

```shell
sudo apt install pax-utils
```

and dump the contents of an ELF file, for example

```shell
dumpelf programs/shell/shell.elf
```

### AI Disclaimer

Most of the code comments in this project were written by AI. I want this project to be well documented, and since most of the implementations of various features were obtained from tutorials and examples (believe it or not: I am not a professional OS developer), I used ChatGPT to add comments and explain what each thing is doing, to help us both (u and I), learn.

### References

1. [Developing a Multithreaded Kernel From Scratch! by Daniel McCarthy](https://www.udemy.com/course/developing-a-multithreaded-kernel-from-scratch)
2. [The Little Book About OS Development by Erik Helin and Adam Renberg](https://littleosbook.github.io/)
3. [OSdev.org](https://wiki.osdev.org/Expanded_Main_Page)
4. [Modern Operating Systems 4th Edition by Andrew Tanenbaum and Herbert Bos](https://csc-knu.github.io/sys-prog/books/Andrew%20S.%20Tanenbaum%20-%20Modern%20Operating%20Systems.pdf)

[1]: gif/toyos-demo.gif 'ToyOS running in QEMU'
