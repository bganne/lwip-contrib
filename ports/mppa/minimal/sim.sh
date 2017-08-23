#!/bin/bash
NET=192.168.10
MTU=9000
IFACE='magic:veth-simu'
OPTS='tags=120:jumbo:verbose:hashpolicy=[P0{@12/0x3=0x800}{@23/0x1=6}{@26#0xff}{@34#0xf}][P1{@12/0x3=0x800}{@23/0x1=17}{@26#0xff}{@34#0xf}][P2{@12/0x3=0x800}{@26#0xff}'

sudo ip link add dev veth-simu type veth peer name veth-user
sudo ip link set dev veth-simu mtu $MTU up
sudo ip link set dev veth-user mtu $MTU up
sudo ethtool --offload veth-user rx off tx off
sudo ethtool -K veth-user gso off
sudo ip addr add ${NET}.1 peer ${NET}.2 scope link dev veth-user
sudo $K1_TOOLCHAIN_DIR/bin/k1-cluster --functional --march=k1b --user-syscall=$K1_TOOLCHAIN_DIR/lib64/libodp_syscall.so -- ./echop -d -f "$IFACE:$OPTS" -i "${NET}.2" -g "${NET}.1"
