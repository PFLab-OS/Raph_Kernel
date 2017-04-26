#!/bin/sh

ssh -F .ssh_config default "cd /vagrant/; addr2line -Cife build/kernel.elf FFFFFFFF$1"

