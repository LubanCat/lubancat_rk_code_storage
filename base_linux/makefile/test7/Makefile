#定义变量
TARGET = hello_main
CC = gcc
CFLAGS = -I.
DEPS = hello_func.h
OBJS = hello_main.o hello_func.o

#目标文件
$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)

#*.o文件的生成规则
%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

#伪目标
.PHONY: clean
clean:
	rm -f *.o hello_main