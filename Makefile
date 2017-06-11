include common.mk

VNC_PORT = 15900

define make_wrapper
	$(if $(shell if [ -e /etc/bootstrapped ]; then echo "guest"; fi), \
	  # guest environment
	  $(MAKE) -f $(RULE_FILE) $(1), \
	  # host environment
	  $(if $(shell ssh -F .ssh_config default "exit"; if [ $$? != 0 ]; then echo "no-guest"; fi), rm -f .ssh_config
	  vagrant halt
	  vagrant up
	  vagrant ssh-config > .ssh_config; )
	  ssh -F .ssh_config default "cd /vagrant/; env MAKEFLAGS=$(MAKEFLAGS) make -f $(RULE_FILE) $(1)"
	)
endef

define check_guest
	$(if $(shell if [ -e /etc/bootstrapped ]; then echo "guest"; fi), \
	  @echo "error: run this command on the host environment."; exit 1)
endef

UNAME = ${shell uname}
ifeq ($(OS),Windows_NT)
define vnc
	@echo Windows is not supported; exit 1
endef
else ifeq ($(UNAME),Linux)
define vnc
	@echo open with VNC Viewer.
	@echo Please install it in advance.
	vncviewer localhost::$(VNC_PORT)
endef
else ifeq ($(UNAME),Darwin)
define vnc
	@echo open with the default VNC viewer.
	open vnc://localhost:$(VNC_PORT)
endef
else
define vnc
	@echo non supported OS; exit 1
endef
endif

.PHONY: clean disk run image mount umount debugqemu showerror numerror vboxrun run_pxeserver pxeimg burn_ipxe burn_ipxe_remote vboxkill vnc synctime

default: image

###################################
# for remote host (Vagrant)
###################################

_run:
	$(MAKE) _qemuend
	$(MAKE) _qemurun
	-telnet 127.0.0.1 1235
	$(MAKE) _qemuend

_qemurun: _image
  # for UEFI Boot
	sudo qemu-system-x86_64 -drive if=pflash,readonly,file=$(OVMF_DIR)OVMF_CODE.fd,format=raw -drive if=pflash,file=$(OVMF_DIR)OVMF_VARS.fd,format=raw -cpu qemu64 -smp 8 -machine q35 -clock hpet -monitor telnet:127.0.0.1:1235,server,nowait -vnc 0.0.0.0:0,password $(IMAGE)&
  # for BIOS Boot
	#sudo qemu-system-x86_64 -cpu qemu64 -smp 8 -machine q35 -clock hpet -monitor telnet:127.0.0.1:1235,server,nowait -vnc 0.0.0.0:0,password $(IMAGE)&
#	sudo qemu-system-x86_64 -cpu qemu64,+x2apic -smp 8 -machine q35 -monitor telnet:127.0.0.1:1235,server,nowait -vnc 0.0.0.0:0,password -net nic -net bridge,br=br0 -drive id=disk,file=$(IMAGE),if=virtio -usb -usbdevice keyboard &
#	sudo qemu-system-x86_64 -cpu qemu64,+x2apic -smp 8 -machine q35 -monitor telnet:127.0.0.1:1235,server,nowait -vnc 0.0.0.0:0,password -net nic -net bridge,br=br0 -drive id=disk,file=$(IMAGE),if=none -device ahci,id=ahci -device ide-drive,drive=disk,bus=ahci.0 &
	sleep 0.2s
	echo "set_password vnc a" | netcat 127.0.0.1 1235

_debugqemu:
	sudo gdb -x ./.gdbinit -p `ps aux | grep qemu | sed -n 2P | awk '{ print $$2 }'`

_qemuend:
	-sudo pkill -KILL qemu

$(BUILD_DIR)/script:
	cp script $(BUILD_DIR)/script

$(BUILD_DIR)/init: $(INIT_FILE)
	cp $(INIT_FILE) $(BUILD_DIR)/init

_bin_sub: $(BUILD_DIR)/script $(BUILD_DIR)/init
	$(MAKE) -C source

_bin:
	mkdir -p $(BUILD_DIR)
	$(MAKE) _bin_sub

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
	dd if=/dev/zero of=$(IMAGE) bs=1M count=200
	sh disk.sh disk-setup
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

default:
	$(call make_wrapper, image)

.PHONY: run hd image mount umount clean

run:
	$(call make_wrapper, run)

hd:
	$(call make_wrapper, hd)

image:
	$(call make_wrapper, image)

mount:
	$(call make_wrapper, mount)

umount:
	$(call make_wrapper, umount)

clean:
	$(call make_wrapper, clean)


.PHONY: vnc debug vboxrun vboxkill run_pxeserver pxeimg burn_ipxe burn_ipxe_remote debugqemu showerror numerror doc
vnc:
	$(call check_guest)
	@echo info: vnc password is "a"
	$(call vnc)

debug:
	vagrant ssh -c "cd /vagrant/; gdb -x .gdbinit_for_kernel"

vboxrun: vboxkill synctime
	@$(SSH_CMD) "$(REMOTE_COMMAND) cd /vagrant/; make -j3 _cpimage"
	-vboxmanage unregistervm RK_Test --delete
	-rm $(VDI)
	vboxmanage createvm --name RK_Test --register
	vboxmanage modifyvm RK_Test --cpus 4 --ioapic on --chipset ich9 --hpet on --x2apic on --nic1 nat --nictype1 82540EM --firmware efi
	vboxmanage convertfromraw $(IMAGEFILE) $(VDI)
	vboxmanage storagectl RK_Test --name SATAController --add sata --controller IntelAHCI --bootable on
	vboxmanage storageattach RK_Test --storagectl SATAController --port 0 --device 0 --type hdd --medium disk.vdi
	vboxmanage startvm RK_Test --type gui

vboxkill:
	-vboxmanage controlvm RK_Test poweroff

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

debugqemu:
	$(call make_wrapper, debugqemu)

showerror:
	$(call make_wrapper, showerror)

numerror:
	$(call make_wrapper, numerror)

doc:
	$(call make_wrapper, doc)

.ssh_config:
	vagrant ssh-config > .ssh_config
