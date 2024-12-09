/*
*
*   file: main.c
*   update: 2024-12-05
*   usage: 
*       make
*       sudo ./main FF0000
*
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <signal.h> 
#include <pthread.h>

#include "spi.h"

#define SPI_BUS         "/dev/spidev3.0"

unsigned char send_buf[24];
void update_sendbuff(unsigned char r, unsigned char g, unsigned char b)
{
    int i = 0;

    // update g
    for (i = 0; i < 8; i++) 
    {
        send_buf[i] = (g & 0x80) ? (0xFC) : (0xC0);		
        g <<= 1;		
    }

    // update r
    for (i = 8; i < 16; i++) 
    {
        send_buf[i] = (r & 0x80) ? (0xFC) : (0xC0);	
        r <<= 1;			
    }

    // update b
    for (i = 16; i < 24; i++) 
    {
        send_buf[i] = (b & 0x80) ? (0xFC) : (0xC0);	
        b <<= 1;			
    }
}

int main(int argc, char **argv) 
{
    int ret;
    unsigned char r, g, b;
    spi_operations_t *spi_ops;      // spi操作函数

    // 初始化spi
    ret = spi_init(SPI_BUS);
    if(ret < 0)
        return -1;
    spi_ops = get_spi_ops();
    
    // 参数数量检查
    if(argc != 2) 
    {
        printf("Usage: %s <hex_color>\n", argv[0]);
        printf("e.g. : %s FF0000\n", argv[0]);
        return -1;
    }

    /* 参数1检查 */
    if(strlen(argv[1]) != 6)
    {
        printf("Error: The first argument has illegal length.\n");
        printf("e.g. : %s FF0000\n", argv[0]);
        return -1;
    }
    if (sscanf(argv[1], "%2hhx%2hhx%2hhx", &r, &g, &b) != 3) 
    {  
        printf("Error: Invalid hex color format.\n");  
        return -1;  
    }
    
    // 更新颜色数据
    update_sendbuff(r, g, b);

    spi_ops->spi_write_nbyte_data(send_buf, sizeof(send_buf));     

    return 0;
}
