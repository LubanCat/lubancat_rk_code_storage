KERNEL_DIR=../lubancat_sdk/kernel

ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-
export  ARCH  CROSS_COMPILE

obj-m := ws2812_drv.o
all:
	$(MAKE) -C $(KERNEL_DIR) M=$(CURDIR) modules

.PHONE:clean

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(CURDIR) clean	

