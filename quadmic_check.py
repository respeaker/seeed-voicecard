
##############################################
# QuadMic Check using Pyaudio Package
# ---- this code tells the user if a QuadMic
# ---- is detected or not
#
# -- by Josh Hrisko, Principal Engineer
#       Maker Portal LLC 2021
# 
##############################################
# QuadMic Device Finder
##############################################
#
import pyaudio

audio = pyaudio.PyAudio() # start pyaudio device

quadmic_indx = []
for indx in range(audio.get_device_count()):
    dev = audio.get_device_info_by_index(indx) # get device
    if dev['maxInputChannels']==4 and \
       len([ii for ii in dev['name'].split('-') if ii=='4mic'])>=1:
        print('-'*30)
        print('Found QuadMic!')
        print('Device Index: {}'.format(indx)) # device index
        print('Device Name: {}'.format(dev['name'])) # device name
        print('Device Input Channels: {}'.format(dev['maxInputChannels'])) # channels
        dev_indx = int(indx)
if quadmic_indx == []:
    print('No QuadMic Found')
