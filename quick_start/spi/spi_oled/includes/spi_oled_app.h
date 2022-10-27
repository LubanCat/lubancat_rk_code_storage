#ifndef __SPI_OLED_APP_H
#define	__SPI_OLED_APP_H


#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

//设置的引脚为GPIO1_B2
#define GPIO_INDEX "42"

//路径为"/dev/spidev3.0"
//片选引脚为CS0
#define SPI_DEV_PATH "/dev/spidev3.0"

#define XLevelL			0x00
#define XLevelH			0x10
#define XLevel	    ((XLevelH&0x0F)*16+XLevelL)
#define Max_Column	128
#define Max_Row			64
#define	Brightness	0xCF 
#define X_WIDTH 		128  //oled显示屏列数
#define Y_WIDTH 		64   //oled显示屏行数


void pabort(const char *s);
void spi_and_gpio_init(char *name);
void oled_init(char *name);
void OLED_SetPos(unsigned char x, unsigned char y);//设置起始点坐标
void OLED_Fill(unsigned char bmp_dat);//全屏填充
void OLED_ON(void);
void OLED_OFF(void);
void OLED_ShowStr(unsigned char x, unsigned char y, unsigned char ch[], unsigned char TextSize);
void OLED_ShowCN(unsigned char x, unsigned char y, unsigned char N);
void OLED_DrawBMP(unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1, unsigned char BMP[]);


void spi_oled_send_commend(unsigned char cmd);
void spi_oled_send_data(unsigned char dat);
void transfer(int fd, uint8_t const *tx, uint8_t const *rx, size_t len);

#endif