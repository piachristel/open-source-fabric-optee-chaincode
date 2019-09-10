#!/bin/bash

# Script provided by Christan Göttel.
#
# Setup or remove bridged networking.
#
# Usage:
#   sudo ./qemu_br_tap.sh {down|up}

IFDEV="enp4s0"

# TODO: hardcoded TAP name. Retrieve available TAP name from host interfaces.
GUEST_TAP_NAME="tap2"

# TODO: hardcoded gateway address, e.g., `172.28.0.1'. Retrieve gateway address from host interface.
GATEWAY="192.168.0.1"

# NOTE: hardcoded user, e.g., `ubuntu'. Should match user that launches QEMU.
QEMUUSER="ubuntu"

# Setup bridged networking
up(){
    # Retrieve the IP address for physical device $IFDEV
    ip=$(ip -4 -o address show dev $IFDEV | sed -e 's%^.\+inet[[:space:]]\+\([1-9]\([0-9]\)\{0,2\}\(\.[0-9]\{1,3\}\)\{3\}/[0-9]\{1,2\}\).*$%\1%')

    # Add the bridge
    ip link add name br0 type bridge

    # Bring down $IFDEV and assign it to the bridge
    ip link set dev $IFDEV down
    ip address flush dev $IFDEV
    ip link set dev $IFDEV up
    ip link set $IFDEV master br0

    # Additional information on the bridge
    #bridge link > bridge.log

    # Bring up the bridge
    ip link set dev br0 up
    ip address add dev br0 $ip

    # Add a default route in order for the hypervisor to have network/internet access
    ip route add default via $GATEWAY dev br0 onlink

    ip tuntap add $GUEST_TAP_NAME mode tap user $QEMUUSER
    ip link set $GUEST_TAP_NAME up
    ip link set $GUEST_TAP_NAME master br0
}

# Remove bridged networking
down(){
    # Remove the TAP
    ip link set $GUEST_TAP_NAME down
    ip link set $GUEST_TAP_NAME nomaster
    ip tuntap del $GUEST_TAP_NAME mode tap
    
    # Retrieve the IP address for the bridge br0
    ip=$(ip -4 -o address show dev br0 | sed -e 's%^.\+inet[[:space:]]\+\([1-9]\([0-9]\)\{0,2\}\(\.[0-9]\{1,3\}\)\{3\}/[0-9]\{1,2\}\).*$%\1%')

    # Bring down $IFDEV and detach it from the bridge
    ip link set dev br0 down
    ip link set dev $IFDEV down
    ip link set $IFDEV nomaster
    ip link delete br0 type bridge

    ip address flush dev $IFDEV
    ip link set dev $IFDEV up
    ip address add dev $IFDEV $ip

    # Add a default route in order for the hypervisor to have network/internet access
    ip route add default via $GATEWAY dev $IFDEV onlink
}

case $1 in
    up) up ;;
    down) down ;;
    *) "  ERROR: unknown option '$1'"
esac
