#include "drm-core.h"
#include "font.h"
uint32_t color_table[6] = {RED,GREEN,BLUE,BLACK,WHITE,BLACK_BLUE};

//描点函数
void show_pixel(uint32_t x , uint32_t y , uint32_t color)
{
	if(x > buf.width || y > buf.height){
		printf("wrong set\n");
	}

	buf.vaddr[ y*buf.width + x] = color;
}


//单个8x16字符的描写
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


//256个ascii字符打印出来
void show_string(uint32_t color)
{
	int i,j;
	int row=64;
	int x_offset = (buf.width - 64*8)/2;
	int y_offset = (buf.height - 16*4)/2;
	printf("%d,%d\n",x_offset,y_offset);
	printf("%d,%d\n",buf.width,buf.height);
	for(j=0;j<4;j++){
		for(i=0;i<64;i++){
			show_8x16(i*8+x_offset,16*j+y_offset,color,i+j*64);
		}
	}
}

int main(int argc, char **argv)
{
	int i;
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
	//屏幕中间显示字体
	show_string(WHITE);

	//获取字符--enter键进入下一步
    getchar();
	drm_exit();	

	return 0;
}

