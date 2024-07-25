# toyos

A toy multi-threadded OS. This is a WIP hobby project. Existing features include:

- 16-bit bootloader
- 32-bit kernel targeting x86 platforms
- virtual memory using paging
- virtual filesystem
- FAT16 filesystem including reading and writing (appending is WIP)
- simple terminal (WIP)

Future features will include

- multi-threading
- interactive shell
- user level programs

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
(gdb) target remote | qemu-system-i386 -hda ./bin/os.bin -S -gdb stdio -S
```