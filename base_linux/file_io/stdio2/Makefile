#定义变量
TARGET = stdio
#定义编译器
CC = gcc
#定义头文件的位置()
CFLAGS = -I.
#定义头文件
DEPS = 
#定义目标文件
OBJS = $(TARGET).o

#目标文件
$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)

#*.o文件的生成规则
%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

#伪目标
.PHONY: clean
#make clean清除编译结果
clean:
	rm -f $(TARGET) $(TARGET).o