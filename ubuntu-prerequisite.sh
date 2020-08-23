#!/bin/bash

# Copyright (c) Hin-Tak Leung 2020
#
# Overview:
#   This script compiles and install the Broadcom VideoCore tools,
#   configure the dynamic loader for the non-standard library location,
#   and update the loader cache.
#
#   A few steps explicitly requires root privilege, which are
#   marked with "sudo". The rest is just checking for duplicate/previous
#   action.
#
#   This derived from my command history on ubuntu 20.04.1 .YMMV

sudo apt install -y git gcc make alsa-utils cmake

git clone git://github.com/raspberrypi/userland.git
pushd userland/

arch=$(uname -m)
if [[ "$arch" =~ aarch64 ]]; then
    ./buildme --aarch64
else
    ./buildme
fi
# ./buildme already includes "sudo make install" at the end

popd

# matches Raspbian's location:
if [ ! -f /etc/ld.so.conf.d/00-vmcs.conf ] ; then
    echo "/opt/vc/lib" | sudo tee -a /etc/ld.so.conf.d/00-vmcs.conf
    sudo ldconfig -v
else
    echo "/etc/ld.so.conf.d/00-vmcs.conf exists - no need to update ld.cache!"
fi
