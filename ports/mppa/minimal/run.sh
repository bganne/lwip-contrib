#!/bin/bash
NET=192.168.10
MTU=9000
IFACE='e0'
OPTS='tags=120:jumbo:verbose:hashpolicy=[P0{@12/0x3=0x800}{@23/0x1=6}{@26#0xff}{@34#0xf}][P1{@12/0x3=0x800}{@23/0x1=17}{@26#0xff}{@34#0xf}][P2{@12/0x3=0x800}{@26#0xff}'

[ -z $K1_TOOLCHAIN_DIR ] && K1_TOOLCHAIN_DIR=/usr/local/k1tools
export LD_LIBRARY_PATH=$K1_TOOLCHAIN_DIR/lib64:$LD_LIBRARY_PATH
export PATH=$K1_TOOLCHAIN_DIR/bin:$PATH
FIRMWARE="$K1_TOOLCHAIN_DIR/share/odp/firmware/konic80/k1b/iounified.kelf"
exec "$K1_TOOLCHAIN_DIR/bin/k1-jtag-runner" --progress --exec-file=IODDR0:"$FIRMWARE" --exec-file=IODDR1:"$FIRMWARE" --exec-file=Cluster0:./echop -- echop -d -f "$IFACE:$OPTS" -i "${NET}.2" -g "${NET}.1"
