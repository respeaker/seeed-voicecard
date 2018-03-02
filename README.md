# seeed-voicecard

[![Join the chat at https://gitter.im/seeed-voicecard/Lobby](https://badges.gitter.im/seeed-voicecard/Lobby.svg)](https://gitter.im/seeed-voicecard/Lobby?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

The drivers of [ReSpeaker Mic Hat](https://www.seeedstudio.com/ReSpeaker-2-Mics-Pi-HAT-p-2874.html) and [ReSpeaker 4 Mic Array](https://www.seeedstudio.com/ReSpeaker-4-Mic-Array-for-Raspberry-Pi-p-2941.html) for Raspberry Pi.

### Install seeed-voicecard
Get the seeed voice card source code. and install all linux kernel drivers
```bash
git clone https://github.com/respeaker/seeed-voicecard
cd seeed-voicecard
sudo ./install.sh 
sudo reboot
```

## ReSpeaker Mic Hat

[![](https://github.com/SeeedDocument/MIC_HATv1.0_for_raspberrypi/blob/master/img/mic_hatv1.0.png?raw=true)](https://www.seeedstudio.com/ReSpeaker-2-Mics-Pi-HAT-p-2874.html)

While the upstream wm8960 codec is not currently supported by current Pi kernel builds, upstream wm8960 has some bugs, we had fixed it. we must it build manually.

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
If you want to change the alsa settings, You can use `sudo alsactl --file=/etc/voicecard/wm8960_asound.state  store` to save it.



#### Next step
Go to https://github.com/respeaker/mic_hat to build voice enabled projects with Google Assistant SDK or Alexa Voice Service.

## ReSpeaker 4 Mic Array

[![](https://github.com/SeeedDocument/ReSpeaker-4-Mic-Array-for-Raspberry-Pi/blob/master/img/features.png?raw=true)](https://www.seeedstudio.com/ReSpeaker-4-Mic-Array-for-Raspberry-Pi-p-2941.html)

The 4 Mic Array uses ac108 which includes 4 ADCs, we also write ac108 rapberry pi linux kernel driver.

Check that the sound card name matches the source code seeed-voicecard.

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
If you want to change the alsa settings, You can use `sudo alsactl --file=/etc/voicecard/ac108_asound.state  store` to save it.

## 6-Mics Circular Array Kit for Raspberry Pi

The 6 Mics Circular Array Kit uses ac108 x 2 / ac101 x 1 / 6 micphones, includes 8 ADCs and 2 DACs.

The driver is implemented with 8 input channels & 8 output channels.
>**The first 6 input channel are MIC recording data,  
the rest 2 input channel are echo channel of playback  
The first 2 output channel are playing data, the rest 6 output channel are dummy**


Check that the sound card name matches the source code seeed-voicecard.
```bash
#for ReSpeaker 6-mic
pi@raspberrypi:~ $ arecord -L
null
    Discard all samples (playback) or generate zero samples (capture)
default
playback
dmixed
ac108
multiapps
ac101
sysdefault:CARD=seeed8micvoicec
    seeed-8mic-voicecard,
    Default Audio Device
dmix:CARD=seeed8micvoicec,DEV=0
    seeed-8mic-voicecard,
    Direct sample mixing device
dsnoop:CARD=seeed8micvoicec,DEV=0
    seeed-8mic-voicecard,
    Direct sample snooping device
hw:CARD=seeed8micvoicec,DEV=0
    seeed-8mic-voicecard,
    Direct hardware device without any conversions
plughw:CARD=seeed8micvoicec,DEV=0
    seeed-8mic-voicecard,
    Hardware device with all software conversions
    
pi@raspberrypi:~ $ aplay -L
null
    Discard all samples (playback) or generate zero samples (capture)
default
playback
dmixed
ac108
multiapps
ac101
sysdefault:CARD=ALSA
    bcm2835 ALSA, bcm2835 ALSA
    Default Audio Device
dmix:CARD=ALSA,DEV=0
    bcm2835 ALSA, bcm2835 ALSA
    Direct sample mixing device
dmix:CARD=ALSA,DEV=1
    bcm2835 ALSA, bcm2835 IEC958/HDMI
    Direct sample mixing device
dsnoop:CARD=ALSA,DEV=0
    bcm2835 ALSA, bcm2835 ALSA
    Direct sample snooping device
dsnoop:CARD=ALSA,DEV=1
    bcm2835 ALSA, bcm2835 IEC958/HDMI
    Direct sample snooping device
hw:CARD=ALSA,DEV=0
    bcm2835 ALSA, bcm2835 ALSA
    Direct hardware device without any conversions
hw:CARD=ALSA,DEV=1
    bcm2835 ALSA, bcm2835 IEC958/HDMI
    Direct hardware device without any conversions
plughw:CARD=ALSA,DEV=0
    bcm2835 ALSA, bcm2835 ALSA
    Hardware device with all software conversions
plughw:CARD=ALSA,DEV=1
    bcm2835 ALSA, bcm2835 IEC958/HDMI
    Hardware device with all software conversions
sysdefault:CARD=seeed8micvoicec
    seeed-8mic-voicecard,
    Default Audio Device
dmix:CARD=seeed8micvoicec,DEV=0
    seeed-8mic-voicecard,
    Direct sample mixing device
dsnoop:CARD=seeed8micvoicec,DEV=0
    seeed-8mic-voicecard,
    Direct sample snooping device
hw:CARD=seeed8micvoicec,DEV=0
    seeed-8mic-voicecard,
    Direct hardware device without any conversions
plughw:CARD=seeed8micvoicec,DEV=0
    seeed-8mic-voicecard,
    Hardware device with all software conversions
```

## 4-Mics Linear Array Kit for Raspberry Pi
In contrast to 6-Mics Circular Array Kit for Raspberry Pi,
the difference is only first 4 input channels are valid capturing data.

#### Test:
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

```bash
#for ReSpeaker 6-mic
#It will capture sound on AC108 and save as a.wav
arecord -Dac108 -f S32_LE -r 16000 -c 8 a.wav
#Take care of that the captured mic audio is on the first 6 channels

#It will play sound file a.wav on AC101
aplay -D ac101 a.wav
#Do not use -D plughw:1,0 directly except your wave file is single channel only.
```
**Note: for developer using ReSpeaker 6-mic doing capturing & playback the same time,
capturing must be start first, or else the capturing channels will miss order.**

### uninstall seeed-voicecard
If you want to upgrade the driver , you need uninstall the driver first.

```
pi@raspberrypi:~/seeed-voicecard $ sudo ./uninstall.sh 
...
------------------------------------------------------
Please reboot your raspberry pi to apply all settings
Thank you!
------------------------------------------------------
```




Enjoy !
