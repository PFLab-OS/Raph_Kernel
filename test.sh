#!/bin/sh

grep -R "RAPH""_DEBUG" * && { echo "debug code still remains"; exit 1; }
cd source/kernel/arch/hw/x86
make ARCH=hw/x86 -f build_rule.mk bin
