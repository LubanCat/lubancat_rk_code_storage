#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

void traverse_dir(const char *dir_path);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "用法：%s <目标目录路径>\n", argv[0]);
        return -1;
    }

    printf("========== 递归遍历目录：%s ==========\n", argv[1]);
    traverse_dir(argv[1]);
    printf("========== 递归遍历完成 ==========\n");
    return 0;
}

// 递归遍历目录函数
void traverse_dir(const char *dir_path) {
    DIR *dir = opendir(dir_path);
    if (dir == NULL) {
        perror("opendir 失败");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // 过滤.和..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // 拼接文件/子目录全路径
        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

        // 打印全路径
        printf("%s\n", full_path);

        // 判断是否为子目录，若是则递归遍历
        if (entry->d_type == DT_DIR) {
            traverse_dir(full_path);
        }
    }

    // 读取错误判断
    if (errno != 0) {
        perror("readdir 失败");
    }

    // 关闭目录
    closedir(dir);
}