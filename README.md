# toyos
A toy multi-threadded OS.

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

### Emulation (QEMU) and debugging (GDB)
To run the kernel in the QEMU emulator without debugging, simply run

```shell
qemu-system-x86_64 -hda ./bin/os.bin
```

To debug with GDB, first start GDB

```shell
$ gdb
```

Next, manually load symbol file at the specified address for debugging (because the emulator does not load symbol information from `os.bin` automatically).


```shell
(gdb) add-symbol-file "./build/kernelfull.o" 0x100000
```

Connect to the 64-bit QEMU instance with GDB

```shell
(gdb) target remote | qemu-system-x86_64 -hda ./bin/os.bin -S -gdb stdio -S
```

or thr 32-bit VM using

```shell
target remote | qemu-system-i386 -hda ./bin/os.bin -S -gdb stdio -S
```

Confirm `kernel_main` is being called.

```shell
(gdb) break kernel_main
```

Step through the code using 

```shell
(gdb) stepi
```