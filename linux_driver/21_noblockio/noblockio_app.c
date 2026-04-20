#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>

int main(int argc, char *argv[])
{
    int fd, ret;
    int button_state;
    struct pollfd fds[1];  // 定义 pollfd 结构体数组

    if(argc != 2) {
        printf("Usage: ./blockio_app /dev/blockio\n");
        return -1;
    }

    fd = open(argv[1], O_NONBLOCK);
    if(0 > fd) {
        printf("ERROR: %s file open failed!\n", argv[1]);
        return -1;
    }

    // 初始化 pollfd 结构体
    fds[0].fd = fd;          // 指定要监视的文件描述符
    fds[0].events = POLLIN;  // 指定要监视的事件为可读

    for ( ; ; ) {
        // 调用 poll 函数等待事件发生
        ret = poll(fds, 1, -1);  // -1 表示无限等待

        if (ret < 0) {
            perror("poll");
            break;
        } else if (ret == 0) {
            // 超时，这里不会发生，因为设置了无限等待
            printf("Poll timeout\n");
        } else {
            // 检查是否有可读事件发生
            if (fds[0].revents & POLLIN) {
                // 读取按键状态
                ret = read(fd, &button_state, sizeof(int));
                if (ret < 0) {
                    perror("read");
                    break;
                }
                printf("button_state = %d\n", button_state);
            }
        }
    }

    close(fd);
    return 0;
}