obj-m += chardev.o
KerPath = /lib/modules/$(shell uname -r)/build

all:
	make -C $(KerPath) M=$(PWD) modules

install: chardev.ko
	sudo insmod chardev.ko

remove:
	sudo rmmod chardev

clean:
	make -C $(KerPath) M=$(PWD) clean
