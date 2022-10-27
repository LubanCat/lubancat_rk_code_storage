#定义变量
TARGET = test
#在Makefile路径上，把执行程序软链接出来
OTHER_TARGET = test
#存放中间文件的路径
BUILD_DIR = build
#存放源文件的文件夹
SRC_DIR = sources
#存放头文件的文件夹
INC_DIR = includes .
#源文件
SRCS = $(wildcard $(SRC_DIR)/*.c)
#目标文件（*.o）
OBJS = $(patsubst %.c, $(BUILD_DIR)/%.o, $(notdir $(SRCS)))
#头文件
DEPS = $(wildcard $(INC_DIR)/*.h)
#指定头文件的路径
CFLAGS = $(patsubst %, -I%, $(INC_DIR))
#编译器
CC = gcc
#引用库
LIBDRM = `pkg-config --cflags libdrm` `pkg-config --libs libdrm`
#目标文件
$(BUILD_DIR)/$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBDRM)
	@ln -sf $(BUILD_DIR)/$(TARGET) $(OTHER_TARGET)

#*.o文件的生成规则
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(DEPS)

#创建一个编译目录，用于存放过程文件
#命令前带“@”,表示不在终端上输出
	@mkdir -p $(BUILD_DIR)
	$(CC) -c -o $@ $< $(CFLAGS) $(LIBDRM) 

#伪目标
.PHONY: clean cleanall

#删除输出文件夹
clean:
	rm -rf $(BUILD_DIR)
	@rm $(OTHER_TARGET)
      
#全部删除
cleanall:
	rm -rf $(BUILD_DIR)
	@rm $(OTHER_TARGET)