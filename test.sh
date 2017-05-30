#!/bin/sh

grep -R "RAPH""_DEBUG" * && { echo "debug code still remains"; exit 1; }
make -f build_rule.mk bin
