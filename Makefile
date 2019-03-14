#
# Peter Yang <turmary@126.com>
# Copyright (c) 2019 Seeed Studio
#
# MIT License
#

uname_r=$(shell uname -r)

# If KERNELRELEASE is defined, we've been invoked from the
# kernel build system and can use its language
ifneq ($(KERNELRELEASE),)
# $(warning KERNELVERSION=$(KERNELVERSION))

snd-soc-wm8960-objs := wm8960.o
snd-soc-ac108-objs := ac108.o ac101.o
snd-soc-seeed-voicecard-objs := seeed-voicecard.o
snd-soc-simple-card-utils-objs := simple-card-utils.o

obj-m += snd-soc-wm8960.o
obj-m += snd-soc-ac108.o
obj-m += snd-soc-seeed-voicecard.o
ifndef CONFIG_SND_SIMPLE_CARD_UTILS
obj-m += snd-soc-simple-card-utils.o
endif

ifdef DEBUG
ifneq ($(DEBUG),0)
	ccflags-y += -DDEBUG -DAC101_DEBG
endif
endif



else

DEST := /lib/modules/$(uname_r)/kernel

all:
	make -C /lib/modules/$(uname_r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(uname_r)/build M=$(PWD) clean

install:
	sudo cp snd-soc-ac108.ko ${DEST}/sound/soc/codecs/
	sudo cp snd-soc-wm8960.ko ${DEST}/sound/soc/codecs/
	sudo cp snd-soc-seeed-voicecard.ko ${DEST}/sound/soc/bcm/
	if [ -f snd-soc-simple-card-utils.ko ]; then \
	  -sudo mkdir ${DEST}/sound/soc/generic/;    \
	  sudo cp snd-soc-simple-card-utils.ko ${DEST}/sound/soc/generic/; \
	fi
	sudo depmod -a


.PHONY: all clean install

endif

