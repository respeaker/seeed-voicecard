#!/bin/bash

if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root (use sudo)" 1>&2
   exit 1
fi

#
# make sure that we are on something ARM/Raspberry related
# either a bare metal Raspberry or a qemu session with 
# Raspberry stuff available
# - check for /boot/overlays
# - dtparam and dtoverlay is available
errorFound=0
if [ ! -d /boot/overlays ] ; then
  echo "/boot/overlays not found or not a directory" 1>&2
  errorFound=1
fi
# should we also check for alsactl and amixer used in seeed-voicecard?
for cmd in dtparam dtoverlay ; do
  if ! which $cmd &>/dev/null ; then
    echo "$cmd not found" 1>&2
    errorFound=1
  fi
done
if [ $errorFound = 1 ] ; then
  echo "Errors found, exiting." 1>&2
  exit 1
fi

ver="0.3"
uname_r=$(uname -r)

# we create a dir with this version to ensure that 'dkms remove' won't delete
# the sources during kernel updates
marker="0.0.0"

# update and install required packages
which apt &>/dev/null
if [[ $? -eq 0 ]]; then
  apt update -y
  apt-get -y install raspberrypi-kernel-headers raspberrypi-kernel 
  apt-get -y install dkms git i2c-tools libasound2-plugins
fi

# Arch Linux
which pacman &>/dev/null
if [[ $? -eq 0 ]]; then
  pacman -Syu --needed git gcc automake make dkms linux-raspberrypi-headers i2c-tools
fi

# locate currently installed kernels (may be different to running kernel if
# it's just been updated)
kernels=$(ls /lib/modules | sed "s/^/-k /")

function install_module {
  src=$1
  mod=$2

  if [[ -d /var/lib/dkms/$mod/$ver/$marker ]]; then
    rmdir /var/lib/dkms/$mod/$ver/$marker
  fi

  if [[ -e /usr/src/$mod-$ver || -e /var/lib/dkms/$mod/$ver ]]; then
    dkms remove --force -m $mod -v $ver --all
    rm -rf /usr/src/$mod-$ver
  fi
  mkdir -p /usr/src/$mod-$ver
  cp -a $src/* /usr/src/$mod-$ver/
  dkms add -m $mod -v $ver
  dkms build $kernels -m $mod -v $ver && dkms install --force $kernels -m $mod -v $ver

  mkdir -p /var/lib/dkms/$mod/$ver/$marker
}

install_module "./" "seeed-voicecard"


# install dtbos
cp seeed-2mic-voicecard.dtbo /boot/overlays
cp seeed-4mic-voicecard.dtbo /boot/overlays
cp seeed-8mic-voicecard.dtbo /boot/overlays

#install alsa plugins
# no need this plugin now
# install -D ac108_plugin/libasound_module_pcm_ac108.so /usr/lib/arm-linux-gnueabihf/alsa-lib/
rm -f /usr/lib/arm-linux-gnueabihf/alsa-lib/libasound_module_pcm_ac108.so

#set kernel moduels
grep -q "^snd-soc-seeed-voicecard$" /etc/modules || \
  echo "snd-soc-seeed-voicecard" >> /etc/modules
grep -q "^snd-soc-ac108$" /etc/modules || \
  echo "snd-soc-ac108" >> /etc/modules
grep -q "^snd-soc-wm8960$" /etc/modules || \
  echo "snd-soc-wm8960" >> /etc/modules  

#set dtoverlays
sed -i -e 's:#dtparam=i2c_arm=on:dtparam=i2c_arm=on:g'  /boot/config.txt || true
grep -q "^dtoverlay=i2s-mmap$" /boot/config.txt || \
  echo "dtoverlay=i2s-mmap" >> /boot/config.txt


grep -q "^dtparam=i2s=on$" /boot/config.txt || \
  echo "dtparam=i2s=on" >> /boot/config.txt

#install config files
mkdir /etc/voicecard || true
cp *.conf /etc/voicecard
cp *.state /etc/voicecard

#create git repo
git_email=$(git config --global --get user.email)
git_name=$(git config --global --get user.name)
if [ "x${git_email}" == "x" ] || [ "x${git_name}" == "x" ] ; then
    echo "setup git config"
    git config --global user.email "respeaker@seeed.cc"
    git config --global user.name "respeaker"
fi
echo "git init"
git --git-dir=/etc/voicecard/.git init
echo "git add --all"
git --git-dir=/etc/voicecard/.git --work-tree=/etc/voicecard/ add --all
echo "git commit -m \"origin configures\""
git --git-dir=/etc/voicecard/.git --work-tree=/etc/voicecard/ commit  -m "origin configures"

cp seeed-voicecard /usr/bin/
cp seeed-voicecard.service /lib/systemd/system/
systemctl enable  seeed-voicecard.service 

echo "------------------------------------------------------"
echo "Please reboot your raspberry pi to apply all settings"
echo "Enjoy!"
echo "------------------------------------------------------"
