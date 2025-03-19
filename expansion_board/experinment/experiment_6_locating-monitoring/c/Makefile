# 指定编译器  
CC = $(CROSS_COMPILE)gcc  
  
# 指定编译选项  
CFLAGS = -Wall \
	-Iinclude \
	-Iinclude/bsp/i2c/ \
	-Iinclude/bsp/uart/ \
	-Iinclude/hal/mpu6050/ \
	-Iinclude/hal/atgm332d/ \
	-Iinclude/utils/mqtt/ \
	-Iinclude/utils/ringbuff/ \
	-Iinclude/utils/config/ \
	-Iinclude/utils/http/ \
	-Iinclude/utils/gaodeMap/ \
  
# 指定源文件目录  
SRC_DIRS = \
	src/app \
	src/bsp/i2c \
	src/bsp/uart \
	src/hal/mpu6050 \
	src/hal/atgm332d \
	src/utils/ringbuff \
	src/utils/config \
	src/utils/http \
	src/utils/gaodeMap \
  
# 获取所有的 .c 文件  
SRCS = $(foreach dir, $(SRC_DIRS), $(wildcard $(dir)/*.c))  
  
# 从 .c 文件生成 .o 文件列表  
OBJS = $(SRCS:.c=.o)  
  
# 最终的可执行文件名  
TARGET = main  
  
# 链接库  
LDLIBS = -lgpiod -lpthread -lmqttclient -lm -lcurl -lxml2 -lssl -lcrypto
  
# 默认目标  
all: $(TARGET)  
  
# 链接所有的 .o 文件和指定的库生成可执行文件  
$(TARGET): $(OBJS)  
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)  
  
# 编译每个 .c 文件生成 .o 文件  
%.o: %.c  
	$(CC) $(CFLAGS) -c $< -o $@  
  
# 清理生成的 .o 文件和可执行文件  
clean:  
	rm -f $(OBJS) $(TARGET)  
  
# 打印编译信息  
.PHONY: all clean