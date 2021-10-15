##############################################
# QuadMic Device Finder
##############################################
#
import pyaudio
audio = pyaudio.PyAudio() # start pyaudio device
for indx in range(audio.get_device_count()):
    dev = audio.get_device_info_by_index(indx) # get device
    if dev['maxInputChannels']==4 and \
       len([ii for ii in dev['name'].split('-') if ii=='4mic'])>=1:
        print('-'*30)
        print('Found QuadMic!')
        print('Device Index: {}'.format(indx)) # device index
        print('Device Name: {}'.format(dev['name'])) # device name
        print('Device Input Channels: {}'.format(dev['maxInputChannels'])) # channels