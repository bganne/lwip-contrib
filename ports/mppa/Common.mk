#
# Copyright (c) 2001, 2002 Swedish Institute of Computer Science.
# All rights reserved. 
# 
# Redistribution and use in source and binary forms, with or without modification, 
# are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
# 3. The name of the author may not be used to endorse or promote products
#    derived from this software without specific prior written permission. 
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
# SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
# OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
# IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
# OF SUCH DAMAGE.
#
# This file is part of the lwIP TCP/IP stack.
# 
# Author: Adam Dunkels <adam@sics.se>
#

NO_SYS?=1
SIMU?=0

ifeq ($(SIMU),0)
TARGET:=hw
else
TARGET:=simu
endif

CC:=k1-gcc
AR:=k1-ar
LD:=k1-ld

# Architecture specific files.
LWIPARCH?=$(CONTRIBDIR)/ports/mppa/port
SYSARCH?=$(LWIPARCH)/sys_arch.c
ARCHFILES=$(LWIPARCH)/perf.c $(SYSARCH) \
		  $(LWIPARCH)/netif/dummyif.c \
		  $(LWIPARCH)/netif/odpif.c \
		  $(LWIPARCH)/netif/pktio.c

include ../../Common.allports.mk

CFLAGS+= -march=k1b -mhypervisor -ffunction-sections -fdata-sections -DNO_SYS=$(NO_SYS)
LDFLAGS+= -mhypervisor -lodp -lmppapower -lmpparouting -lmppanoc -Wl,--gc-sections

ifeq ($(NO_SYS),1)
CFLAGS+= -mos=bare
LDFLAGS+= -lutask -L$(K1_TOOLCHAIN_DIR)/lib/odp/$(TARGET)/k1b-kalray-mos -L$(K1_TOOLCHAIN_DIR)/k1-elf/lib/mOS
else
CFLAGS+= -mos=nodeos -pthread
LDFLAGS+= -pthread -L$(K1_TOOLCHAIN_DIR)/lib/odp/$(TARGET)/k1b-kalray-nodeos -L$(K1_TOOLCHAIN_DIR)/k1-nodeos/lib/mOS
endif
