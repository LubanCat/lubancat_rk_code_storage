
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <linux/input.h>

int main(int argc, char *argv[]) {
    int fd;
    int ret;
    
    // 输入事件结构体，用于存储读取到的输入事件
    struct input_event ev;

    // 检查参数，需传入输入设备event节点路径
    if (argc != 2) {
        printf("Usage: ./input_app /dev/input/eventX\n");
        return -1;
    }

    // 打开输入设备节点
    fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open input device failed");
        return -1;
    }

    printf("Start reading input event...\n");
    
    // 循环读取输入事件
    while (1) {
        // 读取输入事件，struct input_event 是输入子系统的标准事件格式
        ret = read(fd, &ev, sizeof(struct input_event));
        if (ret < 0) {
            perror("read input event failed");
            close(fd);
            return -1;
        }

        // 解析事件：仅处理按键事件 EV_KEY
        if (ev.type == EV_KEY) {
            // 打印事件信息：时间戳、事件类型、按键编码、事件值
            printf("Event: type=%d, code=%d value=%d\n", 
                   ev.type, ev.code, ev.value);
            
            // 根据事件值判断按键状态
            if (ev.value == 1) {
                printf("=== 按键按下 ===\n");
            } else if (ev.value == 0) {
                printf("=== 按键松开 ===\n");
            }
        }

    }

    close(fd);

    return 0;
}
    