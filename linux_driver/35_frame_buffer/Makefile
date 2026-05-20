#指定目标架构为arm64
ARCH=arm64

#指定交叉编译工具链的前缀
CROSS_COMPILE=aarch64-linux-gnu-

#导出为环境变量
export  ARCH  CROSS_COMPILE

test_app1 = frame_buffer_app
test_app2 =frame_buffer_double_app

all:
	$(CROSS_COMPILE)gcc -o $(test_app1) $(test_app1).c
	$(CROSS_COMPILE)gcc -o $(test_app2) $(test_app2).c

.PHONE:clean

#清理编译生成的文件
clean:
	rm $(test_app1) $(test_app2)