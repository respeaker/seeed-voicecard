#!/bin/bash

# Color
RED='\033[0;31m'
NC='\033[0m' # No Color

if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root (use sudo)" 1>&2
   exit 1
fi

# Check for enough space on /boot volume
boot_line=$(df -h | grep /boot | head -n 1)
if [ "x${boot_line}" = "x" ]; then
  echo "Warning: /boot volume not found .."
else
  boot_space=$(echo $boot_line | awk '{print $4;}')
  free_space=$(echo "${boot_space%?}")
  unit="${boot_space: -1}"
  if [[ "$unit" = "K" ]]; then
    echo "Error: Not enough space left ($boot_space) on /boot"
    exit 1
  elif [[ "$unit" = "M" ]]; then
    if [ "$free_space" -lt "25" ]; then
      echo "Error: Not enough space left ($boot_space) on /boot"
      exit 1
    fi
  fi
fi

#
# make sure that we are on something ARM/Raspberry related
# either a bare metal Raspberry or a qemu session with 
# Raspberry stuff available
# - check for /boot/overlays
# - dtparam and dtoverlay is available
errorFound=0
OVERLAYS=/boot/overlays
[ -d /boot/firmware/overlays ] && OVERLAYS=/boot/firmware/overlays

if [ ! -d $OVERLAYS ] ; then
  echo "$OVERLAYS not found or not a directory" 1>&2
  errorFound=1
fi
# should we also check for alsactl and amixer used in seeed-voicecard?
PATH=$PATH:/opt/vc/bin
for cmd in dtparam dtoverlay ; do
  if ! which $cmd &>/dev/null ; then
    echo "$cmd not found" 1>&2
    echo "You may need to run ./ubuntu-prerequisite.sh"
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

_VER_RUN=
function get_kernel_version() {
  local ZIMAGE IMG_OFFSET

  _VER_RUN=""
  [ -z "$_VER_RUN" ] && {
    ZIMAGE=/boot/kernel.img
    [ -f /boot/firmware/vmlinuz ] && ZIMAGE=/boot/firmware/vmlinuz
    IMG_OFFSET=$(LC_ALL=C grep -abo $'\x1f\x8b\x08\x00' $ZIMAGE | head -n 1 | cut -d ':' -f 1)
    _VER_RUN=$(dd if=$ZIMAGE obs=64K ibs=4 skip=$(( IMG_OFFSET / 4)) 2>/dev/null | zcat | grep -a -m1 "Linux version" | strings | awk '{ print $3; }')
  }
  echo "$_VER_RUN"
  return 0
}

function check_kernel_headers() {
  VER_RUN=$(get_kernel_version)
  VER_HDR=$(dpkg -L raspberrypi-kernel-headers | egrep -m1 "/lib/modules/[^-]+/build" | awk -F'/' '{ print $4; }')
  [ "X$VER_RUN" == "X$VER_HDR" ] && {
    return 0
  }
  VER_HDR=$(dpkg -L linux-headers-$VER_RUN | egrep -m1 "/lib/modules/[[:print:]]+/build" | awk -F'/' '{ print $4; }')
  [ "X$VER_RUN" == "X$VER_HDR" ] && {
    return 0
  }

  # echo RUN=$VER_RUN HDR=$VER_HDR
  echo " !!! Your kernel version is $VER_RUN"
  echo "     Not found *** corresponding *** kernel headers with apt-get."
  echo "     This may occur if you have ran 'rpi-update'."
  echo " Choose  *** y *** will revert the kernel to version $VER_HDR then continue."
  echo " Choose  *** N *** will exit without this driver support, by default."
  read -p "Would you like to proceed? (y/N)" -n 1 -r -s
  echo
  if ! [[ $REPLY =~ ^[Yy]$ ]]; then
    exit 1;
  fi

  apt-get -y --reinstall install raspberrypi-kernel
}

# update and install required packages
which apt &>/dev/null
if [[ $? -eq 0 ]]; then
  apt update -y
  # Raspbian kernel packages
  apt-get -y install raspberrypi-kernel-headers raspberrypi-kernel 
  # Ubuntu kernel packages
  apt-get -y install linux-raspi linux-headers-raspi linux-image-raspi
  apt-get -y install dkms git i2c-tools libasound2-plugins
  # rpi-update checker
  check_kernel_headers
fi

# Arch Linux
which pacman &>/dev/null
if [[ $? -eq 0 ]]; then
  pacman -Syu --needed git gcc automake make dkms linux-raspberrypi-headers i2c-tools
fi

# locate currently installed kernels (may be different to running kernel if
# it's just been updated)
base_ver=$(get_kernel_version)
base_ver=${base_ver%%[-+]*}
#kernels="${base_ver}+ ${base_ver}-v7+ ${base_ver}-v7l+"
kernels=$(uname -r)
kernel_base_ver=${kernels%%[-+]*}

if [[ "$base_ver" != "$kernel_base_ver" ]] ; then
  echo "------------------------------------------------------"
  echo -e " ${RED}WARNING${NC} Your loaded kernel version is $kernel_base_ver"
  echo " Not matching the updated version $base_ver."
  echo " Kernel was updated, but new kernel was not loaded yet"
  echo -e " Please ${RED}reboot${NC} your machine AND THEN run this script ${RED}again"
  exit 1;
fi

function install_module {
  local _i

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
  for _i in $kernels; do
    dkms build -k $_i -m $mod -v $ver && {
      dkms install --force -k $_i -m $mod -v $ver
    }
  done

  mkdir -p /var/lib/dkms/$mod/$ver/$marker
}

install_module "./" "seeed-voicecard"

# install dtbos
cp seeed-2mic-voicecard.dtbo $OVERLAYS
cp seeed-4mic-voicecard.dtbo $OVERLAYS
cp seeed-8mic-voicecard.dtbo $OVERLAYS

#install alsa plugins
# no need this plugin now
# install -D ac108_plugin/libasound_module_pcm_ac108.so /usr/lib/arm-linux-gnueabihf/alsa-lib/
rm -f /usr/lib/arm-linux-gnueabihf/alsa-lib/libasound_module_pcm_ac108.so

#set kernel modules
grep -q "^snd-soc-seeed-voicecard$" /etc/modules || \
  echo "snd-soc-seeed-voicecard" >> /etc/modules
grep -q "^snd-soc-ac108$" /etc/modules || \
  echo "snd-soc-ac108" >> /etc/modules
grep -q "^snd-soc-wm8960$" /etc/modules || \
  echo "snd-soc-wm8960" >> /etc/modules  

#set dtoverlays
CONFIG=/boot/config.txt
[ -f /boot/firmware/usercfg.txt ] && CONFIG=/boot/firmware/usercfg.txt

sed -i -e 's:#dtparam=i2c_arm=on:dtparam=i2c_arm=on:g'  $CONFIG || true
grep -q "^dtoverlay=i2s-mmap$" $CONFIG || \
  echo "dtoverlay=i2s-mmap" >> $CONFIG


grep -q "^dtparam=i2s=on$" $CONFIG || \
  echo "dtparam=i2s=on" >> $CONFIG

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
systemctl start   seeed-voicecard

echo "------------------------------------------------------"
echo "Please reboot your raspberry pi to apply all settings"
echo "Enjoy!"
echo "------------------------------------------------------"
