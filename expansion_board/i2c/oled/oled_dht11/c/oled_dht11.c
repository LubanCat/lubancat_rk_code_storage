/*
*
*   file: main.c
*   update: 2024-08-09
*   usage: 
*       sudo make
*       sudo ./oled_dht11
*
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <signal.h> 

#include "oled.h"

/* dht11设备名 */
#define DEV_NAME    "/dev/dht11"

/* 温湿度阈值 */
#define MAX_TEMP    (35)
#define MAX_HUMI    (60)

int dht11_fd;

static void sigint_handler(int sig_num) 
{    
    /* oled清屏 */
    oled_clear();

    /* 关闭dht11 */
    close(dht11_fd);

    exit(0);  
}

int main(int argc, char **argv)
{
    int ret = 0;
    uint8_t data[6];
    float temp, humi;
    char temp_str[50], humi_str[50];

    /* register exit signal ( Ctrl + c ) */
    signal(SIGINT, sigint_handler);

    /* dht11初始化 */
    dht11_fd = open(DEV_NAME, O_RDWR);
	if(dht11_fd < 0)
	{
		printf("can not open file %s, %d\n", DEV_NAME, dht11_fd);
		return -1;
	}

    /* oled初始化 */
    ret = oled_init(3);
    if(ret == -1)
    {
        printf("oled init err!\n");
        return -1;
    }
    
    while(1)
    {
        /* 读取dht11温湿度数据 */
		ret = read(dht11_fd, &data, sizeof(data));	
		if(ret)
        {
            temp = data[2] + data[3] * 0.01;
            humi = data[0] + data[1] * 0.01;
        }
        else
            printf("read data from dth11 err!\n");

        sprintf(temp_str, "temp: %.2f", temp);
        sprintf(humi_str, "humi: %.2f", humi);
        oled_show_string(0, 2, temp_str);
        oled_show_string(0, 4, humi_str);

        sleep(1);
    }

    close(dht11_fd);

    return 0;
}