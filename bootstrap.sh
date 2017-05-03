#!/bin/sh
sudo -sh c 'test -f /etc/bootstrapped && exit'

sudo apt-get update
sudo apt-get install -y git g++ make parted emacs language-pack-ja-base language-pack-ja kpartx gdb bridge-utils libyaml-dev silversearcher-ag
sudo apt-get install -y grub-efi gdisk dosfstools
sudo update-locale LANG=ja_JP.UTF-8 LANGUAGE="ja_JP:ja"

# install qemu
sudo apt-get install -y libglib2.0-dev libfdt-dev libpixman-1-dev zlib1g-dev
wget http://download.qemu-project.org/qemu-2.9.0.tar.xz
tar xvJf qemu-2.9.0.tar.xz
mkdir build-qemu
cd build-qemu
../qemu-2.9.0/configure --target-list=x86_64-softmmu --disable-kvm --enable-debug
make -j2
sudo make install
cd ..

# install OVMF
sudo apt-get install -y build-essential uuid-dev nasm iasl
git clone http://github.com/tianocore/edk2
cd edk2
git checkout UDK2017
make -j2 -C BaseTools
. ./edksetup.sh
build -a X64 -t GCC48 -p OvmfPkg/OvmfPkgX64.dsc

wget http://downloads.sourceforge.net/project/edk2/OVMF/OVMF-X64-r15214.zip
mkdir OVMF-X64-r15214
cd OVMF-X64-r15214
unzip ~/OVMF-X64-r15214.zip
cd ..

# make & install musl with CFLAGS="-fpie -fPIE"
git clone git://git.musl-libc.org/musl
cd musl
git checkout refs/tags/v0.9.15
export CFLAGS="-fpie -fPIE"
./configure --prefix=/usr/local/musl --exec-prefix=/usr/local
unset CFLAGS
make -j2
sudo make install
cd ..

# install grub
#sudo apt-get install -y autoconf bison flex libdevmapper-dev
#wget http://alpha.gnu.org/gnu/grub/grub-2.02~beta3.tar.gz
#tar zxvf grub-2.02~beta3.tar.gz
#cd grub-2.02~beta3
#./autogen.sh
#./configure
#make
#sudo make install
#cd ..

# install iPXE
sudo apt-get install -y build-essential binutils-dev zlib1g-dev libiberty-dev liblzma-dev
git clone git://git.ipxe.org/ipxe.git
cd ipxe/
make bin-x86_64-pcbios/ipxe.usb
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

# setup ntp
sudo apt-get install ntp
sudo service ntp stop
sudo ntpdate ntp.nict.jp
sed -i -e 's/^server/#server/g' /etc/ntp.conf
sudo sh -c 'echo "server ntp.nict.jp" >> /etc/ntp.conf'
sudo service ntp start

sudo sh -c 'date > /etc/bootstrapped'

echo "setup done!"
