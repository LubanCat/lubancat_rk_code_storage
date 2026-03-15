#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main() {
    // 打开日志文件
    int log_fd = open("output.log", O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (log_fd == -1) {
        perror("open log file failed");
        return -1;
    }

    // 关闭标准输出，复制log_fd至标准输出
    close(STDOUT_FILENO);
    if (dup(log_fd) == -1) {
        perror("dup failed");
        close(log_fd);
        return -1;
    }
    // 关闭原文件描述符，仅保留重定向后的标准输出
    close(log_fd);

    // 以下内容会写入output.log，而非终端
    printf("这行内容将写入日志文件\n");
    printf("标准输出重定向成功\n");

    return 0;
}