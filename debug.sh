#!/bin/sh

addr2line -Cife build/kernel.elf FFFFFFFF$1
