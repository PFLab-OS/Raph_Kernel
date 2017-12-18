#!/bin/sh -x
MOUNT_DIR="/mnt/Raph_Kernel"
MOUNT_DIR_EFI="/mnt/efi"
IMAGE="/tmp/disk.img"

umount() {
    sudo umount ${MOUNT_DIR} > /dev/null 2>&1
    sudo umount ${MOUNT_DIR_EFI} > /dev/null 2>&1
    sudo kpartx -d ${IMAGE}
    sudo losetup -d /dev/loop[0-9] > /dev/null 2>&1 || return 0
    sudo dmsetup remove_all
}

loopsetup() {
    umount
    if [ -e /dev/mapper/loop0p1 ]; then
	      echo "failed to cleanup"
	      exit 1
    fi
    sudo kpartx -avs ${IMAGE}
    LOOPDEVICE=/dev/loop0
    MAPPEDDEVICE_MBR=/dev/mapper/loop0p1
    MAPPEDDEVICE_EFI_PT=/dev/mapper/loop0p2
    MAPPEDDEVICE_DATA_PT=/dev/mapper/loop0p3
}

mount() {
    loopsetup
    sudo mount -t ext2 ${MAPPEDDEVICE_DATA_PT} ${MOUNT_DIR}
    sudo mount -t vfat ${MAPPEDDEVICE_EFI_PT} ${MOUNT_DIR_EFI}
}

if [ $1 = "mount" ]; then
    mount
fi

if [ $1 = "umount" ]; then
    umount
fi

if [ $1 = "grub-install" ]; then
    mount
    sudo grub-install --target=i386-pc --no-floppy ${LOOPDEVICE} --root-directory ${MOUNT_DIR}
    sudo grub-install --target=x86_64-efi --no-nvram --efi-directory=${MOUNT_DIR_EFI} --boot-directory=${MOUNT_DIR}/boot
    sudo grub-mkfont -o ${MOUNT_DIR}/boot/grub/fonts/unicode.pf2 /usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf
    sudo mv ${MOUNT_DIR_EFI}/EFI/grub ${MOUNT_DIR_EFI}/EFI/boot
    sudo mv ${MOUNT_DIR_EFI}/EFI/boot/grubx64.efi ${MOUNT_DIR_EFI}/EFI/boot/bootx64.efi
    umount
fi

if [ $1 = "disk-setup" ]; then
    umount
    sgdisk -a 1 -n 1::2047 -t 1:EF02 -a 2048 -n 2::100M -t 2:EF00 -n 3:: -t 3:8300 ${IMAGE}
    loopsetup
    sudo mkfs.vfat -F32 ${MAPPEDDEVICE_EFI_PT}
    sudo mkfs.ext2 ${MAPPEDDEVICE_DATA_PT}
    umount
fi
