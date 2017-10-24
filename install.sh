#!/bin/bash

if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root (use sudo)" 1>&2
   exit 1
fi

is_Raspberry=$(cat /proc/device-tree/model | awk  '{print $1}')
if [ "x${is_Raspberry}" != "xRaspberry" ] ; then
  echo "Sorry, this drivers only works on raspberry pi"
  exit 1
fi

ver="0.2"
card=$1

if [ "x${card}" = "x" ] ; then
  echo "Usage: ./install 2mic|4mic"
  exit 1
fi

# we create a dir with this version to ensure that 'dkms remove' won't delete
# the sources during kernel updates
marker="0.0.0"

apt update
apt-get -y install raspberrypi-kernel-headers raspberrypi-kernel
apt-get -y install  dkms

# locate currently installed kernels (may be different to running kernel if
# it's just been updated)
kernels=$(ls /lib/modules | sed "s/^/-k /")
uname_r=$(uname -r)

function install_module {
  src=$1
  mod=$2

  if [[ -d /var/lib/dkms/$mod/$ver/$marker ]]; then
    rmdir /var/lib/dkms/$mod/$ver/$marker
  fi

  if [[ -e /usr/src/$mod-$ver || -e /var/lib/dkms/$mod/$ver ]]; then
    dkms remove -m $mod -v $ver --all
    rm -rf /usr/src/$mod-$ver
  fi
  mkdir -p /usr/src/$mod-$ver
  cp -a $src/* /usr/src/$mod-$ver/
  dkms add -m $mod -v $ver
  dkms build $kernels -m $mod -v $ver && dkms install $kernels -m $mod -v $ver

  mkdir -p /var/lib/dkms/$mod/$ver/$marker
}
if [ ! -f "/boot/overlays/seeed-4mic-voicecard.dtbo" ] && [ ! -f "/lib/modules/${uname_r}/kernel/sound/soc/codecs/snd-soc-ac108.ko" ] ; then
  install_module "./" "seeed-voicecard"
  cp seeed-2mic-voicecard.dtbo /boot/overlays
  cp seeed-4mic-voicecard.dtbo /boot/overlays
  install -D ac108_plugin/libasound_module_pcm_ac108.so   /usr/lib/arm-linux-gnueabihf/alsa-lib/libasound_module_pcm_ac108.so
else
  echo "card driver already installed"
fi

grep -q "snd-soc-ac108" /etc/modules || \
  echo "snd-soc-ac108" >> /etc/modules
grep -q "snd-soc-wm8960" /etc/modules || \
  echo "snd-soc-wm8960" >> /etc/modules  


grep -q "dtoverlay=i2s-mmap" /boot/config.txt || \
  echo "dtoverlay=i2s-mmap" >> /boot/config.txt


grep -q "dtparam=i2s=on" /boot/config.txt || \
  echo "dtparam=i2s=on" >> /boot/config.txt

has_2mic=$(grep seeed-2mic-voicecard /boot/config.txt)
has_4mic=$(grep seeed-4mic-voicecard /boot/config.txt)
case "${card}" in
   "2mic") 
    echo "cp wm8960_asound.state /var/lib/alsa/asound.state"
    cp wm8960_asound.state /var/lib/alsa/asound.state
    cp asound_2mic.conf /etc/asound.conf
    if [ "x${has_4mic}" != x ] ; then
      echo "has 4mic before, now remove it"
      sed -i "s/dtoverlay=seeed-4mic-voicecard//g" /boot/config.txt
    fi
    grep -q "dtoverlay=seeed-2mic-voicecard" /boot/config.txt || \
      echo "dtoverlay=seeed-2mic-voicecard" >> /boot/config.txt
      
   ;;
   "4mic") 
    echo "cp ac108_asound.state /var/lib/alsa/asound.state"
    cp ac108_asound.state /var/lib/alsa/asound.state
    cp asound_4mic.conf /etc/asound.conf
    if [ "x${has_2mic}" != x ] ; then
      echo "has 2mic before, now remove it"
      sed -i "s/dtoverlay=seeed-2mic-voicecard//g" /boot/config.txt
    fi
    grep -q "dtoverlay=seeed-4mic-voicecard" /boot/config.txt || \
      echo "dtoverlay=seeed-4mic-voicecard" >> /boot/config.txt    
   ;;
   *) 
    echo "Please use 2mic or 4mic"
   ;;
esac

echo "------------------------------------------------------"
echo "Please reboot your raspberry pi to apply all settings"
echo "Enjoy!"
echo "------------------------------------------------------"
