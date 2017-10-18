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

uname_r=$(uname -r)
card=$1
if [ x${card} = "x" ] ; then
	echo "Usage: ./uninstall 2mic|4mic"
	exit 1
fi

if [ x${card} = "x2mic" ] ; then
	echo "delete dtoverlay=seeed-2mic-voicecard in /boot/config.txt"
	sed -i "s/dtoverlay=seeed-2mic-voicecard//g" /boot/config.txt
	if [ -f /boot/overlays/seeed-2mic-voicecard.dtbo ] ; then
		echo "remove seeed-2mic-voicecard.dtbo in /boot/overlays"
		rm /boot/overlays/seeed-2mic-voicecard.dtbo
	fi 

	if [ -f /lib/modules/${uname_r}/kernel/sound/soc/codecs/snd-soc-wm8960.ko ] ; then
		echo "remove snd-soc-wm8960.ko"
		rm  /lib/modules/${uname_r}/kernel/sound/soc/codecs/snd-soc-wm8960.ko
	fi

	if [ -d /var/lib/dkms/seeed-voicecard ] ; then
		echo "remove seeed-voicecard dkms"
		rm  -rf  /var/lib/dkms/seeed-voicecard
	fi

	echo "delete snd-soc-wm8960 in /etc/modules"
	sed -i "s/snd-soc-wm8960//g" /etc/modules

	if [ -f /var/lib/alsa/asound.state ] ; then 
		echo  "remove wm8960_asound.state"
		rm /var/lib/alsa/asound.state
	fi

	if [ -f /etc/asound.conf ] ; then
        echo  "remove asound_2mic.conf"
        rm /etc/asound.conf
	fi	
fi

if [ x${card} = "x4mic" ] ; then
        echo "delete dtoverlay=seeed-4mic-voicecard in /boot/config.txt"
        sed -i "s/dtoverlay=seeed-4mic-voicecard//g" /boot/config.txt
	
	if [ -f /boot/overlays/seeed-4mic-voicecard.dtbo ] ; then
        echo "remove seeed-4mic-voicecard.dtbo in /boot/overlays"
        rm /boot/overlays/seeed-4mic-voicecard.dtbo
	fi

	if [ -f /lib/modules/${uname_r}/kernel/sound/soc/codecs/snd-soc-ac108.ko ] ; then
        echo "remove snd-soc-ac108.ko"
        rm  /lib/modules/${uname_r}/kernel/sound/soc/codecs/snd-soc-ac108.ko
	fi

	if [ -d /var/lib/dkms/seeed-voicecard ] ; then
        echo "remove seeed-voicecard dkms"
        rm  -rf  /var/lib/dkms/seeed-voicecard
	fi 

        echo "delete snd-soc-ac108 in /etc/modules"
        sed -i "s/snd-soc-ac108//g" /etc/modules                

	if [ -f /var/lib/alsa/asound.state ] ; then
        echo  "remove ac108_asound.state"
        rm /var/lib/alsa/asound.state
	fi

	if [ -f /etc/asound.conf ] ; then
        echo  "remove asound_4mic.conf"
        rm /etc/asound.conf
	fi

	if [ -f /usr/lib/arm-linux-gnueabihf/alsa-lib/libasound_module_pcm_ac108.so ] ; then
		echo "remove libasound_module_pcm_ac108.so in /usr/lib/arm-linux-gnueabihf/alsa-lib/ "
		rm  /usr/lib/arm-linux-gnueabihf/alsa-lib/libasound_module_pcm_ac108.so
	fi 
fi

echo "------------------------------------------------------"
echo "Please reboot your raspberry pi to apply all settings"
echo "Thank you!"
echo "------------------------------------------------------"
