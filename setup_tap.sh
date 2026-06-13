#!/bin/bash

# Setup TAP interface for ToyOS networking
#
# TAP creates a real virtual network interface on the host, allowing direct
# Layer 2 communication with the QEMU guest (e.g., ping, ARP, UDP).
# Unlike user-mode networking, there is no NAT — the guest and host are
# on the same network, like plugging two machines into the same switch.
#
# Network topology:
#   Host (tap0): 10.0.2.1/24
#   Guest:       10.0.2.15/24 (once IP stack is implemented)
#
# Requires sudo because creating network interfaces is a privileged operation.
# Only needs to be run once per boot.

TAP_DEV="tap0"
TAP_ADDR="10.0.2.1/24"

if ip link show "$TAP_DEV" &>/dev/null; then
    echo "TAP interface $TAP_DEV already exists"
    sudo ip link set "$TAP_DEV" up
else
    echo "Creating TAP interface $TAP_DEV..."
    sudo ip tuntap add dev "$TAP_DEV" mode tap user "$USER"
    sudo ip addr add "$TAP_ADDR" dev "$TAP_DEV"
    sudo ip link set "$TAP_DEV" up
    echo "TAP interface $TAP_DEV is up with address $TAP_ADDR"
fi
