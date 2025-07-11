#/bin/bash

qemu-system-i386 \
    -hda ./bin/os.bin \
    -netdev user,id=net0,hostfwd=udp::8080-:7 \
    -device rtl8139,netdev=net0 \
    -m 32M \
    -monitor stdio \
    -S -gdb tcp::1234 &

QEMU_PID=$!

sleep 1

echo "Starting GDB... (QEMU PID: $QEMU_PID)"
gdb

kill $QEMU_PID 2>/dev/null