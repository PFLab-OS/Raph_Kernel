#!/bin/bash

sudo sh -c 'test -f /etc/bootstrapped && exit'

sudo sed -i'~' -E "s@http://(..\.)?(archive|security)\.ubuntu\.com/ubuntu@http://linux.yz.yamagata-u.ac.jp/pub/linux/ubuntu-archive/@g" /etc/apt/sources.list


sudo DEBIAN_FRONTEND=noninteractive apt-get -qq update
sudo DEBIAN_FRONTEND=noninteractive apt-get -qq install -y gdebi git g++ make autoconf bison flex parted emacs language-pack-ja-base language-pack-ja kpartx gdb bridge-utils libyaml-dev silversearcher-ag ccache doxygen graphviz
sudo DEBIAN_FRONTEND=noninteractive apt-get -qq -o Dpkg::Options::="--force-confdef" -o Dpkg::Options::="--force-confold" install -y grub-efi
sudo DEBIAN_FRONTEND=noninteractive apt-get -qq install -y gdisk dosfstools
sudo update-locale LANG=ja_JP.UTF-8 LANGUAGE="ja_JP:ja"

echo 'export USE_CCACHE=1' >> ~/.bashrc
echo 'export CCACHE_DIR=~/.ccache' >> ~/.bashrc
echo 'export PATH="/usr/lib/ccache:$PATH"' >> ~/.bashrc

# install qemu
wget -q "http://drive.google.com/uc?export=download&id=0BzboiC2yUBwnZkU3QzMzMUc3cW8" -O qemu_2.9.0-1_amd64.deb
sudo gdebi --n -q qemu_2.9.0-1_amd64.deb
# sudo DEBIAN_FRONTEND=noninteractive apt-get -qq install -y libglib2.0-dev libfdt-dev libpixman-1-dev zlib1g-dev
# wget http://download.qemu-project.org/qemu-2.9.0.tar.xz
# tar xvJf qemu-2.9.0.tar.xz
# mkdir build-qemu
# cd build-qemu
# ../qemu-2.9.0/configure --target-list=x86_64-softmmu --disable-kvm --enable-debug
# make -j2
# sudo make install
# cd ..

# install OVMF
wget -q "http://drive.google.com/uc?export=download&id=0BzboiC2yUBwnYXhxNjRuNENCUVE" -O edk2-UDK2017.tar.gz
tar zxvf edk2-UDK2017.tar.gz
#sudo DEBIAN_FRONTEND=noninteractive apt-get -qq install -y build-essential uuid-dev nasm iasl
#git clone -b UDK2017 http://github.com/tianocore/edk2 --depth=1
#cd edk2
#make -C BaseTools
# . ./edksetup.sh
# build -a X64 -t GCC48 -p OvmfPkg/OvmfPkgX64.dsc
# mkdir ~/edk2-UDK2017
# cp Build/OvmfX64/DEBUG_GCC48/FV/*.fd ~/edk2-UDK2017
#cd ..

# make & install musl with CFLAGS="-fpie -fPIE"
wget -q "https://drive.google.com/uc?export=download&id=0BzboiC2yUBwnY3dVVGZBXzVia00" -O musl_20170509-1_amd64.deb
sudo gdebi --n -q musl_20170509-1_amd64.deb
# git clone -b v0.9.15 git://git.musl-libc.org/musl --depth=1
# cd musl
# export CFLAGS="-fpie -fPIE"
# ./configure --prefix=/usr/local/musl --exec-prefix=/usr/local
# unset CFLAGS
# make -j2
# sudo make install
# cd ..

# install grub
#sudo DEBIAN_FRONTEND=noninteractive apt-get -qq install -y libdevmapper-dev
#wget http://alpha.gnu.org/gnu/grub/grub-2.02~beta3.tar.gz
#tar zxvf grub-2.02~beta3.tar.gz
#cd grub-2.02~beta3
#./autogen.sh
#./configure
#make
#sudo make install
#cd ..

# install iPXE
sudo DEBIAN_FRONTEND=noninteractive apt-get -qq install -y build-essential binutils-dev zlib1g-dev libiberty-dev liblzma-dev
git clone git://git.ipxe.org/ipxe.git --depth=1
cd ipxe/
#make bin-x86_64-pcbios/ipxe.usb
cd ../

# install rust
#(curl -sSf https://static.rust-lang.org/rustup.sh | sh) || return 0
#echo "export PATH=\$PATH:~/.cargo/bin\n" >> /home/vagrant/.bashrc
#cargo install rustfmt --verbose

# setup bridge initialize script
sudo sed -i -e 's/exit 0//g' /etc/rc.local
sudo sh -c 'echo "ifconfig eth0 down" >> /etc/rc.local'
sudo sh -c 'echo "ifconfig eth0 up" >> /etc/rc.local'
sudo sh -c 'echo "ip addr flush dev eth0" >> /etc/rc.local'
sudo sh -c 'echo "brctl addbr br0" >> /etc/rc.local'
sudo sh -c 'echo "brctl stp br0 off" >> /etc/rc.local'
sudo sh -c 'echo "brctl setfd br0 0" >> /etc/rc.local'
sudo sh -c 'echo "brctl addif br0 eth0" >> /etc/rc.local'
sudo sh -c 'echo "ifconfig br0 up" >> /etc/rc.local'
sudo sh -c 'echo "dhclient br0" >> /etc/rc.local'
sudo sh -c 'echo "exit 0" >> /etc/rc.local'
sudo /etc/rc.local

sudo mkdir /usr/local/etc/qemu
sudo sh -c 'echo "allow br0" > /usr/local/etc/qemu/bridge.conf'

sudo mkdir "/mnt/Raph_Kernel"
sudo mkdir "/mnt/efi"

sudo sh -c 'date > /etc/bootstrapped'

echo "setup done!"
