obj-m += threads_race.o
KerPath = /lib/modules/$(shell uname -r)/build

all:
	make -C $(KerPath) M=$(PWD) modules

install: threads_race.ko
	sudo insmod threads_race.ko

remove:
	sudo rmmod threads_race

clean:
	make -C $(KerPath) M=$(PWD) clean
