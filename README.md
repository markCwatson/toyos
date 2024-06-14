## toyos
A toy multi-threadded OS.

### Make
From the room of the project, invoke the make build system

```shell
make
```

### Assembler
`nasm` can be involked using 

```shell
nasm -f bin ./src/boot/boot.asm -o ./bin/boot.bin
```

### Emulator
`qemu` can be involked using

```shell
qemu-system-x86_64 -hda ./bin/boot.bin
```

### gdb
Attach gdb to the emulator

```shell
$ gdb
(gdb) target remote | qemu-system-x86_64 -hda ./bin/boot.bin -S -gdb stdio
```