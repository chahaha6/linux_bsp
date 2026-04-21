KERN_DIR = /home/book/100ask_imx6ull-sdk/Linux-4.9.88
CROSS_COMPILE =  /home/book/100ask_imx6ull-sdk/ToolChain/arm-buildroot-linux-gnueabihf_sdk-buildroot/bin/arm-buildroot-linux-gnueabihf-
obj-m	+= adc.o

all:
	make -C $(KERN_DIR) M=`pwd` modules
#	$(CROSS_COMPILE)gcc -o app app.c

clear:
	make -C $(KERN_DIR) M=`pwd` modules clean
	rm -rf modules.order
	rm -f app