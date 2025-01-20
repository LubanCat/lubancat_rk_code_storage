# 指定编译器  
CC = $(CROSS_COMPILE)gcc  
  
# 指定编译选项  
CFLAGS = -Wall -I./include  
  
# 指定源文件目录  
SRC_DIRS = cjson ringbuff .   
  
# 获取所有的 .c 文件  
SRCS = $(foreach dir, $(SRC_DIRS), $(wildcard $(dir)/*.c))  
  
# 从 .c 文件生成 .o 文件列表  
OBJS = $(SRCS:.c=.o)  
  
# 最终的可执行文件名  
TARGET = main  
  
# 链接库  
LDLIBS = -lgpiod -lpthread -lmqttclient -lm
  
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