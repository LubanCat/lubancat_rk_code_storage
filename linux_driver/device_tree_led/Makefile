# 指定内核目录,根据实际编译内核时指定的输出目录，编译时没有指定，就是内核源码目录
KERNEL_DIR=../../../kernel

ARCH=arm64
CROSS_COMPILE=aarch64-linux-gnu-
export  ARCH  CROSS_COMPILE

obj-m := led_test.o
out = test_app

all:
	$(MAKE) -C $(KERNEL_DIR) M=$(CURDIR) modules
	$(CROSS_COMPILE)gcc -o $(out) test_app.c

.PHONE:clean

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(CURDIR) clean	
	rm $(out)

