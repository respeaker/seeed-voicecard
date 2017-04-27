obj-m := wm8960.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

install:
	sudo cp wm8960.ko /lib/modules/$(shell uname -r)
	sudo depmod -a
