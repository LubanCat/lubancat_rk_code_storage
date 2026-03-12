#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BUF_SIZE 4096  // 4KB缓冲区

int main(int argc, char *argv[]) {
    // 校验参数
    if (argc != 3) {
        fprintf(stderr, "usage: %s <src_file> <dest_file>\n", argv[0]);
        return -1;
    }

    // 打开源文件，只读模式
    int src_fd = open(argv[1], O_RDONLY);
    if (src_fd == -1) {
        perror("open src file failed");
        return -1;
    }

    // 打开目标文件，只写+创建+清空
    int dest_fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dest_fd == -1) {
        perror("open dest file failed");
        close(src_fd);
        return -1;
    }

    // 循环读写复制文件
    char buf[BUF_SIZE];
    ssize_t len;
    while ((len = read(src_fd, buf, BUF_SIZE)) > 0) {
        if (write(dest_fd, buf, len) != len) {
            perror("write dest file failed");
            close(src_fd);
            close(dest_fd);
            return -1;
        }
    }

    if (len == -1) {
        perror("read src file failed");
        close(src_fd);
        close(dest_fd);
        return -1;
    }

    // 关闭文件描述符
    close(src_fd);
    close(dest_fd);
    printf("文件复制成功！\n");

    return 0;
}