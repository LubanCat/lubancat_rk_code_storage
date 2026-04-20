#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int main(int argc, char *argv[])
{   
    int fd, ret;
    char *file_path;
    char usr_data[] = "Hello World\n";
    char wbuf[128];
    char rbuf[128];

    if(argc != 2){
		printf("Error Usage!\r\n");
		return -1;
	}

    file_path = argv[1];

    printf("--------------写入数据--------------\r\n");

    //打开文件
    fd = open(file_path, O_RDWR);
	if(fd < 0){
		printf("Can't open file: %s\r\n", file_path);
		return -1;
	}

    //写入数据
    memcpy(wbuf, usr_data, sizeof(usr_data));
    ret = write(fd, wbuf, strlen(wbuf));
    if(ret < 0){
        printf("write file %s failed!\r\n", file_path);
        return -1;
    }

    //写入完毕，关闭文件
    close(fd);

    sleep(1);

    printf("--------------读取数据--------------\r\n");

    //打开文件
    fd = open(file_path, O_RDWR);
    if(fd < 0){
		printf("Can't open file: %s\r\n", file_path);
		return -1;
	}
    //读取文件内容
    ret = read(fd, rbuf, 128);
    if(ret < 0){
        printf("read file %s failed!\r\n", file_path);
        return -1;
    }else{
        printf("read data:%s\r\n",rbuf);
    }

    //读取完毕，关闭文件
    close(fd);

    return 0;
}