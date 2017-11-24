#!/bin/sh

grep -R "RAPH""_DEBUG" * && { echo "debug code still remains"; exit 1; }
make ARCH=hw/x86 -f build_rule.mk bin
