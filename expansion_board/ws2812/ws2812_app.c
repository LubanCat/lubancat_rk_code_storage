/*
*
*   file: ws2812_app.c
*   date: 2024-09-03
*   usage: 
*       sudo gcc -o ws2812_app ws2812_app.c
*       sudo ./ws2812_app
*
*/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <signal.h> 

#define WS2812_DATA_GPIOCHIP     3
#define WS2812_DATA_GPIONUM      19

struct ws2812_mes {
    unsigned int gpiochip;      // data引脚的gpiochip
    unsigned int gpionum;       // data引脚的gpionum
    unsigned int lednum;        // 要控制灯带的第几个LED，序号从1开始
    unsigned char color[3];     // color[0]:color[1]:color[2]   R:G:B 
};

int main(int argc, char **argv)
{
    struct ws2812_mes ws2812;
    int fd;
    int ret;
    char hex_str[10];
    char *endptr;

    ws2812.gpiochip = WS2812_DATA_GPIOCHIP;       
    ws2812.gpionum  = WS2812_DATA_GPIONUM;
    ws2812.lednum   = 1;
    ws2812.color[0] = 0x00;
    ws2812.color[1] = 0x00;
    ws2812.color[2] = 0x00;

    if(argc != 3) 
    {
        printf("Usage: %s <led num> <hex_color>\n", argv[0]);
        printf("e.g. : %s 3 FF0000\n", argv[0]);
        return -1;
    }

    /* 参数1检查 */
    ws2812.lednum = (int)strtol(argv[1], &endptr, 10);
    if (*endptr != '\0' || ws2812.lednum < 1 || ws2812.lednum > 20) {
        printf("Error: The first argument must be a number between 1 and 20.\n");
        printf("e.g. : %s 3 FF0000\n", argv[0]);
        return -1;
    }

    /* 参数2检查 */
    if(strlen(argv[2]) != 6)
    {
        printf("Error: The second argument has illegal length.\n");
        printf("e.g. : %s 3 FF0000\n", argv[0]);
        return -1;
    }
    if (sscanf(argv[2], "%2hhx%2hhx%2hhx", &ws2812.color[0], &ws2812.color[1], &ws2812.color[2]) != 3) 
    {  
        printf("Error: Invalid hex color format.\n");  
        return -1;  
    } 

    /* 打开ws2812设备节点 */
    fd = open("/dev/ws2812", O_RDWR);
	if (fd == -1)
	{
		printf("can not open file /dev/ws2812\n");
		return -1;
	}

    ret = write(fd, &ws2812, sizeof(struct ws2812_mes));
    if(ret < 0)
    {
        printf("ws2812 write err!\n");
    }

    close(fd);

    return 0;
}