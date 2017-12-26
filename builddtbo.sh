#dtoverlay -r seeed-2mic-voicecard

dtc -@ -I dts -O dtb -o seeed-2mic-voicecard-pi.dtbo seeed-2mic-voicecard-overlay.dts
dtc -@ -I dts -O dtb -o seeed-2mic-voicecard-artik.dtbo seeed-2mic-voicecard-artik.dts
dtc -@ -I dts -O dtb -o seeed-4mic-voicecard-pi.dtbo seeed-4mic-voicecard-overlay.dts
dtc -@ -I dts -O dtb -o seeed-4mic-voicecard-artik.dtbo seeed-4mic-voicecard-artik.dts

# cp *.dtbo /boot/overlays
# dtoverlay seeed-2mic-voicecard
