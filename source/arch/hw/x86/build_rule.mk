include $(RAPH_PROJECT_ROOT)/common.mk

MOUNT_DIR = /mnt/Raph_Kernel
IMAGE = /tmp/$(IMAGEFILE)
BUILD_DIR = $(RAPH_PROJECT_ROOT)/build

OVMF_DIR = $(HOME)/edk2-UDK2017/

MAKE_SUBDIR := $(MAKE) -C
MAKE := $(MAKE) ARCH=$(ARCH) -f $(BUILD_RULE_FILE)

ifdef INIT
	REMOTE_COMMAND += export INIT=$(INIT); 
	INIT_FILE = $(INIT)
else
	BRANCH_INIT_FILE = $(shell git rev-parse --abbrev-ref HEAD)
	INIT_FILE = $(if $(shell if [ -e init_script/$(BRANCH_INIT_FILE) ] ; then echo "file exist"; fi),$(BRANCH_INIT_FILE),default)
endif

GRUBCFG_COMMAND := sed -e "s/\/core\/init_script\/default/\/core\/init_script\/$(INIT_FILE)/g" grub.cfg

ifdef KERNEL_DEBUG
	QEMU_OPTIONS += -s -S 
endif

doc: export PROJECT_NUMBER:=$(shell git rev-parse HEAD ; git diff-index --quiet HEAD || echo "(with uncommitted changes)")

.PHONY: clean all disk run image image_file_copy image_mount_and_file_copy mount umount debugqemu showerror numerror doc debug

default: image

###################################
# for remote host (Vagrant)
###################################

all: image

run:
	$(MAKE) qemuend
	$(MAKE) qemurun
	-telnet 127.0.0.1 1235
	$(MAKE) qemuend

qemurun: image
	sudo service networking restart
	sudo qemu-system-x86_64 \
		-cpu qemu64 -smp 8 -machine q35 -clock hpet \
		-monitor telnet:127.0.0.1:1235,server,nowait \
		-vnc 0.0.0.0:0,password \
		-net nic \
		-net bridge,br=br0 \
		-drive if=pflash,readonly,file=$(OVMF_DIR)OVMF_CODE.fd,format=raw \
		-drive if=pflash,file=$(OVMF_DIR)OVMF_VARS.fd,format=raw \
		$(QEMU_OPTIONS) \
		$(IMAGE)&
#    -usb -usbdevice keyboard \
# -net dump,file=./packet.pcap \
#	-drive id=disk,file=$(IMAGE),if=none -device ahci,id=ahci -device ide-drive,drive=disk,bus=ahci.0
	sleep 0.2s
	echo "set_password vnc a" | netcat 127.0.0.1 1235

debugqemu:
	sudo gdb -x ./.gdbinit -p `ps aux | grep qemu | sed -n 2P | awk '{ print $$2 }'`

qemuend:
	(echo "quit" | netcat 127.0.0.1 1235) || :
	sleep 0.2s
	(sudo pkill -KILL qemu && echo "force killing QEMU") || :

$(BUILD_DIR)/script:
	cp script $@

$(BUILD_DIR)/rump.bin:
	cp rump.bin $(BUILD_DIR)/rump.bin

$(BUILD_DIR)/init_script/$(INIT_FILE): init_script/$(INIT_FILE)
	mkdir -p $(BUILD_DIR)/init_script/
	cp $^ $@

$(RAPH_PROJECT_ROOT)/source/tool/mkfs:
	@$(MAKE_SUBDIR) $(RAPH_PROJECT_ROOT)/source/tool build

$(BUILD_DIR)/fs.img: $(RAPH_PROJECT_ROOT)/source/tool/mkfs
	-rm $@ &> /dev/null
	cp $(RAPH_PROJECT_ROOT)/README.md readme.md
	$(RAPH_PROJECT_ROOT)/source/tool/mkfs $@ readme.md
	rm readme.md

bin_sub: $(BUILD_DIR)/script $(BUILD_DIR)/init_script/$(INIT_FILE) $(BUILD_DIR)/fs.img $(BUILD_DIR)/rump.bin
	@$(MAKE_SUBDIR) ../libc
	@$(MAKE_SUBDIR) $(RAPH_PROJECT_ROOT)/source/kernel build
	@$(MAKE_SUBDIR) $(RAPH_PROJECT_ROOT)/source/testmodule build

bin:
	mkdir -p $(BUILD_DIR)
	$(MAKE) bin_sub

$(MOUNT_DIR)/boot/memtest86+.bin: memtest86+.bin
	sudo cp $^ $@

image_file_copy: $(MOUNT_DIR)/boot/memtest86+.bin
	@sudo mkdir -p $(MOUNT_DIR)/core
	$(GRUBCFG_COMMAND) > /tmp/grub.cfg
	sudo sh -c '$(GRUBCFG_COMMAND) > $(MOUNT_DIR)/boot/grub/grub.cfg'
	sudo cp -ruv $(BUILD_DIR)/* $(MOUNT_DIR)/core/

image_mount_and_file_copy:
	$(MAKE) mount
	$(MAKE) image_file_copy
	$(MAKE) umount

image:
	$(MAKE) bin
	@mkdir -p /tmp/img
	@if [ -n "`cp -ruv $(BUILD_DIR)/* /tmp/img`" ] || [ ! -e /tmp/grub.cfg ] || [ "`$(GRUBCFG_COMMAND) | sum`" != "`sum /tmp/grub.cfg`" ]; then $(MAKE) image_mount_and_file_copy; fi

cpimage: image
	cp $(IMAGE) /vagrant/

$(IMAGE):
	$(MAKE) umount
	dd if=/dev/zero of=$(IMAGE) bs=1M count=200
	@echo "[disk.sh] disk-setup..."
	@sh disk.sh disk-setup > /dev/null
	@echo "[disk.sh] grub-install..."
	@sh disk.sh grub-install > /dev/null

hd: image
	@if [ ! -e /dev/sdb ]; then echo "error: insert usb memory!"; exit -1; fi
	sudo dd if=$(IMAGE) of=/dev/sdb bs=1M

disk:
	$(MAKE) diskclean
	$(MAKE) $(IMAGE)
	$(MAKE) image

mount: $(IMAGE)
	@echo "[disk.sh] mount image..."
	@sh disk.sh mount > /dev/null

umount:
	@echo "[disk.sh] umount image......"
	@sh disk.sh umount > /dev/null

deldisk: umount
	-rm -rf $(IMAGE) /tmp/img /tmp/grub.cfg

clean: deldisk
	-rm -rf $(BUILD_DIR)
	@$(MAKE_SUBDIR) ../libc clean
	@$(MAKE_SUBDIR) $(RAPH_PROJECT_ROOT)/source/kernel clean
	@$(MAKE_SUBDIR) $(RAPH_PROJECT_ROOT)/source/testmodule clean
	@$(MAKE_SUBDIR) $(RAPH_PROJECT_ROOT)/source/tool clean

diskclean: deldisk clean

showerror:
	$(MAKE) image 2>&1 | egrep "In function|error:"

numerror:
	@echo -n "number of error: "
	@$(MAKE) image 2>&1 | egrep "error:" | wc -l

doc:
	doxygen

debug:
	gdb -x .gdbinit_for_kernel
