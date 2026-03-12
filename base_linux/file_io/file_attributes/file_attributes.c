#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

void print_file_attr(struct stat *statbuf);

int main(int argc, char *argv[])
{
    // 校验参数
    if(argc != 2){
        fprintf(stderr, "用法：%s <文件路径>\n", argv[0]);
        return -1;
    }

    struct stat statbuf;
    // 方式1：通过stat函数，路径获取文件属性
    if(stat(argv[1], &statbuf) == -1){
        perror("stat获取文件属性失败");
        return -1;
    }
    printf("========== 通过stat函数获取文件属性 ==========\n");
    print_file_attr(&statbuf);

    // 方式2：通过fstat函数，文件描述符获取属性
    int fd = open(argv[1], O_RDONLY);
    if(fd == -1){
        perror("open文件失败");
        return -1;
    }
    struct stat fstat_buf;
    if(fstat(fd, &fstat_buf) == -1){
        perror("fstat获取文件属性失败");
        close(fd);
        return -1;
    }
    printf("\n========== 通过fstat函数获取文件属性 ==========\n");
    print_file_attr(&fstat_buf);

    // 关闭文件描述符
    close(fd);
    return 0;
}

// 解析并打印文件所有核心属性
void print_file_attr(struct stat *statbuf)
{
    // 1. 打印文件索引节点号
    printf("文件inode号：%ld\n", statbuf->st_ino);

    // 2. 打印文件类型
    printf("文件类型：");
    if(S_ISREG(statbuf->st_mode))          printf("普通文件\n");
    else if(S_ISDIR(statbuf->st_mode))     printf("目录文件\n");
    else if(S_ISLNK(statbuf->st_mode))     printf("软链接文件\n");
    else if(S_ISCHR(statbuf->st_mode))     printf("字符设备文件\n");
    else if(S_ISBLK(statbuf->st_mode))     printf("块设备文件\n");
    else if(S_ISFIFO(statbuf->st_mode))    printf("管道文件\n");
    else if(S_ISSOCK(statbuf->st_mode))    printf("套接字文件\n");
    else                                   printf("未知文件类型\n");

    // 3. 打印文件权限，八进制格式
    printf("文件权限：%03o\n", statbuf->st_mode & 0777);

    // 4. 打印文件大小
    printf("文件大小：%ld 字节\n", statbuf->st_size);

    // 5. 打印文件硬链接数
    printf("硬链接数：%ld\n", statbuf->st_nlink);

    // 6. 打印文件属主与属组
    struct passwd *pw = getpwuid(statbuf->st_uid);
    struct group *gr = getgrgid(statbuf->st_gid);
    printf("文件所有者：%s\n", pw->pw_name);
    printf("文件所属组：%s\n", gr->gr_name);

    // 7. 打印文件时间戳
    printf("最后访问时间：%s", ctime(&statbuf->st_atime));
    printf("最后修改时间：%s", ctime(&statbuf->st_mtime));
    printf("最后状态改变时间：%s", ctime(&statbuf->st_ctime));
}