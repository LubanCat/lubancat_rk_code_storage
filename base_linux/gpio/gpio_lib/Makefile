#定义变量
TARGET = gpio_lib
#定义编译器
CC = gcc
#定义头文件的位置()
CFLAGS = -I.
#定义头文件
DEPS = 
#定义目标文件
OBJS = $(TARGET).o
#定义.o文件存放位置
BUILD_DIR  = build
#添加额外库
LIBGPIOD = `pkg-config --cflags libgpiod` `pkg-config --libs libgpiod`

#目标文件
$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBGPIOD)
#创建编译输出文件夹
	@mkdir -p $(BUILD_DIR)
#移动.o文件到输出文件夹
	@mv *.o $(BUILD_DIR)
#复制可执行程序到输出文件夹
	@cp $(TARGET) $(BUILD_DIR)

#*.o文件的生成规则
%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) $(LIBGPIOD)

#伪目标
.PHONY: clean
#make clean清除编译结果
clean:
#删除可执行程序
	rm -f $(TARGET)
#删除输出文件夹
	rm -rf $(BUILD_DIR)
