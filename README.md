## toyos
A toy multi-threadded OS.

### Make
From the root of the project, invoke the make build system (will need to make build script executable beforehand: `sudo chmod +x ./build.sh`)

```shell
make clean
./build.sh
```

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