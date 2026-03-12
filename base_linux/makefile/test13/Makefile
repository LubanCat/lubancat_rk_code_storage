# 基础目录定义
SRC_DIR = sources
INC_DIR = includes
OBJ_DIR = build_obj
LIB_DIR = build_lib

# 编译器和编译选项
CC = gcc
# 编译选项：-fPIC动态库必需，位置无关代码
CFLAGS = -Wall -g -I$(INC_DIR) -fPIC

# 安装配置，PREFIX是Linux标准变量，默认为/usr/local
PREFIX ?= /usr/local
INSTALL_BIN = $(PREFIX)/bin
INSTALL_LIB = $(PREFIX)/lib
INSTALL_INC = $(PREFIX)/include

# 明确指定所有文件
SRCS_LIB = $(wildcard $(SRC_DIR)/hello_func.c)
SRCS_MAIN = $(SRC_DIR)/hello_main.c
OBJS_LIB = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS_LIB))

LIB_DYNAMIC = libhello.so
LIB_DYNAMIC_PATH = $(LIB_DIR)/$(LIB_DYNAMIC)

TARGET = hello_main

# 伪目标声明
.PHONY: all lib_dynamic main clean install uninstall

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


# 安装目标，需要sudo权限
install: all
	@echo "Installing to $(PREFIX)..."
	# 1. 安装可执行程序，权限755
	sudo install -m 755 $(TARGET) $(INSTALL_BIN)/
	# 2. 安装动态库，权限755
	sudo install -m 755 $(LIB_DYNAMIC_PATH) $(INSTALL_LIB)/
	# 3. 安装头文件，权限644
	sudo install -m 644 $(INC_DIR)/hello_func.h $(INSTALL_INC)/
	# 4. 刷新动态库缓存
	sudo ldconfig
	@echo "Installation complete."


# 卸载目标，需要sudo权限
uninstall:
	@echo "Uninstalling from $(PREFIX)..."
	# -f 表示文件不存在也不报错
	sudo rm -f $(INSTALL_BIN)/$(TARGET)
	sudo rm -f $(INSTALL_LIB)/$(LIB_DYNAMIC)
	sudo rm -f $(INSTALL_INC)/hello_func.h
	# 刷新缓存
	sudo ldconfig
	@echo "Uninstallation complete."

clean:
	rm -rf $(OBJ_DIR) $(LIB_DIR) $(TARGET)