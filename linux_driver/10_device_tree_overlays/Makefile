#指定内核路径,可以是相对路径或绝对路径
KERNEL_DIR=../../kernel/
#KERNEL_DIR=/home/guest/LubanCat_Linux_rk356x_SDK/kernel/

#指定目标架构为arm64
ARCH=arm64


#指定交叉编译工具链的前缀
CROSS_COMPILE=aarch64-linux-gnu-


#导出为环境变量
export  ARCH  CROSS_COMPILE

#指定要编译的内核模块目标文件
obj-m := dts_led.o

#all :默认目标，执行时会编译驱动模块
#$(MAKE) ：调用make工具
#-C $(KERNEL_DIR) ：指定的内核源码目录
#M=$(CURDIR) ：模块的源码位于当前目录
#modules ：编译模块
all:
	$(MAKE) -C $(KERNEL_DIR) M=$(CURDIR) modules

.PHONE:clean

#清理编译生成的文件
clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(CURDIR) clean