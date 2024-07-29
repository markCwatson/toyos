# ToyOS

ToyOS is a work-in-progress (WIP) hobby operating system designed for educational and experimental purposes. The OS is being developed with a focus on foundational operating system concepts and practical implementations. Current features include:

- **16-bit Bootloader:** A basic bootloader that initializes the system and loads the 32-bit kernel.
- **32-bit Kernel:** The core of ToyOS, built for x86 platforms, handling essential system tasks.
- **Virtual Memory with Paging:** Implements paging to manage memory, providing virtual memory support.
- **Virtual Filesystem (VFS):** An abstraction layer for file system operations, allowing different filesystem types to be used seamlessly.
- **FAT16 Filesystem:** Supports reading and writing files using the FAT16 format. Writing in append mode is under development.
- **Simple Terminal:** A basic terminal interface for user interaction, currently under development.
- **Simple Test Framework:** A basic test framework for functional testing, currently under development.

Future planned features:

- **Multi-threading:** Support for concurrent execution of processes, enabling more complex and efficient program execution.
- **User-Level Programs:** Support for running user programs, expanding the OS's functionality beyond system-level operations.
- **Interactive Shell:** A user-friendly command-line interface to interact with the OS.
- **Loading ELF files at runtime:** The ability to dynamically load shared libraries and user programs at runtime.

### Building

From the root of the project, invoke the make build system (will need to make build script executable beforehand: `sudo chmod +x ./build.sh`)

```shell
make clean
./build.sh
```

<br />

> [!NOTE]
> The makefile invokes a gcc cross-compiler with a generic target (i686-elf) custom built to not include any reminants of the host OS (stdlib, etc.). It needs to be built from source. Follow the instructions [here](https://osdev.org/GCC_Cross-Compiler).

<br />

### Tests

To run the tests (see [tests](https://github.com/markCwatson/toyos/tree/main/tests) folder at root of project), use the `--tests` flag

```shell
make clean
./build.sh --tests
```

### Emulation (QEMU) and debugging (GDB)

To run the kernel in the QEMU emulator without debugging, simply run the 32-bit x86 emulator

```shell
qemu-system-i386 -hda ./bin/os.bin
```

To debug with GDB, first start GDB

```shell
$ gdb
```

Next, manually load symbol file at the specified address for debugging (because the emulator does not load symbol information from `os.bin` automatically).

```shell
(gdb) add-symbol-file "./build/kernelfull.o" 0x100000
```

Connect to the 32-bit QEMU instance with GDB

```shell
(gdb) target remote | qemu-system-i386 -hda ./bin/os.bin -S -gdb stdio
```

To debug user programs, use address `0x400000` for user space.

```shell
(gdb) break *0x400000
```

### Elf Files

To view the ocntent of ELF files, install `dumpelf`

```shell
sudo apt install pax-utils
```

and dump the contents of an ELF file, for example

```shell
dumpelf programs/shell/shell.elf
```

### References

1. [Developing a Multithreaded Kernel From Scratch! by Daniel McCarthy](https://www.udemy.com/course/developing-a-multithreaded-kernel-from-scratch)
2. [The Little Book About OS Development by Erik Helin and Adam Renberg](https://littleosbook.github.io/)
3. [OSdev.org](https://wiki.osdev.org/Expanded_Main_Page)
4. [Modern Operating Systems 4th Edition by Andrew Tanenbaum and Herbert Bos](https://csc-knu.github.io/sys-prog/books/Andrew%20S.%20Tanenbaum%20-%20Modern%20Operating%20Systems.pdf)
5. Various OS courses on Udemy by the group [Abhishek CSEPracticals](https://www.udemy.com/user/abhishek-sagar-8/), [Ekta Ekta](https://www.udemy.com/user/ekta-272/), and [Shiwani Nigam](https://www.udemy.com/user/shivani-nigam-2/)
