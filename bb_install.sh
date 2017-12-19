#!/bin/bash

card=$1
uname_r=$(uname -r)
_conf_file=/boot/uEnv.txt
_4mic_dtbo=/lib/firmware/seeed-4mic-voicecard-00A0.dtbo
_8mic_dtbo=/lib/firmware/seeed-8mic-voicecard-00A0.dtbo


if [[ $EUID -ne 0 ]]; then
	echo "This script must be run as root (use sudo)" 1>&2
	exit 1
fi


is_bbb=$(cat /proc/device-tree/model | awk '{ print $3 $4 }')
if [ "x${is_bbb}" != "xBeagleBoneBlack" ] ; then
	echo "Sorry, this drivers only works on BeagleBone Black and Variants"
	exit 1
fi


if [ "x${card}" = "x" ] ; then
	echo "Usage: $0 4mic|8mic"
	exit 1
fi


# rebuild the modules
apt-get update
apt-get -y install linux-headers-$uname_r
make

# install the modules
make install


# install dtbo overlays
if [ ! -f "$_4mic_dtbo" ]; then
	cp `basename $_4mic_dtbo` $_4mic_dtbo
fi
if [ ! -f "$_8mic_dtbo" ]; then
	cp `basename $_8mic_dtbo` $_8mic_dtbo
fi


# remove 4mic configuration
has_4mic=$(grep seeed-4mic-voicecard $_conf_file | head -n 1)
if [ "x${has_4mic}" != "x" ]; then
	sed -i "s,${has_4mic},,g" $_conf_file
fi

# remove 8mic configuration
has_8mic=$(grep seeed-8mic-voicecard $_conf_file | head -n 1)
if [ "x${has_8mic}" != "x" ]; then
	sed -i "s,${has_8mic},,g" $_conf_file
fi

case "${card}" in
"4mic")
	cmd="cp ac108_8mic.state /var/lib/alsa/asound.state"
	echo $cmd
	$cmd

	echo "uboot_overlay_addr4=$_4mic_dtbo  >> $_conf_file"
	echo "uboot_overlay_addr4=$_4mic_dtbo" >> $_conf_file
	;;

"8mic")
	cmd="cp ac108_8mic.state /var/lib/alsa/asound.state"
	echo $cmd
	$cmd

	echo "uboot_overlay_addr4=$_8mic_dtbo  >> $_conf_file"
	echo "uboot_overlay_addr4=$_8mic_dtbo" >> $_conf_file
	;;

*)
	echo "Please use 4mic or 8mic"
	;;
esac

sync
sleep 1
sync

echo "------------------------------------------------------"
echo "Please reboot your BeagleBone/Black to apply all settings"
echo "Enjoy!"
echo "------------------------------------------------------"
