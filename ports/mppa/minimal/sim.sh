#!/bin/bash
NET=192.168.10
sudo ip link add dev veth-simu type veth peer name veth-user
sudo ip link set dev veth-simu up
sudo ip link set dev veth-user up
sudo ethtool --offload veth-user rx off tx off
sudo ethtool -K veth-user gso off
sudo ip addr add ${NET}.1 peer ${NET}.2 scope link dev veth-user
sudo $K1_TOOLCHAIN_DIR/bin/k1-cluster --functional --march=k1b --user-syscall=$K1_TOOLCHAIN_DIR/lib64/libodp_syscall.so -- ./echop -d -f magic:veth-simu -i ${NET}.2 -g ${NET}.1
