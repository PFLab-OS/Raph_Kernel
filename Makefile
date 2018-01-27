include common.mk

ARCH ?= hw/x86
export ARCH

VNC_PORT = 15900
VDI = disk.vdi

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

define check_guest
	$(if $(shell if [ -e /etc/bootstrapped ]; then echo "guest"; fi), \
	  @echo "error: run this command on the host environment."; exit 1)
endef

define run_remote
	vagrant ssh -c 'trap "cd /vagrant; make ARCH=$(ARCH) -f $(BUILD_RULE_FILE) qemuend; exit 1" SIGINT; $(1)'
endef

define make_wrapper
	$(if $(shell if [ -e /etc/bootstrapped ]; then echo "guest"; fi), \
	  # guest environment
    cd /vagrant; $(MAKE) ARCH=$(ARCH) -f $(BUILD_RULE_FILE) $(1), \
	  # host environment
	  @echo Running \"make $(1)\" on the remote build environment.
	  @$(call run_remote, cd /vagrant; env MAKEFLAGS="$(MAKEFLAGS)" make ARCH=$(ARCH) -f $(BUILD_RULE_FILE) $(1))
	)
endef

default:
	$(call make_wrapper,all)

.PHONY: vnc vboxrun vboxkill run_pxeserver pxeimg burn_ipxe burn_ipxe_remote
vnc:
	$(call check_guest)
	@echo info: vnc password is "a"
	$(call vnc)

vboxrun: vboxkill
	$(call make_wrapper,cpimage)
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

pxeimg:
	$(call make_wrapper,cpimage)
	gzip $(IMAGEFILE)
	mv $(IMAGEFILE).gz net/

burn_ipxe:
	$(call check_guest)
	./lan.sh local
	$(call run_remote, cd ipxe/src; make bin-x86_64-pcbios/ipxe.usb EMBED=/vagrant/load.cfg; if [ ! -e /dev/sdb ]; then echo 'error: insert usb memory!'; exit -1; fi; sudo dd if=bin-x86_64-pcbios/ipxe.usb of=/dev/sdb)

burn_ipxe_remote:
	$(call check_guest)
	./lan.sh remote
	$(call run_remote, cd ipxe/src; make bin-x86_64-pcbios/ipxe.usb EMBED=/vagrant/load.cfg; if [ ! -e /dev/sdb ]; then echo 'error: insert usb memory!'; exit -1; fi; sudo dd if=bin-x86_64-pcbios/ipxe.usb of=/dev/sdb)

%:
	$(call make_wrapper,$@)
