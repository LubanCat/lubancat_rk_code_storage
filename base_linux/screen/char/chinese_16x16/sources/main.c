#include "drm-core.h"
#include "font.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

int fd_hzk16;
struct stat hzk_stat;
unsigned char *hzkmem;

uint32_t color_table[6] = {RED,GREEN,BLUE,BLACK,WHITE,BLACK_BLUE};

//描点函数
void show_pixel(uint32_t x , uint32_t y , uint32_t color)
{
	if(x > buf.width || y > buf.height){
		printf("wrong set\n");
	}

	buf.vaddr[ y*buf.width + x] = color;
}
//显示一个字母
void show_8x16(uint32_t x , uint32_t y , uint32_t color, unsigned char num)
{
	int i,j;
	unsigned char dot;
	for(i = 0 ; i<16 ; i++){
		dot = fontdata_8x16[num*16+i];
		for(j=0;j<8;j++){
			if(dot & 0x80)
				show_pixel(x+j,y+i,color);
			dot = dot << 1;
		}
	}
}
//显示256个字母
void show_string(uint32_t color)
{
	int i,j;
	int row=64;
	int x_offset = (buf.width - 64*8)/2;
	int y_offset = (buf.height - 16*4)/2;
	for(j=0;j<4;j++){
		for(i=0;i<64;i++){
			show_8x16(i*8+x_offset,16*j+y_offset,color,i+j*64);
		}
	}
}

//根据GBK2312码，显示一个汉字
void show_chinese(int x, int y, unsigned char *str)
{
	unsigned int area  = str[0] - 0xA1;
	unsigned int where = str[1] - 0xA1;
	unsigned char *dots = hzkmem + (area * 94 + where)*32;
	unsigned char byte;

	int i, j, b;
	for (i = 0; i < 16; i++){
		for (j = 0; j < 2; j++){
			byte = dots[i*2 + j];
			for (b = 7; b >=0; b--){
				if (byte & (1<<b))
					show_pixel(x+j*8+7-b, y+i, WHITE);
				else
					show_pixel(x+j*8+7-b, y+i, BLACK_BLUE); 
			}
		}
	}
}

int main(int argc, char **argv)
{
	int i;
	//野火科技 - GBK2312
	unsigned char YHKJ[8] = {0XD2,0XB0,0XBB,0XF0,0XBF,0XC6,0XBC,0XBC};
	//官网
	unsigned char *web = "www.embedfire.com";
	int ret;
	//初始化
	ret = drm_init();
	if(ret < 0){
		printf("drm init fail\n");
		return -1;
	}
	//屏幕颜色变化---浅蓝
	for(i = 0;i< buf.width*buf.height;i++)
		buf.vaddr[i] = BLACK_BLUE;
	//打开汉字库
	fd_hzk16 = open("file/HZK16", O_RDONLY);
	if (fd_hzk16 < 0){
		printf("can't open HZK16\n");
		return -1;
	}
	//获取文件长度
	if(fstat(fd_hzk16, &hzk_stat)){
		printf("can't get fstat\n");
		return -1;
	}
	//将整个文件映射到内存里
	hzkmem = (unsigned char *)mmap(NULL , hzk_stat.st_size, PROT_READ, MAP_SHARED, fd_hzk16, 0);
	if (hzkmem == (unsigned char *)-1){
		printf("can't mmap for hzk16\n");
		return -1;
	}
	//显示255个字母
	show_string(WHITE);
	//显示中文“野火科技”
	for(i = 0 ;i<4;i++)
		show_chinese((buf.width-64)/2+i*16,buf.height - 150,YHKJ+i*2);
	//显示网站"www.embedfire.com"
	for(i=0;i<strlen(web);i++){
		show_8x16((buf.width-8*strlen(web))/2+i*8,buf.height - 125,RED,web[i]);
	}
	//按键后进入
	getchar();
	//drm退出
	drm_exit();	

	return 0;
}