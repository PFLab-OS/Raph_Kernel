MOUNT_DIR="/mnt/Raph_Kernel"
IMAGE="disk.img"

umount() {
    sudo umount ${MOUNT_DIR}
    sudo kpartx -d ${IMAGE}
    sudo losetup -d /dev/loop[0-9] > /dev/null 2>&1 || return 0
}

loopsetup() {
    set -- $(sudo kpartx -av ${IMAGE})
    LOOPDEVICE=$8
    MAPPEDDEVICE=/dev/mapper/$3
}

mount() {
    loopsetup
    sudo mkdir ${MOUNT_DIR}
    sudo mount -t ext2 ${MAPPEDDEVICE} ${MOUNT_DIR}
}

if [ $1 = "mount" ]; then
    mount
fi

if [ $1 = "umount" ]; then
    umount
fi

if [ $1 = "grub-install" ]; then
    umount
    loopsetup
    sudo mkfs -t ext2 ${MAPPEDDEVICE}
    mount
    sudo grub-install --no-floppy ${LOOPDEVICE} --root-directory ${MOUNT_DIR}
    umount
fi
