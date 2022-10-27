#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

#include "oled_app.h"

#define Address         0x3c

extern int fd;
extern const unsigned char BMP1[];

/* 主函数  */
int main(int argc, char *argv[])
{
    int i = 0; //用于循环
    if(argc < 2){
        printf("Wrong use !!!!\n");
	 	printf("Usage: %s [dev]\n",argv[0]);
        return -1;
    }
    fd = open(argv[1], O_RDWR); // open file and enable read and  write
    if (fd < 0){
        printf("Can't open %s \n",argv[1]); // open i2c dev file fail
        exit(1);
    }

    OLED_Init(fd,Address); //初始化oled
    usleep(1000 * 100);
    OLED_Fill(Address,0xff); //全屏填充
    sleep(10);
    OLED_CLS(Address); //清屏
    close(fd);
}
