#!/bin/bash

# Network analysis script for ToyOS debugging
echo "=== ToyOS Network Analysis ==="

if [ ! -f "network.pcap" ]; then
    echo "Error: network.pcap file not found"
    echo "Run QEMU with networking first:"
    echo "qemu-system-i386 -hda ./bin/os.bin -m 32M -netdev tap,id=net0 -device rtl8139,netdev=net0 -object filter-dump,id=dump0,netdev=net0,file=network.pcap"
    exit 1
fi

echo
echo "1. Packet count and basic info:"
tcpdump -r network.pcap | wc -l
echo "Total packets captured"

echo
echo "2. Protocol breakdown:"
tshark -r network.pcap -q -z ptype,tree

echo
echo "3. All packets (first 20):"
tcpdump -r network.pcap -n | head -20

echo
echo "4. Looking for RTL8139-related traffic:"
echo "   - ARP requests/replies:"
tcpdump -r network.pcap arp

echo
echo "   - ICMP packets (ping attempts):"
tcpdump -r network.pcap icmp

echo
echo "   - Any broadcast traffic:"
tcpdump -r network.pcap broadcast

echo
echo "5. Ethernet frame analysis:"
tshark -r network.pcap -T fields -e eth.src -e eth.dst -e eth.type | head -10

echo
echo "=== Analysis complete ==="
echo "For detailed analysis, open with: wireshark network.pcap"
