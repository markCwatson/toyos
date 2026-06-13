#!/bin/bash

# Run ToyOS in QEMU with TAP networking.
# Run ./setup_tap.sh first to create the TAP interface.

TAP_DEV="tap0"

if ! ip link show "$TAP_DEV" &>/dev/null; then
    echo "Error: TAP interface $TAP_DEV not found. Run ./setup_tap.sh first."
    exit 1
fi

qemu-system-i386 \
    -hda ./bin/os.bin \
    -netdev tap,id=net0,ifname="$TAP_DEV",script=no,downscript=no \
    -device rtl8139,netdev=net0 \
    -monitor stdio \
    -m 128M