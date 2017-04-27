dtoverlay -r seeed-voicecard
dtc -@ -I dts -O dtb -o seeed-voicecard.dtbo seeed-voicecard-overlay.dts
cp seeed-voicecard.dtbo /boot/overlays
dtoverlay seeed-voicecard
