MOUNT_DIR="/mnt/Raph_Kernel"
MOUNT_DIR_EFI="/mnt/efi"
IMAGE="/tmp/disk.img"

umount() {
    sudo umount ${MOUNT_DIR}
    sudo umount ${MOUNT_DIR_EFI}
    sudo kpartx -d ${IMAGE}
    sudo losetup -d /dev/loop[0-9] > /dev/null 2>&1 || return 0
}

loopsetup() {
    set -- $(sudo kpartx -av ${IMAGE})
    LOOPDEVICE=${8}
    MAPPEDDEVICE_MBR=/dev/mapper/${3}
    MAPPEDDEVICE_EFI_PT=/dev/mapper/${12}
    MAPPEDDEVICE_DATA_PT=/dev/mapper/${21}
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
    umount
    mount
    sudo grub-install --target=i386-pc --no-floppy ${LOOPDEVICE} --root-directory ${MOUNT_DIR}
    sudo grub-install --target=x86_64-efi --no-nvram --efi-directory=${MOUNT_DIR_EFI} --boot-directory=${MOUNT_DIR}/boot
    sudo mv ${MOUNT_DIR_EFI}/EFI/ubuntu ${MOUNT_DIR_EFI}/EFI/boot
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
