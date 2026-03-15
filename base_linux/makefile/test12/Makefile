# 基础目录定义
SRC_DIR = sources
INC_DIR = includes
OBJ_DIR = build_obj
LIB_DIR = build_lib

# 编译器和编译选项
CC = gcc
# 编译选项：-fPIC动态库必需，位置无关代码
CFLAGS = -Wall -g -I$(INC_DIR) -fPIC

# 明确指定所有文件
SRCS_LIB = $(wildcard $(SRC_DIR)/hello_func.c)
SRCS_MAIN = $(SRC_DIR)/hello_main.c
OBJS_LIB = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS_LIB))

LIB_DYNAMIC = libhello.so
LIB_DYNAMIC_PATH = $(LIB_DIR)/$(LIB_DYNAMIC)

TARGET = hello_main

# 伪目标声明
.PHONY: all lib_dynamic main clean

# 默认目标
all: lib_dynamic main

# 动态库目标：使用gcc -shared生成动态库
lib_dynamic: $(OBJS_LIB)
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(LIB_DIR)
# -shared：生成动态库
	$(CC) -shared -o $(LIB_DYNAMIC_PATH) $(OBJS_LIB)

# 生成可执行程序，链接动态库
main: $(SRCS_MAIN) $(LIB_DYNAMIC_PATH)
	$(CC) $(SRCS_MAIN) -o $(TARGET) -L$(LIB_DIR) -lhello $(CFLAGS)

# 编译动态库所需的中间文件
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# 清理目标
clean:
	rm -rf $(OBJ_DIR) $(LIB_DIR) $(TARGET)