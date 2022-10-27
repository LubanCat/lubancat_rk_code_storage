#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <time.h>
#include <sys/ioctl.h>
#include <linux/ioctl.h>
#include <sys/stat.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

#include "spi_oled_app.h"
int fd;
/*程序中使用到的点阵 字库*/
extern const unsigned char BMP1[];


int main(int argc, char *argv[])
{
	int i = 0; //用于循环
	if(argc < 3){
    printf("Wrong use !!!!\n");
        printf("Usage: %s [dev] [D/C PinNum]\n",argv[0]);
        return -1;
    }
	printf("%s\n",argv[1]);
	/*打开 SPI 设备*/
    fd = open(argv[1], O_RDWR); // open file and enable read and  write
    if (fd < 0){
        printf("Can't open %s \n",argv[1]); // open i2c dev file fail
        exit(1);
    }

	oled_init(argv[2]);
	printf("oled_init\n");
	OLED_Fill(0xFF);
	while (1){
		OLED_Fill(0xff); //清屏
		sleep(1);
		OLED_Fill(0x00); //清屏
		OLED_ShowStr(0, 3, (unsigned char *)"Wildfire Tech", 1);  //测试6*8字符
		OLED_ShowStr(0, 4, (unsigned char *)"Hello wildfire", 2); //测试8*16字符
		sleep(1);
		OLED_Fill(0x00); //清屏

		for (i = 0; i < 4; i++)
			OLED_ShowCN(22 + i * 16, 0, i); //测试显示中文
	
		sleep(1);
		OLED_Fill(0x00); //清屏

		OLED_DrawBMP(0, 0, 128, 8, (unsigned char *)BMP1); //测试BMP位图显示
		sleep(1);
		OLED_Fill(0x00); //清屏
	}

	close(fd);
	return 0;
}

