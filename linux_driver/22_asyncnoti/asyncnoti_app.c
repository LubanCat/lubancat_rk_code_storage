#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

// 定义全局变量，用于存储文件描述符
int fd;

// 信号处理函数，当接收到 SIGIO 信号时调用
void sigio_handler(int signum) {
    int ret;
    int button_state;

    // 读取按键状态
    ret = read(fd, &button_state, sizeof(int));
    if (ret < 0) {
        perror("read error");
        return;
    }
    printf("button_state = %d\n", button_state);
}

int main(int argc, char *argv[]) {
    int ret;

    if (argc != 2) {
        printf("Usage: ./asyncnoti_app /dev/asyncnoti\n");
        return -1;
    }

    // 打开设备文件
    fd = open(argv[1], O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        printf("ERROR: %s file open failed!\n", argv[1]);
        return -1;
    }

    // 注册信号处理函数
    signal(SIGIO, sigio_handler);

    // 设置文件的拥有者为当前进程
    ret = fcntl(fd, F_SETOWN, getpid());
    if (ret < 0) {
        perror("fcntl F_SETOWN");
        close(fd);
        return -1;
    }

    // 获取当前文件状态标志
    int flags = fcntl(fd, F_GETFL);
    if (flags < 0) {
        perror("fcntl F_GETFL");
        close(fd);
        return -1;
    }

    // 开启异步通知功能
    ret = fcntl(fd, F_SETFL, flags | FASYNC);
    if (ret < 0) {
        perror("fcntl F_SETFL");
        close(fd);
        return -1;
    }

    // 进入无限循环，等待信号
    while (1) {
        pause();
    }

    // 关闭文件描述符
    close(fd);
    return 0;
}