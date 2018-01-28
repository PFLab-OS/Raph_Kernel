#!/bin/sh

wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add -
add-apt-repository "deb http://apt.llvm.org/xenial/ llvm-toolchain-xenial-6.0 main"
apt-get update; apt-get install -y clang-format-6.0
ln -s /usr/bin/clang-format-6.0 /usr/bin/clang-format

