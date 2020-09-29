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

CONFIG=/boot/config.txt
[ -f /boot/firmware/usercfg.txt ] && CONFIG=/boot/firmware/usercfg.txt

get_overlay() {
    ov=$1
    if grep -q -E "^dtoverlay=$ov" $CONFIG; then
      echo 0
    else
      echo 1
    fi
}

do_overlay() {
    ov=$1
    RET=$2
    DEFAULT=--defaultno
    CURRENT=0
    if [ $(get_overlay $ov) -eq 0 ]; then
        DEFAULT=
        CURRENT=1
    fi
    if [ $RET -eq $CURRENT ]; then
        ASK_TO_REBOOT=1
    fi
    if [ $RET -eq 0 ]; then
        sed $CONFIG -i -e "s/^#dtoverlay=$ov/dtoverlay=$ov/"
        if ! grep -q -E "^dtoverlay=$ov" $CONFIG; then
            printf "dtoverlay=$ov\n" >> $CONFIG
        fi
        STATUS=enabled
    elif [ $RET -eq 1 ]; then
        sed $CONFIG -i -e "s/^dtoverlay=$ov/#dtoverlay=$ov/"
        STATUS=disabled
    else
        return $RET
    fi
}

RPI_HATS="seeed-2mic-voicecard seeed-4mic-voicecard seeed-8mic-voicecard"

PATH=$PATH:/opt/vc/bin
echo -e "\n### Remove dtbos"
for i in $RPI_HATS; do
    dtoverlay -r $i
done
OVERLAYS=/boot/overlays
[ -d /boot/firmware/overlays ] && OVERLAYS=/boot/firmware/overlays

rm -v ${OVERLAYS}/seeed-2mic-voicecard.dtbo || true
rm -v ${OVERLAYS}/seeed-4mic-voicecard.dtbo || true
rm -v ${OVERLAYS}/seeed-8mic-voicecard.dtbo || true

echo -e "\n### Remove alsa configs"
rm -rfv  /etc/voicecard/ || true

echo -e "\n### Disable seeed-voicecard.service "
systemctl stop seeed-voicecard.service
systemctl disable seeed-voicecard.service 

echo -e "\n### Remove seeed-voicecard"
rm -v /usr/bin/seeed-voicecard || true
rm -v /lib/systemd/system/seeed-voicecard.service || true

echo -e "\n### Remove dkms"
rm -rfv /var/lib/dkms/seeed-voicecard || true

echo -e "\n### Unload codec driver by modprobe -r"
modprobe -r snd_soc_ac108
modprobe -r snd_soc_wm8960
modprobe -r snd_soc_seeed_voicecard

echo -e "\n### Remove kernel modules"
rm -v /lib/modules/*/kernel/sound/soc/codecs/snd-soc-wm8960.ko || true
rm -v /lib/modules/*/kernel/sound/soc/codecs/snd-soc-ac108.ko || true
rm -v /lib/modules/*/kernel/sound/soc/bcm/snd-soc-seeed-voicecard.ko || true
rm -v /lib/modules/*/updates/dkms/snd-soc-wm8960.ko || true
rm -v /lib/modules/*/updates/dkms/snd-soc-ac108.ko || true
rm -v /lib/modules/*/updates/dkms/snd-soc-seeed-voicecard.ko || true

echo -e "\n### Remove $CONFIG configuration"
for i in $RPI_HATS; do
    echo Uninstall $i ...
    do_overlay $i 1
done

echo "------------------------------------------------------"
echo "Please reboot your raspberry pi to apply all settings"
echo "Thank you!"
echo "------------------------------------------------------"
