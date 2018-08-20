# seeed-voicecard

[![Join the chat at https://gitter.im/seeed-voicecard/Lobby](https://badges.gitter.im/seeed-voicecard/Lobby.svg)](https://gitter.im/seeed-voicecard/Lobby?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

The drivers of [ReSpeaker Mic Hat](https://www.seeedstudio.com/ReSpeaker-2-Mics-Pi-HAT-p-2874.html),[ReSpeaker 4 Mic Array](https://www.seeedstudio.com/ReSpeaker-4-Mic-Array-for-Raspberry-Pi-p-2941.html),[6-Mics Circular Array Kit](), and [4-Mics Linear Array Kit]() for Raspberry Pi.

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
#for ReSpeaker 4 Mic Array
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

## 6-Mics Circular Array Kit

[![](https://user-images.githubusercontent.com/3901856/37268348-6adef768-2600-11e8-8861-588b1c3ea142.png)]()

The 6 Mics Circular Array Kit uses ac108 x 2 / ac101 x 1 / micphones x 6, includes 8 ADCs and 2 DACs.

The driver is implemented with 8 input channels & 8 output channels.
>**The first 6 input channel are MIC recording data,  
the rest 2 input channel are echo channel of playback  
The first 2 output channel are playing data, the rest 6 output channel are dummy**


Check that the sound card name matches the source code seeed-voicecard.
```bash
#for 6 Mic Circular Array
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

## 4-Mics Linear Array Kit

[![](https://user-images.githubusercontent.com/3901856/37194106-a0ccebce-23a7-11e8-88c5-ec611e44ec49.png)]()

In contrast to 6-Mics Circular Array Kit for Raspberry Pi,
the difference is only first 4 input channels are valid capture data.

### Usage:
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
#for 6-Mics Circular Array Kit and 4-Mics Linear Array Kit
#It will capture sound on AC108 and save as a.wav
arecord -Dac108 -f S32_LE -r 16000 -c 8 a.wav
#Take care of that the captured mic audio is on the first 6 channels

#It will play a mono channel sound file mono_to_play.wav
#The file to play must be mono channel or else the speaker output nothing.
aplay -D plughw:1,0 mono_to_play.wav

#Doing capture && playback the same time
arecord -D hw:1,0 -f S32_LE -r 16000 -c 8 to_be_record.wav &
#mono_to_play.wav is a mono channel wave file to play
aplay -D plughw:1,0 -r 16000 mono_to_play.wav
```
**Note: Limit for developer using 6-Mics Circular Array Kit(or 4-Mics Linear Array Kit) doing capture & playback the same time:  
1. capture must be start first, or else the capture channels will possibly be disorder.  
2. playback output channels must fill with 8 same channels data or 4 same stereo channels data, or else the speaker or headphone will output nothing possibly.**

### Coherence

Estimate the magnitude squared coherence using Welch’s method.
![4-mics-linear-array-kit coherence](https://user-images.githubusercontent.com/3901856/37277486-beb1dd96-261f-11e8-898b-84405bfc7cea.png)  
Note: 'CO 1-2' means the coherence between channel 1 and channel 2.

```bash
# How to get the coherence of the captured audio(a.wav for example).
sudo apt install python-numpy python-scipy python-matplotlib
python tools/coherence.py a.wav

# Requirement of the input audio file:
- format: WAV(Microsoft) signed 16-bit PCM
- channels: >=2
```

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

### FAQ

When you encounter any installation and use problems when you start your ReSpeaker Pi hat, please use the following image for testing. We have installed seeed-voicecard based on the latest PI image, which can be used by burning it directly on SD. If this still cannot solve your problem, you can ask in the issue. We will try our best to solve your problem.

<p style="text-align:center"><a href="https://v2.fangcloud.com/share/7395fd138a1cab496fd4792fe5" target="_blank"><img src="https://github.com/SeeedDocument/Respeaker_V2/raw/master/img/efangyun.png" width="200" height="40"  border=0 /></a></p>
