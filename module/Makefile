obj-m += ospad.o

module:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
overlay:
	sudo armbian-add-overlay ospad_overlay.dts
install:
	sudo insmod ospad.ko
	
