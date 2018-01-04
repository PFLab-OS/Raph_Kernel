#!/bin/sh -eu

grep -R "RAPH""_DEBUG" * && { echo "debug code still remains"; exit 1; }
make ARCH=hw/x86 -f build_rule.mk test
make ARCH=hw/x86 -f build_rule.mk bin

