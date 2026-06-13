#!/bin/bash

# Tear down TAP interface for ToyOS networking

TAP_DEV="tap0"

if ip link show "$TAP_DEV" &>/dev/null; then
    echo "Removing TAP interface $TAP_DEV..."
    sudo ip link set "$TAP_DEV" down
    sudo ip tuntap del dev "$TAP_DEV" mode tap
    echo "TAP interface $TAP_DEV removed"
else
    echo "TAP interface $TAP_DEV does not exist"
fi
