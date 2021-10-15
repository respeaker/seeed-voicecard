# I Need to Rename This Project

---------------

## A Presentation and Interactive Coding & IoT Activity for Cisco DevNet.

---

- **Project Manager**: Alexander Stevenson							


- **Purpose**: Abstract for Interactive Coding / IoT Presentation


- **Organization**: Cisco DevNet, Developer Success (managed by Paul Zimmerman)


- **Date**: August 16, 2021


- **Concept**: Three ringable metal bells (see image below under Equipment) will a simple and small three ring coat rack. At least one sensor will be placed near the bells and measure sound / frequency so that each bell can be differentiated from the others and will be able to activate a process in a unique way (assuming each bell rings at a sperate frequency from each other, as well as from any sounds in the immediate environment.

	I will use a Raspberry Pi (probably) for compute power, along with a compatible sensor. The sensor will be connected physically (Cat5/6 Ethernet). The 	Raspberry Pi, acting as the Controller, will connect to the Network through WiFi or similar transmission medium which is conducive to this project and environment. 

	For the interactive portion, we (DevNet) will develop a script which will be able to recognize the different bell sounds and initiate different actions based on those stimuli, such as turning devices on/off, and/or sending or receiving data (to name a few). The users will be asked to adjust the script, used basic timing methods built into Python, so that by reproducing that pattern on the bells they can initiate one of the actions listed above. Advanced users could even work on coding unique actions to be performed (we will provide code-snippet templates to assist).

- **Motivation**

In my never-ending quest for peak efficiency (emphasis on never-ending), I’ve formulated a concept where scripts can be activated by IoT sensors that measure sound and/or pitch. Thus, the ringing of three non-identical bells will make three distinct sounds which in turn can activate different script or parts of a script into action.


- **Equipment**

	-	Bell x 3

		<img src="https://user-images.githubusercontent.com/27918923/130514323-ffc52509-00d8-4851-ac73-80bf04180a6c.jpeg" data-canonical-src="https://gyazo.com/eb5c5741b6a9a16c692170a41a49c858.png" width="300" height="300" />

	-	Rack with hooks (for holding rope on top of bells) x 1

	-	Raspberry Pi 4B

	-	[QuadMic 4-Microphone Array for Raspberry Pi](https://makersportal.com/shop/quadmic-4-microphone-array) x 1 - sensor for measuring sound / frequency. The QuadMic Array is a 4-microphone array based around the AC108 quad-channel analog-to-digital converter (ADC) with Inter-IC Sound (I2S) audio output capable of interfacing with the Raspberry Pi. The QuadMic can be used for applications in voice detection and recognition, acoustic localization, noise control, and other applications in audio and acoustic analysis. 

		<img src="https://images.squarespace-cdn.com/content/v1/59b037304c0dbfb092fbe894/1610935558030-QZA5VMICYJBRCMOZ0FGS/quadmic_bottom.JPG?format=2500w" width="300" height="200" />


	-	Cat 5/6 Ethernet or WiFi connection x 1


- **Language**: Python

	
- **Method of Operation**: Code forthcoming….




Original ReadMe from Fork Origin (https://github.com/respeaker/seeed-voicecard) found below....

# seeed-voicecard

The drivers for [ReSpeaker Mic Hat](https://www.seeedstudio.com/ReSpeaker-2-Mics-Pi-HAT-p-2874.html), [ReSpeaker 4 Mic Array](https://www.seeedstudio.com/ReSpeaker-4-Mic-Array-for-Raspberry-Pi-p-2941.html), [6-Mics Circular Array Kit](), and [4-Mics Linear Array Kit]() for Raspberry Pi.

### Install seeed-voicecard
Get the seeed voice card source code and install all linux kernel drivers
```bash
git clone https://github.com/respeaker/seeed-voicecard
cd seeed-voicecard
sudo ./install.sh
sudo reboot
```

**Note** If you have 64-bit version of Rasperry Pi OS, use install_arm64.sh script for driver installation. 

## ReSpeaker Documentation

Up to date documentation for reSpeaker products can be found in [Seeed Studio Wiki](https://wiki.seeedstudio.com/ReSpeaker/)!
![](https://files.seeedstudio.com/wiki/ReSpeakerProductGuide/img/Raspberry_Pi_Mic_Array_Solutions.png)


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

### Technical support

For hardware testing purposes we made a Rasperry Pi OS 5.10.17-v7l+ 32-bit image with reSpeaker drivers pre-installed, which you can download by clicking on [this link](https://files.seeedstudio.com/linux/Raspberry%20Pi%204%20reSpeaker/2021-05-07-raspios-buster-armhf-lite-respeaker.img.xz).

We provide official support for using reSpeaker with the following OS:
- 32-bit Raspberry Pi OS
- 64-bit Raspberry Pi OS (experimental support)

And following hardware platforms:
- Raspberry Pi 3 (all models), Raspberry Pi 4 (all models), Raspberry Pi Zero and Zero W

Anything beyond the scope of official support is considered to be community supported. Support for other OS/hardware platforms can be added, provided MOQ requirements can be met. 

If you have a technical problem when using reSpeaker with one of the officially supported platforms/OS, feel free to create an issue on Github. For general questions or suggestions, please use [Seeed forum](https://forum.seeedstudio.com/c/products/respeaker/15). 


