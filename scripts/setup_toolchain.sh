#!/bin/bash
set -e

export PREFIX="/opt/cross"
export TARGET=x86_64-elf
export PATH="$PREFIX/bin:$PATH"

mkdir -p /tmp/src && cd /tmp/src

# Binutils
wget https://ftp.gnu.org/gnu/binutils/binutils-2.41.tar.gz
tar xf binutils-2.41.tar.gz
mkdir build-binutils && cd build-binutils
../binutils-2.41/configure --target=$TARGET --prefix=$PREFIX \
    --with-sysroot --disable-nls --disable-werror
make -j$(nproc)
make install
cd ..

# GCC
wget https://ftp.gnu.org/gnu/gcc/gcc-13.2.0/gcc-13.2.0.tar.gz
tar xf gcc-13.2.0.tar.gz
mkdir build-gcc && cd build-gcc
../gcc-13.2.0/configure --target=$TARGET --prefix=$PREFIX \
    --disable-nls --enable-languages=c --without-headers
make all-gcc -j$(nproc)
make all-target-libgcc -j$(nproc)
make install-gcc
make install-target-libgcc