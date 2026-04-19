#内核源码路径
KERN_DIR = /home/book/100ask_imx6ull-sdk/Linux-4.9.88

#交叉编译器路径
CROSS_COMPILE = /home/book/100ask_imx6ull-sdk/ToolChain/arm-buildroot-linux-gnueabihf_sdk-buildroot/bin/arm-buildroot-linux-gnueabihf-

#要生成的.ko文件
#obj-m += uart_self.o

all:
#这一行生产的是.ko文件
#	make -C $(KERN_DIR) M=`pwd` modules 

#这一行生成的是可执行文件
	$(CROSS_COMPILE)gcc -o uart_self uart_self.c

clean:
	make -C $(KERN_DIR) M=`pwd` modules clean
	rm -rf modules.order
	rm uart_self


