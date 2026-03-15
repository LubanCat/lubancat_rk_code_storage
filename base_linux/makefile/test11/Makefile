# 基础目录定义
SRC_DIR := sources
INC_DIR := includes
OBJ_DIR := build_obj
LIB_DIR := build_lib

# 编译器和编译选项
CC := gcc
CFLAGS := -Wall -g -I$(INC_DIR)

# 明确指定所有文件
LIB_SRC := $(wildcard $(SRC_DIR)/hello_func.c)
MAIN_SRC := $(wildcard $(SRC_DIR)/hello_main.c)
LIB_OBJ := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(LIB_SRC))
MAIN_OBJ := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(MAIN_SRC))

STATIC_LIB := $(LIB_DIR)/libhello.a
TARGET := hello_main

# 伪目标声明
.PHONY: all clean

# 默认目标
all: $(TARGET)

# 创建目录
$(OBJ_DIR) $(LIB_DIR):
	mkdir -p $@

# 编译静态库的.o 文件
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# 打包生成静态库
$(STATIC_LIB): $(LIB_OBJ) | $(LIB_DIR)
	ar rcs $@ $^

# 编译链接生成可执行程序
$(TARGET): $(MAIN_OBJ) $(STATIC_LIB)
	$(CC) $(MAIN_OBJ) -o $@ -L$(LIB_DIR) -lhello $(CFLAGS)

# 清理命令
clean:
	rm -rf $(OBJ_DIR) $(LIB_DIR) $(TARGET)