## toyos
A toy multi-threadded OS.

### Assembler
`nasm` can be involked using 

```shell
nasm -f bin ./boot/boot.asm -o ./boot/boot.bin
```

### Emulator
`qemu` can be involked using

```shell
qemu-system-x86_64 -hda ./boot/boot.bin
```