SRC_DIR = sources
INC_DIR = includes
# 调试版中间文件目录
BUILD_DIR_DEBUG = build_debug
# 发布版中间文件目录
BUILD_DIR_RELEASE = build_release

# 编译器
CC = gcc
# 调试版编译选项：-g生成调试信息，-Wall显示所有警告，指定头文件目录
CFLAGS_DEBUG = -Wall -g -I$(INC_DIR) -MMD
# 发布版编译选项：-O2代码优化无调试信息，-Wall显示所有警告
CFLAGS_RELEASE = -Wall -O2 -I$(INC_DIR) -MMD

# 自动获取所有源文件
SRCS = $(wildcard $(SRC_DIR)/*.c)
# 调试版中间文件列表
OBJS_DEBUG = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR_DEBUG)/%.o, $(SRCS))
# 发布版中间文件列表
OBJS_RELEASE = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR_RELEASE)/%.o, $(SRCS))
# 调试版依赖文件列表
DEPS_DEBUG = $(patsubst $(BUILD_DIR_DEBUG)/%.o, $(BUILD_DIR_DEBUG)/%.d, $(OBJS_DEBUG))
# 发布版依赖文件列表
DEPS_RELEASE = $(patsubst $(BUILD_DIR_RELEASE)/%.o, $(BUILD_DIR_RELEASE)/%.d, $(OBJS_RELEASE))

# 声明伪目标
.PHONY: all debug release clean clean_debug clean_release

# 默认目标：同时构建调试版和发布版
all: debug release

# 调试版目标：可执行程序名加后缀_debug，便于区分
debug: TARGET = hello_main_debug
debug: $(OBJS_DEBUG)
	@mkdir -p $(BUILD_DIR_DEBUG)
	$(CC) $(OBJS_DEBUG) -o $(TARGET)

# 发布版目标：可执行程序名加后缀_release
release: TARGET = hello_main_release
release: $(OBJS_RELEASE)
	@mkdir -p $(BUILD_DIR_RELEASE)
	$(CC) $(OBJS_RELEASE) -o $(TARGET)

# 调试版中间文件编译规则
$(BUILD_DIR_DEBUG)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR_DEBUG)
	$(CC) $(CFLAGS_DEBUG) -c $< -o $@

# 发布版中间文件编译规则
$(BUILD_DIR_RELEASE)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR_RELEASE)
	$(CC) $(CFLAGS_RELEASE) -c $< -o $@

# 加载两个版本的依赖文件
-include $(DEPS_DEBUG)
-include $(DEPS_RELEASE)

# 清理所有版本产物
clean: clean_debug clean_release

# 单独清理调试版
clean_debug:
	rm -rf $(BUILD_DIR_DEBUG) hello_main_debug

# 单独清理发布版
clean_release:
	rm -rf $(BUILD_DIR_RELEASE) hello_main_release