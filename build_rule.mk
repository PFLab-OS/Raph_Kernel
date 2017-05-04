include common.mk

MOUNT_DIR = /mnt/Raph_Kernel
IMAGE = /tmp/$(IMAGEFILE)
BUILD_DIR = build

OVMF_DIR = /home/vagrant/edk2-UDK2017/

MAKE_SUBDIR := $(MAKE) -C
MAKE := $(MAKE) -f $(RULE_FILE)

ifdef INIT
	REMOTE_COMMAND += export INIT=$(INIT); 
	INIT_FILE = init_$(INIT)
else
	BRANCH_INIT_FILE = init_$(shell git rev-parse --abbrev-ref HEAD)
	INIT_FILE = $(if $(shell ls | grep $(BRANCH_INIT_FILE)),$(BRANCH_INIT_FILE),init)
endif

doc: export PROJECT_NUMBER:=$(shell git rev-parse HEAD ; git diff-index --quiet HEAD || echo "(with uncommitted changes)")

.PHONY: clean disk run image mount umount debugqemu showerror numerror doc

default: image

###################################
# for remote host (Vagrant)
###################################

run:
	$(MAKE) qemuend
	$(MAKE) qemurun
	-telnet 127.0.0.1 1235
	$(MAKE) qemuend

qemurun: image
	sudo qemu-system-x86_64 -drive if=pflash,readonly,file=$(OVMF_DIR)OVMF_CODE.fd,format=raw -drive if=pflash,file=$(OVMF_DIR)OVMF_VARS.fd,format=raw -cpu qemu64 -smp 8 -machine q35 -clock hpet -monitor telnet:127.0.0.1:1235,server,nowait -vnc 0.0.0.0:0,password $(IMAGE)&
#	sudo qemu-system-x86_64 -cpu qemu64,+x2apic -smp 8 -machine q35 -monitor telnet:127.0.0.1:1235,server,nowait -vnc 0.0.0.0:0,password -net nic -net bridge,br=br0 -drive id=disk,file=$(IMAGE),if=virtio -usb -usbdevice keyboard &
#	sudo qemu-system-x86_64 -cpu qemu64,+x2apic -smp 8 -machine q35 -monitor telnet:127.0.0.1:1235,server,nowait -vnc 0.0.0.0:0,password -net nic -net bridge,br=br0 -drive id=disk,file=$(IMAGE),if=none -device ahci,id=ahci -device ide-drive,drive=disk,bus=ahci.0 &
	sleep 0.2s
	echo "set_password vnc a" | netcat 127.0.0.1 1235

debugqemu:
	sudo gdb -x ./.gdbinit -p `ps aux | grep qemu | sed -n 2P | awk '{ print $$2 }'`

qemuend:
	-sudo pkill -KILL qemu

$(BUILD_DIR)/script:
	cp script $(BUILD_DIR)/script

$(BUILD_DIR)/init: $(INIT_FILE)
	cp $(INIT_FILE) $(BUILD_DIR)/init

$(BUILD_DIR)/fs.img:
	./source/tool/mkfs $(BUILD_DIR)/fs.img README.md

bin_sub: $(BUILD_DIR)/script $(BUILD_DIR)/init
	$(MAKE_SUBDIR) source

bin:
	mkdir -p $(BUILD_DIR)
	$(MAKE) bin_sub

image:
	$(MAKE) mount
	$(MAKE) bin
	sudo cp memtest86+.bin $(MOUNT_DIR)/boot/memtest86+.bin
	sudo cp grub.cfg $(MOUNT_DIR)/boot/grub/grub.cfg
	-sudo rm -rf $(MOUNT_DIR)/core
	sudo cp -r $(BUILD_DIR) $(MOUNT_DIR)/core
	$(MAKE) umount

_cpimage: _image
	cp $(IMAGE) /vagrant/

$(IMAGE):
	$(MAKE) umount
	dd if=/dev/zero of=$(IMAGE) bs=1M count=200
	sh disk.sh disk-setup
	sh disk.sh grub-install
	$(MAKE) mount
	sudo cp memtest86+.bin $(MOUNT_DIR)/boot/memtest86+.bin
	$(MAKE) umount

hd: image
	@if [ ! -e /dev/sdb ]; then echo "error: insert usb memory!"; exit -1; fi
	sudo dd if=$(IMAGE) of=/dev/sdb

disk:
	$(MAKE) diskclean
	$(MAKE) $(IMAGE)
	$(MAKE) image

mount: $(IMAGE)
	sh disk.sh mount

umount:
	sh disk.sh umount

deldisk: umount
	-rm -f $(IMAGE)

clean: deldisk
	-rm -rf $(BUILD_DIR)
	$(MAKE_SUBDIR) source clean

diskclean: deldisk clean

showerror:
	$(MAKE) image 2>&1 | egrep "In function|error:"

numerror:
	@echo -n "number of error: "
	@$(MAKE) image 2>&1 | egrep "error:" | wc -l

doc:
	doxygen

debug:
	vagrant ssh -c "cd /vagrant/; gdb -x .gdbinit_for_kernel"
