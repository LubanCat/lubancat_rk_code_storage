KERNEL_DIR=../../kernel/

ARCH=arm64
CROSS_COMPILE=aarch64-linux-gnu-
export  ARCH  CROSS_COMPILE

obj-m := led_cdev.o
all:
	$(MAKE) -C $(KERNEL_DIR) M=$(CURDIR) modules

.PHONE:clean

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(CURDIR) clean	

