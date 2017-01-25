MOUNT_DIR = /mnt/Raph_Kernel
IMAGEFILE = disk.img
IMAGE = /tmp/$(IMAGEFILE)
BUILD_DIR = build

SSH_CMD = ssh -F .ssh_config default

VDI = disk.vdi
UNAME = ${shell uname}
ifeq ($(OS),Windows_NT)
	VNC = @echo windows is not supported; exit 1
else ifeq ($(UNAME),Linux)
	VNC = vncviewer localhost::15900
else ifeq ($(UNAME),Darwin)
	VNC = open vnc://localhost:15900
else
	VNC = @echo non supported OS; exit 1
endif

ifdef INIT
	REMOTE_COMMAND += export INIT=$(INIT); 
	INIT_FILE = init.$(INIT)
else
	INIT_FILE = init
endif

.PHONY: clean disk run image mount umount debugqemu showerror numerror vboxrun run_pxeserver pxeimg burn_ipxe burn_ipxe_remote vboxkill vnc synctime

default: image

###################################
# for remote host (Vagrant)
###################################

_run:
	$(MAKE) _qemurun
	-telnet 127.0.0.1 1234
	$(MAKE) _qemuend

_qemurun: _image
	sudo qemu-system-x86_64 -cpu qemu64,+x2apic -smp 8 -machine q35 -monitor telnet:127.0.0.1:1234,server,nowait -vnc 0.0.0.0:0,password -net nic -net bridge,br=br0 -drive id=disk,file=$(IMAGE),if=virtio -usb -usbdevice keyboard &
#	sudo qemu-system-x86_64 -cpu qemu64,+x2apic -smp 8 -machine q35 -monitor telnet:127.0.0.1:1234,server,nowait -vnc 0.0.0.0:0,password -net nic -net bridge,br=br0 -drive id=disk,file=$(IMAGE),if=none -device ahci,id=ahci -device ide-drive,drive=disk,bus=ahci.0 &
#	sudo qemu-system-x86_64 -smp 8 -machine q35 -monitor telnet:127.0.0.1:1234,server,nowait -vnc 0.0.0.0:0,password -net nic -net bridge,br=br0 -drive file=$(IMAGE),if=virtio &
	sleep 0.2s
	echo "set_password vnc a" | netcat 127.0.0.1 1234

_debugqemu:
	sudo gdb -x ./.gdbinit -p `ps aux | grep qemu | sed -n 2P | awk '{ print $$2 }'`

_qemuend:
	-sudo pkill -KILL qemu

_bin:
	-mkdir $(BUILD_DIR)
	cp script $(BUILD_DIR)/script
	cp $(INIT_FILE) $(BUILD_DIR)/init
	$(MAKE) -C source

_image:
	$(MAKE) _mount
	$(MAKE) _bin
	sudo cp memtest86+.bin $(MOUNT_DIR)/boot/memtest86+.bin
	sudo cp grub.cfg $(MOUNT_DIR)/boot/grub/grub.cfg
	-sudo rm -rf $(MOUNT_DIR)/core
	sudo cp -r $(BUILD_DIR) $(MOUNT_DIR)/core
	$(MAKE) _umount

_cpimage: _image
	cp $(IMAGE) /vagrant/

$(IMAGE):
	$(MAKE) _umount
	dd if=/dev/zero of=$(IMAGE) bs=1M count=20
	parted -s $(IMAGE) mklabel msdos -- mkpart primary 2048s -1
	sh disk.sh grub-install
	$(MAKE) _mount
	sudo cp memtest86+.bin $(MOUNT_DIR)/boot/memtest86+.bin
	$(MAKE) _umount

_hd: _image
	@if [ ! -e /dev/sdb ]; then echo "error: insert usb memory!"; exit -1; fi
	sudo dd if=$(IMAGE) of=/dev/sdb

_disk:
	$(MAKE) _diskclean
	$(MAKE) $(IMAGE)
	$(MAKE) _image

_mount: $(IMAGE)
	sh disk.sh mount

_umount:
	sh disk.sh umount

_deldisk: _umount
	-rm -f $(IMAGE)

_clean: _deldisk
	-rm -rf $(BUILD_DIR)
	$(MAKE) -C source clean

_diskclean: _deldisk _clean

_showerror:
	$(MAKE) _image 2>&1 | egrep "In function|error:"

_numerror:
	@echo -n "number of error: "
	@$(MAKE) _image 2>&1 | egrep "error:" | wc -l

###################################
# for local host
###################################

image: synctime
	@$(SSH_CMD) "$(REMOTE_COMMAND) cd /vagrant/; make -j3 _image"

run: synctime
	@$(SSH_CMD) "$(REMOTE_COMMAND) cd /vagrant/; make -j3 _run"

debugqemu: synctime
	@$(SSH_CMD) "$(REMOTE_COMMAND) cd /vagrant/; make _debugqemu"

hd: synctime
	@$(SSH_CMD) "$(REMOTE_COMMAND) cd /vagrant/; make -j3 _hd"

clean: synctime
	@$(SSH_CMD) "$(REMOTE_COMMAND) cd /vagrant/; make _clean"

showerror: synctime
	@$(SSH_CMD) "$(REMOTE_COMMAND) cd /vagrant/; make -j3 _showerror"

numerror: synctime
	@$(SSH_CMD) "$(REMOTE_COMMAND) cd /vagrant/; make -j3 _numerror"

vboxrun: vboxkill synctime
	@$(SSH_CMD) "$(REMOTE_COMMAND) cd /vagrant/; make -j3 _cpimage"
	-vboxmanage unregistervm RK_Test --delete
	-rm $(VDI)
	vboxmanage createvm --name RK_Test --register
	vboxmanage modifyvm RK_Test --cpus 4 --ioapic on --chipset ich9 --hpet on --x2apic on --nic1 nat --nictype1 82540EM
	vboxmanage convertfromraw $(IMAGEFILE) $(VDI)
	vboxmanage storagectl RK_Test --name SATAController --add sata --controller IntelAHCI --bootable on
	vboxmanage storageattach RK_Test --storagectl SATAController --port 0 --device 0 --type hdd --medium disk.vdi
	vboxmanage startvm RK_Test --type gui

run_pxeserver:
	make pxeimg
	@echo info: allow port 8080 in your firewall settings
	cd net; python -m SimpleHTTPServer 8080

pxeimg: synctime
	@$(SSH_CMD) "$(REMOTE_COMMAND) cd /vagrant/; make -j3 _cpimage"
	gzip $(IMAGEFILE)
	mv $(IMAGEFILE).gz net/

burn_ipxe: synctime
	./lan.sh local
	@$(SSH_CMD) "cd ipxe/src; make bin-x86_64-pcbios/ipxe.usb EMBED=/vagrant/load.cfg; if [ ! -e /dev/sdb ]; then echo 'error: insert usb memory!'; exit -1; fi; sudo dd if=bin-x86_64-pcbios/ipxe.usb of=/dev/sdb"

burn_ipxe_remote: synctime
	./lan.sh remote
	@$(SSH_CMD) "cd ipxe/src; make bin-x86_64-pcbios/ipxe.usb EMBED=/vagrant/load.cfg; if [ ! -e /dev/sdb ]; then echo 'error: insert usb memory!'; exit -1; fi; sudo dd if=bin-x86_64-pcbios/ipxe.usb of=/dev/sdb"

vboxkill:
	-vboxmanage controlvm RK_Test poweroff

vnc:
	@echo info: vnc password is "a"
	$(VNC)

synctime: .ssh_config
	@./time.sh

.ssh_config:
	vagrant ssh-config > .ssh_config
