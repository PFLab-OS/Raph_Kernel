#!/bin/sh

vagrant ssh -c "cd /vagrant/; addr2line -Cife build/kernel.elf FFFFFFFF$1"

