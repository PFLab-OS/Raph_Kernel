MOUNT_DIR = /mnt/Raph_Kernel
IMAGE = disk.img
BUILD_DIR = build

.PHONY: clean disk mount

default: image

run: image
	qemu-system-x86_64 -hda $(IMAGE) -monitor stdio -nographic

#$(CORE_FILE): $(subst $(MOUNT_DIR)/core,$(BUILD),$@)
#	cp $< $@

image:
	make mount
	-mkdir $(BUILD_DIR)
	make -C source
	sudo cp grub.cfg $(MOUNT_DIR)/boot/grub/grub.cfg 
	-sudo rm -r $(MOUNT_DIR)/core
	sudo cp -r $(BUILD_DIR) $(MOUNT_DIR)/core
	make umount

$(IMAGE):
	dd if=/dev/zero of=$(IMAGE) bs=1M count=10
	parted -s $(IMAGE) mklabel msdos -- mkpart primary 2048s -1
	sh disk.sh grub-install

disk: $(IMAGE)

mount: $(IMAGE)
	sh disk.sh mount

umount:
	sh disk.sh umount

deldisk: umount
	-rm $(IMAGE)

clean: deldisk
	-rm -r $(BUILD_DIR)
	make -C source clean

diskclean: deldisk clean
