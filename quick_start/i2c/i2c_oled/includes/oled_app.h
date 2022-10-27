
#ifndef __OLED_APP_H
#define	__OLED_APP_H

#define OLED_COMMEND_ADDR 0x00
#define OLED_DATA_ADDR 0x40

static int i2c_write(int fd, unsigned char addr,unsigned char reg,unsigned char val);
void OLED_Init(int fd,unsigned char addr);
void OLED_Fill(unsigned char addr,unsigned char fill_Data); //全屏填充
void OLED_CLS(unsigned char addr);//清屏
void oled_set_Pos(unsigned char addr,unsigned char x, unsigned char y); //设置起始点坐标
void OLED_ShowStr(unsigned char addr,unsigned char x, unsigned char y, unsigned char ch[], unsigned char TextSize);
void OLED_ShowCN(unsigned char addr,unsigned char x, unsigned char y, unsigned char N);
void OLED_DrawBMP(unsigned char addr,unsigned char x0,unsigned char y0,unsigned char x1,unsigned char y1,unsigned char BMP[]);

#endif