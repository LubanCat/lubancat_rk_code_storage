KERNEL_DIR=../../kernel/build

ARCH=arm64
CROSS_COMPILE=aarch64-linux-gnu-
export  ARCH  CROSS_COMPILE

obj-m := led_pdev.o led_pdrv.o

all:
	$(MAKE) -C $(KERNEL_DIR) M=$(CURDIR) modules
modules clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(CURDIR) clean	
