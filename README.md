#seeed-voicecard

Firstly, get an updated kernel 4.9 :
```
sudo apt update
sudo apt dist-upgrade
sudo apt install git rpi-update bc dkms
sudo BRANCH=next rpi-update
sudo reboot
```

Install matching kernel header files(come from https://www.raspberrypi.org/forums/):
eg. Get your kernel verson "uname -r", then
```
wget  https://www.niksula.hut.fi/~mhiienka/Rpi/linux-headers-rpi/linux-headers-4.9.13-v7%2B_4.9.13-v7%2B-2_armhf.deb 
```
Next, while the upstream wm8960 codec is not currently supported by current Pi kernel builds, upstream wm8960 has some bugs, we had fixed it. we must it build manually.

Get the seeed voice card source code.
```
git clone http://git.oschina.net/seeed-se/seeed-voicecard
cd seeed-voicecard
sudo  make all install
```
Next, copy then voice card overlay to boot dir, then apply it.
```
sudo cp seeed-voicecard.dtbo /boot/overlays
sudo reboot
sudo modprobe  wm8960
sudo dtoverlay seeed-voicecard
```
Load the driver default. 
```
echo 'wm8960' | sudo tee --append /etc/modules > /dev/null
```
Invoke usage by adding this to /boot/config.txt:
dtoverlay=seeed-voicecard

Check that the sound card name matches the source code seeed-voicecard.

```
pi@raspberrypi:~/seeed-voicecard$ aplay -l
**** List of PLAYBACK Hardware Devices ****
card 0: ALSA [bcm2835 ALSA], device 0: bcm2835 ALSA [bcm2835 ALSA]
  Subdevices: 8/8
  Subdevice #0: subdevice #0
  Subdevice #1: subdevice #1
  Subdevice #2: subdevice #2
  Subdevice #3: subdevice #3
  Subdevice #4: subdevice #4
  Subdevice #5: subdevice #5
  Subdevice #6: subdevice #6
  Subdevice #7: subdevice #7
card 0: ALSA [bcm2835 ALSA], device 1: bcm2835 ALSA [bcm2835 IEC958/HDMI]
  Subdevices: 1/1
  Subdevice #0: subdevice #0
card 1: seeedvoicecard [seeed-voicecard], device 0: bcm2835-i2s-wm8960-hifi wm8960-hifi-0 []
  Subdevices: 1/1
  Subdevice #0: subdevice #0
pi@raspberrypi:~/seeed-voicecard$ 
```
Next apply the alsa controls setting
```
sudo alsactl --file ./asound.state restore
```
If you want to change the alsa settings, You can use "sudo alsactl --file ./asound.state store 1" to save it.

Test:
``` 
arecord -f cd -Dhw:1 | aplay -Dhw:1

```
Enjoy !