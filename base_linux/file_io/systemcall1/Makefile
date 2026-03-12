#定义变量
TARGET = systemcall
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
#删除可执行程序
	rm -f $(TARGET) $(TARGET).o
#程序创建的文件
	rm -f testscript.sh