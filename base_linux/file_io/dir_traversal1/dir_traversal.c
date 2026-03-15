#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>


const char *get_file_type(unsigned char d_type);

int main(int argc, char *argv[]) {
    // 校验命令行参数
    if (argc != 2) {
        fprintf(stderr, "用法：%s <目标目录路径>\n", argv[0]);
        return -1;
    }

    // 1. 打开目录
    DIR *dir = opendir(argv[1]);
    if (dir == NULL) {
        perror("opendir 打开目录失败");
        return -1;
    }
    printf("========== 开始遍历目录：%s ==========\n", argv[1]);

    struct dirent *entry;
    // 2. 循环读取目录项
    while ((entry = readdir(dir)) != NULL) {
        // 过滤.和..特殊目录项
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        // 打印目录项信息
        printf("文件名：%-25s | inode号：%-10ld | 文件类型：%s\n",
               entry->d_name, entry->d_ino, get_file_type(entry->d_type));
    }

    // 区分读取完毕和读取失败
    if (errno != 0) {
        perror("readdir 读取目录项失败");
        closedir(dir);
        return -1;
    }

    // 3. 关闭目录
    if (closedir(dir) == -1) {
        perror("closedir 关闭目录失败");
        return -1;
    }
    printf("========== 目录遍历完成 ==========\n");
    return 0;
}

// 根据d_type打印文件类型
const char *get_file_type(unsigned char d_type) {
    switch (d_type) {
        case DT_REG: return "普通文件";
        case DT_DIR: return "目录文件";
        case DT_LNK: return "软链接文件";
        case DT_CHR: return "字符设备";
        case DT_BLK: return "块设备";
        case DT_FIFO: return "管道文件";
        case DT_SOCK: return "套接字文件";
        default: return "未知类型";
    }
}