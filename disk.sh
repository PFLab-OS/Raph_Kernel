MOUNT_DIR="/mnt/Naias"
IMAGE="disk.img"

umount() {
    sudo umount ${MOUNT_DIR}
    sudo kpartx -d ${IMAGE}
}

loopsetup() {
    umount
    set -- $(sudo kpartx -av ${IMAGE})
    LOOPDEVICE=$8
    MAPPEDDEVICE=/dev/mapper/$3
}

mount() {
    loopsetup
    mkdir ${MOUNT_DIR}
    sudo mount -t ext2 ${MAPPEDDEVICE} ${MOUNT_DIR}
}

if [ $1 = "mount" ]; then
    mount
fi

if [ $1 = "umount" ]; then
    umount
fi

if [ $1 = "grub-install" ]; then
    loopsetup
	  mkfs -t ext2 ${MAPPEDDEVICE}
    mount
    sudo grub-install --no-floppy ${LOOPDEVICE} --root-directory ${MOUNT_DIR}
    umount
fi
