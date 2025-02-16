#!/bin/bash


apt install git zip
cd /opt
git clone https://github.com/mxe/mxe.git;
sudo chown -R `whoami`: mxe
cd mxe;
sudo apt update
sudo apt-get install \
    autoconf \
    automake \
    autopoint \
    bash \
    bison \
    bzip2 \
    flex \
    g++ \
    g++-multilib \
    gettext \
    git \
    gperf \
    intltool \
    libc6-dev-i386 \
    libgdk-pixbuf2.0-dev \
    libltdl-dev \
    libgl-dev \
    libssl-dev \
    libtool-bin \
    libxml-parser-perl \
    lzip \
    make \
    openssl \
    p7zip-full \
    patch \
    perl \
    python3 \
    python3-mako \
    python3-pkg-resources \
    python-is-python3 \
    ruby \
    sed \
    unzip \
    wget \
    xz-utils;
make MXE_TARGETS="x86_64-w64-mingw32.static i686-w64-mingw32.static" JOBS=8 sdl sdl_gfx sdl_mixer sdl_image
make MXE_TARGETS="x86_64-w64-mingw32.static i686-w64-mingw32.static" MXE_PLUGIN_DIRS=plugins/gcc14 JOBS=8 gcc

cd ..
git clone https://github.com/MeridianOXC/OpenXcom.git
cd OpenXcom
rm deps/dummy.txt
rmdir deps


#windows build
mkdir -p build/win64
cd build/win64
export PATH=/opt/mxe/usr/bin:$PATH
/opt/mxe/usr/bin/x86_64-w64-mingw32.static-cmake -DCMAKE_BUILD_TYPE=Release -DDEV_BUILD=OFF -DBUILD_PACKAGE=OFF -B. -S../..
make -j8
cd -


#linux build

mkdir -p build/linux
cd build/linux
cmake -DCMAKE_BUILD_TYPE=Release -DDEV_BUILD=OFF -DBUILD_PACKAGE=OFF -B. -S../..
make -j8
cd -
