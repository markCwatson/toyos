## toyos
A toy multi-threadded OS.

<br />

### Building
From the root of the project, invoke the make build system (will need to make build script executable beforehand: `sudo chmod +x ./build.sh`)

```shell
make clean
./build.sh
```

<br />

> [!NOTE] 
> The makefile invokes a gcc cross-compiler with a generic target (i686-elf) custom build to not include any reminants of the host OS (stdlib, etc.). It needs to be built from source. Follow the instructions [here](https://osdev.org/GCC_Cross-Compiler).

<br />

### Emulator
`qemu` can be involked using

```shell
qemu-system-x86_64 -hda ./bin/os.bin
```

### gdb
Attach gdb to the emulator

```shell
$ gdb
(gdb) target remote | qemu-system-x86_64 -hda ./bin/os.bin -S -gdb stdio
```