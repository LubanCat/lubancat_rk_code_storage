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

    while (1)
    {
        OLED_Fill(Address,0xff); //全屏填充
        sleep(1);

        OLED_CLS(Address); //清屏
        sleep(1);

        OLED_ShowStr(Address,0, 3, (unsigned char *)"Wildfire Tech", 1);  //测试6*8字符
        OLED_ShowStr(Address,0, 4, (unsigned char *)"Hello wildfire", 2); //测试8*16字符
        sleep(1);
        OLED_CLS(Address); //清屏

        for (i = 0; i < 4; i++){
            OLED_ShowCN(Address,22 + i * 16, 0, i); //测试显示中文
        }
        sleep(1);
        OLED_CLS(Address); //清屏

        OLED_DrawBMP(Address,0, 0, 128, 8, (unsigned char *)BMP1); //测试BMP位图显示
        sleep(1);
        OLED_CLS(Address); //清屏
    }

    close(fd);
}
