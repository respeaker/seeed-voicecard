# seeed-voicecard

[![Join the chat at https://gitter.im/seeed-voicecard/Lobby](https://badges.gitter.im/seeed-voicecard/Lobby.svg)](https://gitter.im/seeed-voicecard/Lobby?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

While the upstream wm8960 codec is not currently supported by current Pi kernel builds, upstream wm8960 has some bugs, we had fixed it. we must it build manually.

We also write ac108 rapberry pi linux kernel driver.

Get the seeed voice card source code.
```bash
git clone https://github.com/respeaker/seeed-voicecard
cd seeed-voicecard
#for ReSpeaker 2-mic
sudo ./install.sh 2mic

#for ReSpeaker 4-mic
sudo ./install.sh 4mic

#reboot your Raspbian OS
reboot
```

Check that the sound card name matches the source code seeed-voicecard.

```bash
#for ReSpeaker 2-mic
pi@raspberrypi:~/seeed-voicecard $ aplay -l
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
card 1: seeed2micvoicec [seeed-2mic-voicecard], device 0: bcm2835-i2s-wm8960-hifi wm8960-hifi-0 []
  Subdevices: 1/1
  Subdevice #0: subdevice #0
pi@raspberrypi:~/seeed-voicecard $ arecord -l
**** List of CAPTURE Hardware Devices ****
card 1: seeed2micvoicec [seeed-2mic-voicecard], device 0: bcm2835-i2s-wm8960-hifi wm8960-hifi-0 []
  Subdevices: 1/1
  Subdevice #0: subdevice #0
pi@raspberrypi:~/seeed-voicecard $ 
```

```bash
#for ReSpeaker 4-mic
pi@raspberrypi:~ $ arecord -L
null
    Discard all samples (playback) or generate zero samples (capture)
playback
capture
dmixed
array
ac108
default:CARD=seeed4micvoicec
    seeed-4mic-voicecard, 
    Default Audio Device
sysdefault:CARD=seeed4micvoicec
    seeed-4mic-voicecard, 
    Default Audio Device
dmix:CARD=seeed4micvoicec,DEV=0
    seeed-4mic-voicecard, 
    Direct sample mixing device
dsnoop:CARD=seeed4micvoicec,DEV=0
    seeed-4mic-voicecard, 
    Direct sample snooping device
hw:CARD=seeed4micvoicec,DEV=0
    seeed-4mic-voicecard, 
    Direct hardware device without any conversions
plughw:CARD=seeed4micvoicec,DEV=0
    seeed-4mic-voicecard, 
    Hardware device with all software conversions
pi@raspberrypi:~ $ 
```
If you want to change the alsa settings, You can use `sudo alsactl --file=asound.state store` to save it.

Test:
```bash
#for ReSpeaker 2-mic
#It will capture sound an playback on hw:1
arecord -f cd -Dhw:1 | aplay -Dhw:1
```

```bash
#for ReSpeaker 4-mic
#It will capture sound on AC108 and save as a.wav
arecord -Dac108 -f S32_LE -r 16000 -c 4 a.wav
```

### with Google Assistant
if you run the assistant but the playback is speed up considerably, try to configure alsa:

```bash
sudo cp asound.conf /etc/asound.conf
```

If the alsa configuration doesn't solve the issue, try to use pulseaudio. See [#4](https://github.com/respeaker/seeed-voicecard/issues/4)


If you want dump wm8960 runtime reg value
```bash
sudo su
root@raspberrypi:~# cat /sys/devices/platform/soc/3f804000.i2c/i2c-1/1-001a/wm8960_debug/wm8960
#Usage:
root@raspberrypi:~# dmesg | tail

[  418.279499] echo flag|reg|val > wm8960
[  418.279512] eg read star addres=0x06,count 0x10:echo 0610 >wm8960
[  418.279516] eg write value:0xfe to address:0x06 :echo 106fe > wm8960
root@raspberrypi:~# echo 0020 > /sys/devices/platform/soc/3f804000.i2c/i2c-1/1-001a/wm8960_debug/wm8960
root@raspberrypi:~# dmesg 
               Read: start REG:0x00,count:0x20
[  520.402765] REG[0x00]: 0x27;  
[  520.402773] REG[0x01]: 0x27;  
[  520.402778] REG[0x02]: 0x7f;  
[  520.402784] REG[0x03]: 0x7f;  

[  520.402792] REG[0x04]: 0x69;  
[  520.402797] REG[0x05]: 0x08;  
[  520.402802] REG[0x06]: 0x00;  
[  520.402808] REG[0x07]: 0x0e;  

[  520.402816] REG[0x08]: 0xcb;  
[  520.402821] REG[0x09]: 0x00;  
[  520.402826] REG[0x0a]: 0xff;  
[  520.402832] REG[0x0b]: 0xff;  

[  520.402840] REG[0x0c]: 0xff;  
[  520.402846] REG[0x0d]: 0xff;  
[  520.402850] REG[0x0e]: 0xff;  
[  520.402856] REG[0x0f]: 0xff;  

[  520.402864] REG[0x10]: 0x00;  
[  520.402870] REG[0x11]: 0x7b;  
[  520.402875] REG[0x12]: 0x00;  
[  520.402880] REG[0x13]: 0x32;  

[  520.402888] REG[0x14]: 0x00;  
[  520.402895] REG[0x15]: 0xc3;  
[  520.402900] REG[0x16]: 0xc3;  
[  520.402905] REG[0x17]: 0xc0;  

[  520.402913] REG[0x18]: 0x00;  
[  520.402919] REG[0x19]: 0x40;  
[  520.402923] REG[0x1a]: 0x01;  
[  520.402929] REG[0x1b]: 0x03;  

[  520.402937] REG[0x1c]: 0x08;  
[  520.402943] REG[0x1d]: 0x00;  
[  520.402948] REG[0x1e]: 0xff;  
[  520.402953] REG[0x1f]: 0xff;
.......

```

Enjoy !
